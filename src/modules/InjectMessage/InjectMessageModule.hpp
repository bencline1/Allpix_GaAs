/**
 * @file
 * @brief Definition of InjectMessage module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.

 */

#include <functional>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Event.hpp"
#include "core/module/Module.hpp"

// Contains tuple of all defined objects
#include "objects/objects.h"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to generate random objects of different types and to dispatch them as messages
     *
     * This module can be used to generate random input data to the framework by specifying data types for which objects
     * should be created and dispatched as messages. This is particularly useful for unit testing of individual modules that
     * require input from previous simulation stages.
     */
    class InjectMessageModule : public Module {
    public:
        using MessageCreatorMap =
            std::map<std::string, std::function<std::shared_ptr<BaseMessage>(Event*, std::shared_ptr<const Detector>)>>;

        /**
         * @brief Constructor for InjectMessage module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param detector Pointer to the detector for this module instance
         */
        InjectMessageModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector);

        /**
         * @brief Module initialization: reading of object types from config, creation of generators for these messages
         */
        void initialize() override;

        /**
         * @brief Run function generating the requested objects
         */
        void run(Event* event) override;

        /**
         * @brief Template object generator, produces random objects of the requested type using the current event's PRNG
         * @param  event    Pointer to current event object
         * @param  detector Shared pointer to the detector to generate objects for
         * @return          Object of the requested type
         */
        template <class T> static T generate_object(Event* event, std::shared_ptr<const Detector> detector);

    private:
        Messenger* messenger_;
        std::shared_ptr<const Detector> detector_;
        std::list<MessageCreatorMap::iterator> message_list_;

        // Internal map to construct an object from it's type index
        MessageCreatorMap message_creator_map_;
    };

    template <class T> T InjectMessageModule::generate_object(Event*, std::shared_ptr<const Detector>) { return T(); }

} // namespace allpix
