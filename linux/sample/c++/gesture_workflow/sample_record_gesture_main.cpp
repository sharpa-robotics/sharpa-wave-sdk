// C++ counterpart of sample/python/gesture_workflow/sample_record_gesture.py (CLI subset).

#include "gesture_device.hpp"
#include "gesture_io.hpp"

#include <iostream>
#include <string>

static std::string argAfter(int argc, char** argv, const char* flag, const std::string& def) {
  for (int i = 1; i + 1 < argc; ++i)
    if (std::string(argv[i]) == flag) return argv[i + 1];
  return def;
}
static bool hasArg(int argc, char** argv, const char* flag) {
  for (int i = 1; i < argc; ++i)
    if (std::string(argv[i]) == flag) return true;
  return false;
}

static void usage() {
  std::cerr
      << "Usage:\n"
      << "  sharpa_sample_record_gesture [--output-dir DIR] [--filename-prefix PREFIX]\n"
      << "      (writes predefined_gestures.json data from built-in path or --predefined-json)\n"
      << "  sharpa_sample_record_gesture --from-device [--hand-side LEFT|RIGHT] [--output-dir DIR]\n"
      << "  sharpa_sample_record_gesture --input-file FILE.txt [--output-dir DIR] [--filename-prefix P]\n"
      << "  --predefined-json PATH   default: gesture_workflow/predefined_gestures.json\n";
}

int main(int argc, char** argv) {
  using namespace gesture;
  std::string out_dir = argAfter(argc, argv, "--output-dir", "exports");
  std::string prefix = argAfter(argc, argv, "--filename-prefix", "Gesture");
  std::string predef = argAfter(argc, argv, "--predefined-json", "gesture_workflow/predefined_gestures.json");

  if (hasArg(argc, argv, "--help") || hasArg(argc, argv, "-h")) {
    usage();
    return 0;
  }

  ensureExportDirs(out_dir, "output");

  if (hasArg(argc, argv, "--from-device")) {
    std::string hs = argAfter(argc, argv, "--hand-side", "RIGHT");
    sharpa::HandSide side =
        (hs == "LEFT" || hs == "left") ? sharpa::HandSide::LEFT : sharpa::HandSide::RIGHT;
    double timeout = std::stod(argAfter(argc, argv, "--discovery-timeout", "10.0"));
    std::string fn = argAfter(argc, argv, "--filename", "");
    std::string path = recordGestureFromDevice(out_dir, side, timeout, fn);
    if (path.empty()) return 1;
    std::cout << "Saved: " << path << "\n";
    return 0;
  }

  std::string in_file = argAfter(argc, argv, "--input-file", "");
  if (!in_file.empty()) {
    auto gestures = parseTxtFileToGestures(in_file);
    if (gestures.empty()) return 1;
    auto paths = saveGesturesFromArray(gestures, out_dir, prefix);
    if (paths.empty()) return 1;
    for (const auto& p : paths) std::cout << p << "\n";
    return 0;
  }

  auto gestures = loadGesturesJsonArray(predef);
  if (gestures.empty()) {
    std::cerr << "No gestures in " << predef << " (set --predefined-json)\n";
    usage();
    return 1;
  }
  auto paths = saveGesturesFromArray(gestures, out_dir, prefix);
  if (paths.empty()) return 1;
  for (const auto& p : paths) std::cout << p << "\n";
  return 0;
}
