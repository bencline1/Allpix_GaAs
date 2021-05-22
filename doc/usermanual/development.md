---
template: overrides/main.html
title: "Module & Detector Development"
---

This chapter provides a few brief recipes for developing new simulation
modules and detector models for the Allpix² framework. Before starting
the development, the `CONTRIBUTING.md` file in the repository should be
consulted for further information on the development process, code
contributions and the preferred coding style for Allpix².

Coding and Naming Conventions
-----------------------------

The code base of the Allpix² is well-documented and follows concise
rules on naming schemes and coding conventions. This enables maintaining
a high quality of code and ensures maintainability over a longer period
of time. In the following, some of the most important conventions are
described. In case of doubt, existing code should be used to infer the
coding style from.

### Naming Schemes

The following coding and naming conventions should be adhered to when
writing code which eventually should be merged into the main repository.

*   **Namespace**: The `allpix` namespace is to be used for all classes which are part
    of the framework, nested namespaces may be defined. It is encouraged
    to make use of `using namespace allpix;` in implementation files
    only for this namespace. Especially the namespace `std` should
    always be referred to directly at the function to be called, e.g.
    `std::string test`. In a few other cases, such as `ROOT::Math`, the
    `using` directive may be used to improve readability of the code.

*   **Class names**: Class names are typeset in CamelCase, starting with a capital
    letter, e.g. `class ModuleManager`. Every class should provide
    sensible Doxygen documentation for the class itself as well as for
    all member functions.

*   **Member functions**: Naming conventions are different for public and private class
    members. Public member function names are typeset as camelCase names
    without underscores, e.g. `getElectricFieldType()`. Private member
    functions use lower-case names, separating individual words by an
    underscore, e.g. `create_detector_modules(...)`. This allows to
    visually distinguish between public and restricted access when
    reading code.

    In general, public member function names should follow the
    `get`/`set` convention, i.e. functions which retrieve
    information and alter the state of an object should be marked
    accordingly. Getter functions should be made `const` where possible
    to allow usage of constant objects of the respective class.

*   **Member variables**: Member variables of classes should always be private and accessed
    only via respective public member functions. This allows to change
    the class implementation and its internal members without requiring
    to rewrite code which accesses them. Member names should be typeset
    in lower-case letters, a trailing underscore is used to mark them as
    member variables, e.g. `bool terminate_`. This immediately sets them apart from local
    variables declared within a function.

### Formatting

A set of formatting rules is applied to the code base in order to avoid
unnecessary changes from different editors and to maintain readable
code. It is vital to follow these rules during development in order to
avoid additional changes to the code, just to adhere to the formatting.
There are several options to integrate this into the development
workflow:

-   Many popular editors feature direct integration either with
    `clang-format` or their own formatting facilities.

-   A build target called `make format` is provided if the
    `clang-format` tool is installed. Running this command before
    committing code will ensure correct formatting.

-   This can be further simplified by installing the *git hook* provided
    in the directory `/etc/git-hooks/`. A hook is a script called by
    `git` before a certain action. In this case, it is a pre-commit hook
    which automatically runs `clang-format` in the background and offers
    to update the formatting of the code to be committed. It can be
    installed by calling

    ```shell
    $ ./etc/git-hooks/install-hooks.sh
    ```

    once.

The formatting rules are defined in the `.clang-format` file in the
repository in machine-readable form (for `clang-format`, that is) but
can be summarized as follows:

-   The column width should be 125 characters, with a line break
    afterwards.

-   New scopes are indented by four whitespaces, no tab characters are
    to be used.

-   Namespaces are indented just as other code is.

-   No spaces should be introduced before parentheses ().

-   Included header files should be sorted alphabetically.

-   The pointer asterisk should be left-aligned, i.e. `int* foo` instead
    of `int *foo`.

