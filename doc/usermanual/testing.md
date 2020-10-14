Development Tools & Continuous Integration
==========================================

The following chapter will introduce a few tools included in the framework to ease development and help to maintain a high code quality. This comprises tools for the developer to be used while coding, as well as a continuous integration (CI) and automated test cases of various framework and module functionalities.

The chapter is structured as follows. Section [sec:targets] describes the available targets for code quality and formatting checks, Section [sec:ci] briefly introduces the CI, and Section [sec:tests] provides an overview of the currently implemented framework, module, and performance test scenarios.

Additional Targets
------------------

A set of testing targets in addition to the standard compilation targets are automatically created by CMake to enable additional code quality checks and testing. Some of these targets are used by the project’s CI, others are intended for manual checks. Currently, the following targets are provided:

  
invokes the tool to apply the project’s coding style convention to all files of the code base. The format is defined in the file in the root directory of the repository and mostly follows the suggestions defined by the standard LLVM style with minor modifications. Most notably are the consistent usage of four whitespace characters as indentation and the column limit of 125 characters.

  
also invokes the tool but does not apply the required changes to the code. Instead, it returns an exit code 0 (pass) if no changes are necessary and exit code 1 (fail) if changes are to be applied. This is used by the CI.

  
invokes the tool to provide additional linting of the source code. The tool tries to detect possible errors (and thus potential bugs), dangerous constructs (such as uninitialized variables) as well as stylistic errors. In addition, it ensures proper usage of modern standards. The configuration used for the command can be found in the file in the root directory of the repository.

  
also invokes the tool but does not report the issues found while parsing the code. Instead, it returns an exit code 0 (pass) if no errors have been produced and exit code 1 (fail) if issues are present. This is used by the CI.

  
runs the command for additional static code analysis. The output is stored in the file in XML2.0 format. It should be noted that some of the issues reported by the tool are to be considered false positives.

  
compiles a HTML report from the defects list gathered by . This target is only available if the executable is found in the .

  
creates a binary release tarball as described in Section [sec:packaging].

Packaging
---------

comes with a basic configuration to generate tarballs from the compiled binaries using the CPack command. In order to generate a working tarball from the current build, the of the executable should not be set, otherwise the binary will not be able to locate the dynamic libraries. If not set, the global is used to search for the required libraries:

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_SKIP_RPATH=ON ..
    $ make package

Since the CMake installation path defaults to the project’s source directory, certain files are excluded from the default installation target created by CMake. This includes the detector models in the directory as well as the additional tools provided in folder. In order to include them in a release tarball produced by CPack, the installation path should be set to a location different from the project source folder, for example:

    $ cmake -DCMAKE_INSTALL_PREFIX=/tmp ..

The content of the produced tarball can be extracted to any location of the file system, but requires the ROOT6 and Geant4 libraries as well as possibly additional libraries linked by individual at runtime.

For this purpose, a shell script is automatically generated and added to the tarball. By default, it contains the ROOT6 path used for the compilation of the binaries. Additional dependencies, either library paths or shell scripts to be sourced, can be added via CMake for individual modules using the CMake functions described below. The paths stored correspond to the dependencies used at compile time, it might be necessary to change them manually when deploying on a different computer.

##### `ADD_RUNTIME_DEP(name)`

This CMake command can be used to add a shell script to be sourced to the setup file. The mandatory argument can either be an absolute path to the corresponding file, or only the file name when located in a search path known to CMake, for example:

``` {.cmake frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
# Add "geant4.sh" as runtime dependency for setup.sh file:
ADD_RUNTIME_DEP(geant4.sh)
```

The command uses the command of CMake with the option. Duplicates are removed from the list automatically. Each file found will be written to the setup file as

    source <absolute path to the file>

##### `ADD_RUNTIME_LIB(names)`

This CMake command can be used to add additional libraries to the global search path. The mandatory argument should be the absolute path of a library or a list of paths, such as:

