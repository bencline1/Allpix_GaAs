---
template: overrides/main.html
title: "Frequently Asked Questions"
---

This chapter provides answers to some of the most frequently asked
questions concerning usage, configuration and extension of the Allpix²
framework.

Installation & Usage
--------------------

What is the easiest way to use Allpix² on CERN’s LXPLUS?
:   Central installations of Allpix² on LXPLUS are provided via CVMFS
    for both supported LXPLUS operating systems, SLC6 and CERN CentOS7.
    Please refer to Section [Software deployment to CVMFS](testing.md#software-deployment-to-cvmfs) for the details of how to access
    these installations.

What is the quickest way to get a local installation of Allpix²?
:   The project provides ready-to-use Docker containers which contain
    all dependencies such as Geant4 and ROOT. Please refer to
    Section [Docker images](installation.md#docker-images) for more information on how to start and use
    these containers.

Configuration
-------------

How do I run a module only for one detector?
:   This is only possible for detector modules (which are constructed to
    work on individual detectors). To run it on a single detector, one
    should add a parameter `name` specifying the name of the detector
    (as defined in the detector configuration file):

    ``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    [ElectricFieldReader]
    name = "dut"
    model = "mesh"
    file_name = "../example_electric_field.init"
    ```

How do I run a module only for a specific detector type?
:   This is only possible for detector modules (which are constructed to
    work on individual detectors). To run it for a specific type of
    detector, one should add a parameter `type` with the type of the
    detector model (as set in the detector configuration file by the
    `model` parameter):

    ``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    [ElectricFieldReader]
    type = "timepix"
    model = "linear"
    bias_voltage = -50V
    depletion_voltage = -30V
    ```

    Please refer to Section [Module instantiation](framework-modules-manager.md#module-instantiation) for more
    information.

How can I run the exact same type of module with different settings?
:   This is possible by using the `input` and `output` parameters of a
    module that specify the messages of the module:

    ``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    [DefaultDigitizer]
    name = "dut0"
    adc_resolution = 4
    output = "low_adc_resolution"

    [DefaultDigitizer]
    name = "dut0"
    adc_resolution = 12
    output = "high_adc_resolution"
    ```

    By default, both the input and the output of module are messages
    with an empty name. In order to further process the data, subsequent
    modules require the `input` parameter to not receive multiple
    messages:

    ``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    [DetectorHistogrammer]
    input = "low_adc_resolution"
    name = "dut0"

    [DetectorHistogrammer]
    input = "high_adc_resolution"
    name = "dut0"
    ```

    Please refer to Section [Passing Objects using Messages](framework-passing-objects-using-messages.md) for more
    information.

How can I temporarily ignore a module during development?
:   The section header of a particular module in the configuration file
    can be replaced by the string `Ignore`. The section and all of its
    key/value pairs are then ignored. Modules can also be excluded from
    the compilation process as explained in Section [Conﬁguration via CMake](installation.md#configuration-via-cmake).

Can I get a high verbosity level only for a specific module?
:   Yes, it is possible to specify verbosity levels and log formats per
    module. This can be done by adding the `loglevel` and/or `logformat`
    key to a specific module to replace the parameter in the global
    configuration sections.

Can I import an electric field from TCAD and use it for simulating propagation?
:   Yes, the framework includes a tool to convert DF-ISE files from TCAD
    to an internal format which Allpix² can parse. More information
    about this tool can be found in
    Section [Octree](../tools/mesh_converter.md#octree), instructions to
    import the generated field are provided in
    Section [Electric Fields](getting_started.md#electric-fields).

Detector Models
---------------

I want to use a detector model with one or several small changes, do I have to create a whole new model for this?
:   No, models can be specialized in the detector configuration file. To
    specialize a detector model, the key that should be changed in the
    standard detector model, e.g. like `sensorthickness`, should be
    added as key to the section of the detector configuration (which
    already contains the position, orientation and the base model of the
    detector). Only parameters in the header of detector models can be
    changed. If support layers should be changed, or new support layers
    are needed, a new model should be created instead. Please refer to
    Section [Detector models](framework-geometry-detectors.md#detector-models) for more information.

Data Analysis
-------------

How do I access the history of a particular object?
:   Many objects can include an internal link to related other objects
    (for example `getPropagatedCharges` in the `PixelCharge` object),
    containing the history of the object (thus the objects that were
    used to construct the current object). These referenced objects are
    stored as special ROOT pointers inside the object, which can only be
    accessed if the referenced object is available in memory. In Allpix²
    this requirement can be automatically fulfilled by also binding the
    history object of interest in a module. During analysis, the tree
    holding the referenced object should be loaded and pointing to the
    same event entry as the object that requests the reference. If the
    referenced object can not be loaded, an exception is thrown by the
    retrieving method. Please refer to Section [Object History](objects.md#object-history) for more
    information.

How do I access the Monte Carlo truth of a specific PixelHit?
:   The Monte Carlo truth is part of the history of a PixelHit. This
    means that the Monte Carlo truth can be retrieved as described in
    the question above. Because accessing the Monte Carlo truth of a
    PixelHit is quite a common task, these references are stored
    directly for every new object created. This allows to retain the
    information without the necessity to keep the full object history
    including all intermediate steps in memory. Please refer to
    Section [Object History](objects.md#object-history) for more information.

How do I find out, which Monte Carlo particles are primary particles and which have been generated in the sensor?
:   The Monte Carlo truth information is stored per-sensor as MCParticle
    objects. Each MCParticle stores, among other information, a
    reference to its parent. Particles which have entered the sensor
    from the outside world do not have parent MCParticles in the
    respective sensor and are thus primaries.

    Using this approach it is possible, to e.g. treat a secondary
    particle produced in one detector as primary in a following
    detector.

    Below is some pseudo-code to filter a list of MCParticle objects for
    primaries based on their parent relationship:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    // Collect all primary particles of the event:
    std::vector<const MCParticle*> primaries;

    // Loop over all MCParticles available
    for(auto& mc_particle : my_mc_particles) \texttt{
        // Check for possible parents:
        if(mc_particle.getParent() != nullptr) \texttt{
            // Has a parent, thus was created inside this sensor.
            continue;
        }

        // Has no parent particles in this sensor, add to primary list.
        primaries.push_back(&mc_particle);
    }
    ```

    A similar function is used e.g. in the DetectorHistogrammer module
    to filter primary particles and create position-resolved graphs.

How do I access data stored in a file produced with the ROOTObjectWriter from an analysis script?
:   Allpix² uses ROOT trees to directly store the relevant C++ objects as
    binary data in the file. This retains all information present during
    the simulation run, including relations between different objects
    such as assignment of Monte Carlo particles. In order to read such a
    data file in an analysis script, the relevant C++ library as well as its
    header have to be loaded.

    In ROOT this can be done interactively by loading a data file, the
    necessary shared library objects and a macro for the analysis:

    ``` {.bash frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    $ root -l data_file.root
    root [1] .L ~/path/to/your/allpix-squared/lib/libAllpixObjects.so
    root [2] .L analysisMacro.C+
    root [3] readTree(_file0, "detector1")
    ```

    A simple macro for reading DepositedCharges from a file and
    displaying their position is presented below:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    #include <TFile.h>
    #include <TTree.h>

    // FIXME: adapt path to the include file of APSQ installation
    #include "/path/to/your/allpix-squared/DepositedCharge.hpp"

    // Read data from tree
    void readTree(TFile* file, std::string detector) \texttt{

        // Read tree of deposited charges:
        TTree* dc_tree = static_cast<TTree*>(file->Get("DepositedCharge"));
        if(!dc_tree) \texttt{
            throw std::runtime_error("Could not read tree");
        }

        // Find branch for the detector requested:
        TBranch* dc_branch = dc_tree->FindBranch(detector.c_str());
        if(!dc_branch) \texttt{
            throw std::runtime_error("Could not find detector branch");
        }

        // Allocate object vector and link to ROOT branch:
        std::vector<allpix::DepositedCharge*> deposited_charges;
        dc_branch->SetObject(&deposited_charges);

        // Go through the tree event-by-event:
        for(int i = 0; i < dc_tree->GetEntries(); ++i) \texttt{
            dc_tree->GetEntry(i);
            // Loop over all deposited charge objects
            for(auto& charge : deposited_charges) \texttt{
                std::cout << "Event " << i << ": "
                          << "charge = " << charge->getCharge() << ", "
                          << "position = " << charge->getGlobalPosition()
                          << std::endl;
            }
        }
    }
    ```

    A more elaborate example for a data analysis script can be found in
    the `tools` directory of the repository and in
    Section [ROOT Analysis & Helper Macros](../../tools/root_analysis_macros/) of this user manual. Scripts
    written in both C++ and in Python are provided.

How can I convert data from the ROOTObject format to other formats?
:   Since the ROOTObject format is the native format of Allpix², the
    stored data can be read into the framework again. To convert it to
    another format, a simple pseudo-simulation setup can be used, which
    reads in data with one module and stores it with another.

    In order to convert for example from ROOTObjects to the data format
    used by the Corryvreckan reconstruction framework, the following
    configuration could be used:

    ``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    [Allpix]
    number_of_events = 999999999
    detectors_file = "telescope.conf"
    random_seed_core = 0

    [ROOTObjectReader]
    file_name = "input_data_rootobjects.root"

    [CorryvreckanWriter]
    file_name = "output_data_corryvreckan.root"
    reference = "mydetector0"
    ```

Development
-----------

How do I write my own output module?
:   An essential requirement of any output module is its ability to
    receive any message of the framework. This can be implemented by
    defining a private `listener` function for the module as described
    in Section [Passing Objects using Messages](framework-passing-objects-using-messages.md). This function will be called for
    every new message dispatched within the framework, and should
    contain code to decide whether to discard or cache a message for
    processing. Heavy-duty tasks such as handling data should not be
    performed in the `listener` routine, but deferred to the `run`
    function of the respective output module.

How do I process data from multiple detectors?
:   When developing a new Allpix² module which processes data from
    multiple detectors, e.g. as the simulation of a track trigger
    module, this module has to be of type `unique` as described in
    Section [Modules and the Module Manager](framework-modules-manager.md). As a `detector` module, it would
    always only have access to the information linked to the specific
    detector is has been instantiated for. The module should then
    request all messages of the desired type using the messenger call
    `bindMulti` as described in Section [Passing Objects using Messages](framework-passing-objects-using-messages.md). For
    `PixelHit` messages, an example code would be:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    TrackTriggerModule(Configuration&, Messenger* messenger, GeometryManager* geo_manager) \texttt{
        messenger->bindMulti(this,
                             &TrackTriggerModule::messages,
                             MsgFlags::NONE);
    }
    std::vector<std::shared_ptr<PixelHitMessage>> messages;
    ```

    The correct detectors have then to be selected in the `run` function
    of the module implementation.

How do I calculate an efficiency in a module?
:   Calculating efficiencies always requires a reference. For hit
    detection efficiencies in Allpix², this could be the Monte Carlo
    truth information available via the `MCParticle` objects. Since the
    framework only runs modules, if all input message requirements are
    satisfied, the message flags described in Section [Message ﬂags](framework-passing-objects-using-messages.md#message-flags)
    have to be set up accordingly. For the hit efficiency example, two
    different message types are required, and the Monte Carlo truth
    should always be required (using `MsgFlags::REQUIRED`) while the
    `PixelHit` message should be optional:

    ``` {.c++ frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    MyModule::MyModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
        : Module(config, detector), detector_(std::move(detector)) \texttt{

        // Bind messages
        messenger->bindSingle(this, &MyModule::pixels_message_);
        messenger->bindSingle(this, &MyModule::mcparticle_message_, MsgFlags::REQUIRED);
    }
    ```

Miscellaneous
-------------

How can I produce nicely looking drift-diffusion line graphs?
:   The GenericPropagation module offers the possibility to produce line
    graphs depicting the path each of the charge carrier groups have
    taken during the simulation. This is a very useful way to visualize
    the drift and diffusion along field lines.

    An optional parameter allows to reduce the lines drawn to those
    charge carrier groups which have reached the sensor surface to
    provide some insight into where from the collected charge carriers
    originate and how they reached the implants. One graph is written
    per event simulated, usually this option should thus only be used
    when simulating one or a few events but not during a production run.

    In order to produce a precise enough line graph, the integration
    time steps have to be chosen carefully - usually finer than
    necessary for the actual simulation. Below is a set of settings used
    to simulate the drift and diffusion in a high resistivity CMOS
    silicon sensor. Settings of the module irrelevant for the line graph
    production have been omitted.

    ``` {.ini frame="single" framesep="3pt" breaklines="true" tabsize="2" linenos=""}
    [GenericPropagation]
    charge_per_step = 5
    timestep_min = 1ps
    timestep_max = 5ps
    timestep_start = 1ps
    spatial_precision = 0.1nm

    output_linegraphs = true
    output_plots_step = 100ps
    output_plots_align_pixels = true
    output_plots_use_pixel_units = true

    # Optional to only draw charge carrier groups which reached the implant side:
    # output_plots_lines_at_implants = true
    ```

    ![Drift and diffusion visualization of charge carrier groups being
    transported through a high-resistivity CMOS silicon sensor. The plot
    shows the situation after an integration time of 20 ns, only charge
    carrier groups which reached the implant side of the sensor are
    drawn.](../assets/images/linegraph_hrcmos_collected.png)

    With these settings, a graph of similar precision to the one
    presented in the figure can be produced. The required
    time stepping size and number of output plot steps varies greatly
    with the sensor and its applied electric field. The number of charge
    carriers per group can be used to vary the density of lines drawn.
    Larger groups result in fewer lines.