The continuous integration automatically checks if the code adheres to
the defined format as described in Section [Continuous Integration](testing.md#continuous-integration).

Building Modules Outside the Framework
--------------------------------------

Allpix² provides CMake modules which allow to build modules for the
framework outside the actual code repository. The macros required to
build a module are provided through the CMake modules and are
automatically included when using the `FIND_PACKAGE(Allpix)` CMake
command. By this, modules can easily be moved into and out from the
module directory of the framework without requiring changes to its
`CMakeLists.txt`.

A minimal CMake setup for compiling and linking external modules to the
core and object library of the Allpix² framework is the following:

```cmake
CMAKE_MINIMUM_REQUIRED(VERSION 3.4.3 FATAL_ERROR)

FIND_PACKAGE(ROOT REQUIRED)
FIND_PACKAGE(Allpix REQUIRED)

ALLPIX_DETECTOR_MODULE(MODULE_NAME)
ALLPIX_MODULE_SOURCES(${MODULE_NAME} MySimulationModule.cpp)
```

It might be necessary to set the `CMAKE_CXX_STANDARD` according to the
settings used to build the framework. Additional libraries can be linked
to the module using the standard CMake command

```cmake
TARGET_LINK_LIBRARIES(${MODULE_NAME} MyExternalLibrary)
```

A more complete CMake structure, suited to host multiple external
modules, is provided in a separate repository[^43].

In order to load modules which have been compiled and installed in a
different location than the ones shipped with the framework itself, the
respective search path has to be configured properly in the Allpix² main
configuration file:

```toml
[Allpix]
# Library search paths
library_directories = "~/allpix-modules/build", "/opt/apsq-modules"
```

The relevant parameter is described in detail in
Section [Framework parameters](getting_started.md#framework-parameters).

Implementing a New Module
-------------------------

Owing to its modular structure, the functionality of the Allpix² can
easily be extended by adding additional modules which can be placed in
the simulation chain. Since the framework serves a wide community,
modules should be as generic as possible, i.e. not only serve the
simulation of a single detector prototype but implement the necessary
algorithms such that they are re-usable for other applications.
Furthermore, it may be beneficial to split up modules to support the
modular design of Allpix².

Before starting the development of a new module, it is essential to
carefully read the documentation of the framework module manager which
can be found in Section [Modules and the Module Manager](framework-modules-manager.md). The basic steps to
implement a new module, hereafter referred to as `ModuleName`, are the
following:

1.  Initialization of the code for the new module, using the script
    `/etc/scripts/makemodule.sh` in the repository. The script will ask
    for the name of the model and the type (unique or
    detector-specific). It creates the directory with a minimal example
    to get started together with the rough outline of its documentation
    in *README.md*.

2.  Before starting to implement the actual module, it is recommended to
    update the introductory documentation in *README.md*. This
    Markdown-formatted file[^44] is automatically included in the user manual.
    Formulae can be included by enclosure
    in Dollar-backtick markers, i.e. \`$` E(z) = 0`$\`. The Doxygen
    documentation in *`ModuleName`.hpp* should also be extended to
    provide a basic description of the module.

3.  Finally, the constructor and `initialize`, `run` and/or `finalize` methods
    can be written, depending on the requirements of the new module.

Additional sources of documentation which may be useful during the
development of a module include:

-   The framework documentation in Chapter [Structure & Components of the Framework](framework.md) for an
    introduction to the different parts of the framework.

-   The module documentation in Chapter [Modules](modules.md) for a description
    of the functionality of other modules already implemented, and to
    look for similar modules which can help during development.

-   The [Doxygen (core) reference documentation](/codereference/index.html) included in the
    framework.

-   The latest version of the source code of all modules and the Allpix²
    core itself.

Any module potentially useful for other users should be contributed back
to the main repository after is has been validated. It is strongly
encouraged to send a merge-request through the mechanism provided by the
software repository[^11].

### Files of a Module

Every module directory should at minimum contain the following documents
(with `ModuleName` replaced by the name of the module):

-   **CMakeLists.txt**: The build script to load the dependencies and
    define the source files of the library.

-   **README.md**: Full documentation of the module.

-   **`ModuleName`Module.hpp**: The header file of the module.

-   **`ModuleName`Module.cpp**: The implementation file of the module.

These files are discussed in more detail below. By default, all modules
added to the `/src/modules/` directory will be built automatically by
CMake. If a module depends on additional packages which not every user
may have installed, one can consider adding the following line to the
top of the module’s *CMakeLists.txt*:

```cmake
ALLPIX_ENABLE_DEFAULT(OFF)
```

General guidelines and instructions for implementing new modules are
provided in Section [Implementing a New Module](development.md#implementing-a-new-module).

##### CMakeLists.txt

Contains the build description of the module with the following
components:

1.  On the first line either `ALLPIX_DETECTOR_MODULE(MODULE_NAME)` or
    `ALLPIX_UNIQUE_MODULE(MODULE_NAME)` depending on the type of module
    defined. The internal name of the module is automatically saved in
    the variable `${MODULE_NAME}` which should be used as an argument to other functions.
    Another name can be used by overwriting the variable content, but in the examples below, `${MODULENAME}` is used exclusively and is the preferred method of implementation.

2.  The following lines should contain the logic to load possible
    dependencies of the module (below is an example to load *Geant4*).
    Only ROOT is automatically included and linked to the module.

3.  A line with `ALLPIX_MODULE_SOURCES(${MODULE_NAME} sources)` defines
    the module source files. Here, `sources` should be replaced by a
    list of all source files relevant to this module.

4.  Possible lines to include additional directories and to link
    libraries for dependencies loaded earlier.

5.  A line with `ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME})` adds the *Geant4* interface library as explained in Section [Geant4 Interface](geant4-interface.md).

6.  A line containing `ALLPIX_MODULE_INSTALL(${MODULE_NAME})` to set up the required target for the module to be installed to.

A simple *CMakeLists.txt* for a module named `Test` which requires *Geant4*
is provided below as an example.

```cmake
# Define module and save name to MODULE_NAME
# Replace by ALLPIX_DETECTOR_MODULE(MODULE_NAME) to define a detector module
ALLPIX_UNIQUE_MODULE(MODULE_NAME)

