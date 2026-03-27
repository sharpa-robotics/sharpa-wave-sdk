// C++ counterpart of sample/python/gesture_workflow/gesture_workflow_example.py (main workflows).

#include "gesture_device.hpp"
#include "gesture_io.hpp"
#include "h5_replay.hpp"
#include "hdf5_gestures.hpp"

#include "SharpaWaveSDK.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <iostream>
#include <string>

namespace {

std::string argAfter(int argc, char** argv, const char* flag, const std::string& def) {
  for (int i = 1; i + 1 < argc; ++i)
    if (std::string(argv[i]) == flag) return argv[i + 1];
  return def;
}
bool hasArg(int argc, char** argv, const char* flag) {
  for (int i = 1; i < argc; ++i)
    if (std::string(argv[i]) == flag) return true;
  return false;
}

sharpa::HandSide parseHand(const std::string& s) {
  return (s == "LEFT" || s == "left") ? sharpa::HandSide::LEFT : sharpa::HandSide::RIGHT;
}

void usage() {
  std::cout
      << "sharpa_gesture_workflow_example --workflow <name> [options]\n"
      << "  predefined | external | record | advanced | batch | pipeline | interactive |\n"
      << "  grouped | replay | replay-existing\n"
      << "Options:\n"
      << "  --gesture-file PATH     (grouped / external, default gesture_workflow/example_gesture_data.txt)\n"
      << "  --input-file PATH       (external)\n"
      << "  --h5-file PATH          (replay-existing)\n"
      << "  --hand-side LEFT|RIGHT\n"
      << "  --discovery-timeout SEC (default 10)\n"
      << "  --predefined-json PATH\n";
}

bool replayH5OnDevice(const std::string& h5_path, sharpa::HandSide side, double disc_timeout) {
  using namespace gesture;
  auto& mgr = sharpa::SharpaWaveManager::get_instance();
  if (!waitForDeviceDiscovery(mgr, disc_timeout)) {
    std::cerr << "No device for replay\n";
    return false;
  }
  try {
    sharpa::SharpaWave& hand = mgr.connect(side);
    if (!hand.start()) std::cerr << "Warning: start() failed\n";
    sharpa::Error em = hand.set_control_mode(sharpa::ControlMode::POSITION);
    if (em.code != 0) std::cerr << "set_control_mode: " << em.message << "\n";
    H5Replay replay(&hand);
    replay.start(h5_path);
    waitForReplayCompletion(replay, "replay");
    hand.stop();
    mgr.disconnect_all();
    return true;
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    try {
      mgr.disconnect_all();
    } catch (...) {
    }
    return false;
  }
}

}  // namespace

