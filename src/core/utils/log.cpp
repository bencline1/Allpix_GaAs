/**
 * @file
 * @brief Implementation of logger
 *
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "log.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <regex>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

using namespace allpix;

// Last name used while printing (for identifying process logs)
std::string DefaultLogger::last_identifier_;
// Last message send used to check if extra spaces are needed
std::string DefaultLogger::last_message_;
// Mutex to guard output writing
std::mutex DefaultLogger::write_mutex_;

/**
 * The logger will save the number of uncaught exceptions during construction to compare that with the number of exceptions
 * during destruction later.
 */
DefaultLogger::DefaultLogger() : exception_count_(get_uncaught_exceptions(true)) {}

/**
 * The output is written to the streams as soon as the logger gets out-of-scope and destructed. The destructor checks
 * specifically if an exception is thrown while output is written to the stream. In that case the log stream will not be
 * forwarded to the output streams and the message will be discarded.
 */
DefaultLogger::~DefaultLogger() {
    // Check if an exception is thrown while adding output to the stream
    if(exception_count_ != get_uncaught_exceptions(false)) {
        return;
    }

    // TODO [doc] any extra exceptions here need to be caught

    // Get output string
    std::string out(os.str());

    // Replace every newline by indented code if necessary
    auto start_pos = out.find('\n');
    if(start_pos != std::string::npos) {
        std::string spcs(indent_count_ + 1, ' ');
        spcs[0] = '\n';
        do {
            out.replace(start_pos, 1, spcs);
            start_pos += spcs.length();
        } while((start_pos = out.find('\n', start_pos)) != std::string::npos);
    }

    // Lock the mutex to guard last identifier usage
    std::unique_lock<std::mutex> lock(write_mutex_);

    // Add extra spaces if necessary
    size_t extra_spaces = 0;
    if(!identifier_.empty() && last_identifier_ == identifier_) {
        // Put carriage return for process logs
        out = '\r' + out;

        // Set extra spaces to fully cover previous message
        if(last_message_.size() > out.size()) {
            extra_spaces = last_message_.size() - out.size();
        }
    } else if(!last_identifier_.empty()) {
        // End process log and continue normal logging
        out = '\n' + out;
    }
    last_identifier_ = identifier_;

    // Save last message
    last_message_ = out;
    last_message_ += " ";

    // Add extra spaces if required
    if(extra_spaces > 0) {
        out += std::string(extra_spaces, ' ');
    }

    // Add final newline if not a progress log
    if(identifier_.empty()) {
        out += '\n';
    }

    // Create a version without any special terminal characters
    std::string out_no_special;
    size_t prev = 0, pos = 0;
    while((pos = out.find("\x1B[", prev)) != std::string::npos) {
        out_no_special += out.substr(prev, pos - prev);
        prev = out.find('m', pos) + 1;
        if(prev == std::string::npos) {
            break;
        }
    }
    out_no_special += out.substr(prev);

    // Replace carriage return by newline:
    try {
        out_no_special = std::regex_replace(out_no_special, std::regex("\\\r"), "\n");
    } catch(std::regex_error&) {
    }

    // Print output to streams
    for(auto stream : get_streams()) {
        if(is_terminal(*stream)) {
            (*stream) << out;
        } else {
            (*stream) << out_no_special;
        }
        (*stream).flush();
    }
    lock.unlock();
}

/**
 * @warning No other log message should be send after this method
 * @note Does not close the streams
 */
void DefaultLogger::finish() {
    // Lock the mutex to guard output writing
    std::lock_guard<std::mutex> lock(write_mutex_);

    if(!last_identifier_.empty()) {
        // Flush final line if necessary
        for(auto stream : get_streams()) {
            (*stream) << std::endl;
            (*stream).flush();
        }
    }

    last_identifier_ = "";
    last_message_ = "";

    // Enable cursor again if stream supports it
    for(auto stream : get_streams()) {
        if(is_terminal(*stream)) {
            (*stream) << "\x1B[?25h";
            (*stream).flush();
        }
    }

    get_streams().clear();
}

/**
 * This method is typically automatically called by the \ref LOG macro to return a stream after constructing the logger. The
 * header of the stream is added before returning the output stream.
 */