# Load Geant4
FIND_PACKAGE(Geant4)
IF(NOT Geant4_FOUND)
    MESSAGE(FATAL_ERROR "Could not find Geant4, make sure to source the Geant4 environment\n$ source YOUR_GEANT4_DIR/bin/geant4.sh")
ENDIF()

# Add the sources for this module
ALLPIX_MODULE_SOURCES(${MODULE_NAME}
    TestModule.cpp
)

# Add Geant4 to the include directories
TARGET_INCLUDE_DIRECTORIES(${MODULE_NAME} SYSTEM PRIVATE ${Geant4_INCLUDE_DIRS})

# Allpix Geant4 interface is required for this module
ALLPIX_MODULE_REQUIRE_GEANT4_INTERFACE(${MODULE_NAME})

# Link the Geant4 libraries to the module library
TARGET_LINK_LIBRARIES(${MODULE_NAME} ${Geant4_LIBRARIES})

# Provide standard install target
ALLPIX_MODULE_INSTALL(${MODULE_NAME})
```

##### README.md

The `README.md` serves as the documentation for the module and should be
written in Markdown format[^44]. It is automatically included in the user manual in
Chapter [Modules](modules.md). By documenting the module functionality in
Markdown, the information is also viewable with a web browser in the
repository within the module sub-folder.

The `README.md` should follow the structure indicated in the `README.md`
file of the `DummyModule` in `/src/modules/Dummy`, and should contain at
least the following sections:

-   The H1-size header with the name of the module and at least the
    following required elements: the **Maintainer** and the **Status**
    of the module. If the module is working and well-tested, the status
    of the module should be *Functional*. By default, new modules are
    given the status *Immature*. The maintainer should mention the
    full name of the module maintainer, with their email address in
    parentheses. A minimal header is therefore:

    ```
    # ModuleName
    Maintainer: Example Author (<example@example.org>)
    Status: Functional
    ```

    In addition, the **Input** and **Output** objects to be received and
    dispatched by the module should be mentioned.

-   An H3-size section named **Description**, containing a short
    description of the module.

-   An H3-size section named **Parameters**, with all available
    configuration parameters of the module. The parameters should be
    briefly explained in an itemized list with the name of the parameter
    set as an inline code block.

-   An H3-size section with the title **Usage** which should contain at
    least one simple example of a valid configuration for the module.

##### `ModuleName`Module.hpp and `ModuleName`Module.cpp

All modules should consist of both a header file and a source file. In
the header file, the module is defined together with all of its methods.
Brief Doxygen documentation should be added to explain what each method
does. The source file should provide the implementation of every method
and also its more detailed Doxygen documentation. Methods should only be
declared in the header and defined in the source file in order to keep
the interface clean.

### Module structure

All modules must inherit from the `Module` base class, which can be
found in `/src/core/module/Module.hpp`. The module base class provides
two base constructors, a few convenient methods and several methods
which the user is required to override. Each module should provide a
constructor using the fixed set of arguments defined by the framework;
this particular constructor is always called during by the module
instantiation logic. These arguments for the constructor differ for
unique and detector modules.

For unique modules, the constructor for a `TestModule` should be:

```cpp
TestModule(Configuration& config, Messenger* messenger, GeometryManager* geo_manager): Module(config)
{}
```

For detector modules, the first two arguments are the same, but the last
argument is a `std::shared_ptr` to the linked detector. It should always
forward this detector to the base class together with the configuration
object. Thus, the constructor of a detector module is:

```cpp
TestModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector): Module(config, std::move(detector))
{}
```

The pointer to a Messenger can be used to bind variables to either
receive or dispatch messages as explained in
Section [Passing Objects using Messages](framework-passing-objects-using-messages.md). The constructor should be used to bind
required messages, set configuration defaults and to throw exceptions in
case of failures. Unique modules can access the GeometryManager to fetch
all detector descriptions, while detector modules directly receive a
link to their respective detector.

In addition to the constructor, each module can override the following
methods:

-   `initialize()`: Called once per module from the main thread after loading and constructing all modules and before starting the event loop. This method can for example be used to initialize histograms.

-   `initializeThread()`: Called after global initialization but before event processing and gives
    the possibility to initialize worker thread-specific members for modules if multi-threading is used.

-   `run(Event* event)`: Called for every event in the simulation, with a pointer to the current event
    object as parameter. An exception should be thrown for serious errors, otherwise a warning should be logged.

-   `finalizeThread()`: Called for each worker thread after processing all events in the run by each
    worker thread separately if multi-threading is used.

-   `finalize()`: Called once per module from the main thread after processing all events in the run and before destructing the module. Typically used to save the output
    data (like histograms). Any exceptions should be thrown from here
    instead of the destructor.

If necessary, modules can also access the ConfigurationManager directly
in order to obtain configuration information from other module instances
or other modules in the framework using the `getConfigManager()` call.
This allows to retrieve and e.g. store the configuration actually used
for the simulation alongside the data.

If a module should be run using multi-threading but requires to execute its run method in the order of event numbers, for example a module that writes to an output file, then the module can inherit from the `SequentialModule` class, without implementing additional functionality.
This will ensure that the run method will receive events one-by-one and in the correct sequence.


Writing Thread-Safe Code
------------------------

In Allpix² events are processed fully parallel which requires some consideration when writing module code.
This section briefly lists the most important aspects to take into account.

#### Member Variables

While the `initialize()` and `finalize()` of the module are guaranteed to be called sequentially, the `run()` method will be called simultaneously from different threads and for different events.
Therefore, no module data members must be altered from within the `run()` function, otherwise these changes will affect other events being processed in parallel.
Configuration parameters cached as member variables should therefore be set only in the `initialize()` function.

For initialization and finalization of thread-local data members, i.e. structures which have to be configured for each of the worker threads the module is executed on, the `initializeThread()` and `finalizeThread()` methods are available. They are called once on each worker thread after the `initialize()` and before the `finalize()` methods, respectively.

#### Histograms

Allpix² uses ROOT histograms for collecting and storing statistics and other additional information about the simulation process.
ROOT provides the template class `ROOT::TThreadedObject` which allows to use histograms in multi-threaded environments but slightly alters the interface of the histogram objects.
Furthermore, there have been significant changes to the class between minor release version of ROOT and it doesn't scale very well with a large number of predefined threads.
Therefore, Allpix² provides its own re-implementation of this class, `allpix::ThreadedHistogram` which also restores the original interface of the histogram classes, i.e. it is possible to instantiate, fill and store histograms the same way as in a single-threaded environment.

This class can be used as follows:
```cpp
// Declaration of a new histogram of type "TH1D"
Histogram<TH1D> my_histogram;

