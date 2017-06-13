/**
 * @author Koen Wolters <koen.wolters@cern.ch>
 */

#include "VisualizationGeant4Module.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include <G4LogicalVolume.hh>
#include <G4RunManager.hh>
#ifdef G4UI_USE_QT
#include <G4UIQt.hh>
#endif
#include <G4UImanager.hh>
#include <G4UIsession.hh>
#include <G4UIterminal.hh>
#include <G4VisAttributes.hh>
#include <G4VisExecutive.hh>

#include "core/utils/log.h"

using namespace allpix;

VisualizationGeant4Module::VisualizationGeant4Module(Configuration config, Messenger*, GeometryManager* geo_manager)
    : Module(config), config_(std::move(config)), geo_manager_(geo_manager), has_run_(false), session_param_ptr_(nullptr) {
    // Set default mode and driver for display
    config_.setDefault("mode", "gui");
    config_.setDefault("driver", "OGL");

    // Set to accumulate all hits and display at the end by default
    config_.setDefault("accumulate", true);

    // Check mode
    std::string mode = config_.get<std::string>("mode");
    if(mode != "gui" && mode != "terminal" && mode != "none") {
        throw InvalidValueError(config_, "mode", "viewing mode should be 'gui', 'terminal' or 'none'");
    }
}
VisualizationGeant4Module::~VisualizationGeant4Module() {
    // Invoke VRML2FILE workaround if necessary to prevent visualisation in case of exceptions
    if(!has_run_ && vis_manager_g4_ != nullptr && vis_manager_g4_->GetCurrentViewer() != nullptr &&
       config_.get<std::string>("driver", "") == "VRML2FILE") {
        LOG(TRACE) << "Invoking VRML workaround to prevent visualization under error conditions";

        // FIXME: workaround to skip VRML visualization in case we stopped before reaching the run method
        auto str = getenv("G4VRMLFILE_VIEWER");
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", "NONE", 1);
        }
        vis_manager_g4_->GetCurrentViewer()->ShowView();
        if(str != nullptr) {
            setenv("G4VRMLFILE_VIEWER", str, 1);
        }
    }
}

void VisualizationGeant4Module::init() {
    // Suppress all geant4 output
    SUPPRESS_STREAM(G4cout);

    // Check if we have a running G4 manager
    G4RunManager* run_manager_g4 = G4RunManager::GetRunManager();
    if(run_manager_g4 == nullptr) {
        RELEASE_STREAM(G4cout);
        throw ModuleError("Cannot visualize using Geant4 without a Geant4 geometry builder");
    }

    // Create the gui if required
    if(config_.get<std::string>("mode") == "gui") {
        // Need to provide parameters, simulate this behaviour
        session_param_ = ALLPIX_PROJECT_NAME;
        session_param_ptr_ = const_cast<char*>(session_param_.data()); // NOLINT
#ifdef G4UI_USE_QT
        gui_session_ = std::make_unique<G4UIQt>(1, &session_param_ptr_);
#else
        throw InvalidValueError(config_, "mode", "GUI session cannot be started because Qt is not available in this Geant4");
#endif
    }

    // Get the UI commander
    G4UImanager* UI = G4UImanager::GetUIpointer();

    // Disable auto refresh while we are simulating and building
    UI->ApplyCommand("/vis/viewer/set/autoRefresh false");

    // Set the visibility attributes for visualization
    set_visibility_attributes();

    // Initialize the session and the visualization manager
    LOG(TRACE) << "Initializing visualization";
    vis_manager_g4_ = std::make_unique<G4VisExecutive>("quiet");
    vis_manager_g4_->Initialize();

    // Create the viewer
    UI->ApplyCommand("/vis/scene/create");

    // Initialize the driver and checking it actually exists
    int check_driver = UI->ApplyCommand("/vis/sceneHandler/create " + config_.get<std::string>("driver"));
    if(check_driver != 0) {
        RELEASE_STREAM(G4cout);
        std::set<G4String> candidates;
        for(auto system : vis_manager_g4_->GetAvailableGraphicsSystems()) {
            for(auto& nickname : system->GetNicknames()) {
                if(!nickname.contains("FALLBACK")) {
                    candidates.insert(nickname);
                }
            }
        }

        std::string candidate_str;
        for(auto& candidate : candidates) {
            candidate_str += candidate;
            candidate_str += ", ";
        }
        if(!candidate_str.empty()) {
            candidate_str = candidate_str.substr(0, candidate_str.size() - 2);
        }

        vis_manager_g4_.reset();
        throw InvalidValueError(
            config_, "driver", "visualization driver does not exists (options are " + candidate_str + ")");
    }
    UI->ApplyCommand("/vis/sceneHandler/attach");
    UI->ApplyCommand("/vis/viewer/create");

    // Set default visualization settings
    set_visualization_settings();

    // Release the stream early in debugging mode
    IFLOG(DEBUG) { RELEASE_STREAM(G4cout); }

    // Execute initialization macro if provided
    if(config_.has("macro_init")) {
        UI->ApplyCommand("/control/execute " + config_.getPath("macro_init", true));
    }

    // Release the g4 output
    RELEASE_STREAM(G4cout);
}

