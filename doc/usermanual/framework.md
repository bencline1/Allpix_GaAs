Structure & Components of the Framework
=======================================

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
    be found in Section [Passing Objects using Messages](framework.md#passing-objects-using-messages).

4.  **Tools**: Allpix² provides a set of header-only ’tools’ that allow
    access to common logic shared by various modules. Examples are the
    Runge-Kutta solver @fehlberg implemented using the Eigen3 library
    and the set of template specializations for ROOT and Geant4
    configurations. More information about the tools can be found in
    Chapter [Additional Tools & Resources](additional.md). This set of tools is
    different from the set of core utilities the framework itself
    provides, which is part of the core and explained in
    Section [Logging and other Utilities](framework.md#logging-and-other-utilities).

Finally, Allpix² provides an executable which instantiates the core of
the framework, receives and distributes the configuration object and
runs the simulation chain.

The chapter is structured as follows. Section [Architecture of the Core](framework.md#architecture-of-the-core) provides an
overview of the architectural design of the core and describes its
interaction with the rest of the Allpix² framework. The different
subcomponents such as configuration, modules and messages are discussed
in Sections [Configuration and Parameters](framework.md#configuration-and-parameters)–[Passing Objects using Messages](framework.md#passing-objects-using-messages). The
chapter closes with a description of the available framework tools in
Section [Logging and other Utilities](framework.md#logging-and-other-utilities). Some code will be provided in the
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
    together with a TOML-like @tomlgit parser to read configuration
    files. It also contains the Allpix² configuration manager which
    provides access to the main configuration file and its sections. It
    is used by the module manager system to find the required
    instantiations and access the global configuration. More information
    is given in Section [Configuration and Parameters](framework.md#configuration-and-parameters).

2.  **Module**: The module subsystem contains the base class of all
    Allpix² modules as well as the manager responsible for loading and
    executing the modules (using the configuration system). This
    component is discussed in more detail in
    Section [Modules and the Module Manager](framework.md#modules-and-the-module-manager).

3.  **Geometry**: The geometry subsystem supplies helpers for the
    simulation geometry. The manager instantiates all detectors from the
    detector configuration file. A detector object contains the position
    and orientation linked to an instantiation of a particular detector
    model, itself containing all parameters describing the geometry of
    the detector. More details about geometry and detector models is
    provided in Section [Geometry and Detectors](framework.md#geometry-and-detectors).

4.  **Messenger**: The messenger is responsible for sending objects from
    one module to another. The messenger object is passed to every
    module and can be used to bind to messages to listen for. Messages
    with objects are also dispatched through the messenger as described
    in Section [Passing Objects using Messages](framework.md#passing-objects-using-messages).

5.  **Utilities**: The framework provides a set of utilities for
    logging, file and directory access, and unit conversion. An
    explanation on how to use of these utilities can be found in
    Section [Logging and other Utilities](framework.md#logging-and-other-utilities). A set of exceptions is also
    provided in the utilities, which are inherited and extended by the
    other components. Proper use of exceptions, together with logging
    information and reporting errors, makes the framework easier to use
    and debug. A few notes about the use and structure of exceptions are
    provided in Section [Error Reporting and Exceptions](framework.md#error-reporting-and-exceptions).

Configuration and Parameters
----------------------------

Individual modules as well as the framework itself are configured
through configuration files, which all follow the same format.
Explanations on how to use the various configuration files together with
several examples have been provided in
Section [Conﬁguration Files](getting_started.md#configuration-files).

### File format

Throughout the framework, a simplified version of TOML @tomlgit is used
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
    a string of alphabetic characters, numbers, dots (`.`), colons (``)
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
and the type `TYPE` is a valid type the value should represent. The
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
Section [Internal utilities](framework.md#internal-utilities). These conversions largely follow
standard parsing, with one important exception. If (and only if) the
value is retrieved as a C/C++ string and the string is fully enclosed by a
pair of `"` characters, these are stripped before returning the value.
Strings can thus also be provided with or without quotation marks.

It should be noted that a conversion from string to the requested type
is a comparatively heavy operation. For performance-critical sections of
the code, one should consider fetching the configuration value once and
caching it in a local variable.

Modules and the Module Manager
------------------------------

Allpix² is a modular framework and one of the core ideas is to partition
functionality in independent modules which can be inserted or removed as
required. These modules are located in the subdirectory *src/modules/*
of the repository, with the name of the directory the unique name of the
module. The suggested naming scheme is CamelCase, thus an example module
name would be *GenericPropagation*. There are two different kind of
modules which can be defined:

-   **Unique**: Modules for which a single instance runs, irrespective
    of the number of detectors.

-   **Detector**: Modules which are concerned with only a single
    detector at a time. These are then replicated for all required
    detectors.

The type of module determines the constructor used, the internal unique
name and the supported configuration parameters. For more details about
the instantiation logic for the different types of modules, see
Section [Module instantiation](framework.md#module-instantiation).

### Module instantiation

Modules are dynamically loaded and instantiated by the Module Manager.
They are constructed, initialized, executed and finalized in the linear
order in which they are defined in the configuration file; for this
reason the configuration file should follow the order of the real
process. For each section in the main configuration file
(see [Configuration and Parameters](framework.md#configuration-and-parameters) for more details), a corresponding library
is searched for which contains the module (the exception being the
global framework section). Module libraries are always named following
the scheme **libAllpixModule`ModuleName`**, reflecting the `ModuleName`
configured via CMake. The module search order is as follows:

1.  Modules already loaded before from an earlier section header

2.  All directories in the global configuration parameter
    `librarydirectories` in the provided order, if this parameter
    exists.

3.  4.  The internal library paths of the executable, that should
    automatically point to the libraries that are built and installed
    together with the executable. These library paths are stored in
    `RPATH` on Linux, see the next point for more information.

5.  The other standard locations to search for libraries depending on
    the operating system. Details about the procedure Linux follows can
    be found in @linuxld.

If the loading of the module library is successful, the module is
checked to determine if it is a unique or detector module. As a single
module may be called multiple times in the configuration, with
overlapping requirements (such as a module which runs on all detectors
of a given type, followed by the same module but with different
parameters for one specific detector, also of this type) the Module
Manager must establish which instantiations to keep and which to
discard. The instantiation logic determines a unique name and priority,
where a lower number indicates a higher priority, for every
instantiation. The name and priority for the instantiation are
determined differently for the two types of modules:

-   **Unique**: Combination of the name of the module and the `input`
    and `output` parameter (both defaulting to an empty string). The
    priority is always zero.

-   **Detector**: Combination of the name of the module, the `input` and
    `output` parameter (both defaulting to an empty string) and the name
    of detector this module is executed for. If the name of the detector
    is specified directly by the `name` parameter, the priority is
    `high`. If the detector is only matched by the `type` parameter, the
    priority is `medium`. If the `name` and `type` are both unspecified
    and the module is instantiated for all detectors, the priority is
    `low`.

In the end, only a single instance for every unique name is allowed. If
there are multiple instantiations with the same unique name, the
instantiation with the highest priority is kept. If multiple
instantiations with the same unique name and the same priority exist, an
exception is raised.

### Parallel execution of modules

The framework has experimental support for running several modules in
parallel. This feature is disabled for new modules by default, and has
to be both supported by the module and enabled by the user as described
in Section [Framework parameters](getting_started.md#framework-parameters). A significant speed improvement
can be achieved if the simulation contains multiple detectors or
simulates the same module using different parameters.

The framework allows to parallelize the execution of the same type of
module, if these would otherwise be executed directly after each other
in a linear order. Thus, as long as the name of the module remains the
same, while going through the execution order of all `run()` methods,
all instances are added to a work queue. The instances are then
distributed to a set of worker threads as specified in the configuration
or determined from system parameters, which will execute the individual
modules. The module manager will wait for all jobs to finish before
continuing to process the next type of module.

To enable parallelization for a module, the following line of code has
to be added to the constructor of a module:

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
// Enable parallelization of this module if multithreading is enabled
enable_parallelization();
```

By adding this, the module promises that it will work correctly if the
run-method is executed multiple times in parallel, in separate
instantiations. This means in particular that the module will safely
handle access to shared (for example static) variables and it will
properly bind ROOT histograms to their directory before the
`run()`-method. Access to constant operations in the GeometryManager,
Detector and DetectorModel is always valid between various threads. In
addition, sending and receiving messages is thread-safe.

Geometry and Detectors
----------------------

Simulations are frequently performed for a set of different detectors
(such as a beam telescope and a device under test). All of these
individual detectors together form what Allpix² defines as the geometry.
Each detector has a set of properties attached to it:

-   A unique detector `name` to refer to the detector in the
    configuration.

-   The `position` in the world frame. This is the position of the
    geometric center of the sensitive device (sensor) given in world
    coordinates as X, Y and Z as defined in
    Section [Coordinate systems](framework.md#coordinate-systems) (note that any additional
    components like the chip and possible support layers are ignored
    when determining the geometric center).

-   An `orientationmode` that determines the way that the orientation is
    applied. This can be either `xyz`, `zyx` or `zxz`, where **`xyz` is
    used as default if the parameter is not specified**. Three angles
    are expected as input, which should always be provided in the order
    in which they are applied.

    -   The `xyz` option uses extrinsic Euler angles to apply a rotation
        around the global $X$ axis, followed by a rotation around the
        global $Y$ axis and finally a rotation around the global $Z$
        axis.

    -   The `zyx` option uses the **extrinsic Z-Y-X convention** for
        Euler angles, also known as Pitch-Roll-Yaw or 321 convention.
        The rotation is represented by three angles describing first a
        rotation of an angle $\phi$ (yaw) about the $Z$ axis, followed
        by a rotation of an angle $\theta$ (pitch) about the initial $Y$
        axis, followed by a third rotation of an angle $\psi$ (roll)
        about the initial $X$ axis.

    -   The `zxz` uses the **extrinsic Z-X-Z convention** for Euler
        angles instead. This option is also known as the 3-1-3 or the
        “x-convention” and the most widely used definition of Euler
        angles @eulerangles.

    It is highly recommended to always explicitly state the orientation
    mode instead of relying on the default configuration.

-   The `orientation` to specify the Euler angles in logical order (e.g.
    first $X$, then $Y$, then $Z$ for the `xyz` method), interpreted
    using the method above (or with the `xyz` method if the
    `orientationmode` is not specified). An example for three Euler
    angles would be

    ``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    orientation_mode = "zyx"
    orientation = 45deg 10deg 12deg
    ```

    which describes the rotation of around the $Z$ axis, followed by a
    rotation around the initial $Y$ axis, and finally a rotation of
    around the initial $X$ axis.

    All supported rotations are extrinsic active rotations, i.e. the
    vector itself is rotated, not the coordinate system. All angles in
    configuration files should be specified in the order they will be
    applied.

-   A `type` parameter describing the detector model, for example
    `timepix` or `mimosa26`. These models define the geometry and
    parameters of the detector. Multiple detectors can share the same
    model, several of which are shipped ready-to-use with the framework.

-   An optional parameter `alignmentprecisionposition` to specify the
    alignment precision along the three global axes as described in
    Section [Detector configuration](getting_started.md#detector-configuration).

-   An optional parameter `alignmentprecisionorientation` for the
    alignment precision in the three rotation angles as described in
    Section [Detector configuration](getting_started.md#detector-configuration).

-   An optional **electric field** in the sensitive device. An electric
    field can be added to a detector by a special module as demonstrated
    in Section [Electric Fields](getting_started.md#electric-fields).

The detector configuration is provided in the detector configuration
file as explained in Section [Detector configuration](getting_started.md#detector-configuration).

### Coordinate systems

Local coordinate systems for each detector and a global frame of
reference for the full setup are defined. The global coordinate system
is chosen as a right-handed Cartesian system, and the rotations of
individual devices are performed around the geometrical center of their
sensor.

Local coordinate systems for the detectors are also right-handed
Cartesian systems, with the x- and y-axes defining the sensor plane. The
origin of this coordinate system is the center of the lower left pixel
in the grid, i.e. the pixel with indices (0,0). This simplifies
calculations in the local coordinate system as all positions can either
be stated in absolute numbers or in fractions of the pixel pitch.

A sketch of the actual coordinate transformations performed, including
the order of transformations, is provided in
Figure [fig:transformations]. The global coordinate system used for
tracking of particles through detetector setup is shown on the left
side, while the local coordinate system used to describe the individual
sensors is located at the right.

[fig:transformations]
*Caption Coordinate transformations from global to local and revers. The first row shows the detector positions in the respective coordinate systems in top view, the second row in side view.*

The global reference for time measurements is the beginning of the event, i.e. the start of the particle tracking through the setup.
The local time reference is the time of entry of the *first* primary particle of the event into the sensor.
This means that secondary particles created within the sensor inherit the local time reference from their parent particles in order to have a uniform time reference in the sensor.
It should be noted that Monte Carlo particles that start the local time frame on different detectors do not necessarily have to belong to the same particle track.


### Changing and accessing the geometry

The geometry is needed at a very early stage because it determines the
number of detector module instantiations as explained in
Section [Module instantiation](framework.md#module-instantiation). The procedure of finding and
loading the appropriate detector models is explained in more detail in
Section [Detector models](framework.md#detector-models).

The geometry is directly added from the detector configuration file
described in Section [Detector configuration](getting_started.md#detector-configuration). The geometry manager parses
this file on construction, and the detector models are loaded and linked
later during geometry closing as described above. It is also possible to
add additional models and detectors directly using `addModel` and
`addDetector` (before the geometry is closed). Furthermore it is
possible to add additional points which should be part of the world
geometry using `addPoint`. This can for example be used to add the beam
source to the world geometry.

The detectors and models can be accessed by name and type through the
geometry manager using `getDetector` and `getModel`, respectively. All
detectors can be fetched at once using the `getDetectors` method. If the
module is a detector-specific module its related Detector can be
accessed through the `getDetector` method of the module base class
instead (returns a null pointer for unique modules) as follows:

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
void run(unsigned int event_id) \texttt{
    // Returns the linked detector
    std::shared_ptr<Detector> detector = this->getDetector();
}
```

### Detector models

Different types of detector models are available and distributed
together with the framework: these models use the configuration format
introduced in Section [File format](framework.md#file-format) and can be found in the
*models* directory of the repository. Every model extends from the
`DetectorModel` base class, which defines the minimum required
parameters of a detector model within the framework. The coordinates
place the detector in the global coordinate system, with the reference
point taken as the geometric center of the active matrix. This is
defined by the number of pixels in the sensor in both the x- and
y-direction, and together with the pitch of the individual pixels the
total size of the pixel matrix is determined. Outside the active matrix,
the sensor can feature excess material in all directions in the
x-y-plane. A detector of base class type does not feature a separate
readout chip, thus only the thickness of an additional, inactive silicon
layer can be specified. Derived models allow for separate readout chips,
optionally connected with bump bonds.

The base detector model can be extended to provide more detailed
geometries. Currently implemented derived models are the
`MonolithicPixelDetectorModel`, which describes a monolithic detector
with all electronics directly implemented in the same silicon wafer as
the sensor, and the `HybridPixelDetectorModel`, which in addition to the
features described above also includes a separate readout chip with
configurable size and bump bonds between the sensor and readout chip.

Models are defined in configuration files which are used to instantiate
the actual model classes; these files contain various types of
parameters, some of which are required for all models while others are
optional or only supported by certain model types. For more details on
how to add and use a new detector model,
Section [Adding a New Detector Model](development.md#adding-a-new-detector-model) should be consulted.

The set of base parameters supported by every model is provided below.
These parameters should be given at the top of the file before the start
of any sub-sections.

-   `type`: A required parameter describing the type of the model. At
    the moment either `monolithic` or `hybrid`. This value determines
    the supported parameters as discussed later.

-   `numberofpixels`: The number of pixels in the 2D pixel matrix.
    Determines the base size of the sensor together with the `pixelsize`
    parameter below.

-   `pixelsize`: The pitch of a single pixel in the pixel matrix.
    Provided as 2D parameter in the x-y-plane. This parameter is
    required for all models.

-   `implantsize`: The size of the collection diode implant in each
    pixel of the matrix. Provided as 2D parameter in the x-y-plane. This
    parameter is optional, the implant size defaults to the pixel pitch
    if not specified otherwise.

-   `sensorthickness`: Thickness of the active area of the detector
    model containing the individual pixels. This parameter is required
    for all models.

-   `sensor_excess_direction`: With direction either `top`, `bottom`,
    `right` or `left`, where the top, bottom, right and left direction
    are the positive y-axis, the negative y-axis, the positive x-axis
    and the negative x-axis, respectively. Specifies the extra material
    added to the sensor outside the active pixel matrix in the given
    direction.

-   `sensorexcess`: Fallback for the excess width of the sensor in all
    four directions (top, bottom, right and left). Used if the
    specialized parameters described below are not given. Defaults to
    zero, thus having a sensor size equal to the number of pixels times
    the pixel pitch.

-   `chipthickness`: Thickness of the readout chip, placed next to the
    sensor.

The base parameters described above are the only set of parameters
supported by the **monolithic** model. For this model, the
`chipthickness` parameter represents the first few micrometers of
silicon which contain the chip circuitry and are shielded from the bias
voltage and thus do not contribute to the signal formation.

The **hybrid** model adds bump bonds between the chip and sensor while
automatically making sure the chip and support layers are shifted
appropriately. Furthermore, it allows the user to specify the chip
dimensions independently from the sensor size, as the readout chip is
treated as a separate entity. The additional parameters for the
**hybrid** model are as follows:

-   `chip_excess_direction`: With direction either `top`, `bottom`,
    `right` or `left`. The chip excess in the specific direction,
    similar to the `sensor_excess_direction` parameter described above.

-   `chipexcess`: Fallback for the excess width of the chip, defaults to
    zero and thus to a chip size equal to the dimensions of the pixel
    matrix. See the `sensorexcess` parameter above.

-   `bumpheight`: Height of the bump bonds (the separation distance
    between the chip and the sensor)

-   `bumpsphereradius`: The individual bump bonds are simulated as union
    solids of a sphere and a cylinder. This parameter sets the radius of
    the sphere to use.

-   `bumpcylinderradius`: The radius of the cylinder part of the bump.
    The height of the cylinder is determined by the `bumpheight`
    parameter.

-   `bumpoffset`: A 2D offset of the grid of bumps. The individual bumps
    are by default positioned at the center of each single pixel in the
    grid.

[sec:support~l~ayers] In addition to the active layer, multiple layers
of support material can be added to the detector description. It is
possible to place support layers at arbitrary positions relative to the
sensor, while the default position is behind the readout chip (or
inactive silicon layer). The support material can be chosen from a set
of predefined materials, including PCB and Kapton.

Every support layer should be defined in its own section headed with the
name `[support]`. By default, no support layers are added. Support
layers allow for the following parameters.

-   `size`: Size of the support in 2D (the thickness is given separately
    below). This parameter is required for all support layers.

-   `thickness`: Thickness of the support layers. This parameter is
    required for all support layers.

-   `location`: Location of the support layer. Either *sensor* to attach
    it to the sensor (opposite to the readout chip/inactive silicon
    layer), *chip* to add the support layer behind the chip/inactive
    layer or *absolute* to specify the offset in the z-direction
    manually. Defaults to *chip* if not specified. If the parameter is
    equal to *sensor* or *chip*, the support layers are stacked in the
    respective direction when multiple layers of support are specified.

-   `offset`: If the parameter `location` is equal to *sensor* or
    *chip*, an optional 2D offset can be specified using this parameter,
    the offset in the z-direction is then automatically determined.
    These support layers are by default centered around the middle of
    the pixel matrix (the rotation center of the model). If the
    `location` is set to *absolute*, the offset is a required parameter
    and should be provided as a 3D vector with respect to the center of
    the model (thus the center of the active sensor). Care should be
    taken to ensure that these support layers and the rest of the model
    do not overlap.

-   `holesize`: Adds an optional cut-out hole to the support with the 2D
    size provided. The hole always cuts through the full support
    thickness. No hole will be added if this parameter is not present.

-   `holetype`: Type of hole to be punched into the support layer.
    Currently supported are *rectangle* and *cylinder*. Defaults to
    *rectangle*.

-   `holeoffset`: If present, the hole is by default placed at the
    center of the support layer. A 2D offset with respect to its default
    position can be specified using this parameter.

-   `material`: Material of the support. Allpix² does not provide a set
    of materials to choose from; it is up to the modules using this
    parameter to implement the materials such that they can use it.
    Chapter [Modules](modules.md) provides details about the materials supported
    by the geometry builder module (<span>GeometryBuilderGeant4</span>).

Some modules are written to act on only a particular type of detector
model. In order to ensure that a specific detector model has been used,
the model should be downcast: the downcast returns a null pointer if the
class is not of the appropriate type. An example for fetching a
`HybridPixelDetectorModel` would thus be:

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
// "detector" is a pointer to a Detector object
auto model = detector->getModel();
auto hybrid_model = std::dynamic_pointer_cast<HybridPixelDetectorModel>(model);
if(hybrid_model != nullptr) \texttt{
    // The model of this Detector is a HybridPixelDetectorModel
}
```

A detector model contains default values for all parameters. Some
parameters like the sensor thickness can however vary between different
detectors of the same model. To allow for easy adjustment of these
parameters, models can be specialized in the detector configuration file
introduced in [Detector configuration](getting_started.md#detector-configuration). All model parameters, except the
type parameter and the support layers, can be changed by adding a
parameter with the exact same key and the updated value to the detector
configuration. The framework will then automatically create a copy of
this model with the requested change.

Before re-implementing models, it should be checked if the desired
change can be achieved using the detector model specialization. For most
cases this provides a quick and flexible way to adapt detectors to
different needs and setups (for example, detectors with different sensor
thicknesses).

To support different detector models and storage locations, the
framework searches different paths for model files in the following
order:

1.  If defined, the paths provided in the global `modelpaths` parameter
    are searched first. Files are read and parsed directly. If the path
    is a directory, all files in the directory are added (without
    recursing into subdirectories).

2.  The location where the models are installed to (refer to the
    description of the `MODELDIRECTORY` variable in
    Section [Conﬁguration via CMake](installation.md#configuration-via-cmake)).

3.  The standard data paths on the system as given by the environmental
    variable ``` XDG_DATA_DIRS} with ``\project/models'' appended.
    The \texttt{XDGDATADIRS ``` variable defaults to */usr/local/share/*
    (thus effectively */usr/local/share//models*) followed by
    */usr/share/* (effectively */usr/share//models*).

Passing Objects using Messages
------------------------------

Communication between modules is performed by the exchange of messages.
Messages are templated instantiations of the `Message` class carrying a
vector of objects. The list of objects available in the Allpix² objects
library are discussed in Chapter [Objects](objects.md). The messaging system has
a dispatching mechanism to send messages and a receiving part that
fetches incoming messages. Messages are always received by modules in
the order they have been dispatched by preceding modules.

The dispatching module can specify an optional name for the messages,
but modules should normally not specify this name directly. If the name
is not given (or equal to `-`) the `output` parameter of the module is
used to determine the name of the message, defaulting to an empty
string. Dispatching messages to their receivers is then performed
following these rules:

1.  The receiving module will receive a message if it has the exact same
    type as the message dispatched (thus carrying the same objects). If
    the receiver is however listening to the `BaseMessage` type which
    does not specify the type of objects it is carrying, it will instead
    receive all dispatched messages.

2.  The receiving module will receive messages with the exact name it is
    listening for. The module uses the `input` parameter to determine
    which message names it should listen for; if the `input` parameter
    is equal to `*` the module will listen to all messages. Each module
    by default listens to messages with no name specified (thus
    receiving the messages of dispatching modules without output name
    specified).

3.  If the receiving module is a detector module, it will receive
    messages bound to that specific detector messages that are not bound
    to any detector.

An example of how to dispatch a message containing an array of `Object`
types bound to a detector named `dut` is provided below. As usual, the
message is dispatched at the end of the `run()` function of the module.

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
void run(unsigned int event_id) \texttt{
    std::vector<Object> data;
    // ..fill the data vector with objects ...

    // The message is dispatched only for the module's detector, stored in "detector_"
    auto message = std::make_shared<Message<Object>>(data, detector_);

    // Send the message using the Messenger object
    messenger->dispatchMessage(this, message);
}
```

### Methods to process messages

The message system has multiple methods to process received messages.
The first two are the most common methods and the third should be
avoided in almost every instance.

1.  Bind a **single message** to a variable. This should usually be the
    preferred method, where a module expects only a single message to
    arrive per event containing the list of all relevant objects. The
    following example binds to a message containing an array of objects
    and is placed in the constructor of a detector-type `TestModule`:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    TestModule(Configuration&, Messenger* messenger, std::shared_ptr<Detector>) \texttt{
        messenger->bindSingle(this,
                              /* Pointer to the message variable */
                              &TestModule::message,
                              /* No special messenger flags */
                              MsgFlags::NONE);
    }
    std::shared_ptr<Message<Object>> message;
    ```

2.  Bind a **set of messages** to a `std::vector` variable. This method
    should be used if the module can (and expects to) receive the same
    message multiple times (possibly because it wants to receive the
    same type of message for all detectors). An example to bind multiple
    messages containing an array of objects in the constructor of a
    unique-type `TestModule` would be:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    TestModule(Configuration&, Messenger* messenger, GeometryManager* geo_manager) \texttt{
        messenger->bindMulti(this,
                              /* Pointer to the message vector */
                              &TestModule::messages,
                              /* No special messenger flags */
                              MsgFlags::NONE);
    }
    std::vector<std::shared_ptr<Message<Object>>> messages;
    ```

3.  Listen to a particular message type and execute a **listener
    function** as soon as an object is received. This can be used for
    more advanced strategies of retrieving messages, but the other
    methods should be preferred whenever possible. The listening module
    should do any heavy work in the listening function as this is
    supposed to take place in the module `run` method instead. Using a
    listener function can lead to unexpected behaviour because the
    function is executed during the run method of the dispatching
    module. This means that logging is performed at the level of the
    dispatching module and that the listener method can be accessed from
    multiple threads if the dispatching module is parallelized.
    Listening to a message containing an array of objects in a
    detector-specific `TestModule` could be performed as follows:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    TestModule(Configuration&, Messenger* messenger, std::shared_ptr<Detector>) \texttt{
        messenger->registerListener(this,
                                    /* Pointer to the listener method */
                                    &TestModule::listener,
                                    /* No special message flags */
                                    MsgFlags::NONE);
    }
    void listener(std::shared_ptr<Message<Object>> message) \texttt{
        // Do something with the received message ...
    }
    ```

### Message flags

Flags can be added to the bind and listening methods which enable a
particular behaviour of the framework.

-   **REQUIRED**: Specifies that this message is required during the
    event processing. If this particular message is not received before
    it is time to execute the module’s run function, the execution of
    the method is automatically skipped by the framework for the current
    event. This can be used to ignore modules which cannot perform any
    action without received messages, for example charge carrier
    propagation without any deposited charge carriers.

-   `ALLOWOVERWRITE`: By default an exception is automatically raised if
    a single bound message is overwritten (thus receiving it multiple
    times instead of once). This flag prevents this behaviour. It can
    only be used for variables bound to a single message.

-   `IGNORENAME`: If this flag is specified, the name of the dispatched
    message is not considered. Thus, the `input` parameter is ignored
    and forced to the value `*`.

### Persistency

As objects may contain information relating to other objects, in
particular for storing their corresponding Monte Carlo history (see
Section [Object History](objects.md#object-history)), objects are by default persistent until the
end of each event. All messages are stored as shared pointers by the
modules which send them, and are released at the end of each event. If
no other copies of the shared message pointer are created, then these
will be subsequently deleted, including the objects stored therein.
Where a module requires access to data from a previous event (such as to
simulate the effects of pile-up etc.), local copies of the data objects
must be created. Note that at the point of creating copies the
corresponding history will be lost.

Redirect Module Inputs and Outputs
----------------------------------

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
easy-to-use logging system, introduced in Section [Logging system](framework.md#logging-system) and an
easy-to-use framework for units that supports converting arbitrary
combinations of units to common base units which can be used
transparently throughout the framework, and which will be discussed in
more detail in Section [Unit system](framework.md#unit-system). The latter comprise tools
which provide functionality the C++14 standard does not contain. These
utilities are used internally in the framework and are only shortly
discussed in Section [Internal utilities](framework.md#internal-utilities) (file system support) and
Section [Internal utilities](framework.md#internal-utilities) (string utilities).

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

[Internal utilities](framework.md#internal-utilities) STL only provides string conversions for
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

Error Reporting and Exceptions
------------------------------

Allpix² generally follows the principle of throwing exceptions in all
cases where something is definitely wrong. Exceptions are also thrown to
signal errors in the user configuration. It does not attempt to
circumvent problems or correct configuration mistakes, and the use of
error return codes is to be discouraged. The asset of this method is
that errors cannot easily be ignored and the code is more predictable in
general.

For warnings and information messages, the logging system should be used
extensively. This helps both in following the progress of the simulation
and in debugging problems. Care should however be taken to limit the
amount of messages in levels higher than `DEBUG` or `TRACE`. More
details about the logging levels and their usage can be found in
Section [Logging and Verbosity Levels](getting_started.md#logging-and-verbosity-levels).

The base exceptions in Allpix² are available via the utilities. The most
important exception base classes are the following:

-   `ConfigurationError`: All errors related to incorrect user
    configuration. This could indicate a non-existing configuration
    file, a missing key or an invalid parameter value.

-   `RuntimeError`: All other errors arising at run-time. Could be
    related to incorrect configuration if messages are not correctly
    passed or non-existing detectors are specified. Could also be raised
    if errors arise while loading a library or executing a module.

-   `LogicError`: Problems related to modules which do not properly
    follow the specifications, for example if a detector module fails to
    pass the detector to the constructor. These methods should never be
    raised for correctly implemented modules and should therefore not be
    of any concern for the end users. Reporting this type of error can
    help developers during the development of new modules.

There are only four exceptions that are supposed to be used in specific
modules, outside of the core framework. These exceptions should be used
to indicate errors that modules cannot handle themselves:

-   `InvalidValueError`: Derived from configuration exceptions. Signals
    any problem with the value of a configuration parameter not related
    to parsing or conversion to the required type. Can for example be
    used for parameters where the possible valid values are limited,
    like the set of logging levels, or for paths that do not exist. An
    example is shown below:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    void run(unsigned int event_id) \texttt{
        // Fetch a key from the configuration
        std::string value = config.get("key");

        // Check if it is a 'valid' value
        if(value != 'A' && value != "B") \texttt{
            // Raise an error if it the value is not valid
            //   provide the configuration object, key and an explanation
            throw InvalidValueError(config, "key", "A and B are the only allowed values");
        }
    }
    ```

-   `InvalidCombinationError`: Derived from configuration exceptions.
    Signals any problem with a combination of configuration parameters
    used. This could be used if several optional but mutually exclusive
    parameters are present in a module, and it should be ensured that
    only one is specified at the time. The exceptions accepts the list
    of keys as initializer list. An example is shown below:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    void run(unsigned int event_id) \texttt{
        // Check if we have mutually exclusive options defined:
        if(config.count({"exclusive_opt_a", "exclusive_opt_b"}) > 1) \texttt{
            // Raise an error if the combination of keys is not valid
            //   provide the configuration object, keys and an explanation
            throw InvalidCombinationError(config, \texttt{"exclusive_opt_a", "exclusive_opt_b"}, "Options A and B are mutually exclusive, specify only one.");
        }
    }
    ```

-   `ModuleError`: Derived from module exceptions. Should be used to
    indicate any runtime error in a module not directly caused by an
    invalid configuration value, for example that it is not possible to
    write an output file. A reason should be given to indicate what the
    source of problem is.

-   `EndOfRunException`: Derived from module exceptions. Should be used
    to request the end of event processing in the current run, e.g. if a
    module reading in data from a file reached the end of its input
    data.


