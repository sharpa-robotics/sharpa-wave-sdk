#include "hdf5_gestures.hpp"

#include "gesture_io.hpp"

#include <hdf5.h>

#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/stat.h>

namespace gesture {

static double convertTimeUnit(double value, const std::string& unit) {
  std::string u = unit;
  for (auto& c : u) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (u == "s" || u == "sec" || u == "second" || u == "seconds") return value;
  if (u == "ms" || u == "millisecond" || u == "milliseconds") return value * 1e-3;
  if (u == "us" || u == "microsecond" || u == "microseconds") return value * 1e-6;
  if (u == "ns" || u == "nanosecond" || u == "nanoseconds") return value * 1e-9;
  std::cerr << "Unknown interval unit '" << unit << "', assuming seconds\n";
  return value;
}

static int gestureNumberFromFilename(const std::string& path) {
  const std::string base = path.substr(path.find_last_of('/') + 1);
  size_t pos = base.find('_');
  if (pos == std::string::npos) return 1 << 30;
  try {
    return std::stoi(base.substr(pos + 1));
  } catch (...) {
    return 1 << 30;
  }
}

static void sortJsonFiles(std::vector<std::string>& files, const std::string& sort_by) {
  if (sort_by == "none") return;
  if (sort_by == "number") {
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
      return gestureNumberFromFilename(a) < gestureNumberFromFilename(b);
    });
  } else if (sort_by == "time") {
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
      struct stat sa {}, sb {};
      stat(a.c_str(), &sa);
      stat(b.c_str(), &sb);
      return sa.st_mtime < sb.st_mtime;
    });
  }
}

static bool readDataset1D(hid_t file, const char* path, std::vector<double>& out) {
  hid_t d = H5Dopen2(file, path, H5P_DEFAULT);
  if (d < 0) return false;
  hid_t sp = H5Dget_space(d);
  hsize_t dims[16];
  int nd = H5Sget_simple_extent_dims(sp, dims, nullptr);
  H5Sclose(sp);
  if (nd != 1) {
    H5Dclose(d);
    return false;
  }
  out.resize(static_cast<size_t>(dims[0]));
  herr_t st = H5Dread(d, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, out.data());
  H5Dclose(d);
  return st >= 0;
}

static bool readDataset2D(hid_t file, const char* path, std::vector<std::vector<float>>& out) {
  hid_t d = H5Dopen2(file, path, H5P_DEFAULT);
  if (d < 0) return false;
  hid_t sp = H5Dget_space(d);
  hsize_t dims[2];
  int nd = H5Sget_simple_extent_dims(sp, dims, nullptr);
  H5Sclose(sp);
  if (nd != 2) {
    H5Dclose(d);
    return false;
  }
  const hsize_t T = dims[0], J = dims[1];
  std::vector<float> flat(T * J);
  herr_t st = H5Dread(d, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, flat.data());
  H5Dclose(d);
  if (st < 0) return false;
  out.assign(T, std::vector<float>(J));
  for (hsize_t t = 0; t < T; ++t)
    for (hsize_t j = 0; j < J; ++j) out[t][j] = flat[t * J + j];
  return true;
}

static bool loadExistingH5(const std::string& path, std::vector<double>& stamp,
                           std::vector<std::vector<float>>& act,
                           std::vector<std::vector<float>>& st) {
  hid_t f = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if (f < 0) return false;
  bool ok = readDataset1D(f, "/stamp", stamp) && readDataset2D(f, "/action/position", act) &&
            readDataset2D(f, "/state/position", st);
  H5Fclose(f);
  return ok && stamp.size() == act.size() && stamp.size() == st.size();
}