// Set default visualization settings
void VisualizationGeant4Module::set_visualization_settings() {
    // Get the UI commander
    G4UImanager* UI = G4UImanager::GetUIpointer();

    // Set the background to white
    std::string bkg_color = config_.get<std::string>("background_color", "white");
    auto ret_code = UI->ApplyCommand("/vis/viewer/set/background " + bkg_color);
    if(ret_code != 0) {
        throw InvalidValueError(config_, "background_color", "background color not defined");
    }

    // Accumulate all events if requested
    bool accumulate = config_.get<bool>("accumulate");
    if(accumulate) {
        UI->ApplyCommand("/vis/scene/endOfEventAction accumulate");
        UI->ApplyCommand("/vis/scene/endOfRunAction accumulate");
    } else {
        UI->ApplyCommand("/vis/scene/endOfEventAction refresh");
        UI->ApplyCommand("/vis/scene/endOfRunAction refresh");
    }

    // Display trajectories if specified
    auto display_trajectories = config_.get<bool>("display_trajectories", true);
    if(display_trajectories) {
        // Add smooth trajectories
        UI->ApplyCommand("/vis/scene/add/trajectories rich smooth");

        // Store trajectories if accumulating
        if(accumulate) {
            UI->ApplyCommand("/tracking/storeTrajectory 2");
        }

        // Hide trajectories inside the detectors
        bool hide_trajectories = config_.get<bool>("hide_trajectories", true);
        if(hide_trajectories) {
            UI->ApplyCommand("/vis/viewer/set/hiddenEdge 1");
            UI->ApplyCommand("/vis/viewer/set/hiddenMarker 1");
        }

        // color trajectories by charge or particle id
        auto traj_color = config_.get<std::string>("trajectories_color_mode", "charge");
        if(traj_color == "generic") {
            UI->ApplyCommand("/vis/modeling/trajectories/create/generic allpixModule");

            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setLineColor " +
                             config_.get<std::string>("trajectories_color", "blue"));
        } else if(traj_color == "charge") {
            // Create draw by charge
            UI->ApplyCommand("/vis/modeling/trajectories/create/drawByCharge allpixModule");

            // Set colors for charges
            ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set 1 " +
                                        config_.get<std::string>("trajectories_color_positive", "blue"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_color_positive", "charge color not defined");
            }
            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set 0 " +
                             config_.get<std::string>("trajectories_color_neutral", "green"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_color_neutral", "charge color not defined");
            }
            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set -1 " +
                             config_.get<std::string>("trajectories_color_negative", "red"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_color_negative", "charge color not defined");
            }
        } else if(traj_color == "particle") {
            UI->ApplyCommand("/vis/modeling/trajectories/create/drawByParticleID allpixModule");

            auto particle_colors = config_.getArray<std::string>("trajectories_particle_colors");
            for(auto& particle_color : particle_colors) {
                ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/set " + particle_color);
                if(ret_code != 0) {
                    throw InvalidValueError(
                        config_, "trajectories_particle_colors", "combination particle type and color not valid");
                }
            }
        } else {
            throw InvalidValueError(
                config_, "trajectories_color_mode", "only 'generic', 'charge' or 'particle' are supported");
        }

        // Set default settings for steps
        auto draw_steps = config_.get<bool>("trajectories_draw_step", true);
        if(draw_steps) {
            UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setDrawStepPts true");
            ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setStepPtsSize " +
                                        config_.get<std::string>("trajectories_draw_step_size", "2"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_draw_step_size", "step size not valid");
            }
            ret_code = UI->ApplyCommand("/vis/modeling/trajectories/allpixModule/default/setStepPtsColour " +
                                        config_.get<std::string>("trajectories_draw_step_color", "red"));
            if(ret_code != 0) {
                throw InvalidValueError(config_, "trajectories_draw_step_color", "step color not defined");
            }
        }
    }

    // Display hits if specified
    auto display_hits = config_.get<bool>("display_hits", false);
    if(display_hits) {
        UI->ApplyCommand("/vis/scene/add/hits");
    }

    // Set viewer style
    std::string view_style = config_.get<std::string>("view_style", "surface");
    ret_code = UI->ApplyCommand("/vis/viewer/set/style " + view_style);
    if(ret_code != 0) {
        throw InvalidValueError(config_, "view_style", "viewing style is not defined");
    }

    // Set default viewer orientation
    UI->ApplyCommand("/vis/viewer/set/viewpointThetaPhi 70 20");

    // Do auto refresh if not accumulating and start viewer already
    if(!accumulate) {
        UI->ApplyCommand("/vis/viewer/set/autoRefresh true");
    }
}

