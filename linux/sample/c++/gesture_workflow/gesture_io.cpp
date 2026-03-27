#include "gesture_io.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace gesture {

const std::vector<std::string> kJointNames = {
    "thumb_CMC_FE",  "thumb_CMC_AA",  "thumb_MCP_FE", "thumb_MCP_AA", "thumb_IP",
    "index_MCP_FE",  "index_MCP_AA",  "index_PIP",    "index_DIP",    "middle_MCP_FE",
    "middle_MCP_AA", "middle_PIP",    "middle_DIP",   "ring_MCP_FE",  "ring_MCP_AA",
    "ring_PIP",      "ring_DIP",      "pinky_CMC",    "pinky_MCP_FE", "pinky_MCP_AA",
    "pinky_PIP",     "pinky_DIP"};

static std::string trim(std::string s) {
  auto not_space = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

static bool startsWith(const std::string& s, const std::string& p) {
  return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

std::vector<float> jointVectorFromObject(const nlohmann::json& obj) {
  std::vector<float> v(kJointNames.size(), 0.f);
  if (!obj.is_object()) return v;
  for (size_t i = 0; i < kJointNames.size(); ++i) {
    auto it = obj.find(kJointNames[i]);
    if (it != obj.end() && it->is_number()) v[i] = static_cast<float>(it->get<double>());
  }
  return v;
}

std::vector<float> jointVectorFromJsonValue(const nlohmann::json& v) {
  if (v.is_array()) {
    std::vector<float> r(kJointNames.size(), 0.f);
    for (size_t i = 0; i < kJointNames.size() && i < v.size(); ++i) {
      if (v[i].is_number()) r[i] = static_cast<float>(v[i].get<double>());
    }
    return r;
  }
  if (v.is_object()) return jointVectorFromObject(v);
  return std::vector<float>(kJointNames.size(), 0.f);
}

std::vector<nlohmann::json> loadGesturesJsonArray(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    std::cerr << "Error: cannot open " << path << "\n";
    return {};
  }
  nlohmann::json j;
  try {
    in >> j;
  } catch (const std::exception& e) {
    std::cerr << "Error: JSON parse failed: " << e.what() << "\n";
    return {};
  }
  std::vector<nlohmann::json> out;
  if (j.is_array()) {
    for (auto& el : j) {
      if (el.is_object() && el.contains("action") && el.contains("state")) out.push_back(el);
    }
  } else if (j.is_object() && j.contains("action") && j.contains("state")) {
    out.push_back(j);
  }
  return out;
}

static void parseKeyValueBlock(const std::string& block,
                               std::vector<nlohmann::json>* out_gestures) {
  std::istringstream iss(block);
  std::string line;
  nlohmann::json current = nlohmann::json::object();
  current["action"] = nlohmann::json::object();
  current["state"] = nlohmann::json::object();
  std::string section;  // "action", "state", or empty (both)

  while (std::getline(iss, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;
    std::string low = line;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);
    if (low == "action:" || low == "[action]") {
      section = "action";
      continue;
    }
    if (low == "state:" || low == "[state]") {
      section = "state";
      continue;
    }
    auto eq = line.find('=');
    if (eq != std::string::npos) {
      std::string key = trim(line.substr(0, eq));
      std::string val = trim(line.substr(eq + 1));
      try {
        double d = std::stod(val);
        if (section == "action")
          current["action"][key] = d;
        else if (section == "state")
          current["state"][key] = d;
        else {
          current["action"][key] = d;
          current["state"][key] = d;
        }
      } catch (...) {
        continue;
      }
      continue;
    }
    auto comma = line.find(',');
    if (comma != std::string::npos) {
      std::istringstream ls(line);
      std::string key, av, sv;
      std::getline(ls, key, ',');
      std::getline(ls, av, ',');
      std::getline(ls, sv, ',');
      key = trim(key);
      av = trim(av);
      sv = trim(sv);
      try {
        double da = std::stod(av);
        double ds = sv.empty() ? da : std::stod(sv);
        current["action"][key] = da;
        current["state"][key] = ds;
      } catch (...) {
        continue;
      }
    }
  }
  if (!current["action"].empty() || !current["state"].empty()) out_gestures->push_back(current);
}

std::vector<nlohmann::json> parseTxtFileToGestures(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    std::cerr << "Error: file not found: " << path << "\n";
    return {};
  }
  std::stringstream buffer;
  buffer << in.rdbuf();
  std::string content = buffer.str();

  // Try whole-file JSON first
  try {
    nlohmann::json j = nlohmann::json::parse(content);
    std::vector<nlohmann::json> gestures;
    if (j.is_array()) {
      for (auto& el : j) {
        if (el.is_object() && el.contains("action") && el.contains("state"))
          gestures.push_back(el);
      }
      if (!gestures.empty()) return gestures;
    } else if (j.is_object() && j.contains("action") && j.contains("state")) {
      return {j};
    }
  } catch (...) {
    // fall through to txt blocks
  }

  std::vector<nlohmann::json> gestures;
  std::string block;
  std::istringstream lines(content);
  std::string ln;
  while (std::getline(lines, ln)) {
    std::string t = trim(ln);
    if (startsWith(t, "---") || startsWith(t, "===")) {
      if (!block.empty()) {
        parseKeyValueBlock(block, &gestures);
        block.clear();
      }
    } else {
      if (!block.empty()) block += '\n';
      block += ln;
    }
  }
  if (!block.empty()) parseKeyValueBlock(block, &gestures);

  if (gestures.empty()) std::cerr << "Warning: no valid gestures in " << path << "\n";
  return gestures;
}

bool saveGestureToJson(const nlohmann::json& data, const std::string& dir,
                       const std::string& filename) {
  mkdir(dir.c_str(), 0755);
  std::string path = dir;
  if (!path.empty() && path.back() != '/') path += '/';
  path += filename;
  std::ofstream out(path);
  if (!out) return false;
  out << data.dump(2);
  return true;
}

std::vector<std::string> saveGesturesFromArray(const std::vector<nlohmann::json>& gestures,
                                               const std::string& output_dir,
                                               const std::string& filename_prefix) {
  mkdir(output_dir.c_str(), 0755);
  std::vector<std::string> paths;
  for (size_t i = 0; i < gestures.size(); ++i) {
    if (!gestures[i].is_object() || !gestures[i].contains("action") ||
        !gestures[i].contains("state")) {
      std::cerr << "Warning: skip index " << i << " (bad gesture object)\n";
      continue;
    }
    std::string fn = filename_prefix + "_" + std::to_string(i) + ".json";
    if (saveGestureToJson(gestures[i], output_dir, fn))
      paths.push_back(output_dir + (output_dir.back() == '/' ? "" : "/") + fn);
  }
  return paths;
}

void ensureExportDirs(const std::string& exports_dir, const std::string& output_dir) {
  mkdir(exports_dir.c_str(), 0755);
  mkdir(output_dir.c_str(), 0755);
}

std::vector<std::string> listJsonFilesInFolder(const std::string& folder) {
  std::vector<std::string> out;
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!fs::is_directory(folder, ec)) return out;
  for (const auto& e : fs::directory_iterator(folder, ec)) {
    if (!e.is_regular_file()) continue;
    if (e.path().extension() == ".json") out.push_back(e.path().string());
  }
  std::sort(out.begin(), out.end());
  return out;
}

}  // namespace gesture