``` {.cmake frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
# This module requires the LCIO library:
FIND_PACKAGE(LCIO REQUIRED)
# The FIND routine provides all libraries in the LCIO_LIBRARIES variable:
ADD_RUNTIME_LIB(${LCIO_LIBRARIES})
```

The command uses the command of CMake with the option to determine the directory of the corresponding shared library. Duplicates are removed from the list automatically. Each directory found will be added to the global library search path by adding the following line to the setup file:

    export LD_LIBRARY_PATH="<library directory>:$LD_LIBRARY_PATH"

Continuous Integration
----------------------

Quality and compatibility of the framework is ensured by an elaborate continuous integration (CI) which builds and tests the software on all supported platforms. The CI uses the GitLab Continuous Integration features and consists of seven distinct stages as depicted in Figure [fig:ci]. It is configured via the file in the repository’s root directory, while additional setup scripts for the GitLab Ci Runner machines and the Docker instances can be found in the directory.

![Typical continuous integration pipeline with 32 jobs distributed over seven distinct stages. In this example, all jobs passed.](ci.png "fig:") [fig:ci]

The **compilation** stage builds the framework from the source on different platforms. Currently, builds are performed on Scientific Linux 6, CentOS7, and Mac OS X. On Linux type platforms, the framework is compiled with recent versions of GCC and Clang, while the latest AppleClang is used on Mac OS X. The build is always performed with the default compiler flags enabled for the project:

        -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wconversion
        -Wuseless-cast -Wctor-dtor-privacy -Wzero-as-null-pointer-constant
        -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op
        -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept
        -Wold-style-cast -Woverloaded-virtual -Wredundant-decls
        -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel
        -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wshadow
        -Wformat-security -Wdeprecated -fdiagnostics-color=auto
        -Wheader-hygiene

The **testing** stage executes the framework system and unit tests described in Section [sec:tests]. Different jobs are used to run different test types. This allows to optimize the CI setup depending on the demands of the test to be executed. All tests are expected to pass, and no code that fails to satisfy all tests will be merged into the repository.

The **formatting** stage ensures proper formatting of the source code using the and following the coding conventions defined in the file in the repository. In addition, the tool is used for “linting” of the source code. This means, the source code undergoes a static code analysis in order to identify possible sources of bugs by flagging suspicious and non-portable constructs used. Tests are marked as failed if either of the CMake targets or fail. No code that fails to satisfy the coding conventions and formatting tests will be merged into the repository.

The **performance** stage runs a longer simulation with several thousand events and measures the execution time. This facilitates monitoring of the simulation performance, a failing job would indicate a degradation in speed. These CI jobs run on dedicated machines with only one concurrent job as described in Section [sec:tests]. Performance tests are separated into their own CI stage because their execution is time consuming and they should only be started once proper formatting of the new code is established.

The **documentation** stage prepares this user manual as well as the Doxygen source code documentation for publication. This also allows to identify e.g. failing compilation of the LaTeXdocuments or additional files which accidentally have not been committed to the repository.

The **packaging** stage wraps the compiled binaries up into distributable tarballs for several platforms. This includes adding all libraries and executables to the tarball as well as preparing the script to prepare run-time dependencies using the information provided to the build system. This procedure is described in more detail in Section [sec:packaging].

Finally, the **deployment** stage is only executed for new tags in the repository. Whenever a tag is pushed, this stages receives the build artifacts of previous stages and publishes them to the project website through the EOS file system . More detailed information on deployments is provided in the following.

Automatic Deployment
--------------------

The CI is configured to automatically deploy new versions of and its user manual and code reference to different places to make them available to users. This section briefly describes the different deployment end-points currently configured and in use. The individual targets are triggered either by automatic nightly builds or by publishing new tags. In order to prevent accidental publications, the creation of tags is protected. Only users with *Maintainer* privileges can push new tags to the repository. For new tagged versions, all deployment targets are executed.