// Create all visualization attributes
void VisualizationGeant4Module::set_visibility_attributes() {
    // To add some transparency in the solids, set to 0.4. 0 means opaque.
    // Transparency can be switched off in the visualisation.
    auto alpha = config_.get<double>("transparency", 0.4);
    if(alpha < 0 || alpha > 1) {
        throw InvalidValueError(config_, "transparency", "transparency level should be between 0 and 1");
    }

    // Wrapper
    G4VisAttributes wrapperVisAtt = G4VisAttributes(G4Color(1, 0, 0, 0.1)); // Red
    wrapperVisAtt.SetVisibility(false);

    // PCB
    auto pcbColor = G4Color(0.36, 0.66, 0.055, alpha); // Greenish
    G4VisAttributes pcbVisAtt = G4VisAttributes(pcbColor);
    pcbVisAtt.SetLineWidth(1);
    pcbVisAtt.SetForceSolid(false);

    // Chip
    auto chipColor = G4Color(0.18, 0.2, 0.21, alpha); // Blackish
    G4VisAttributes ChipVisAtt = G4VisAttributes(chipColor);
    ChipVisAtt.SetForceSolid(false);

    // Bumps
    auto bumpColor = G4Color(0.5, 0.5, 0.5, alpha); // Grey
    G4VisAttributes BumpVisAtt = G4VisAttributes(bumpColor);
    BumpVisAtt.SetForceSolid(false);

    // The logical volume holding all the bumps
    G4VisAttributes BumpBoxVisAtt = G4VisAttributes(bumpColor);

    // Sensors, ie pixels
    auto sensorColor = G4Color(0.18, 0.2, 0.21, alpha); // Blackish
    G4VisAttributes SensorVisAtt = G4VisAttributes(chipColor);
    SensorVisAtt.SetForceSolid(false);

    // Guard rings
    G4VisAttributes guardRingsVisAtt = G4VisAttributes(sensorColor);
    guardRingsVisAtt.SetForceSolid(false);

    // The box holding all the pixels
    G4VisAttributes BoxVisAtt = G4VisAttributes(sensorColor);

    // In default simple view mode, pixels and bumps are set to invisible, not to be displayed.
    // The logical volumes holding them are instead displayed.
    auto simple_view = config_.get<bool>("simple_view", true);
    if(simple_view) {
        SensorVisAtt.SetVisibility(false);
        BoxVisAtt.SetVisibility(true);
        BumpVisAtt.SetVisibility(false);
        BumpBoxVisAtt.SetVisibility(true);
    } else {
        SensorVisAtt.SetVisibility(true);
        BoxVisAtt.SetVisibility(false);
        BumpVisAtt.SetVisibility(true);
        BumpBoxVisAtt.SetVisibility(false);
    }

    // Apply the visualization attributes to all detectors that exist
    for(auto& detector : geo_manager_->getDetectors()) {
        auto wrapper_log = detector->getExternalObject<G4LogicalVolume>("wrapper_log");
        if(wrapper_log != nullptr) {
            wrapper_log->SetVisAttributes(wrapperVisAtt);
        }

        auto sensor_log = detector->getExternalObject<G4LogicalVolume>("sensor_log");
        if(sensor_log != nullptr) {
            sensor_log->SetVisAttributes(BoxVisAtt);
        }

        auto slice_log = detector->getExternalObject<G4LogicalVolume>("slice_log");
        if(slice_log != nullptr) {
            slice_log->SetVisAttributes(SensorVisAtt);
        }

        auto pixel_log = detector->getExternalObject<G4LogicalVolume>("pixel_log");
        if(pixel_log != nullptr) {
            pixel_log->SetVisAttributes(SensorVisAtt);
        }

        auto bumps_wrapper_log = detector->getExternalObject<G4LogicalVolume>("bumps_wrapper_log");
        if(bumps_wrapper_log != nullptr) {
            bumps_wrapper_log->SetVisAttributes(BumpBoxVisAtt);
        }

        auto bumps_cell_log = detector->getExternalObject<G4LogicalVolume>("bumps_cell_log");
        if(bumps_cell_log != nullptr) {
            bumps_cell_log->SetVisAttributes(BumpVisAtt);
        }

        auto guard_rings_log = detector->getExternalObject<G4LogicalVolume>("guard_rings_log");
        if(guard_rings_log != nullptr) {
            guard_rings_log->SetVisAttributes(guardRingsVisAtt);
        }

        auto chip_log = detector->getExternalObject<G4LogicalVolume>("chip_log");
        if(chip_log != nullptr) {
            chip_log->SetVisAttributes(ChipVisAtt);
        }

        auto PCB_log = detector->getExternalObject<G4LogicalVolume>("pcb_log");
        if(PCB_log != nullptr) {
            PCB_log->SetVisAttributes(pcbVisAtt);
        }
    }
}