// Creation of the histogram using the CreateHistogram helper method:
my_histogram = CreateHistogram<TH1D>("name", "title", 100, 0., 100.);

// Filling, setting bin contents and writing the histogram works as before:
my_histogram->Fill(12.);
my_histogram->SetBinContent(15, 23.);
my_histogram->Write();
```

#### Declaring a Module Thread-Safe

If a module is thread-safe, i.e. its `run()` function can be called from different threads in parallel without locking, it can be declared as thread-safe to the framework.
In this case the ModuleManager will allow parallelization of calls to this module.

This declaration is done by placing the following call in the constructor of the module:
```cpp
  MyParallelModule::MyParallelModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
      : Module(std::move(config), std::move(detector)) {
      // This module is thread-safe and can be called from different threads simultaneously:
      enable_parallelization();
}
```

By adding this statement, the module certifies to work correctly if its `run()` method is executed multiple times in parallel, for different events.
This means in particular that the module will safely handle access to shared (for example static) variables as described in Section [Member Variables](#member-variables) and that it will properly assign and bind ROOT histograms to their respective directories in the output ROOT file before the event processing starts and the `run()` method is called the first time.
Access to constant operations in the GeometryManager, Detector and DetectorModel is always valid between various threads. In addition, sending and receiving messages is thread-safe.


Adding a New Detector Model
---------------------------

Custom detector models based on the detector classes provided with
Allpix² can easily be added to the framework. In particular
Section [Detector models](framework-geometry-detectors.md#detector-models) explains all parameters of the detector
models currently available. The default models provided in the `/models`
directory of the repository can serve as examples. To create a new
detector model, the following steps should be taken:

1.  Create a new file with the name of the model followed by the `.conf`
    suffix (for example `your_model.conf`).

2.  Add a configuration parameter `type` with the type of the model, at
    the moment either ’monolithic’ or ’hybrid’ for respectively
    monolithic sensors or hybrid models with bump bonds and a separate
    readout chip.

3.  Add all required parameters and possibly optional parameters as
    explained in Section [Detector models](framework-geometry-detectors.md#detector-models).

4.  Include the detector model in the search path of the framework by
    adding the `model_paths` parameter to the general setting of the main
    configuration (see Section [Framework parameters](getting_started.md#framework-parameters)), pointing
    either directly to the detector model file or the directory
    containing it.

	!!! note
	    Files in this path will overwrite models with the same name in the default model folder.

Models should be contributed to the main repository to make them
available to other users of the framework. To add the detector model to
the framework the configuration file should be moved to the `/models`
folder of the repository. The file should then be added to the
installation target in the `CMakeLists.txt` file of the `/models`
directory. Afterwards, a merge-request can be created via the mechanism
provided by the software repository[^11].

[^11]:The Allpix² Project Repository. Aug. 2, 2017. url: [https://gitlab.cern.ch/allpix-squared/allpix-squared/](https://gitlab.cern.ch/allpix-squared/allpix-squared/).
[^43]:CMake for External Allpix² Modules Repository. Sept. 20, 2019. url: [https://gitlab.cern.ch/allpix-squared/external-modules/](https://gitlab.cern.ch/allpix-squared/external-modules/).
[^44]:John Gruber and Aaron Swartz. Markdown. url: [https://daringﬁreball.net/projects/markdown/](https://daringﬁreball.net/projects/markdown/).
[^45]:John MacFarlane. Pandoc. A universal document converter. url: [http://pandoc.org/](http://pandoc.org/).
