#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

#include "SharpaWaveSDK.h"

int main() {
  std::cout << "=== Sharpa Tactile Frame Data Acquisition Example ==="
            << std::endl;
  auto& manager = sharpa::SharpaWaveManager::get_instance();

  try {
    // Wait for device connection
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto devices = manager.get_all_devices();

    while (devices.empty()) {
      std::cout << "Waiting for device connection..." << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(1));
      devices = manager.get_all_devices();
    }

    // Connect to all available devices
    std::cout << "Device found: " << devices.size() << std::endl;
    std::vector<std::reference_wrapper<sharpa::SharpaWave>> hands;
    std::vector<sharpa::HandSide> hand_sides;

    for (auto device_info : devices) {
      if (device_info.device_type != sharpa::DeviceType::HAND) continue;
      auto device_sn = device_info.sn;
      auto& hand = manager.connect(device_sn);
      hands.push_back(std::ref(hand));
      std::cout << "Device " << device_sn << " connected successfully"
                << std::endl;
      hand_sides.push_back(device_info.hand_side);

      std::string hand_name =
          (device_info.hand_side == sharpa::HandSide::LEFT) ? "LEFT" : "RIGHT";
      std::cout << "Device " << device_sn << " hand_side: " << hand_name
                << std::endl;

      // Optional: Set custom tactile configuration file
      // Uncomment the following lines to use a custom tactile config file:
      // std::string custom_config_path = "/path/to/your/tactile_config.json";
      // auto config_error = hand.set_tactile_config_file(custom_config_path);
      // if (config_error.code == 0) {
      //   std::cout << "Custom tactile config set: " << custom_config_path << std::endl;
      // } else {
      //   std::cout << "Failed to set tactile config: " << config_error.message << std::endl;
      // }

      if (!hand.start()) {
        std::cout << "Wave startup failed, check if other application using "
                     "tactile port:"
                  << hand.get_device_info().tactile_data_config.target_port
                  << std::endl;
        // throw std::runtime_error("Wave startup failed");
      } else {
        std::cout << "Wave started successfully" << std::endl;
      }
    }

    // Data statistics
    int frame_count = 0;
    std::vector<double> latencies;

    std::cout << "\nStarting to acquire tactile frame data..." << std::endl;
    std::cout << "Press Ctrl+C to exit" << std::endl;

    // Main loop: acquire tactile frame data
    auto start_time = std::chrono::steady_clock::now();

    // Main loop: continuous tactile data acquisition
    while (true) {
      // Get tactile frame data for all channels
      for (size_t i = 0; i < hands.size(); ++i) {
        auto& hand = hands[i];
        auto hand_side = hand_sides[i];
        std::string hand_name =
            (hand_side == sharpa::HandSide::LEFT) ? "LEFT" : "RIGHT";

        int channel_start = 0;
        int channel_end = 5;
        if (hand_side == sharpa::HandSide::LEFT) {
          channel_start = 5;
          channel_end = 10;
        } else {
          channel_start = 0;
          channel_end = 5;
        }

        for (int ch = channel_start; ch < channel_end; ++ch) {
          try {
            auto frame = hand.get().fetch_tactile_frame(ch);
            if (!frame) {
              continue;  // Timeout, skip
            }

            // Check if RAW data exists
            auto raw_iter = frame->content.find("RAW");
            bool has_raw_data =
                (raw_iter != frame->content.end() && raw_iter->second);
            if (!has_raw_data) {
              std::cout
                  << "Channel " << ch
                  << ": RAW data does not exist, continuing with other data"
                  << std::endl;
            }

            // Calculate latency
            auto current_time = std::chrono::system_clock::now();
            auto current_timestamp =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    current_time.time_since_epoch())
                    .count() /
                1000.0;
            double latency = current_timestamp - frame->ts;
            latencies.push_back(latency);

            // Print key information
            std::cout << "Hand " << hand_name << " - Channel " << ch
                      << " - Frame " << frame_count++
                      << " - Timestamp: " << std::fixed << std::setprecision(6)
                      << frame->ts << " - Latency: " << std::fixed
                      << std::setprecision(3) << latency << "s" << std::endl;

            // Print data shape information (simplified version)
            for (const auto& content : frame->content) {
              const std::string& data_type = content.first;
              const auto& data = content.second;

              if (data) {
                auto shape = data->shape();
                std::cout << "  " << data_type << ": ";
                if (shape.dim() == 0) {
                  std::cout << "scalar";
                } else {
                  std::cout << "shape[";
                  for (size_t i = 0; i < shape.dim(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << shape[i];
                  }
                  std::cout << "]";
                }
                std::cout << " (size: " << data->size() << ")" << std::endl;
              }
            }

            // If latency array is too large, clean up old data
            if (latencies.size() > 100) {
              latencies.erase(latencies.begin(), latencies.begin() + 50);
            }

            // Display statistics every 10 frames
            if (frame_count % 10 == 0) {
              if (!latencies.empty()) {
                double avg_latency = 0.0;
                for (double lat : latencies) {
                  avg_latency += lat;
                }
                avg_latency /= latencies.size();
                std::cout << "--- Statistics: Total frames=" << frame_count
                          << ", Average latency=" << std::fixed
                          << std::setprecision(3) << avg_latency << "s ---"
                          << std::endl;
              }
            }

          } catch (const std::exception& e) {
            std::cout << "Error processing channel " << ch << ": " << e.what()
                      << std::endl;
            continue;
          }
        }
      }
      // Short sleep to avoid excessive CPU usage
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop device
    for (auto& hand : hands) {
      hand.get().stop();
      hand.get().destroy();
      std::cout << "\nWave stopped" << std::endl;
    }
    manager.disconnect_all();
    // Print final statistics
    std::cout << "\n=== Statistics ===" << std::endl;
    std::cout << "Total frames: " << frame_count << std::endl;

    if (!latencies.empty()) {
      double avg_latency = 0.0;
      double min_latency = latencies[0];
      double max_latency = latencies[0];

      for (double latency : latencies) {
        avg_latency += latency;
        if (latency < min_latency) min_latency = latency;
        if (latency > max_latency) max_latency = latency;
      }
      avg_latency /= latencies.size();

      std::cout << "Latency statistics:" << std::endl;
      std::cout << "  Average latency: " << std::fixed << std::setprecision(3)
                << avg_latency << " seconds" << std::endl;
      std::cout << "  Minimum latency: " << std::fixed << std::setprecision(3)
                << min_latency << " seconds" << std::endl;
      std::cout << "  Maximum latency: " << std::fixed << std::setprecision(3)
                << max_latency << " seconds" << std::endl;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    std::cout << "Total runtime: " << duration.count() << " seconds"
              << std::endl;

    std::cout << "\nProgram execution completed!" << std::endl;

  } catch (const std::exception& e) {
    std::cerr << "Program exception: " << e.what() << std::endl;
    return -1;
  }
  manager.disconnect_all();
  return 0;
}