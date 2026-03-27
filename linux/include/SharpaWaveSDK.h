#ifndef SHARPA_SDK_H
#define SHARPA_SDK_H

#include <stdint.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define SHARPA_SDK_VERSION "4.6.5"
/**
 * @file SharpaWaveSDK.h
 * @brief All-in-one header file for SharpaWaveSDK
 *
 * This header file contains all necessary declarations and definitions for
 * using libSharpaHand.so. Users can simply include this single header to access
 * all SharpaWaveSDK functionality.
 *
 * Usage:
 * @code
 * #include "SharpaWaveSDK.h"
 *
 * auto manager = sharpa::SharpaWaveManager::get_instance();
 * // Get available devices
 * auto devices = manager.get_available_devices();
 *
 * // Connect to a device
 * auto& wave = manager.connect("HAND-L-0001");
 *
 * // Get device state
 * auto state = wave.get_states();
 *
 * // Set joint positions
 * std::vector<float> angles = {0.1f, 0.2f, 0.3f, ...};
 * wave.set_joint_position(angles);
 *
 * // Use tactile functionality
 * auto frame = wave.fetch_tactile_frame(0);
 *
 * @endcode
 */


namespace sharpa {
// Enums
enum class DeviceType {
  HAND = 0,       ///< 0: Robotic hand (default)
  GLOVE = 1,      ///< 1: Sensor glove
  UNKNOWN = 0xFF  ///< 0xFF: Undetermined device type
};

enum class HandSide {
  LEFT = 0,  ///< 0: Left-handed configuration
  RIGHT = 1  ///< 1: Right-handed configuration
};

/**
 * @brief Control Source
 * @details Control Source is used to filter udp packet by content,
 *          Unmatched control source packet will be dropped
 */
enum class ControlSource {
  APP = 0,      ///< 0: Controlled by master device
  GLOVE = 1,      ///< 1: COntrolled by Glove
  SDK = 4,         ///< 4: Controlled via SDK API
  IDLE = 7  ///< 7: IDLE
};

/**
 * @brief Control mode enum,
 * @details Control mode is used to control the motor behavior
 *          FLOATING: motor keeps current force, you can force joint to move mannully
 *          POSITION: Position control mode
 *          MIT: MIT-style impedance control (same role as legacy force mode id 11)
 *          UNKNOWN: Unknown control mode
 *          ADMITTANCE: Admittance control mode (reserved)
 */
enum class ControlMode {
  FLOATING = 10,    ///< 10: Gravity compensation mode
  POSITION = 13,    ///< 13: Position control mode
  ADMITTANCE = 14,  ///< 14: Admittance control mode
  MIT = 15,         ///< 15: MIT impedance / legacy force control
  UNKNOWN = 0xFF    ///< 0xFF: Unknown control mode
};

/**
 * @brief Error code enum
 * @details Error code is generated with communication with hardware,
 *        thus mainly caused by tcp connection and packet reception.
 */

enum class ErrorCode : int {
  SUCCESS = 0x00,                             ///< 0x00: Operation successful
  INVALID_INPUT_PARAMETER = 0x01,              ///< 0x01: Invalid parameter passed
  FAILURE_TO_CONNECT_TO_SERVER = 0x02,         ///< 0x02: Connection failure
  NO_VALID_DATA_RETURNED = 0x03,              ///< 0x03: No response data received
  SERVER_DOES_NOT_HAVE_ENOUGH_MEMORY = 0x04,  ///< 0x04: Device memory full
  SERVER_DOES_NOT_SUPPORT_THIS_COMMAND_YET = 0x05, ///< 0x05: Unsupported command
  SERVER_FAILS_TO_COMMUNICATE_WITH_DRIVER = 0x06,  ///< 0x06: Internal communication failure
  INNER_ERROR = 0x07,                         ///< 0x07: Internal system error
  SAVE_PARAM_TO_FLASH_ERROR = 0x08,            ///< 0x08: Flash write failure
  INVALID_HEADER = 0x09,                       ///< 0x09: Protocol header invalid
  PAYLOAD_TOO_LARGE = 0x0A,                    ///< 0x0A: Data exceeds max size
  INCOMPLETE_PAYLOAD = 0x0B,                  ///< 0x0B: Partial data received
  CRC_ERROR = 0x0C,                           ///< 0x0C: Checksum validation failed
  OPERATION_NOT_ALLOWED = 0x0D,                ///< 0x0D: Command not permitted
  TCP_SEND_FAILED = 0x10,                     ///< 0x10: TCP transmission failure
  TCP_RECV_FAILED = 0x11,                     ///< 0x11: TCP reception failure
  INVALID_RESPONSE = 0x12,                    ///< 0x12: Malformed response
  TACTILE_NOT_SUPPORTED = 0x13,               ///< 0x13: Tactile feature not supported by firmware
  UNKNOWN_RSP_CODE = 0xFF,                    ///< 0xFF: Unknown response code
};

/**
 * @brief Fault code enum, fault code source from hardware & software
 * @details Fault code enum for normal, system, battery, motor, strain, motion control,
 *          temperature management, tactile, and software.
 *          Two encoding schemes exist: 8-bit legacy (0-255) and 16-bit module-based (0x01xx-0x08xx).
 *          Some faults have both encodings (same meaning, different value), e.g. 2 vs 0x0502
 *          both mean velocity limit exceeded; use the value reported by the device to match.
 */
enum class FaultCode : int {
  // Normal
  NORMAL = 0,  ///< Normal

