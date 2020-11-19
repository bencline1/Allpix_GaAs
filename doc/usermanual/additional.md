Additional Tools & Resources
============================

This chapter briefly describes tools provided with the Allpix²
framework, which might be re-used in new modules or in standalone code.

Framework Tools
---------------

The following tools are part of the Allpix² framework and are located in
the `src/tools` directory. They are intended as centralized components
which can be shared between different modules rather than independent
tools.

### ROOT and Geant4 utilities

The framework provides a set of methods to ease the integration of ROOT
and Geant4 in the framework. An important part is the extension of the
custom conversion `to_string` and `from_string` methods from the
internal string utilities (see Section [Internal utilities](framework-redirect-module-inputs-outputs.md#internal-utilities)) to
support internal ROOT and Geant4 classes. This allows to directly read
configuration parameters to these types, making the code in the modules
both shorter and cleaner. In addition, more conversions functions are
provided together with other useful utilities such as the possibility to
display a ROOT vector with units.

### Runge-Kutta integrator

A fast Eigen-powered[^8] Runge-Kutta integrator is provided as a
tool to numerically solve differential equations[^19]. The
Runge-Kutta integrator is designed in a generic way and supports
multiple methods using different tableaus. It allows to integrate a
system of equations in several steps with customizable step size. The
step size can also be updated during the integration depending on the
error of the Runge-Kutta method (if a tableau with error estimation is
used).

The GenericPropagation module uses Runge-Kutta integrator with the
Runge-Kutta-Fehlberg method (RK5 tableau). After the integrator has been
created with the initial position of the charge carrier to be
transported, the `step()` function allows to advance the simulation to
the next step.

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
// Define lambda functions to compute the charge carrier velocity at each step
std::function<Eigen::Vector3d(double, Eigen::Vector3d)> carrier_velocity =
    [&](double, Eigen::Vector3d cur_pos) -> Eigen::Vector3d \texttt{...};

// Create the Runge-Kutta solver with a RK5 tableau, the carrier velocity function to be used
// as well as the initial timestep and position of the charge carrier
auto runge_kutta = make_runge_kutta(tableau::RK5, carrier_velocity, initial_timestep, position);

// Advance one step with the solver:
auto step = runge_kutta.step();
```

The `getValue()` and `setValue()` methods allow to retrieve, alter and
update the position, e.g. to include additional displacements from
diffusion processes.

### Field Data Parser

A field parser tool is provided, which parses files stored in the INIT
or APF file formats and returns field data on a three-dimensional grid.
The number of field components per grid point is configurable via the
constructor argument, e.g. `FieldQuantity::VECTOR` for a vector field or
`FieldQuantity::SCALAR` for a scalar field map. The parsed field data is
cached internally by the class, and if a file is requested a second
time, the cached field is returned. In conjunction with a static
instance of the field parser class in a module, this allows to share
field data across multiple module instances.

``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
class MyVectorFieldModule(...) : Module(...) \texttt{
private:
    void some_function(std::string canonical_path);
    // Define static field parser instance
    static FieldParser<double> field_parser_;
}

// Create static instance of field parser in the translation unit:
FieldParser<double> MyVectorFieldModule::field_parser_(FieldQuantity::VECTOR);

void MyVectorFieldModule::some_function(std::string canonical_path) \texttt{
    // Get vector field from file:
    auto field_data = field_parser_.getByFileName(canonical_path, "V/cm");
}
```

For the INIT format, the `getByFileName()` function of the parser takes
the units in which the field data should be interpreted, and they are
automatically converted to the framework base units described in
Section [Parsing types and units](getting_started.md#parsing-types-and-units). Fields in the APF format are always stored
in framework base units and do not require conversion. The file path
provided to the field parser should always be canonical, if the file is
not found or cannot be parsed, a `std::runtimeerror` exception is
thrown.

The type of field data to be parsed is automatically deduced from the
file content by checking for binary or ASCII text The field parser
determines whether a file is text or binary by checking the first few
bytes in the file. If every byte in that part of the file is non-null,
the parser considers the file to be text and reads it as INIT file;
otherwise it considers the file to be binary and parses the field as APF
data.