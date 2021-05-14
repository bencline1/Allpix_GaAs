/**
 * @file
 * @brief Implementation of the InjectMessage module to generate random input data to modules
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "InjectMessageModule.hpp"

#include <string>
#include <utility>

#include "core/utils/log.h"

using namespace allpix;

static Pixel gen_pixel(Event* event, std::shared_ptr<const Detector> detector) {
    auto model = detector->getModel();
    std::uniform_int_distribution<unsigned int> x(0, model->getNPixels().x());
    std::uniform_int_distribution<unsigned int> y(0, model->getNPixels().y());
    return detector->getPixel(x(event->getRandomEngine()), y(event->getRandomEngine()));
}

template <>
MCParticle InjectMessageModule::generate_object<MCParticle>(Event* event, std::shared_ptr<const Detector> detector) {
    std::uniform_int_distribution<> type(0, 1);
    std::poisson_distribution<unsigned int> signal(8000);
    auto model = detector->getModel();

    // start at bottom, end at top of sensor, in random pixels
    auto position_bottom = gen_pixel(event, detector).getLocalCenter().SetZ(-model->getSensorSize().Z() / 2);
    auto position_top = gen_pixel(event, detector).getLocalCenter().SetZ(model->getSensorSize().Z() / 2);

    return MCParticle(position_bottom,
                      detector->getGlobalPosition(position_bottom),
                      position_top,
                      detector->getGlobalPosition(position_top),
                      11,
                      0,
                      0);
}

template <>
DepositedCharge InjectMessageModule::generate_object<DepositedCharge>(Event* event,
                                                                      std::shared_ptr<const Detector> detector) {
    std::uniform_int_distribution<> type(0, 1);
    std::poisson_distribution<unsigned int> signal(8000);
    auto model = detector->getModel();

    // Place charge somewhere in a pixel
    std::uniform_real_distribution<> zpos(-model->getSensorSize().Z(), model->getSensorSize().Z());
    auto position = gen_pixel(event, detector).getLocalCenter().SetZ(zpos(event->getRandomEngine()) / 2);

    return DepositedCharge(position,
                           detector->getGlobalPosition(position),
                           (type(event->getRandomEngine()) > 0 ? CarrierType::ELECTRON : CarrierType::HOLE),
                           signal(event->getRandomEngine()),
                           0,
                           0);
}

template <>
PropagatedCharge InjectMessageModule::generate_object<PropagatedCharge>(Event* event,
                                                                        std::shared_ptr<const Detector> detector) {
    std::uniform_int_distribution<> type(0, 1);
    std::poisson_distribution<unsigned int> signal(8000);
    auto model = detector->getModel();
    std::uniform_real_distribution<> zpos(0, model->getSensorSize().Z() / 2);

    // Place at readout implant side
    auto position = gen_pixel(event, detector).getLocalCenter().SetZ(zpos(event->getRandomEngine()));

    return PropagatedCharge(position,
                            detector->getGlobalPosition(position),
                            (type(event->getRandomEngine()) > 0 ? CarrierType::ELECTRON : CarrierType::HOLE),
                            signal(event->getRandomEngine()),
                            0,
                            0);
}

template <>
PixelCharge InjectMessageModule::generate_object<PixelCharge>(Event* event, std::shared_ptr<const Detector> detector) {
    std::poisson_distribution<unsigned int> signal(8000);
    return PixelCharge(gen_pixel(event, detector), signal(event->getRandomEngine()));
}

template <> PixelHit InjectMessageModule::generate_object<PixelHit>(Event* event, std::shared_ptr<const Detector> detector) {
    std::poisson_distribution<> signal(8000);
    return PixelHit(gen_pixel(event, detector), 0, 0, signal(event->getRandomEngine()));
}

InjectMessageModule::InjectMessageModule(Configuration& config, Messenger* messenger, std::shared_ptr<Detector> detector)
    : Module(config, detector), messenger_(messenger), detector_(std::move(detector)) {
    enable_parallelization();
}

/**
 * Adds lambda function map to convert a vector of generic objects to a templated message containing this particular type of
 * object from its typeid.
 */
template <typename T> static void add_creator(InjectMessageModule::MessageCreatorMap& map) {
    auto name = allpix::demangle(typeid(T).name());
    map[name] = [&](Event* event, std::shared_ptr<const Detector> detector) {
        std::uniform_int_distribution<size_t> distrib(1, 10);
        auto num_objects = distrib(event->getRandomEngine());

        std::vector<T> data;
        data.reserve(num_objects);

        for(size_t i = 0; i < num_objects; i++) {
            data.push_back(std::move(InjectMessageModule::generate_object<T>(event, detector)));
            LOG(TRACE) << "Generated " << std::endl << data.back();
        }

        if(detector == nullptr) {
            return std::make_shared<Message<T>>(std::move(data));
        }
        return std::make_shared<Message<T>>(std::move(data), detector);
    };
}

/**
 * Uses SFINAE trick to call the add_creator function for all template arguments of a container class. Used to add creators
 * for every object in a tuple of objects.
 */
template <template <typename...> class T, typename... Args>
static void gen_creator_map_from_tag(InjectMessageModule::MessageCreatorMap& map, type_tag<T<Args...>>) {
    std::initializer_list<int> value{(add_creator<Args>(map), 0)...};
    (void)value;
}

/**
 * Wrapper function to make the SFINAE trick in \ref gen_creator_map_from_tag work.
 */
template <typename T> static InjectMessageModule::MessageCreatorMap gen_creator_map() {
    InjectMessageModule::MessageCreatorMap ret_map;
    gen_creator_map_from_tag(ret_map, type_tag<T>());
    return ret_map;
}

void InjectMessageModule::initialize() {

    // Initialize the call map from the tuple of available objects
    message_creator_map_ = gen_creator_map<allpix::OBJECTS>();
    auto messages = config_.getArray<std::string>("messages");
    for(const auto& message : messages) {
        auto iter = message_creator_map_.find(message);
        if(iter == message_creator_map_.end()) {
            throw InvalidValueError(config_,
                                    "messages",
                                    "Cannot dispatch message with object \"" + message +
                                        "\" because it not registered for messaging");
        }
        message_list_.emplace_back(iter);
    }
}

void InjectMessageModule::run(Event* event) {
    for(auto& message_inf : message_list_) {
        auto message = message_inf->second(event, detector_);
        messenger_->dispatchMessage(this, message, event);
    }
}
