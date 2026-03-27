#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sharpa {
class SharpaWave;
}

namespace gesture {

/// Minimal C++ port of sample/python/gesture_workflow/h5_replay.py — plays /action/position rows via set_joint_position.
class H5Replay {
 public:
  explicit H5Replay(sharpa::SharpaWave* hand) : hand_(hand) {}
  ~H5Replay() { close(); }

  H5Replay(const H5Replay&) = delete;
  H5Replay& operator=(const H5Replay&) = delete;

  void start(const std::string& path);
  void close();

  bool active() const { return active_.load(); }
  bool running() const { return running_.load(); }
  size_t replay_index() const {
    std::lock_guard<std::mutex> lk(mu_);
    return replay_index_;
  }
  size_t stamp_count() const { return stamp_.size(); }

 private:
  void runLoop();

  sharpa::SharpaWave* hand_;
  std::thread th_;
  mutable std::mutex mu_;
  std::atomic<bool> active_{false};
  std::atomic<bool> running_{false};
  std::vector<double> stamp_;
  std::vector<std::vector<float>> actions_;
  size_t replay_index_{0};
};

void waitForReplayCompletion(H5Replay& replay, const char* step_name);

}  // namespace gesture