  // 8-bit legacy fault codes (0-255). Same fault may also be reported as 16-bit below (e.g. 2 vs 0x0502).
  ANGLE_LIMIT_EXCEEDED = 1,           ///< 1: Joint angle exceeds safe limits (16-bit: 0x0501)
  VELOCITY_LIMIT_EXCEEDED = 2,       ///< 2: Joint velocity exceeds safe limits (16-bit: 0x0502)
  TORQUE_LIMIT_EXCEEDED = 3,         ///< 3: Torque (current) exceeds 80% of max (16-bit: 0x0503)
  JOINT_DISCONNECTED = 4,            ///< 4: Partial joints disconnected
  TEMPERATURE_WARM = 5,              ///< 5: Motor temperature 70-80°C (16-bit: 0x0601)
  TEMPERATURE_HOT = 6,               ///< 6: Motor temperature 80-90°C (16-bit: 0x0602)
  TEMPERATURE_CRITICAL = 7,          ///< 7: Motor temperature >90°C (16-bit: 0x0603)
  ENCODER_LINEARITY_WARNING = 8,     ///< 8: Encoder linearity warning >20% (16-bit: 0x0301)
  MOTOR_SOFTWARE_ERROR = 100,        ///< 100: Motor fault: software error (16-bit: 0x0340)
  MOTOR_OVER_VOLTAGE_ERROR = 101,    ///< 101: Motor fault: over-voltage (16-bit: 0x0341)
  MOTOR_UNDER_VOLTAGE_ERROR = 102,   ///< 102: Motor fault: under-voltage (16-bit: 0x0342)
  MOTOR_OVER_TEMPERATURE_ERROR = 103,///< 103: Motor fault: over-temperature (16-bit: 0x0343)
  MOTOR_STARTUP_FAIL_ERROR = 104,   ///< 104: Motor fault: startup failure (16-bit: 0x0344)
  MOTOR_SPEED_FEEDBACK_ERROR = 105,  ///< 105: Motor fault: speed feedback (16-bit: 0x0345)
  MOTOR_ENCODER_H_ERROR = 108,       ///< 108: Motor fault: high-speed encoder (16-bit: 0x0348)
  MOTOR_ENCODER_L_ERROR = 109,       ///< 109: Motor fault: low-speed encoder (16-bit: 0x0349)
  MOTOR_ALIGNMENT_ERROR = 112,       ///< 112: Motor fault: alignment error (16-bit: 0x034C)
  INTERNAL_COMMUNICATION_ERROR = 130,///< 130: Internal communication error
  UNDER_VOLTAGE_ERROR = 131,         ///< 131: Whole-hand under-voltage (16-bit: 0x0240)
  ZERO_ERROR = 140,                  ///< 140: Zero position error (16-bit: 0x0181)
  ENCODER_LINEARITY_ERROR = 145,     ///< 145: Encoder linearity error >50% (16-bit: 0x0352)
  UNKNOWN_ERROR = 255,               ///< 255: Unknown error

  // 01_System (module = 0x01)
  ERROR_SYSTEM_LOAD_HIGH = 0x0101,                ///< Warning System load high
  ERROR_SYSTEM_ZERO_ERROR = 0x0181,               ///< Critical System zero error
  ERROR_SYSTEM_CALIB_PARAM_LOAD_FAILED = 0x0182,  ///< Critical System calibration parameter load failed
  ERROR_SYSTEM_MOTION_CTRL_THREAD_HANG = 0x0183,  ///< Critical System motion control thread hang