std::ostringstream&
DefaultLogger::getStream(LogLevel level, const std::string& file, const std::string& function, uint32_t line) {
    // Add date in all except short format
    if(get_format() != LogFormat::SHORT) {
        os << "\x1B[1m"; // BOLD
        os << "|" << get_current_date() << "| ";
        os << "\x1B[0m"; // RESET
    }

    // Add thread id only in long format
    if(get_format() == LogFormat::LONG) {
        os << "\x1B[1m"; // BOLD
        os << "=" << std::this_thread::get_id() << "= ";
        os << "\x1B[0m"; // RESET
    }

    // Set color for log level
    if(level == LogLevel::FATAL || level == LogLevel::ERROR) {
        os << "\x1B[31;1m"; // RED
    } else if(level == LogLevel::WARNING) {
        os << "\x1B[33;1m"; // YELLOW
    } else if(level == LogLevel::STATUS) {
        os << "\x1B[32;1m"; // GREEN
    } else if(level == LogLevel::TRACE || level == LogLevel::DEBUG) {
        os << "\x1B[36m"; // NON-BOLD CYAN
    } else {
        os << "\x1B[36;1m"; // CYAN
    }

    // Add log level (shortly in the short format)
    if(get_format() != LogFormat::SHORT) {
        std::string level_str = "(";
        level_str += getStringFromLevel(level);
        level_str += ")";
        os << std::setw(9) << level_str << " ";
    } else {
        os << "(" << getStringFromLevel(level).substr(0, 1) << ") ";
    }
    os << "\x1B[0m"; // RESET

    // Add section if available
    if(!get_section().empty()) {
        os << "\x1B[1m"; // BOLD
        os << "[" << get_section() << "] ";
        os << "\x1B[0m"; // RESET
    }

    // Print function name and line number information in debug format
    if(get_format() == LogFormat::LONG) {
        os << "\x1B[1m"; // BOLD
        os << "<" << file << "/" << function << ":L" << line << "> ";
        os << "\x1B[0m"; // RESET
    }

    // Save the indent count to fix with newlines
    size_t prev = 0, pos = 0;
    std::string out = os.str();
    while((pos = out.find("\x1B[", prev)) != std::string::npos) {
        indent_count_ += static_cast<unsigned int>(pos - prev);
        prev = out.find('m', pos) + 1;
        if(prev == std::string::npos) {
            break;
        }
    }
    return os;
}

/**
 * This method is typically automatically called by the \ref LOG_PROGRESS macro. An empty identifier is the same as
 * underscore.
 */
std::ostringstream& DefaultLogger::getProcessStream(
    std::string identifier, LogLevel level, const std::string& file, const std::string& function, uint32_t line) {
    // Get the standard process stream
    std::ostringstream& stream = getStream(level, file, function, line);

    // Replace empty identifier with underscore because empty is already used for check
    if(identifier.empty()) {
        identifier = "_";
    }
    identifier_ = std::move(identifier);

    return stream;
}

/**
 * This method is typically automatically called by the \ref LOG_PROGRESS_BAR macro. An empty identifier is the same as
 * underscore.
 */
void DefaultLogger::drawProgressBar(std::string identifier,
                                    LogLevel level,
                                    uint64_t current,
                                    uint64_t total,
                                    const std::string& file,
                                    const std::string& function,
                                    uint32_t line) {
    // Get a progress stream, set indicator to "#" to indicate progress bar (is overwritten)
    std::ostringstream& stream = getProcessStream(identifier, level, file, function, line);

    std::string events = " " + std::to_string(current) + " / " + std::to_string(total) + " ";
    static std::array<std::string, 8> partial_block{
        " ", "\u258F", "\u258E", "\u258D", "\u258C", "\u258B", "\u258A", "\u2589"};

    auto width = static_cast<uint64_t>(query_line_length() - stream.tellp());
    auto completed = (width * current) / total;
    auto pending = completed - 1;

    auto remainder = (8 * current * width / total) % 8;

    stream << " " << std::setw(3) << std::setfill(' ') << (100 * current) / total << "% ";

    if(events.length() <= pending) {
        stream << "\x1B[7m" << std::string(completed, ' ') << "\x1B[0m";
        stream << partial_block.at(remainder) << events;
        stream << std::string(pending - events.length(), ' ');
    } else {
        stream << "\x1B[7m" << std::string(completed - events.length(), ' ');
        stream << events << "\x1B[0m" << partial_block.at(remainder);
        stream << std::string(pending, ' ');
    }
    stream << '|';
}

// Getter and setters for the reporting level
LogLevel& DefaultLogger::get_reporting_level() {
    thread_local LogLevel reporting_level = LogLevel::NONE;
    return reporting_level;
}
void DefaultLogger::setReportingLevel(LogLevel level) {
    get_reporting_level() = level;
}
LogLevel DefaultLogger::getReportingLevel() {
    return get_reporting_level();
}

// String to LogLevel conversions and vice versa
std::string DefaultLogger::getStringFromLevel(LogLevel level) {
    const std::array<std::string, 8> type = {{"FATAL", "STATUS", "ERROR", "WARNING", "INFO", "DEBUG", "NONE", "TRACE"}};
    return type.at(static_cast<decltype(type)::size_type>(level));
}
/**
 * @throws std::invalid_argument If the string does not correspond with an existing log level
 */