### Software deployment to CVMFS

The software is automatically deployed to CERN’s VM file system (CVMFS)  for every new tag. In addition, the branch is built and deployed every night. New versions are published to the folder where a new folder is created for every new tag, while updates via the branch are always stored in the folder.

The deployed version currently comprises all modules as well as the detector models shipped with the framework. An additional is placed in the root folder of the respective release, which allows to set up all runtime dependencies necessary for executing this version. Versions both for SLC6 and CentOS7 are provided.

The deployment CI job runs on a dedicated computer with a GitLab SSH runner. Job artifacts from the packaging stage of the CI are downloaded via their ID using the script found in , and are made available to the *cvclicdp* user which has access to the CVMFS interface. The job checks for concurrent deployments to CVMFS and then unpacks the tarball releases and publishes them to the CLICdp experiment CVMFS space, the corresponding script for the deployment can be found in . This job requires a private API token to be set as secret project variable through the GitLab interface, currently this token belongs to the service account user *ap2*.

### Documentation deployment to EOS

The project documentation is deployed to the project’s EOS space at for publication on the project website. This comprises both the PDF and HTML versions of the user manual (subdirectory ) as well as the Doxygen code reference (subdirectory ). The documentation is only published only for new tagged versions of the framework.

The CI jobs uses the Docker image from the CERN GitLab CI tools to access EOS, which requires a specific file structure of the artifact. All files in the artifact’s folder will be published to the folder of the given project. This job requires the secret project variables and to be set via the GitLab web interface. Currently, this uses the credentials of the service account user *ap2*.

### Release tarball deployment to EOS

Binary release tarballs are deployed to EOS to serve as downloads from the website to the directory . New tarballs are produced for every tag as well as for nightly builds of the branch, which are deployed with the name .

The files are taken from the packaging jobs and published via the Docker image from the CERN GitLab CI tools. This job requires the secret project variables and to be set via the GitLab web interface. Currently, this uses the credentials of the service account user *ap2*.

Building Docker images
----------------------

New Docker images are automatically created and deployed by the CI for every new tag and as a nightly build from the branch. New versions are published to project Docker container registry . Tagged versions can be found via their respective tag name, while updates via the nightly build are always stored with the tag attached.