  // 02_BatteryControl (module = 0x02)
  ERROR_POWER_UNDERVOLTAGE_THRESHOLD = 0x0240,  ///< Error Power undervoltage threshold
  ERROR_POWER_OVER_CURRENT = 0x0280,            ///< Critical Power over current
  ERROR_POWER_OVER_VOLTAGE = 0x0281,            ///< Critical Power over voltage

  // 03_Motor (module = 0x03)
  ERROR_MOTOR_GEAR_RATIO_WARNING = 0x0301,    ///< Warning Motor gear ratio warning
  ERROR_MOTOR_ANGLE_DEVIATION_WARNING = 0x0302,   ///< Warning Motor angle deviation over 10 degrees
  ERROR_MOTOR_SW = 0x0340,                   ///< Error Motor software error
  ERROR_MOTOR_OVERVOLTAGE = 0x0341,          ///< Error Motor overvoltage protection
  ERROR_MOTOR_UNDERVOLTAGE = 0x0342,         ///< Error Motor undervoltage protection
  ERROR_MOTOR_OVERHEAT = 0x0343,             ///< Error Motor overheat protection
  ERROR_MOTOR_START_FAIL = 0x0344,           ///< Error Motor start fail
  ERROR_MOTOR_SPEED_FB = 0x0345,             ///< Error Motor speed feedback error
  ERROR_MOTOR_ENC_HS = 0x0348,               ///< Error Motor high speed encoder error
  ERROR_MOTOR_ENC_LS = 0x0349,               ///< Error Motor low speed encoder error
  ERROR_MOTOR_ALIGN = 0x034C,                ///< Error Motor alignment error
  ERROR_MOTOR_UNKNOWN = 0x034F,              ///< Error Motor unknown error
  ERROR_MOTOR_COMMUNICATION_ERROR = 0x0351,  ///< Error Motor communication error/offline (corrected: from 0x0350 to 0x0351)
  ERROR_MOTOR_GEAR_RATIO_ERROR = 0x0352,     ///< ERROR High/Low motor ratio error > 50%
  ERROR_MOTOR_ANGLE_DEVIATION_ERROR = 0x0353,         ///< ERROR High/Low motor gap over 15 degrees

  // 04_Strain (module = 0x04)
  ERROR_STRAIN_PHYSICAL_DAMAGE_SUSPECT = 0x0401,  ///< WARNING Physical damage suspected
  ERROR_STRAIN_FORCE_OUT_OF_RANGE = 0x0402,       ///< WARNING Force/torque out of range
  ERROR_STRAIN_CALIB_PARAM_ERROR = 0x0440,        ///< Error Strain calibration parameter error
  ERROR_STRAIN_SAMPLING_ABNORMAL = 0x0441,        ///< Error Strain sampling abnormal

  // 05_Motion Control (module = 0x05)
  ERROR_JOINT_LIMIT_EXCEEDED = 0x0501,       ///< WARNING Joint angle limit exceeded
  ERROR_VELOCITY_LIMIT_EXCEEDED = 0x0502,    ///< WARNING Joint velocity limit exceeded
  ERROR_TORQUE_LIMIT_EXCEEDED = 0x0503,      ///< WARNING Joint torque limit exceeded
  ERROR_CONTROL_JITTER_OVER_LIMIT = 0x0504,  ///< WARNING Control jitter over limit
  ERROR_CONTROL_TASK_TIMEOUT = 0x0505,       ///< WARNING Control task timeout

  // 06_Temperature Management (module = 0x06)
  ERROR_TEMP_WARM = 0x0601,             ///< WARNING 70-80℃ temperature
  ERROR_TEMP_HOT = 0x0602,              ///< WARNING 80-90℃ temperature
  ERROR_TEMP_VERY_HOT = 0x0603,         ///< WARNING >90℃ temperature
  ERROR_TEMP_COOLING_FAILURE = 0x0640,  ///< ERROR Cooling system failure

