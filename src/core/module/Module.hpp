/**
 * @file
 * @brief Base for the module implementation
 * @copyright MIT License
 */

#ifndef ALLPIX_MODULE_H
#define ALLPIX_MODULE_H

#include <memory>
#include <string>
#include <vector>

#include "core/config/Configuration.hpp"
#include "core/geometry/Detector.hpp"
#include "core/messenger/delegates.h"
#include "exceptions.h"

// TODO [doc] All these methods should move to the implementation file

namespace allpix {

    /**
     * @brief Internal identifier for a module
     *
     * Used by the framework to distinguish between different module instantiations and their priority.
     */
    class ModuleIdentifier {
    public:
        /**
          * @brief Constructs an empty identifier
          */
        // TODO [doc] Is this method really necessary
        ModuleIdentifier() : name_(""), identifier_(""), prio_(0) {}
        /**
         * @brief Construct an identifier
         * @param module_name Name of the module
         * @param identifier Unique identifier for the instantiation
         * @param prio Priority of this module
         */
        ModuleIdentifier(std::string module_name, std::string identifier, int prio)
            : name_(std::move(module_name)), identifier_(std::move(identifier)), prio_(prio) {}

        /**
         * @brief Get the name of the module
         * @return Module name
         */
        std::string getName() const { return name_; }
        /**
         * @brief Get the identifier of the instantiation
         * @return Module identifier
         */
        std::string getIdentifier() const { return identifier_; }
        /**
         * @brief Get the unique name of the instantiation
         * @return Unique module name
         *
         * The unique name of the module is the name combined with its identifier separated by a semicolon.
         */
        std::string getUniqueName() const {
            std::string unique_name = name_;
            if(!identifier_.empty())
                unique_name += ":" + identifier_;
            return unique_name;
        }
        /**
         * @brief Get the priority of the instantiation
         * @return Priority level
         *
         * A lower number indicates a higher priority
         *
         * @warning It is important to realize that the priority is ordered from high to low numbers
         */
        int getPriority() const { return prio_; }

        /// @{
        /**
         * @brief Operators for comparing identifiers
         *
         * Identifiers are only compared on their unique name, identifiers are not distinguished on priorities
         */
        bool operator==(const ModuleIdentifier& other) const { return getUniqueName() == other.getUniqueName(); }
        bool operator!=(const ModuleIdentifier& other) const { return getUniqueName() != other.getUniqueName(); }
        bool operator<(const ModuleIdentifier& other) const { return getUniqueName() < other.getUniqueName(); }
        bool operator<=(const ModuleIdentifier& other) const { return getUniqueName() <= other.getUniqueName(); }
        bool operator>(const ModuleIdentifier& other) const { return getUniqueName() > other.getUniqueName(); }
        bool operator>=(const ModuleIdentifier& other) const { return getUniqueName() >= other.getUniqueName(); }
        /// @}

    private:
        std::string name_;
        std::string identifier_;
        int prio_;
    };

    /**
     * @brief Base class for all modules
     *
     * The module base is the core of the modular framework. All modules should be descendants of this class. The base class
     * defines the methods the children can implement:
     * - Module::init(): for initializing the module at the start
     * - Module::run(unsigned int): for doing the job of every module for every event
     * - Module::finalize(): for finalizing the module at the end
     *
     * The module class also provides a few utility methods and stores internal data of instantations. The internal data is
     * used by the ModuleManager and the Messenger to work.
     */
    class Module {
        friend class ModuleManager;
        friend class Messenger;

    public:
        /**
         * @brief Base constructor for unique modules
         */
        explicit Module();
        /**
         * @brief Base constructor for detector modules
         * @param detector Detector bound to this module
         * @warning Detector modules should not forget to forward their detector to the base constructor. An
         *          \ref InvalidModuleStateException will be raised if the module failed to so.
         */
        explicit Module(std::shared_ptr<Detector> detector);
        /**
         * @brief Essential virtual destructor
         */
        virtual ~Module() = default;

        /// @{
        /**
         * @brief Copying a module is not allowed
         */
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;
        /// @}

        /**
         * @brief Get the detector linked to this module
         * @return Linked detector or a null pointer if this is an unique module
         */
        std::shared_ptr<Detector> getDetector();

        /**
         * @brief Get the unique name of this module
         * @return Unique name
         * @warning This method can not be called from the constructor
         * @note Can be used to interact with ROOT objects that require an unique name
         */
        std::string getUniqueName();

        /**
         * @brief Get an absolute path to be used for output from a relative path
         * @param path Relative path to add after the main output directory
         * @param global True if the global output directory should be used instead of the module-specific version
         * @return Canonical path to an output file
         * @warning This method can not be called from the constructor
         */
        // TODO [doc] Should be renamed to getOutputFile
        std::string getOutputPath(const std::string& path, bool global = false);

        /**
         * @brief Initialize the module before the event sequence
         *
         * Does nothing if not overloaded.
         */
        virtual void init() {}

        /**
         * @brief Execute the function of the module for every event
         * @param int Number of the event in the event sequence (starts at 1)
         *
         * Does nothing if not overloaded.
         */
        // TODO [doc] Start the sequence at 0 instead of 1?
        virtual void run(unsigned int /* event_num */) {}

        //
        /**
         * @brief Finalize the module after the event sequence
         * @note Useful to have before destruction to allow for raising exceptions
         *
         * Does nothing if not overloaded.
         */
        virtual void finalize() {}

    private:
        /**
         * @brief Set the output directory
         * @param output_dir Base directory for the module
         */
        void set_output_directory(std::string output_dir);
        std::string output_directory_;
        /**
         * @brief Set the global output directory
         * @param output_dir Base output directory
         */
        void set_global_directory(std::string);
        std::string global_directory_;

        /**
         * @brief Set the module identifier for internal use
         * @param identifier Identifier of the instantiation
         */
        void set_identifier(ModuleIdentifier identifier);
        /**
         * @brief Get the module identifier for internal use
         * @return Identifier of the instantiation
         */
        ModuleIdentifier get_identifier();
        ModuleIdentifier identifier_;

        /**
         * @brief Set the module configuration for internal use
         * @note Modules should use the configuration passed to their constructor
         * @param config Identifier of the instantiation
         */
        void set_configuration(Configuration config);
        /**
         * @brief Get the module configuration for internal use
         * @return Configuration of the module
         */
        Configuration get_configuration();
        Configuration config_;

        /**
         * @brief Add a messenger delegate to this instantiation
         * @param delegate Delegate object
         */
        void add_delegate(BaseDelegate* delegate);
        /**
         * @brief Resets messenger delegates after every event
         */
        void reset_delegates();
        /**
         * @brief Check if all delegates are satisfied
         */
        bool check_delegates();
        std::vector<BaseDelegate*> delegates_;

        std::shared_ptr<Detector> detector_;
    };

} // namespace allpix

#endif /* ALLPIX_MODULE_H */
