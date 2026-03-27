// C++ counterpart of sample/python/gesture_workflow/json_to_h5.py (CLI subset).

#include "hdf5_gestures.hpp"
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
      << "  sharpa_json_to_h5 --json-folder <dir> --output <file.hdf5> [options]\n"
      << "  sharpa_json_to_h5 --json-files a.json b.json --output <file.hdf5> [options]\n"
      << "Options:\n"
      << "  --append\n"
      << "  --interval <float>   (default 1.0)\n"
      << "  --interval-unit s|ms (default s)\n"
      << "  --samples-per-gesture <n> (default 5)\n"
      << "  --repeat <n> (default 1)\n"
      << "  --sort-by number|time|none (default number)\n";
}

int main(int argc, char** argv) {
  using namespace gesture;
  std::string folder, out = "output.hdf5";
  std::vector<std::string> files;
  bool use_folder = false;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "--json-folder" && i + 1 < argc) {
      folder = argv[++i];
      use_folder = true;
    } else if (a == "--output" && i + 1 < argc)
      out = argv[++i];
    else if (a == "-o" && i + 1 < argc)
      out = argv[++i];
    else if (a == "--json-files") {
      while (i + 1 < argc && argv[i + 1][0] != '-') files.push_back(argv[++i]);
    }
  }

  if (use_folder) files = listJsonFilesInFolder(folder);

  if (files.empty()) {
    usage();
    return 1;
  }

  GenH5Params p;
  p.json_files = std::move(files);
  p.h5_file = out;
  p.append = hasArg(argc, argv, "--append");
  p.interval = std::stod(argAfter(argc, argv, "--interval", "1.0"));
  p.interval_unit = argAfter(argc, argv, "--interval-unit", "s");
  p.samples_per_gesture = std::stoi(argAfter(argc, argv, "--samples-per-gesture", "5"));
  p.repeat_count = std::stoi(argAfter(argc, argv, "--repeat", "1"));
  p.sort_by = argAfter(argc, argv, "--sort-by", "number");

  if (!genH5(p)) return 2;
  return 0;
}