void VisualizationGeant4Module::run(unsigned int) {
    if(!config_.get<bool>("accumulate")) {
        vis_manager_g4_->GetCurrentViewer()->ShowView();
        std::this_thread::sleep_for(
            std::chrono::nanoseconds(config_.get<unsigned long>("accumulate_time_step", Units::get(100ul, "ms"))));
    }
}

// Display the visualization after all events have passed
void VisualizationGeant4Module::finalize() {
    // Enable automatic refresh before showing view
    G4UImanager* UI = G4UImanager::GetUIpointer();
    UI->ApplyCommand("/vis/viewer/set/autoRefresh true");

    // Open GUI / terminal or start viewer depending on mode
    if(config_.get<std::string>("mode") == "gui") {
        LOG(INFO) << "Starting visualization session";
        gui_session_->SessionStart();
        LOG(ERROR) << "AFTER";
    } else if(config_.get<std::string>("mode") == "terminal") {
        LOG(INFO) << "Starting terminal session";
        std::unique_ptr<G4UIsession> session = std::make_unique<G4UIterminal>();
        session->SessionStart();
    } else {
        LOG(INFO) << "Starting viewer";
        vis_manager_g4_->GetCurrentViewer()->ShowView();
    }

    // Set that we did succesfully visualize
    has_run_ = true;
}