LogLevel DefaultLogger::getLevelFromString(const std::string& level) {
    if(level == "TRACE") {
        return LogLevel::TRACE;
    }
    if(level == "DEBUG") {
        return LogLevel::DEBUG;
    }
    if(level == "INFO") {
        return LogLevel::INFO;
    }
    if(level == "WARNING") {
        return LogLevel::WARNING;
    }
    if(level == "ERROR") {
        return LogLevel::ERROR;
    }
    if(level == "STATUS") {
        return LogLevel::STATUS;
    }
    if(level == "FATAL") {
        return LogLevel::FATAL;
    }

    throw std::invalid_argument("unknown log level");
}

// Getter and setters for the format
LogFormat& DefaultLogger::get_format() {
    thread_local LogFormat reporting_level = LogFormat::DEFAULT;
    return reporting_level;
}
void DefaultLogger::setFormat(LogFormat level) {
    get_format() = level;
}
LogFormat DefaultLogger::getFormat() {
    return get_format();
}

// Convert string to log format and vice versa
std::string DefaultLogger::getStringFromFormat(LogFormat format) {
    const std::array<std::string, 3> type = {{"SHORT", "DEFAULT", "LONG"}};
    return type.at(static_cast<decltype(type)::size_type>(format));
}
/**
 * @throws std::invalid_argument If the string does not correspond with an existing log format
 */
LogFormat DefaultLogger::getFormatFromString(const std::string& format) {
    if(format == "SHORT") {
        return LogFormat::SHORT;
    }
    if(format == "DEFAULT") {
        return LogFormat::DEFAULT;
    }
    if(format == "LONG") {
        return LogFormat::LONG;
    }

    throw std::invalid_argument("unknown log format");
}

/**
 * The streams are shared by all logger instantiations.
 */
const std::vector<std::ostream*>& DefaultLogger::getStreams() {
    return get_streams();
}
std::vector<std::ostream*>& DefaultLogger::get_streams() {
    static std::vector<std::ostream*> streams;
    return streams;
}
void DefaultLogger::clearStreams() {
    get_streams().clear();
}
/**
 * The caller has to make sure that the added ostream exists for as long log messages may be written. The std::cout stream is
 * added automatically to the list of streams and does not need to be added itself.
 *
 * @note Streams cannot be individually removed at the moment and only all at once using \ref clearStreams().
 */
void DefaultLogger::addStream(std::ostream& stream) {
    // Disable cursor if stream supports it
    if(is_terminal(stream)) {
        stream << "\x1B[?25l";
    }

    get_streams().push_back(&stream);
}

// Getters and setters for the section header
std::string& DefaultLogger::get_section() {
    thread_local std::string section;
    return section;
}
void DefaultLogger::setSection(std::string section) {
    get_section() = std::move(section);
}
std::string DefaultLogger::getSection() {
    return get_section();
}

/**
 * The date is returned in the hh:mm:ss.ms format
 */
std::string DefaultLogger::get_current_date() {
    // FIXME: revise this to get microseconds in a better way
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%X");

    auto seconds_from_epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() - seconds_from_epoch).count();
    ss << ".";
    ss << std::setfill('0') << std::setw(3);
    ss << millis;
    return ss.str();
}

/*
 * It is impossible to know for sure a terminal has support for all extra terminal features, but every modern terminal has
 * this so we just assume it.
 */
bool DefaultLogger::is_terminal(std::ostream& stream) {
    if(&std::cout == &stream) {
        return static_cast<bool>(isatty(fileno(stdout)));
    }
    if(&std::cerr == &stream) {
        return static_cast<bool>(isatty(fileno(stderr)));
    }

    return false;
}

/**
 * The number of uncaught exceptions can only be properly determined in C++17. In earlier versions it is only possible to
 * check if there is at least a single exception thrown and that function is used instead. This means a return value of zero
 * corresponds to no exception and one to at least one exception.
 */
int DefaultLogger::get_uncaught_exceptions(bool cons = false) {
#if __cplusplus > 201402L
    // we can only do this fully correctly in C++17
    (void)cons;
    return std::uncaught_exceptions();
#else
    if(cons) {
        return 0;
    }
    return static_cast<int>(std::uncaught_exception());
#endif
}

/**
 * Query the connected terminal for its line length
 * @return Length of the terminal
 */
int DefaultLogger::query_line_length() {
// from
// https://www.linuxquestions.org/questions/programming-9/get-width-height-of-a-terminal-window-in-c-810739/
#ifdef TIOCGSIZE
    struct ttysize ts;
    ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
    return ts.ts_cols;
#elif defined(TIOCGWINSZ)
    struct winsize ts;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
    return ts.ws_col;
#else
    return 50; // fallback
#endif
}
