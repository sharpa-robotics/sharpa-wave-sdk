#pragma once

#include <nlohmann/json.hpp>

#include <map>
#include <string>
#include <vector>

namespace gesture {

extern const std::vector<std::string> kJointNames;

/// Parse example_gesture_data.txt style file (blocks separated by ---, action:/state: sections).
std::vector<nlohmann::json> parseTxtFileToGestures(const std::string& path);

/// Load a JSON array of gestures from file (same schema as Python GESTURE_DATA).
std::vector<nlohmann::json> loadGesturesJsonArray(const std::string& path);

bool saveGestureToJson(const nlohmann::json& data, const std::string& dir,
                       const std::string& filename);

std::vector<std::string> saveGesturesFromArray(const std::vector<nlohmann::json>& gestures,
                                                 const std::string& output_dir,
                                                 const std::string& filename_prefix);

std::vector<float> jointVectorFromObject(const nlohmann::json& obj);

/// action/state may be a flat object (joint->rad) or a numeric array in joint index order.
std::vector<float> jointVectorFromJsonValue(const nlohmann::json& v);

void ensureExportDirs(const std::string& exports_dir, const std::string& output_dir);

std::vector<std::string> listJsonFilesInFolder(const std::string& folder);

}  // namespace gesture
