#pragma once

#include <string>
#include <vector>

namespace gesture {

struct GenH5Params {
  std::vector<std::string> json_files;
  std::string h5_file;
  double interval = 1.0;
  std::string interval_unit = "s";
  int samples_per_gesture = 5;
  int repeat_count = 1;
  std::string sort_by = "number";  // number | time | none
  bool append = false;
  double start_stamp = -1.0;  // <0 = auto
};

/// Build HDF5 with datasets /stamp, /action/position, /state/position (same layout as Python json_to_h5).
bool genH5(const GenH5Params& p);

}  // namespace gesture