  // 07_Tactile (module = 0x07)
  ERROR_TOUCH_CONFIG_READ_ERROR = 0x0701,    ///< ERROR Touch configuration read error
  ERROR_TOUCH_CONFIG_WRITE_ERROR = 0x0702,   ///< ERROR Touch configuration write error
  ERROR_TOUCH_FLASH_READ_ERROR = 0x0703,     ///< ERROR Touch flash read error
  ERROR_TOUCH_FLASH_WRITE_ERROR = 0x0704,    ///< ERROR Touch flash write error
  ERROR_TOUCH_CONFIG_UNPACK_ERROR = 0x0705,  ///< ERROR Touch configuration unpack error
  ERROR_TOUCH_CONFIG_INVALID = 0x0706,       ///< ERROR Touch configuration invalid
  WARNING_TOUCH_SENSOR_CONNECTION_UNSTABLE = 0x0707,  ///< WARNING Touch sensor connection unstable
  ERROR_TOUCH_SENSOR_NOT_DETECTED = 0x0740,  ///< ERROR Touch sensor not detected
  ERROR_TOUCH_SENSOR_MCU1_COMMUNICATION_ERROR =
      0x0741,                                   ///< ERROR Touch sensor MCU1 communication error
  ERROR_TOUCH_NPU_MODEL_FILE_MISSING = 0x0742,  ///< ERROR Touch NPU model file missing
  ERROR_TOUCH_SENSOR_I2C_COMMUNICATION_ERROR = 0x0743,  ///< ERROR Touch sensor I2C communication error
  // 08_SOFTWARE (module = 0x08)
  SW_CONFIGURED_ERROR = 0x0800,                ///< ERROR [lifecycle]Configured error
  SW_DEVICE_CONNECT_ERROR = 0x0801,            ///< ERROR [lifecycle]Device connect error
  SW_RUNNING_ERROR = 0x0802,                   ///< ERROR [lifecycle]Running error
  SW_STOP_ERROR = 0x0803,                      ///< ERROR [lifecycle]Stop error
  SW_HAND_CONSTRUCTION_ERROR = 0x0811,         ///< ERROR [hand]Hand construction error
  SW_HAND_INITIALIZE_ERROR = 0x0812,           ///< ERROR [hand]Hand initialize error
  SW_TOUCH_CONSTRUCTION_ERROR = 0x0815,        ///< ERROR [touch]Touch construction error
  SW_TOUCH_INITIALIZE_ERROR = 0x0816,          ///< ERROR [touch]Touch initialize error
  SW_TOUCH_HOST_IP_INVALID = 0x0817,           ///< ERROR [touch]Touch host IP invalid
  SW_TOUCH_HOST_PORT_OCCUPIED = 0x0818,        ///< ERROR [touch]Touch host port occupied
  SW_TOUCH_HOST_LISTEN_FAULT = 0x0819,        ///< ERROR [touch]Touch host listen fault
  SW_CONTROL_MODE_NOT_SUPPORTED = 0x0821,      ///< ERROR [function]Control mode not supported
  SW_SEND_HEARTBEAT_PACKET_FAILED = 0x0822,    ///< ERROR [function]Send heartbeat packet failed
  SW_HARDWARE_VERSION_MISMATCH = 0x0830,       ///< ERROR [function]Hardware version mismatch
  SW_FIRMWARE_VERSION_MISMATCH = 0x0831,     ///< WARNING [function]Firmware version mismatch
  SW_FIRMWARE_VERSION_CONFIG_NOT_FOUND = 0x0832,  ///< WARNING [function]Firmware version config file not found
};

/** 功能配置 uint8_t 位图：每位表示一种功能是否支持 */
static constexpr uint8_t FEATURE_FINGERTIP_TACTILE = (1u << 0);  // 位0：指尖触觉

/**
 * @brief Device status structure, status source from heartbeat packet
 * @details Device status structure for temperature, battery, error code,
 *          error joint, and temperature levels.
 */
struct DeviceStatus {
  float temperature;
  uint8_t battery;
  uint16_t error_code;
  uint32_t error_joint;
  uint64_t temperature_levels;
};

/**
 * @brief Device information structure, info source from heartbeat packet
 * @details Device information structure for manufacturer, pn, sn, firmware version,
 *          paired sn, control mode, control source, hand side, device type, mac, ip,
 *          tcp port, udp port, status, and feature_config.
 */
struct DeviceInfo {
  std::string manufacturer;
  std::string pn;
  std::string sn;
  std::string firmware_version;
  std::string paired_sn;
  ControlMode control_mode = ControlMode::UNKNOWN;
  ControlSource control_source = ControlSource::IDLE;
  HandSide hand_side = HandSide::RIGHT;
  DeviceType device_type = DeviceType::UNKNOWN;
  std::string mac;
  std::string ip;
  uint16_t tcp_port;
  uint16_t udp_port;
  DeviceStatus status;

  /** 功能配置位图 (id=20)，来自心跳 V2，每位含义见 FEATURE_* 常量 */
  uint8_t feature_config = 0;

  /** 功能配置位图 (id=20)，每位含义见 FEATURE_* 常量 */
  uint8_t get_feature_config() const { return feature_config; }
  /** 是否支持指尖触觉 */
  bool has_fingertip_tactile() const {
    return (feature_config & FEATURE_FINGERTIP_TACTILE) != 0;
  }

