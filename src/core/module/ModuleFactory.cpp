/**
 *  @author Koen Wolters <koen.wolters@cern.ch>
 *  @author Daniel Hynds <daniel.hynds@cern.ch>
 */

#include "ModuleFactory.hpp"
#include <dlfcn.h>

#include <utility>

using namespace allpix;

// Constructor and destructor
ModuleFactory::ModuleFactory() : config_(), messenger_(), geometry_manager_() {}
ModuleFactory::~ModuleFactory() = default;

void ModuleFactory::setConfiguration(Configuration conf) {
    config_ = std::move(conf);
}
Configuration& ModuleFactory::getConfiguration() {
    return config_;
}

void ModuleFactory::setMessenger(Messenger* messenger) {
    messenger_ = messenger;
}
Messenger* ModuleFactory::getMessenger() {
    return messenger_;
}

void ModuleFactory::setGeometryManager(GeometryManager* geo_manager) {
    geometry_manager_ = geo_manager;
}
GeometryManager* ModuleFactory::getGeometryManager() {
    return geometry_manager_;
}

// Function to create modules from the dynamic library passed from the Module Manager
std::vector<std::pair<ModuleIdentifier, Module*>> ModuleFactory::createModules(std::string name, void* library) {

    // Make the vector to return
    std::vector<std::pair<ModuleIdentifier, Module*>> moduleList;

    // Load an instance of the module from the library
    ModuleIdentifier identifier(name, "", 0);
    Module* module = NULL;

    void* generator = dlsym(library, "generator");
    char* err;
    if((err = dlerror()) != NULL) {
        // handle error, the symbol wasn't found
        throw allpix::DynamicLibraryError(name);
    } else {
        module = reinterpret_cast<Module* (*)(Configuration, Messenger*, GeometryManager*)>(generator)(
            getConfiguration(), getMessenger(), getGeometryManager());
    }

    moduleList.emplace_back(identifier, module);
    return moduleList;
}
