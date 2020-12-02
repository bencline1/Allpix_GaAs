---
template: overrides/main.html
title: "Structure & Components of the Framework"
---

This chapter details the technical implementation of the Allpix²
framework and is mostly intended to provide insight into the gearbox to
potential developers and interested users. The framework consists of the
following four main components that together form Allpix²:

1.  **Core**: The core contains the internal logic to initialize the
    modules, provide the geometry, facilitate module communication and
    run the event sequence. The core keeps its dependencies to a minimum
    (it only relies on ROOT) and remains independent from the other
    components as far as possible. It is the main component discussed in
    this section.

2.  **Modules**: A module is a set of methods which is executed as part
    of the simulation chain. Modules are built as separate libraries and
    loaded dynamically on demand by the core. The available modules and
    their parameters are discussed in detail in Chapter [Modules](modules.md).

3.  **Objects**: Objects form the data passed between modules using the
    message framework provided by the core. Modules can listen and bind
    to messages with objects they wish to receive. Messages are
    identified by the object type they are carrying, but can also be
    renamed to allow the direction of data to specific modules,
    facilitating more sophisticated simulation setups. Messages are
    intended to be read-only and a copy of the data should be made if a
    module wishes to change the data. All objects are compiled into a
    separate library which is automatically linked to every module. More
    information about the messaging system and the supported objects can
    be found in Section [Passing Objects using Messages](framework-passing-objects-using-messages.md).

4.  **Tools**: Allpix² provides a set of header-only ’tools’ that allow
    access to common logic shared by various modules. Examples are the
    Runge-Kutta solver[^19] implemented using the Eigen3 library
    and the set of template specializations for ROOT and Geant4
    configurations. More information about the tools can be found in
    Chapter [Additional Tools & Resources](additional.md). This set of tools is
    different from the set of core utilities the framework itself
    provides, which is part of the core and explained in
    Section [Logging and other Utilities](framework-logging-other-utilities.md).

Finally, Allpix² provides an executable which instantiates the core of
the framework, receives and distributes the configuration object and
runs the simulation chain.