  std::string to_json_string() const;
};

/**
 * @brief Error structure, describe error with driver - firmware
 *         ptc communication error code and message
 * @details Error structure for error code enum and message
 */
struct Error {
  int code = 0;
  std::string message;

  Error(int code = 0, const std::string& msg = "")
      : code(code),
        message(code != 0
                    ? ("[" + std::to_string(code) + "] " +
                       (msg.empty() ? "Unknown error" : msg))
                    : msg) {}
  std::string to_string() const {
    return "Error " + std::to_string(code) + ": " + message;
  }

  operator bool() const {
      return code == 0;
  }
};

/**
 * @brief State structure, source from joint packet
 */
struct State {
  uint64_t timestamp;
  uint16_t sequence;
  std::vector<float> angles;
  std::vector<float> velocities;
  std::vector<float> torques;

  State() : timestamp(0), sequence(0) {}
  State(const uint64_t &timestamp, const uint16_t &sequence,
        const std::vector<float> &angles, const std::vector<float> &velocities,
        const std::vector<float> &torques)
      : timestamp(timestamp),
        sequence(sequence),
        angles(angles),
        velocities(velocities),
        torques(torques) {}
};

namespace tactile {
/**
 * @brief shape of n-Dim data block (tensor)
 */
class Shape {
 public:
  /**
   * @param data sizes of each dim
   */
  Shape(std::vector<size_t> data);
  Shape();

  /**
   * @return total size. e.g. {4, 6, 1} returns 24
   */
  size_t size() const;

  /**
   * @return dimension of Shape
   */
  size_t dim() const;

  /**
   * @param i i-th element of shape
   * @return e.g. shape is {1, 2, 3}, i is 1, return 2
   */
  size_t operator[](size_t i) const;

  /**
   * @param other another shape to compare
   * @return if 2 shapes are exactly the same
   */
  bool operator==(const Shape &other) const;

  /**
   * @param other another shape to compare
   * @return if 2 shapes are not exactly the same
   */
  bool operator!=(const Shape &other) const;

 private:
  std::vector<size_t> data_;
};

/**
 * @brief a block of data with specifing value type(int, float, etc.)
 */
class DataBlock {
 public:
  /** pointer of DataBlock */
  using Ptr = std::shared_ptr<DataBlock>;

  /**
   * constructor with certain shape, data not initialized
   * @param shape shape of DataBlock
   * @param unit_size e.g. 4 for float, 1 for uint8_t
   */
  DataBlock(Shape shape, size_t unit_size);

  /** copy constructor */
  DataBlock(const DataBlock &other);

  /** move constructor */
  DataBlock(DataBlock &&other);

  /** copy assignment */
  DataBlock &operator=(const DataBlock &other);

  /** move assignment */
  DataBlock &operator=(DataBlock &&other);

  /** deconstructor */
  ~DataBlock();

  /**
   * @return shape of DataBlock
   */
  Shape shape() const;

  /**
   * @return number of elements of DataBlock
   */
  size_t size() const;

  /**
   * @return number of dimensions
   */
  size_t dim() const;

  /**
   * @return number of bytes of DataBlock
   */
  size_t nbytes() const;

  /**
   * initialize all bytes to zero
   */
  void set_zero();

  /**
   * @return const data pointer
   */
  const void *data() const;

  /**
   * @return data pointer
   */
  void *data();

 private:
  void *data_;
  Shape shape_;
  size_t unit_size_;
};

/**
 * @brief tactile data frame
 */
struct Frame {
  /** pointer of data frame */
  using Ptr = std::shared_ptr<Frame>;
  /** frame id, unique and increasing for each tactile sensor */
  int frame_id;
  int channel;
  double ts;
  /**
   * contents
   * key can be ["RAW", "DEFORM", "F6", ...]
   */
  std::map<std::string, DataBlock::Ptr> content;
};

/**
 * @brief tactile data frame
 * @details for tactile data frame without shared_ptr
 */
 struct CompactFrame {
  CompactFrame() : frame_id(0), channel(-1), ts(0.0),
                   raw_shape(std::vector<size_t>()),
                   deform_shape(std::vector<size_t>()),
                   f6_shape(std::vector<size_t>()),
                   contact_point_shape(std::vector<size_t>()) {}

