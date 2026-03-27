#include "gesture_device.hpp"

#include "gesture_io.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace gesture {

bool waitForDeviceDiscovery(sharpa::SharpaWaveManager& mgr, double timeout_sec) {
  const auto t0 = std::chrono::steady_clock::now();
  while (std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() <
         timeout_sec) {
    if (!mgr.get_all_device_sn().empty()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  return false;
}

nlohmann::json getJointDataFromHand(sharpa::SharpaWave& hand) {
  nlohmann::json action = nlohmann::json::object();
  nlohmann::json state = nlohmann::json::object();

  try {
    auto [err, payload] = hand.get_parameter({"get_joint_packet_json"});
    if (err.code == 0 && !payload.empty()) {
      nlohmann::json rsp = nlohmann::json::parse(payload);
      std::string pkt = rsp.value("get_joint_packet_json", "");
      if (pkt.empty()) {
        throw std::runtime_error("empty joint packet");
      }
      nlohmann::json jp = nlohmann::json::parse(pkt);
      if (jp.contains("angles") && jp["angles"].is_array()) {
        const auto& ang = jp["angles"];
        for (size_t i = 0; i < ang.size() && i < kJointNames.size(); ++i) {
          double a = ang[i].get<double>();
          action[kJointNames[i]] = a;
          state[kJointNames[i]] = a;
        }
        return nlohmann::json{{"action", action}, {"state", state}};
      }
    }
  } catch (...) {
  }

  sharpa::State st = hand.get_states();
  for (size_t i = 0; i < st.angles.size() && i < kJointNames.size(); ++i) {
    double a = static_cast<double>(st.angles[i]);
    action[kJointNames[i]] = a;
    state[kJointNames[i]] = a;
  }
  return nlohmann::json{{"action", action}, {"state", state}};
}

std::string recordGestureFromDevice(const std::string& output_dir,
                                    sharpa::HandSide hand_side,
                                    double discovery_timeout_sec,
                                    const std::string& filename) {
  auto& mgr = sharpa::SharpaWaveManager::get_instance();
  if (!waitForDeviceDiscovery(mgr, discovery_timeout_sec)) {
    std::cerr << "No devices found within timeout\n";
    return "";
  }
  try {
    sharpa::SharpaWave& hand = mgr.connect(hand_side);
    if (!hand.start()) std::cerr << "Warning: hand.start() returned false\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    nlohmann::json data = getJointDataFromHand(hand);
    std::string fn = filename;
    if (fn.empty()) {
      std::time_t t = std::time(nullptr);
      std::tm tm_buf{};
      localtime_r(&t, &tm_buf);
      std::ostringstream oss;
      oss << "gesture_" << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << ".json";
      fn = oss.str();
    }
    if (!saveGestureToJson(data, output_dir, fn)) return "";
    hand.stop();
    mgr.disconnect_all();
    return output_dir + (output_dir.empty() || output_dir.back() == '/' ? "" : "/") + fn;
  } catch (const std::exception& e) {
    std::cerr << "recordGestureFromDevice: " << e.what() << "\n";
    try {
      mgr.disconnect_all();
    } catch (...) {
    }
    return "";
  }
}

}  // namespace gesture
