#include "h5_replay.hpp"

#include "SharpaWaveSDK.h"

#include <hdf5.h>

#include <chrono>
#include <iostream>
#include <thread>

namespace gesture {

static bool readH5ForReplay(const std::string& path, std::vector<double>& stamp,
                            std::vector<std::vector<float>>& actions) {
  hid_t f = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (f < 0) return false;
  hid_t d_st = H5Dopen2(f, "/stamp", H5P_DEFAULT);
  hid_t d_ac = H5Dopen2(f, "/action/position", H5P_DEFAULT);
  if (d_st < 0 || d_ac < 0) {
    if (d_st >= 0) H5Dclose(d_st);
    if (d_ac >= 0) H5Dclose(d_ac);
    H5Fclose(f);
    return false;
  }
  hid_t sp = H5Dget_space(d_st);
  hsize_t d1[4];
  H5Sget_simple_extent_dims(sp, d1, nullptr);
  H5Sclose(sp);
  stamp.resize(static_cast<size_t>(d1[0]));
  H5Dread(d_st, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, stamp.data());
  H5Dclose(d_st);

  sp = H5Dget_space(d_ac);
  hsize_t dims[2];
  H5Sget_simple_extent_dims(sp, dims, nullptr);
  H5Sclose(sp);
  const hsize_t T = dims[0], J = dims[1];
  std::vector<float> flat(T * J);
  H5Dread(d_ac, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, flat.data());
  H5Dclose(d_ac);
  H5Fclose(f);

  actions.assign(T, std::vector<float>(J));
  for (hsize_t t = 0; t < T; ++t)
    for (hsize_t j = 0; j < J; ++j) actions[t][j] = flat[t * J + j];
  return true;
}

void H5Replay::start(const std::string& path) {
  close();
  std::vector<double> st;
  std::vector<std::vector<float>> act;
  if (!readH5ForReplay(path, st, act) || st.empty()) {
    std::cerr << "H5Replay: failed to load " << path << "\n";
    return;
  }
  {
    std::lock_guard<std::mutex> lk(mu_);
    stamp_ = std::move(st);
    actions_ = std::move(act);
    replay_index_ = 0;
  }
  active_ = true;
  running_ = true;
  th_ = std::thread([this] { runLoop(); });
}

void H5Replay::close() {
  running_ = false;
  active_ = false;
  if (th_.joinable()) th_.join();
  std::lock_guard<std::mutex> lk(mu_);
  stamp_.clear();
  actions_.clear();
  replay_index_ = 0;
}

void H5Replay::runLoop() {
  while (running_) {
    size_t idx = 0;
    bool has_next = false;
    double sleep_sec = 0.0;
    std::vector<float> cmd;

    {
      std::lock_guard<std::mutex> lk(mu_);
      if (!active_ || stamp_.empty()) {
        idx = SIZE_MAX;
      } else if (replay_index_ >= stamp_.size()) {
        active_ = false;
        idx = SIZE_MAX;
      } else {
        idx = replay_index_;
        cmd = actions_[idx];
        has_next = (idx + 1 < stamp_.size());
        if (has_next)
          sleep_sec = std::max(0.0, stamp_[idx + 1] - stamp_[idx]);
        ++replay_index_;
      }
    }

    if (idx == SIZE_MAX) {
      if (!active_ && running_) {
        // idle
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (hand_ && !cmd.empty()) {
      sharpa::Error err = hand_->set_joint_position(cmd, false);
      if (err.code != 0)
        std::cerr << "set_joint_position failed: " << err.message << "\n";
    }

    if (has_next && sleep_sec > 0.0)
      std::this_thread::sleep_for(
          std::chrono::duration<double>(sleep_sec));
    else if (!has_next) {
      active_ = false;
      break;
    }
  }
  running_.store(false);
}

void waitForReplayCompletion(H5Replay& replay, const char* step_name) {
  try {
    while (replay.running() && replay.active()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      if (replay.stamp_count() > 0) {
        size_t i = replay.replay_index();
        double pct = 100.0 * static_cast<double>(i) /
                       static_cast<double>(replay.stamp_count());
        if (pct > 100.0) pct = 100.0;
        std::cout << "  Replay progress: " << pct << "% (" << i << "/"
                  << replay.stamp_count() << ")\r" << std::flush;
      }
    }
  } catch (...) {
  }
  std::cout << "\n[" << step_name << "] Stopping replay...\n";
  replay.close();
  std::cout << "Replay stopped\n";
}

}  // namespace gesture