  int frame_id;
  /** channel, see Touch constructor */
  int channel;
  /** time stamp */
  double ts;
  Shape raw_shape, deform_shape, f6_shape, contact_point_shape;
  std::vector<uint8_t> raw_data;
  std::vector<uint8_t> deform_data;
  std::vector<float> f6_data;
  std::vector<float> contact_point_data;
};
}  // namespace tactile

/**
 * @brief Configuration structure for SharpaWave initialization
 * @details 传入 `SharpaWave(sn, config)` 或 `SharpaWaveManager::connect(sn, config)`。
 *          `tactile_config_file` 非空且未 `disable_tactile` 时，构造阶段校验路径与 JSON，失败抛出 std::invalid_argument。
 */
struct SharpaWaveConfig {
  bool disable_tactile = false;      ///< Disable tactile sensor initialization
  bool disable_sync_time = false;   ///< Disable time synchronization during start
  /// 工厂/高级用户：自定义触觉 JSON；空则 start() 时使用内置默认。
  std::string tactile_config_file;
};

/**
 * @brief SharpaWave class that combines SharpaHand and Touch functionality
 *
 * This class provides a unified interface for both hand control and tactile
 * sensing. It contains both SharpaHand and Touch as public members.
 */
class SharpaWave {
 public:
  SharpaWave(const std::string &sn);
  /// @param config 若启用触觉且 tactile_config_file 非空，构造前校验文件与 JSON，失败抛 std::invalid_argument
  SharpaWave(const std::string &sn, const SharpaWaveConfig &config);
  ~SharpaWave();
   // ============================== lifecycle ==============================
  /// \name lifecycle
  /// @{
  /// Methods for managing the device lifecycle including startup, shutdown, and status checking.
  Error reboot();
  bool start();
  bool is_tactile_ready() const;
  bool is_hand_ready() const;
  bool stop();
  void destroy();
  std::vector<FaultCode> get_sw_fault_code() const;
  /**
   * @brief Try to fix tactile port conflict by switching to an alternate port from the pool.
   * @return true if tactile started successfully on an alternate port, false otherwise.
   */
  bool retry_tactile_alternate_port();
  /**
   * @brief Get list of alternate ports that will be tried by retry_tactile_alternate_port().
   * For display only; ports are not checked for availability.
   * @return Port numbers (excluding current port) in try order.
   */
  std::vector<int> get_tactile_alternate_ports() const;
  /**
   * @brief Probe host bind and return first available port from pool (excluding current and exclude list).
   * @param exclude Port numbers to skip when probing.
   * @return Port number if one is available, 0 otherwise.
   */
  int get_next_available_tactile_port(const std::vector<int>& exclude) const;
  /**
   * @brief Try to bind tactile to the given port only. Does not retry other ports.
   * @param port Port to bind to.
   * @return true if bind and start succeeded, false otherwise.
   */
  bool bind_tactile_port(int port);
  /// @}
  // ============================== Hardware/Firmware ==============================
  /// \name Hardware/Firmware
  /// @{
  /// Methods for accessing device information, updating firmware, and managing hardware settings.
  /**
   * @brief structure device info source from heartbeat packet
   * @return DeviceInfo device info
   */
  DeviceInfo get_device_info() const;

   /**
   * @brief Synchronize device time with the provided timestamp
   * @param timestamp  (e.g."2024-01-01T12:00:00Z")
   * @return Error error code
   */
   Error sync_time();
  /**
   * @brief get device info in json string
   * @return std::string device info in json string
   */
  std::string get_device_info_json() const;
  /**
   * @brief set device IP address
   * @param ip_address device IP address
   * @details device will reboot after this function call,
   * and you should reconnect to the device after reboot,
   * @return Error error(code, message)
   */
  Error set_device_ip(const std::string& ip_address);
  std::pair<Error, std::string> get_recommended_sdk_version() const;


  /// @}
  // ============================== Joint Control ==============================
  /// \name Joint Control
  /// @{
  /// Methods for controlling joint positions, velocities, and accessing joint state information.