The final Docker image is formed from three consecutive images with different layers of software added. The \`base\` image contains all build dependencies such as compilers, CMake, and git. It derives from a CentOS7 Docker image and can be build using the file via the following commands:

    # Log into the CERN GitLab Docker registry:
    $ docker login gitlab-registry.cern.ch
    # Compile the new image version:
    $ docker build --file etc/docker/Dockerfile.base            \
                   --tag gitlab-registry.cern.ch/allpix-squared/\
                         allpix-squared/allpix-squared-base     \
                   .
    # Upload the image to the registry:
    $ docker push gitlab-registry.cern.ch/allpix-squared/\
                  allpix-squared/allpix-squared-base

The two main dependencies of the framework are ROOT6 and Geant4, which are added to the base image via the Docker image created from the file via:

    $ docker build --file etc/docker/Dockerfile.deps            \
                   --tag gitlab-registry.cern.ch/allpix-squared/\
                   allpix-squared/allpix-squared-deps           \
                  .
    $ docker push gitlab-registry.cern.ch/allpix-squared/\
                  allpix-squared/allpix-squared-deps

These images are created manually and only updated when necessary, i.e. if major new version of the underlying dependencies are available.

The dependencies Docker images should not be flattened with commands like

because it strips any variables set or used during the build process. They are used to set up the ROOT6 and Geant4 environments. When flattening, their executables and data paths cannot be found in the final image.

Finally, the latest revision of is built using the file . This job is performed automatically by the continuous integration and the created containers are directly uploaded to the project’s Docker registry.

    $ docker build --file etc/docker/Dockerfile                                \
                   --tag gitlab-registry.cern.ch/allpix-squared/allpix-squared \
                  .

A short summary of potential use cases for Docker images is provided in Section [sec:docker].

Tests
-----

The build system of the framework provides a set of automated tests which are executed by the CI to ensure proper functioning of the framework and its modules. The tests can also be manually invoked from the build directory of with

    $ ctest

The different subcategories of tests described below can be executed or ignored using the (exclude) and (run) switches of the program:

    $ ctest -R test_performance

The configuration of the tests can be found in the directories of the repository and are automatically discovered by CMake. CMake automatically searches for configuration files in the different directories and passes them to the executable (cf. Section [sec:allpix<sub>e</sub>xecutable]).

Adding a new test is as simple as adding a new configuration file to one of the different subdirectories and specifying the pass or fail conditions based on the tags described in the following paragraph.

##### Pass and Fail Conditions

The output of any test is compared to a search string in order to determine whether it passed or failed. These expressions are simply placed in the configuration file of the corresponding tests, a tag at the beginning of the line indicates whether it should be used for passing or failing the test. Each test can only contain one passing and one failing expression. If different functionality and thus outputs need to be tested, a second test should be added to cover the corresponding expression.

Different tags are provided for Mac OS X since the standard does not define the exact implementation of random number distributions such as `std::normal_distribution`. Thus, the distributions produce different results on different platforms even when used with the same random number as input.

Passing a test  
The expression marked with the tag / has to be found in the output in order for the test to pass. If the expression is not found, the test fails.

Failing a test  
If the expression tagged with / is found in the output, the test fails. If the expression is not found, the test passes.

Depending on another test  
The tag can be used to indicate dependencies between tests. For example, the module test 09 described below implements such a dependency as it uses the output of module test 08-1 to read data from a previously produced data file.

Defining a timeout  
For performance tests the runtime of the application is monitored, and the test fails if it exceeds the number of seconds defined using the tag.

Adding additional CLI options  
Additional module command line options can be specified for the executable using the tag, following the format found in Section [sec:allpix<sub>e</sub>xecutable]. Multiple options can be supplied by repeating the tag in the configuration file, only one option per tag is allowed. In exactly the same way options for the detectors can be set as well using the tag.

Defining a test case label  
Tests can be grouped and executed based on labels, e.g. for code coverage reports. Labels can be assigned to individual tests using the tag.

##### Framework Functionality Tests

The framework functionality tests aim at reproducing basic features such as correct parsing of configuration keys or resolution of module instantiations. Currently implemented tests comprise:

  
tests the framework behavior in case of a non-existent detector setup description file.

  
tests the correct parsing of additional model paths and the loading of the detector model.

  
switches the logging format.

  
sets a different logging verbosity level.

  
configures the framework to write log messages into a file.

  
tests the behavior of the framework in case of a missing detector model file.

  
sets a defined random seed to start the simulation with.

  
sets a defined seed for the core component seed generator, e.g. used for misalignment.

  
tests the framework behavior for an invalid module configuration: attempt to specialize a unique module for one detector instance.

  
tests the framework behavior for an invalid module configuration: attempt to specialize a unique module for one detector type.

  
ensures that the and Geant4 coordinate systems and transformations are identical.

  
tests the correct interpretation of rotation angles in the detector setup file.

  
tests the correct calculation of misalignments from alignment precisions given in the detector setup file.

  
checks that detector model parameters are overwritten correctly as described in Section [sec:detector<sub>m</sub>odels].

  
tests whether single configuration values can be overwritten by options supplied via the command line.

  
tests whether command line options are correctly assigned to module instances and do not alter other values.

  
tests whether two modules writing to the same file is disallowed if overwriting is denied.

  
tests whether two modules writing to the same file is allowed if the last one reenables overwriting locally.

##### Module Functionality Tests

These tests ensure the proper functionality of each module covered and thus protect the framework against accidental changes affecting the physics simulation. Using a fixed seed (using the configuration keyword) together with a specific version of Geant4  allows to reproduce the same simulation event.

One event is produced per test and the -level logging output of the respective module is checked against pre-defined expectation output using regular expressions. Once modules are altered, their respective expectation output has to be adapted after careful verification of the simulation result.

Currently implemented tests comprise:

  
takes the provided detector setup and builds the Geant4 geometry from the internal detector description. The monitored output comprises the calculated wrapper dimensions of the detector model.

  
creates a linear electric field in the constructed detector by specifying the bias and depletion voltages. The monitored output comprises the calculated effective thickness of the depleted detector volume.

  
loads an INIT file containing a TCAD-simulated electric field (cf. Section [sec:module<sub>e</sub>lectric<sub>f</sub>ield]) and applies the field to the detector model. The monitored output comprises the number of field cells for each pixel as read and parsed from the input file.

  
creates a linear electric field in the constructed detector by specifying the applied bias voltage and a depletion depth. The monitored output comprises the calculated effective thickness of the depleted detector volume.

  
creates a constant magnetic field for the full volume and applies it to the geometryManager. The monitored output comprises the message for successful application of the magnetic field.

  
executes the charge carrier deposition module. This will invoke Geant4 to deposit energy in the sensitive volume. The monitored output comprises the exact number of charge carriers deposited in the detector.

  
executes the charge carrier deposition module as the previous tests, but monitors the type, entry and exit point of the Monte Carlo particle associated to the deposited charge carriers.

  
executes the charge carrier deposition module as the previous tests, but monitors the start and end point of one of the Monte Carlo tracks in the event.

  
tests the point source in the charge carrier deposition module by monitoring the deposited charges.

  
tests the square source in the charge carrier deposition module by monitoring the deposited charges.

  
tests the sphere source in the charge carrier deposition module by monitoring the deposited charges.

  
tests the G4 macro source in the charge carrier deposition module using the macro file , monitoring the deposited charges.

  
tests the deposition of a point charge at a specified position, checks the position of the deposited charge carrier in global coordinates.

  
tests the scan of a pixel volume by depositing charges for a given number of events, check for the calculated voxel size.

  
tests the calculation of the scanning points by monitoring the warning of the number of events is not a perfect cube.

  
tests the deposition of charges along a line by monitoring the calculated step size and number of charge carriers deposited per step.

  
tests the generation of the Monte Carlo particle when depositing charges along a line by monitoring the start and end positions of the particle.

  
tests the simulation of fluctuations in charge carrier generation by monitoring the total number of generated carrier pairs when altering the Fano factor.

  
tests the deposition of charge carriers around a fixed position with a Gaussian distribution.

  
projects deposited charges to the implant side of the sensor. The monitored output comprises the total number of charge carriers propagated to the sensor implants.

  
uses the Runge-Kutta-Fehlberg integration of the equations of motion implemented in the drift-diffusion model to propagate the charge carriers to the implants. The monitored output comprises the total number of charges moved, the number of integration steps taken and the simulated propagation time.

  
uses the Runge-Kutta-Fehlberg integration of the equations of motion implemented in the drift-diffusion model to propagate the charge carriers to the implants under the influence of a constant magnetic field. The monitored output comprises the total number of charges moved, the number of integration steps taken and the simulated propagation time.

  
projects deposited charges to the implant side of the sensor with a reduced integration time to ignore some charge carriers. The monitored output comprises the total number of charge carriers propagated to the sensor implants.

  
tests the transfer of charges from sensor implants to readout chip. The monitored output comprises the total number of charges transferred and the coordinates of the pixels the charges have been assigned to.

  
digitizes the transferred charges to simulate the front-end electronics. The monitored output of this test comprises the total charge for one pixel including noise contributions and the smeared threshold it is compared to.

  
digitizes the transferred charges and tests the conversion into QDC units. The monitored output comprises the converted charge value in units of QDC counts.

  
digitizes the transferred charges and tests the amplification process by monitoring the total charge after signal amplification and smearing.

  
digitizes the signal and calculates the time-of-arrival of the particle by checking when the threshold was crossed.

  
digitizes the signal and test the conversion of time-of-arrival to TDC units.

  
tests the detector histogramming module and its clustering algorithm. The monitored output comprises the total number of clusters and their mean position.

  
ensures proper functionality of the ROOT file writer module. It monitors the total number of objects and branches written to the output ROOT trees.

  
ensures proper functionality of the RCE file writer module. The correct conversion of the PixelHit position and value is monitored by the test’s regular expressions.

  
ensures proper functionality of the LCIO file writer module. Similar to the above test, the correct conversion of PixelHits (coordinates and charge) is monitored.

  
ensures proper functionality of the Corryvreckan file writer module. The monitored output comprises the coordinates of the pixel produced in the simulation.

  
ensures the correct storage of Monte Carlo truth particle information in the Corryvreckan file writer module by monitoring the local coordinates of the MC particle associated to the pixel hit.

  
ensures proper functionality of the ASCII text writer module by monitoring the total number of objects and messages written to the text file..

  
exercises the assignment of detector IDs to detectors in the LCIO output file. A fixed ID and collection name is assigned to the simulated detector.

  
ensures that simulation results are properly converted to LCIO and stored even without the Monte Carlo truth information available.

  
tests the capability of the framework to read data back in and to dispatch messages for all objects found in the input tree. The monitored output comprises the total number of objects read from all branches.

  
tests the capability of the framework to detect different random seeds for misalignment set in a data file to be read back in. The monitored output comprises the error message including the two different random seed values.

  
tests if core random seeds are properly ignored by the ROOTObjectReader module if requested by the configuration. The monitored output comprises the warning message emitted if a difference in seed values is discovered.

  
ensures the module adds corner points of the passive material in a correct way.

  
ensures proper rotation of the position of the corner points of the passive material.

  
ensures placing a detector inside a passive material will not cause overlapping materials.

  
ensures the added corner points of the passive material increase the world volume accordingly.

  
tests if a warning will be thrown if the material of the passive material is the same as the material of the world volume.

##### Performance Tests

Similar to the module test implementation described above, performance tests use configurations prepared such, that one particular module takes most of the load (dubbed the “slowest instantiation” by ), and a few of thousand events are simulated starting from a fixed seed for the pseudo-random number generator. The keyword in the configuration file will ask CTest to abort the test after the given running time.

In the project CI, performance tests are limited to native runners, i.e. they are not executed on docker hosts where the hypervisor decides on the number of parallel jobs. Only one test is performed at a time.

Despite these countermeasures, fluctuations on the CI runners occur, arising from different loads of the executing machines. Thus, all performance CI jobs are marked with the keyword which allows GitLab to continue processing the pipeline but will mark the final pipeline result as “passed with warnings” indicating an issue in the pipeline. These tests should be checked manually before merging the code under review.

Current performance tests comprise:

  
tests the performance of charge carrier deposition in the sensitive sensor volume using Geant4 . A stepping length of is chosen, and events are simulated. The addition of an electric field and the subsequent projection of the charges are necessary since would otherwise detect that there are no recipients for the deposited charge carriers and skip the deposition entirely.

  
tests the very critical performance of the drift-diffusion propagation of charge carriers, as this is the most computing-intense module of the framework. Charge carriers are deposited and a propagation with 10 charge carriers per step and a fine spatial and temporal resolution is performed. The simulation comprises events.

  
tests the projection of charge carriers onto the implants, taking into account the diffusion only. Since this module is less computing-intense, a total of events are simulated, and charge carriers are propagated one-by-one.

  
tests the performance of multi-threaded simulation. It utilizes the very same configuration as performance test 02-1 but in addition enables multi-threading with four worker threads.