int main(int argc, char** argv) {
  using namespace gesture;
  std::string workflow = argAfter(argc, argv, "--workflow", "");
  if (workflow.empty() || hasArg(argc, argv, "--help")) {
    usage();
    return workflow.empty() ? 1 : 0;
  }

  std::string gesture_file =
      argAfter(argc, argv, "--gesture-file", "gesture_workflow/example_gesture_data.txt");
  std::string input_file = argAfter(argc, argv, "--input-file", gesture_file);
  std::string h5_file = argAfter(argc, argv, "--h5-file", "output/replay_gestures.hdf5");
  std::string predef = argAfter(argc, argv, "--predefined-json", "gesture_workflow/predefined_gestures.json");
  sharpa::HandSide hand_side = parseHand(argAfter(argc, argv, "--hand-side", "RIGHT"));
  double disc = std::stod(argAfter(argc, argv, "--discovery-timeout", "10.0"));

  ensureExportDirs("exports", "output");

  if (workflow == "predefined") {
    auto gestures = loadGesturesJsonArray(predef);
    if (gestures.empty()) return 1;
    auto files = saveGesturesFromArray(gestures, "exports", "PredefinedGesture");
    GenH5Params p;
    p.json_files = files;
    p.h5_file = "output/predefined_gestures.hdf5";
    p.interval = 1.0;
    p.sort_by = "number";
    if (!genH5(p)) return 1;
    return 0;
  }

  if (workflow == "external") {
    auto gestures = parseTxtFileToGestures(input_file);
    if (gestures.empty()) return 1;
    auto files = saveGesturesFromArray(gestures, "exports", "ExternalGesture");
    GenH5Params p;
    p.json_files = files;
    p.h5_file = "output/external_gestures.hdf5";
    p.interval = 1.5;
    p.sort_by = "time";
    if (!genH5(p)) return 1;
    return 0;
  }

  if (workflow == "record") {
    std::string jf = recordGestureFromDevice("exports", hand_side, disc, "");
    if (jf.empty()) return 1;
    GenH5Params p;
    p.json_files = {jf};
    p.h5_file = "output/gesture_from_device.hdf5";
    p.interval = 1.0;
    if (!genH5(p)) return 1;
    return 0;
  }

  if (workflow == "advanced") {
    auto gestures = loadGesturesJsonArray(predef);
    if (gestures.empty()) return 1;
    auto saved = saveGesturesFromArray(gestures, "exports", "AdvancedGesture");
    if (saved.size() < 2) return 1;
    std::vector<std::string> first2(saved.begin(), saved.begin() + 2);
    GenH5Params p0;
    p0.json_files = first2;
    p0.h5_file = "output/advanced_workflow.hdf5";
    p0.sort_by = "time";
    p0.repeat_count = 2;
    p0.interval = 0.5;
    p0.samples_per_gesture = 8;
    if (!genH5(p0)) return 1;
    if (saved.size() > 2) {
      std::vector<std::string> rest(saved.begin() + 2, saved.end());
      GenH5Params p1;
      p1.json_files = rest;
      p1.h5_file = "output/advanced_workflow.hdf5";
      p1.append = true;
      p1.sort_by = "number";
      p1.repeat_count = 1;
      p1.interval = 1.0;
      p1.samples_per_gesture = 5;
      if (!genH5(p1)) return 1;
    }
    return 0;
  }

  if (workflow == "batch" || workflow == "pipeline" || workflow == "interactive") {
    auto gestures = loadGesturesJsonArray(predef);
    if (gestures.empty()) return 1;
    std::string prefix = "BatchGesture";
    if (workflow == "pipeline") prefix = "PipelineGesture";
    if (workflow == "interactive") prefix = "InteractiveGesture";
    std::vector<nlohmann::json> use = gestures;
    if (workflow == "interactive" && gestures.size() > 3)
      use.assign(gestures.begin(), gestures.begin() + 3);
    auto files = saveGesturesFromArray(use, "exports", prefix);
    GenH5Params p;
    p.json_files = files;
    p.h5_file = std::string("output/") +
                (workflow == "batch"
                     ? "batch_gestures.hdf5"
                     : workflow == "pipeline" ? "pipeline_gestures.hdf5" : "interactive_gestures.hdf5");
    p.sort_by = workflow == "interactive" ? "time" : "number";
    p.interval = workflow == "batch" ? 0.8 : 1.0;
    p.repeat_count = workflow == "batch" ? 3 : 1;
    p.samples_per_gesture = workflow == "batch" ? 10 : 5;
    if (!genH5(p)) return 1;
    return 0;
  }

  if (workflow == "grouped") {
    auto gestures = parseTxtFileToGestures(gesture_file);
    if (gestures.empty()) return 1;
    auto saved = saveGesturesFromArray(gestures, "exports", "GroupedGesture");
    const char* out_h5 = "output/grouped_gestures.hdf5";
    std::remove(out_h5);
    if (saved.size() >= 3) {
      GenH5Params g1;
      g1.json_files.assign(saved.begin(), saved.begin() + 3);
      g1.h5_file = out_h5;
      g1.interval = 2.0;
      g1.repeat_count = 5;
      g1.sort_by = "number";
      g1.append = false;
      if (!genH5(g1)) return 1;
    }
    if (saved.size() >= 6) {
      GenH5Params g2;
      g2.json_files.assign(saved.begin() + 3, saved.begin() + 6);
      g2.h5_file = out_h5;
      g2.interval = 3.0;
      g2.repeat_count = 2;
      g2.sort_by = "number";
      g2.append = true;
      if (!genH5(g2)) return 1;
    }
    if (saved.size() >= 9) {
      GenH5Params g3;
      g3.json_files.assign(saved.begin() + 6, saved.begin() + 9);
      g3.h5_file = out_h5;
      g3.interval = 3.0;
      g3.repeat_count = 5;
      g3.sort_by = "number";
      g3.append = true;
      if (!genH5(g3)) return 1;
    }
    if (hasArg(argc, argv, "--no-replay")) return 0;
    replayH5OnDevice(out_h5, hand_side, disc);
    return 0;
  }

  if (workflow == "replay") {
    auto gestures = loadGesturesJsonArray(predef);
    if (gestures.empty()) return 1;
    auto files = saveGesturesFromArray(gestures, "exports", "ReplayGesture");
    GenH5Params p;
    p.json_files = files;
    p.h5_file = "output/replay_gestures.hdf5";
    p.sort_by = "number";
    p.interval = 1.0;
    if (!genH5(p)) return 1;
    replayH5OnDevice("output/replay_gestures.hdf5", hand_side, disc);
    return 0;
  }

  if (workflow == "replay-existing") {
    replayH5OnDevice(h5_file, hand_side, disc);
    return 0;
  }

  std::cerr << "Unknown workflow: " << workflow << "\n";
  return 1;
}
