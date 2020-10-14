Installation
============

This section aims to provide details and instructions on how to build and install . An overview of possible build configurations is given. After installing and loading the required dependencies, there are various options to customize the installation of . This chapter contains details on the standard installation process and information about custom build configurations.

Supported Operating Systems
---------------------------

is designed to run without issues on either a recent Linux distribution or Mac OSX. Furthermore, the continuous integration of the project ensures correct building and functioning of the software framework on CentOS7 (with GCC and LLVM), SLC6 (with GCC only) and Mac OS Mojave (OS X 10.14, with AppleClang).

Prerequisites
-------------

If the framework is to be compiled and executed on CERN’s LXPLUS service, all build dependencies can be loaded automatically from the CVMFS file system as described in Section [sec:initialize<sub>d</sub>ependencies].

The core framework is compiled separately from the individual modules and has therefore only one required dependency: ROOT 6 (versions below 6 are not supported) . Please refer to  for instructions on how to install ROOT. ROOT has several components of which the GenVector package is required to run . This package is included in the default build.

For some modules, additional dependencies exist. For details about the dependencies and their installation see the module documentation in Chapter [ch:modules]. The following dependencies are needed to compile the standard installation:

-   Geant4 : Simulates the desired particles and their interactions with matter, depositing charges in the detectors with the help of the constructed geometry. See the instructions in  for details on how to install the software. All Geant4 data sets are required to run the modules successfully. It is recommended to enable the Geant4 Qt extensions to allow visualization of the detector setup and the simulated particle tracks. A useful set of CMake flags to build a functional Geant4 package would be:

        -DGEANT4_INSTALL_DATA=ON
        -DGEANT4_USE_GDML=ON
        -DGEANT4_USE_QT=ON
        -DGEANT4_USE_XM=ON
        -DGEANT4_USE_OPENGL_X11=ON
        -DGEANT4_USE_SYSTEM_CLHEP=OFF

-   Eigen3 : Vector package used to perform Runge-Kutta integration in the generic charge propagation module. Eigen is available in almost all Linux distributions through the package manager. Otherwise it can be easily installed, comprising a header-only library.

Extra flags need to be set for building an installation without these dependencies. Details about these configuration options are given in Section [sec:cmake<sub>c</sub>onfig].

Downloading the source code
---------------------------

The latest version of can be downloaded from the CERN Gitlab repository . For production environments it is recommended to only download and use tagged software versions, as many of the available git branches are considered development versions and might exhibit unexpected behavior.

For developers, it is recommended to always use the latest available version from the git `master` branch. The software repository can be cloned as follows:

    $ git clone https://gitlab.cern.ch/allpix-squared/allpix-squared
    $ cd allpix-squared

Initializing the dependencies
-----------------------------

Before continuing with the build, the necessary setup scripts for ROOT and Geant4 (unless a build without Geant4 modules is attempted) should be executed. In a Bash terminal on a private Linux machine this means executing the following two commands from their respective installation directories (replacing *\<root\_install\_dir\>* with the local ROOT installation directory and likewise for Geant):

    $ source <root_install_dir>/bin/thisroot.sh
    $ source <geant4_install_dir>/bin/geant4.sh

On the CERN LXPLUS service, a standard initialization script is available to load all dependencies from the CVMFS infrastructure. This script should be executed as follows (from the main repository directory):

    $ source etc/scripts/setup_lxplus.sh

Configuration via CMake
-----------------------

uses the CMake build system to configure, build and install the core framework as well as all modules. An out-of-source build is recommended: this means CMake should not be directly executed in the source folder. Instead, a *build* folder should be created, from which CMake should be run. For a standard build without any additional flags this implies executing:

    $ mkdir build
    $ cd build
    $ cmake ..

CMake can be run with several extra arguments to change the type of installation. These options can be set with -D*option* (see the end of this section for an example). Currently the following options are supported:

-   : The directory to use as a prefix for installing the binaries, libraries and data. Defaults to the source directory (where the folders *bin/* and *lib/* are added).

-   : Type of build to install, defaults to (compiles with optimizations and debug symbols). Other possible options are `Debug` (for compiling with no optimizations, but with debug symbols and extended tracing using the Clang Address Sanitizer library) and `Release` (for compiling with full optimizations and no debug symbols).

-   : Directory to install the internal models to. Defaults to not installing if the is set to the directory containing the sources (the default). Otherwise the default value is equal to the directory *\<CMAKE\_INSTALL\_PREFIX\>/share/allpix/*. The install directory is automatically added to the model search path used by the geometry model parsers to find all of the detector models.

-   : Enable or disable the compilation of additional tools such as the mesh converter. Defaults to .

-   **`BUILD_ModuleName`**: If the specific module should be installed or not. Defaults to ON for most modules, however some modules with large additional dependencies such as LCIO  are disabled by default. This set of parameters allows to configure the build for minimal requirements as detailed in Section [sec:prerequisites].

-   : Build all included modules, defaulting to . This overwrites any selection using the parameters described above.

An example of a custom debug build, without the module and with installation to a custom directory is shown below:

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_INSTALL_PREFIX=../install/ \
            -DCMAKE_BUILD_TYPE=DEBUG \
            -DBUILD_GeometryBuilderGeant4=OFF ..

Compilation and installation
----------------------------

Compiling the framework is now a single command in the build folder created earlier (replacing *\<number\_of\_cores\> \>* with the number of cores to use for compilation):

    $ make -j<number_of_cores>

The compiled (non-installed) version of the executable can be found at *src/exec/allpix* in the folder. Running directly without installing can be useful for developers. It is not recommended for normal users, because the correct library and model paths are only fully configured during installation.

To install the library to the selected installation location (defaulting to the source directory of the repository) requires the following command:

    $ make install

The binary is now available as *bin/allpix* in the installation directory. The example configuration files are not installed as they should only be used as a starting point for your own configuration. They can however be used to check if the installation was successful. Running the allpix binary with the example configuration using `bin/allpix -c examples/example.conf` should pass the test without problems if a standard installation is used.

Docker images
-------------

Docker images are provided for the framework to allow anyone to run simulations without the need of installing on their system. The only required program is the Docker executable, all other dependencies are provided within the Docker images. In order to exchange configuration files and output data between the host system and the Docker container, a folder from the host system should be mounted to the container’s data path , which also acts as the Docker location.

The following command creates a container from the latest Docker image in the project registry and start an interactive shell session with the executable already in the `$PATH`. Here, the current host system path is mounted to the directory of the container.

    $ docker run --interactive --tty                                   \
                 --volume "$(pwd)":/data                               \
                 --name=allpix-squared                                 \
                 gitlab-registry.cern.ch/allpix-squared/allpix-squared \
                 bash

Alternatively it is also possible to directly start the simulation instead of an interactive shell, e.g. using the following command:

    $ docker run --tty --rm                                            \
                 --volume "$(pwd)":/data                               \
                 --name=allpix-squared                                 \
                 gitlab-registry.cern.ch/allpix-squared/allpix-squared \
                 "allpix -c my_simulation.conf"

where a simulation described in the configuration is directly executed and the container terminated and deleted after completing the simulation. This closely resembles the behavior of running natively on the host system. Of course, any additional command line arguments known to the executable described in Section [sec:allpix<sub>e</sub>xecutable] can be appended.

For tagged versions, the tag name should be appended to the image name, e.g. , and a full list of available Docker containers is provided via the project’s container registry . A short description of how Docker images for this project are built can be found in Section [sec:build-docker].