  /**
   * @brief get joint state
   * @return State joint state
   */
  State get_states() const;
  /**
   * @brief set joint position
   *
   * @param angles_rad joint angles in rad
   * @param interpolate whether to use interpolation (default as direct mode)
   * @return Error error(code, message)
   */
  Error set_joint_position(const std::vector<float> &angles_rad, bool interpolate = false);
  /**
   * @brief Set joint positions in radians
   * @param angles_rad joint angles in radians
   * @param interpolate whether to use interpolation (default as direct mode)
   * @return Error error(code, message)
   */
  Error set_joint_position_rad(const std::vector<float> &angles_rad, bool interpolate = false);
  /**
   * @brief Set joint positions in degrees
   * @param angles_degree joint angles in degrees
   * @param interpolate whether to use interpolation (default as direct mode)
   * @return Error error(code, message)
   */
  Error set_joint_position_degree(const std::vector<float> &angles_degree, bool interpolate = false);
  std::pair<Error, int> get_interpolate_mode() const;
  /**
   * @brief convient interface source from get_states
   * @return std::pair<Error, std::vector<float>> error and joint position in rad
   */
  std::pair<Error, std::vector<float>> get_joint_position_rad() const;
  /**
   * @brief convient interface source from get_states
   * @return std::pair<Error, std::vector<float>> error and joint position in degree
   */
  std::pair<Error, std::vector<float>> get_joint_position_degree() const;
  /// @}
  // ============================== Common Motor Setting ==============================
  /// \name Common Motor Setting
  /// @{
  /// Methods for configuring motor enable state, control mode, and control source.
  /**
   * @brief set enable state
   * @param enable True == Running mode, False == Setting mode
   * @details new state may take about 70ms to take effect
   * @return Error error(code, message)
   */
  Error set_enable_state(bool enable);
  /**
   * @brief get enable state
   * @return std::pair<Error, bool> error and enable state
   */
  std::pair<Error, bool> get_enable_state() const;
  /**
   * @brief set control mode
   * @param control_mode control mode
   * @details it will take care of enable state, and set enable state to True before return
   * @return Error error(code, message)
   */
  Error set_control_mode(const ControlMode &control_mode);
  /**
   * @brief get control mode
   * @details get cmd will not change enable state
   * @return std::pair<Error, ControlMode> error and control mode
   */
  std::pair<Error, ControlMode> get_control_mode() const;
  /**
   * @brief set control source
   * @param control_source control source
   * @details it will not change enable state;
   *          it will sync internal state record;
   *          any change of source should call this function first
   * @return Error error(code, message)
   */
  Error set_control_source(const ControlSource &control_source);
  /**
   * @brief get control source
   * @details it will not change enable state
   * @return std::pair<Error, ControlSource> error and control source
   */
  std::pair<Error, ControlSource> get_control_source() const;
  /// @}
  /// @}
  // ============================== Tactile ==============================
  /// \name Tactile
  /// @{
  /// Methods for accessing tactile sensor data, calibration, and configuration.
  /**
     * @brief check if device supports tactile feature
     * @details returns the tactile support status from hand_sdk layer
     * @return bool true if tactile is supported, false otherwise
     */
  bool has_tactile_support() const;
  /**
     * @brief tactile packet receiving status
     * @details include packet got, packet loss, frame got, frame loss, start timestamp
     * @return json string
     */
  std::string tactile_summary() const;
  /**
     * @brief fetch one tactile frame
     * @param channel channel to fetch, RIGHT channel(0,5) LEFT channel(5,10)
     * @param timeout maximum time to wait(in seconds)
     * @return a pointer of frame, or nullptr if timeout
     */
  tactile::Frame::Ptr fetch_tactile_frame(int channel, double timeout=-1);
  /**
     * @brief fetch one tactile frame
     * @param channel channel to fetch, RIGHT channel(0,5) LEFT channel(5,10)
     * @param timeout maximum time to wait(in seconds)
     * @return CompactFrame
     */
  tactile::CompactFrame fetch_tactile_frame_compact(int channel, double timeout=-1);
  /**
     * @brief set custom callback function, which will be called once a Frame is produced
     * @param callback callback function, which takes a pointer of Frame
     */
  void set_tactile_callback(std::function<void(tactile::Frame::Ptr)> callback);
  void set_tactile_callback_compact(std::function<void(const tactile::CompactFrame&)> callback);
  /**
   * @brief reset zero state of all tactile sensors(tare sensors)
   * @param num_frames number of frames used to reset zero state in an attempt
   * @param max_retry max retry times of reseting zero state
   * @return if successfully
   */
  bool calib_tactile(int num_frames=20, int max_retry=10);
  /**
   * @brief get 3D point location on finger surface, given image coordinates on deform image
   * @param channel channel of finger
   * @param row row index of pixel on deform image
   * @param col column index of pixel on deform image
   * @return x y z nx ny nz(in finger coordinate system)
   *         nullopt if there is no corresponding 3D point or channel is invalid
   */
  std::optional<std::array<float, 6>> deform_map_uv(int channel, size_t row, size_t col);
  /**
   * @brief map uint8 deform value into float32 value
   * @param value_ui8 deform value in uint8 format
   * @return deform value in float32 format (unit: mm)
   */
  float deform_map_value(uint8_t value_ui8);
  /// @}
  // ============================== Packet Info ==============================
  /// \name Packet Info
  /// @{
  /// Methods for accessing debug information, packet counters, and raw packet data.
  /**
   * @brief get fault code
   * @details vector indicate with which joint the fault code is associated
   *          some fault code will return empty vector because not associated with joint
   * @return std::map<FaultCode, std::vector<int>> fault code and joint list
   */
  std::map<FaultCode, std::vector<int>> get_fault_code() const;
  /// @}
  // ==================== Advanced Motor Control Setting ====================
  /// \name Advanced Motor Control Setting
  /// @{
  /// Advanced methods for fine-tuning motor parameters and general parameter management.
  /**
   * @brief enable collision_protection
   *
   * @param enable true: enable, false: disable
   * @return Error
   */
  Error enable_collision_protection(bool enable);