static void writeFullH5(const std::string& path, const std::vector<double>& stamp,
                        const std::vector<std::vector<float>>& act,
                        const std::vector<std::vector<float>>& st) {
  const size_t T = stamp.size();
  const size_t J = kJointNames.size();
  hid_t file = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if (file < 0) {
    std::cerr << "H5Fcreate failed: " << path << "\n";
    return;
  }
  hsize_t td = T;
  hid_t space1 = H5Screate_simple(1, &td, nullptr);
  hid_t dstamp =
      H5Dcreate2(file, "stamp", H5T_NATIVE_DOUBLE, space1, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(dstamp, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, stamp.data());
  H5Dclose(dstamp);
  H5Sclose(space1);

  std::vector<float> actflat(T * J), stflat(T * J);
  for (size_t t = 0; t < T; ++t) {
    for (size_t j = 0; j < J; ++j) {
      actflat[t * J + j] =
          (t < act.size() && j < act[t].size()) ? act[t][j] : 0.f;
      stflat[t * J + j] =
          (t < st.size() && j < st[t].size()) ? st[t][j] : 0.f;
    }
  }
  hsize_t dims2[2] = {T, J};
  hid_t space2 = H5Screate_simple(2, dims2, nullptr);
  hid_t ga = H5Gcreate2(file, "action", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t da =
      H5Dcreate2(ga, "position", H5T_NATIVE_FLOAT, space2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(da, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, actflat.data());
  H5Dclose(da);
  H5Gclose(ga);
  hid_t gs = H5Gcreate2(file, "state", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  hid_t ds =
      H5Dcreate2(gs, "position", H5T_NATIVE_FLOAT, space2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  H5Dwrite(ds, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, stflat.data());
  H5Dclose(ds);
  H5Gclose(gs);
  H5Sclose(space2);
  H5Fclose(file);
}

bool genH5(const GenH5Params& p) {
  if (p.json_files.empty()) {
    std::cerr << "genH5: no json files\n";
    return false;
  }
  std::vector<std::string> files = p.json_files;
  sortJsonFiles(files, p.sort_by);

  std::vector<double> stamp;
  std::vector<std::vector<float>> act, st;
  const double interval_sec = convertTimeUnit(p.interval, p.interval_unit);
  const double sub_interval =
      p.samples_per_gesture > 0 ? interval_sec / p.samples_per_gesture : interval_sec;

  double stamp_val = 0.0;
  if (p.start_stamp >= 0.0)
    stamp_val = p.start_stamp;
  else if (p.append) {
    struct stat sb {};
    if (stat(p.h5_file.c_str(), &sb) == 0) {
      if (!loadExistingH5(p.h5_file, stamp, act, st)) {
        std::cerr << "append: failed to read existing H5, overwriting\n";
        stamp.clear();
        act.clear();
        st.clear();
        stamp_val = 0.0;
      } else if (!stamp.empty()) {
        stamp_val = stamp.back() + sub_interval;
        // will append new rows below; keep existing stamp/act/st and continue
      }
    }
  } else {
    stamp.clear();
    act.clear();
    st.clear();
  }

  for (int rep = 0; rep < p.repeat_count; ++rep) {
    for (const auto& jf : files) {
      std::ifstream inf(jf);
      if (!inf) {
        std::cerr << "skip missing " << jf << "\n";
        continue;
      }
      nlohmann::json data;
      try {
        inf >> data;
      } catch (const std::exception& e) {
        std::cerr << "bad json " << jf << ": " << e.what() << "\n";
        continue;
      }
      for (int s = 0; s < p.samples_per_gesture; ++s) {
        stamp_val += sub_interval;
        stamp.push_back(stamp_val);
        nlohmann::json aj, sj;
        if (data.contains("action")) {
          const auto& a = data["action"];
          if (a.is_object() && a.contains("position"))
            aj = a["position"];
          else
            aj = a;
        }
        if (data.contains("state")) {
          const auto& sjroot = data["state"];
          if (sjroot.is_object() && sjroot.contains("position"))
            sj = sjroot["position"];
          else
            sj = sjroot;
        }
        act.push_back(jointVectorFromJsonValue(aj));
        st.push_back(jointVectorFromJsonValue(sj));
      }
    }
  }

  writeFullH5(p.h5_file, stamp, act, st);
  std::cout << "Wrote " << p.h5_file << " (" << stamp.size() << " samples)\n";
  return true;
}

}  // namespace gesture
