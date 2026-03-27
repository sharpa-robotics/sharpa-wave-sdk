#pragma once

#include "SharpaWaveSDK.h"

#include <nlohmann/json.hpp>

#include <string>

namespace gesture {

bool waitForDeviceDiscovery(sharpa::SharpaWaveManager& mgr, double timeout_sec);

nlohmann::json getJointDataFromHand(sharpa::SharpaWave& hand);

/// Connect selected hand, optional start(), capture one pose JSON, disconnect_all. Returns saved path or "".
std::string recordGestureFromDevice(const std::string& output_dir,
                                    sharpa::HandSide hand_side,
                                    double discovery_timeout_sec,
                                    const std::string& filename);

}  // namespace gesture