The chapter is structured as follows. Section [Architecture of the Core](framework.md#architecture-of-the-core) provides an
overview of the architectural design of the core and describes its
interaction with the rest of the Allpix² framework. The different
subcomponents such as configuration, modules and messages are discussed
in Sections [Configuration and Parameters](framework.md#configuration-and-parameters)–[Passing Objects using Messages](framework-passing-objects-using-messages.md). The
chapter closes with a description of the available framework tools in
Section [Logging and other Utilities](framework-logging-other-utilities.md). Some C++ code will be provided in the
text, but readers not interested may skip the technical details.

Architecture of the Core
------------------------

The core is constructed as a light-weight framework which provides
various subsystems to the modules. It contains the part of the software
responsible for instantiating and running the modules from the supplied
configuration file, and is structured around five subsystems, of which
four are centered around a manager and the fifth contains a set of
general utilities. The systems provided are:

1.  **Configuration**: The configuration subsystem provides a
    configuration object from which data can be retrieved or stored,
    together with a TOML-like[^20] parser to read configuration
    files. It also contains the Allpix² configuration manager which
    provides access to the main configuration file and its sections. It
    is used by the module manager system to find the required
    instantiations and access the global configuration. More information
    is given in Section [Configuration and Parameters](framework.md#configuration-and-parameters).

2.  **Module**: The module subsystem contains the base class of all
    Allpix² modules as well as the manager responsible for loading and
    executing the modules (using the configuration system). This
    component is discussed in more detail in
    Section [Modules and the Module Manager](framework-modules-manager.md).

3.  **Geometry**: The geometry subsystem supplies helpers for the
    simulation geometry. The manager instantiates all detectors from the
    detector configuration file. A detector object contains the position
    and orientation linked to an instantiation of a particular detector
    model, itself containing all parameters describing the geometry of
    the detector. More details about geometry and detector models is
    provided in Section [Geometry and Detectors](framework-geometry-detectors.md).

4.  **Messenger**: The messenger is responsible for sending objects from
    one module to another. The messenger object is passed to every
    module and can be used to bind to messages to listen for. Messages
    with objects are also dispatched through the messenger as described
    in Section [Passing Objects using Messages](framework-passing-objects-using-messages.md).

5.  **Utilities**: The framework provides a set of utilities for
    logging, file and directory access, and unit conversion. An
    explanation on how to use of these utilities can be found in
    Section [Logging and other Utilities](framework-logging-other-utilities.md). A set of C++ exceptions is also
    provided in the utilities, which are inherited and extended by the
    other components. Proper use of exceptions, together with logging
    information and reporting errors, makes the framework easier to use
    and debug. A few notes about the use and structure of exceptions are
    provided in Section [Error Reporting and Exceptions](framework-error-reporting-exceptions.md).

Configuration and Parameters
----------------------------

Individual modules as well as the framework itself are configured
through configuration files, which all follow the same format.
Explanations on how to use the various configuration files together with
several examples have been provided in
Section [Conﬁguration Files](getting_started.md#configuration-files).

### File format

Throughout the framework, a simplified version of TOML[^20] is used
as standard format for configuration files. The format is defined as
follows:

1.  All whitespace at the beginning or end of a line are stripped by the
    parser. In the rest of this format specification the *line* refers
    to the line with this whitespace stripped.

2.  Empty lines are ignored.

3.  Every non-empty line should start with either `#`, `[` or an
    alphanumeric character. Every other character should lead to an
    immediate parse error.

4.  If the line starts with a hash character (`#`), it is interpreted as
    comment and all other content on the same line is ignored.

5.  If the line starts with an open square bracket (`[`), it indicates a
    section header (also known as configuration header). The line should
    contain a string with alphanumeric characters and underscores,
    indicating the header name, followed by a closing square bracket
    (`]`), to end the header. After any number of ignored whitespace
    characters there could be a `#` character. If this is the case, the
    rest of the line is handled as specified in point 3. Otherwise there
    should not be any other character (except the whitespace) on the
    line. Any line that does not comply to these specifications should
    lead to an immediate parse error. Multiple section headers with the
    same name are allowed. All key-value pairs following this section
    header are part of this section until a new section header is
    started.

6.  If the line starts with an alphanumeric character, the line should
    indicate a key-value pair. The beginning of the line should contain
    a string of alphabetic characters, numbers, dots (`.`), colons (`:`)
    and underscores (`_`), but it may only start with an alphanumeric
    character. This string indicates the ’key’. After an optional number
    of ignored whitespace, the key should be followed by an equality
    sign (`=`). Any text between the `=` and the first `#` character not
    enclosed within a pair of single or double quotes (`’` or `"`) is
    known as the non-stripped string. Any character after the `#` is
    handled as specified in point 3. If the line does not contain any
    non-enclosed `#` character, the value ends at the end of the line
    instead. The ’value’ of the key-value pair is the non-stripped
    string with all whitespace in front and at the end stripped. The
    value may not be empty. Any line that does not comply to these
    specifications should lead to an immediate parse error.

7.  The value may consist of multiple nested dimensions which are
    grouped by pairs of square brackets (`[` and `]`). The number of
    square brackets should be properly balanced, otherwise an error is
    raised. Square brackets which should not be used for grouping should
    be enclosed in quotation marks. Every dimension is split at every
    whitespace sequence and comma character (`,`) not enclosed in
    quotation marks. Implicit square brackets are added to the begin and
    end of the value, if these are not explicitly added. A few
    situations require explicit addition of outer brackets such as
    matrices with only one column element, i.e. with dimension 1xN.

8.  The sections of the value which are interpreted as separate entities
    are named elements. For a single value the element is on the zeroth
    dimension, for arrays on the first dimension and for matrices on the
    second dimension. Elements can be forced by using quotation marks,
    either single or double quotes (`’` or `"`). The number of both
    types of quotation marks should be properly balanced, otherwise an
    error is raised. The conversion to the elements to the actual type
    is performed when accessing the value.

9.  All key-value pairs defined before the first section header are part
    of a zero-length empty section header.

### Accessing parameters

Values are accessed via the configuration object. In the following
example, the key is a string called `key`, the object is named `config`
and the type `TYPE` is a valid C++ type the value should represent. The
values can be accessed via the following methods:

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
// Returns true if the key exists and false otherwise
config.has("key")
// Returns the number of keys found from the provided initializer list:
config.count({"key1", "key2", "key3"});
// Returns the value in the given type, throws an exception if not existing or a conversion to TYPE is not possible
config.get<TYPE>("key")
// Returns the value in the given type or the provided default value if it does not exist
config.get<TYPE>("key", default_value)
// Returns an array of elements of the given type
config.getArray<TYPE>("key")
// Returns a matrix: an array of arrays of elements of the given type
config.getMatrix<TYPE>("key")
// Returns an absolute (canonical if it should exist) path to a file
config.getPath("key", true /* check if path exists */)
// Return an array of absolute paths
config.getPathArray("key", false /* do not check if paths exists */)
// Returns the value as literal text including possible quotation marks
config.getText("key")
// Set the value of key to the default value if the key is not defined
config.setDefault("key", default_value)
// Set the value of the key to the default array if key is not defined
config.setDefaultArray<TYPE>("key", vector_of_default_values)
// Create an alias named new_key for the already existing old_key or throws an exception if the old_key does not exist
config.setAlias("new_key", "old_key")
```

Conversions to the requested type are using the `fromstring` and
`tostring` methods provided by the string utility library described in
Section [Internal utilities](framework-redirect-module-inputs-outputs.md#internal-utilities). These conversions largely follow
standard C++ parsing, with one important exception. If (and only if) the
value is retrieved as a C/C++ string and the string is fully enclosed by a
pair of `"` characters, these are stripped before returning the value.
Strings can thus also be provided with or without quotation marks.

!!! note
    A conversion from string to the requested type is a comparatively heavy operation. For performance-critical sections of the code, one should consider fetching the configuration value once and caching it in a local variable.

[^19]:Erwin Fehlberg. Low-order classical Runge-Kutta formulas with stepsize control and their application to some heat transfer problems. NASA Technical Report NASA-TR-R-315. [http://hdl.handle.net/2060/19690021375](http://hdl.handle.net/2060/19690021375). 1969.
[^20]:Tom Preston-Werner. TOML. Tom’s Obvious, Minimal Language. url: [https://github.com/toml-lang/toml](https://github.com/toml-lang/toml).