  /**
   * @brief check if collision protection enabled
   *
   * @return bool
   */
  bool is_collision_protection_enabled() const;

  Error set_current_coeff(float coeff);
  std::pair<Error, float> get_current_coeff() const;
  Error set_speed_coeff(float coeff);
  std::pair<Error, float> get_speed_coeff() const;
  /**
   * @brief general parameter setting interface
   * @param json_str  json string like {"sn":"001", "current":0.2}
   * @details any invalid param key will result in set param operation failure
   *          available params see User Instruction Manual
   * @return Error error(code, message)
   */
  Error set_parameter_safe(const std::string &json_str);
  /**
   * @brief general parameter reading interface
   * @param param_names list of parameter names to read
   * @details available params see User Instruction Manual
   * @return std::pair<Error, std::string> error and json string
   */

  std::pair<Error, std::string> get_parameter(
      const std::vector<std::string> &param_names) const;
  /// @}
  // 25051203 - apply to firmware version over 0.35.1
  /**
   * @brief Start zero calibration
   * @details start zero calibration task
   * @return Error error(code, message)
   */
  Error start_zero_calibration();
  /**
   * @brief Stop zero calibration
   * @details stop zero calibration task
   * @return Error error(code, message)
   */
  Error stop_zero_calibration();
  /**
   * @brief Get zero calibration progress
   * @details get zero calibration progress
   * @return std::pair<Error, uint8_t> error and progress (0-100)
   */
  std::pair<Error, uint8_t> get_zero_calibration_progress() const;
  /// @}

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/**
 * @brief SharpaWaveManager class that manages SharpaWave devices
 *
 * This class provides management of devices that combine hand control and tactile
 * sensing capabilities. Implementation details are hidden using PIMPL pattern.
 */
class SharpaWaveManager {
 public:
  static SharpaWaveManager &get_instance();

  ~SharpaWaveManager();

  /**
   * @brief Connect to a device and return SharpaWave instance
   * @param device_sn Device serial number
   * @return Reference to SharpaWave instance
   */
  SharpaWave &connect(const std::string &device_sn, bool skip_tactile = false);

  /**
   * @brief Connect with full SharpaWaveConfig (e.g. tactile_config_file for factory builds)
   */
  SharpaWave &connect(const std::string &device_sn, const SharpaWaveConfig &config);

  /**
   * @brief Connect to a device by hand side and return SharpaWave instance
   * @param hand_side Left or right hand
   * @return Reference to SharpaWave instance
   */
  SharpaWave &connect(const HandSide &hand_side, bool skip_tactile = false);

  SharpaWave &connect(const HandSide &hand_side, const SharpaWaveConfig &config);

  /**
   * @brief Device-manager level JSON dispatch (heartbeat / UDP receiver).
   * @details JSON must contain "secret_function": "send_heartbeat_packet" with "ip","port",
   *          or "restart_heart_receiver_with_port" with "port".
   */
  Error set_parameter_safe(const std::string &json_str);

  /**
   * @brief Check if a device is connected as SharpaWave
   * @param device_sn Device serial number
   * @return true if connected as SharpaWave
   */
  bool is_connected(const std::string &device_sn) const;

  /**
   * @brief Get available device serial numbers
   * @return device serial number vector of all received heartbeat packets
   */
  std::vector<std::string> get_all_device_sn() const;
  /**
   * @brief get all devices info
   * @return device info vector of all received heartbeat packets
   */
  std::vector<DeviceInfo> get_all_devices() const;

  /**
   * @brief disconnect a device
   * @details old reference will be invalid after disconnect
   * @param device_sn device serial number
   */
  void disconnect(const std::string &device_sn);
  /**
   * @brief disconnect all devices
   * @details all references will be invalid after disconnect
   */
  void disconnect_all();
 private:
  SharpaWaveManager();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace sharpa
void setup_cpp_logging(const std::string& log_filepath, bool console_log=true);
#endif  // SHARPA_SDK_H
