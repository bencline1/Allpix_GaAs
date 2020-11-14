Redirect Module Inputs and Outputs
====================================

In the Allpix² framework, modules exchange messages typically based on
their input and output message types and the detector type. It is,
however, possible to specify a name for the incoming and outgoing
messages for every module in the simulation. Modules will then only
receive messages dispatched with the name provided and send named
messages to other modules listening for messages with that specific
name. This enables running the same module several times for the same
detector, e.g. to test different parameter settings.

The message output name of a module can be changed by setting the
`output` parameter of the module to a unique value. The output of this
module is then not sent to modules without a configured input, because
by default modules listens only to data without a name. The `input`
parameter of a particular receiving module should therefore be set to
match the value of the `output` parameter. In addition, it is permitted
to set the `input` parameter to the special value `*` to indicate that
the module should listen to all messages irrespective of their name.

An example of a configuration with two different settings for the
digitisation module is shown below:

``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
# Digitize the propagated charges with low noise levels
[DefaultDigitizer]
# Specify an output identifier
output = "low_noise"
# Low amount of noise added by the electronics
electronics_noise = 100e
# Default values are used for the other parameters

# Digitize the propagated charges with high noise levels
[DefaultDigitizer]
# Specify an output identifier
output = "high_noise"
# High amount of noise added by the electronics
electronics_noise = 500e
# Default values are used for the other parameters

# Save histogram for 'low_noise' digitized charges
[DetectorHistogrammer]
# Specify input identifier
input = "low_noise"

# Save histogram for 'high_noise' digitized charges
[DetectorHistogrammer]
# Specify input identifier
input = "high_noise"
```

Logging and other Utilities
---------------------------

The Allpix² framework provides a set of utilities which improve the
usability of the framework and extend the functionality provided by the
Standard Template Library (STL). The former includes a flexible and
easy-to-use logging system, introduced in Section [Logging system](framework-redirect-module-inputs-outputs.md#logging-system) and an
easy-to-use framework for units that supports converting arbitrary
combinations of units to common base units which can be used
transparently throughout the framework, and which will be discussed in
more detail in Section [Unit system](framework-redirect-module-inputs-outputs.md#unit-system). The latter comprise tools
which provide functionality the C++14 standard does not contain. These
utilities are used internally in the framework and are only shortly
discussed in Section [Internal utilities](framework-redirect-module-inputs-outputs.md#internal-utilities) (file system support) and
Section [Internal utilities](framework-redirect-module-inputs-outputs.md#internal-utilities) (string utilities).

### Logging system

The logging system is built to handle input/output in the same way as
`std::cin` and `std::cout` do. This approach is both very flexible and
easy to read. The system is globally configured, thus only one logger
instance exists. The following commands are available for sending
messages to the logging system at a level of `LEVEL`:

<span>LOG(LEVEL)</span>
:   Send a message with severity level `LEVEL` to the logging system.

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
        LOG(LEVEL) << "this is an example message with an integer and a double " << 1 << 2.0;
        
    ```

    A new line and carriage return is added at the end of every log
    message. Multi-line log messages can be used by adding new line
    commands to the stream. The logging system will automatically align
    every new line under the previous message and will leave the header
    space empty on new lines.

<span>LOG~O~NCE(LEVEL)</span>
:   Same as `LOG`, but will only log this message once over the full
    run, even if the logging function is called multiple times.

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
        LOG_ONCE(INFO) << "This message will appear once only, even if present in every event...";
        
    ```

    This can be used to log warnings or messages e.g. from the `run()`
    function of a module without flooding the log output with the same
    message for every event. The message is preceded by the information
    that further messages will be suppressed.

<span>LOG~N~(LEVEL, NUMBER)</span>
:   Same as `LOGONCE` but allows to specify the number of times the
    message will be logged via the additional parameter `NUMBER`.

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
        LOG_N(INFO, 10) << "This message will appear maximally 10 times throughout the run.";
        
    ```

    The last message is preceded by the information that further
    messages will be suppressed.

<span>LOG~P~ROGRESS(LEVEL, IDENTIFIER)</span>
:   This function allows to update the message to be updated on the same
    line for simple progressbar-like functionality.

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
        LOG_PROGRESS(STATUS, "EVENT_LOOP") << "Running event " << n << " of " << number_of_events;
        
    ```

    Here, the `IDENTIFIER` is a unique string identifying this output
    stream in order not to mix different progress reports.

If the output is a terminal screen the logging output will be coloured
to make it easier to identify warnings and error messages. This is
disabled automatically for all non-terminal outputs.

More details about the logging levels and formats can be found in
Section [Logging and Verbosity Levels](getting_started.md#logging-and-verbosity-levels).

### Unit system

Correctly handling units and conversions is of paramount importance.
Having a separate type for every unit would however be too cumbersome
for a lot of operations, therefore units are stored in standard floating
point types in a default unit which all code in the framework should use
for calculations. In configuration files, as well as for logging, it is
however very useful to provide quantities in different units.

The unit system allows adding, retrieving, converting and displaying
units. It is a global system transparently used throughout the
framework. Examples of using the unit system are given below:

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
// Define the standard length unit and an auxiliary unit
Units::add("mm", 1);
Units::add("m", 1e3);
// Define the standard time unit
Units::add("ns", 1);
// Get the units given in m/ns in the defined framework unit (mm/ns)
Units::get(1, "m/ns");
// Get the framework unit (mm/ns) in m/ns
Units::convert(1, "m/ns");
// Return the unit in the best type (lowest number larger than one) as string.
// The input is in default units 2000mm/ns and the 'best' output is 2m/ns (string)
Units::display(2e3, \texttt{"mm/ns", "m/ns"});
```

A description of the use of units in config files within Allpix² was
presented in Section [Parsing types and units](getting_started.md#parsing-types-and-units).

### Internal utilities

The **filesystem** utilities provide functions to convert relative to
absolute canonical paths, to iterate through all files in a directory
and to create new directories. These functions should be replaced by the
``17 file system API @cppfilesystem as soon as the framework minimum
standard is updated to ``17.

[Internal utilities](framework-redirect-module-inputs-outputs.md#internal-utilities) STL only provides string conversions for
standard types using `std::stringstream` and `std::to_string`, which do
not allow parsing strings encapsulated in pairs of double quote (`"`)
characters nor integrating different units. Furthermore it does not
provide wide flexibility to add custom conversions for other external
types in either way.

The Allpix² `to_string` and `from_string` methods provided by its
**string utilities** do allow for these flexible conversions, and are
extensively used in the configuration system. Conversions of numeric
types with a unit attached are automatically resolved using the unit
system discussed above. The string utilities also include trim and split
strings functions missing in the STL.

Furthermore, the Allpix² tool system contains extensions to allow
automatic conversions for ROOT and Geant4 types as explained in
Section [ROOT and Geant4 utilities](additional.md#root-and-geant4-utilities).