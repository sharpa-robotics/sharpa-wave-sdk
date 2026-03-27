# Makefile for Sharpa sample programs
# Tactile examples need OpenCV: sudo apt install libopencv-dev
#   (pkg-config opencv4 or opencv)

CXX = g++
CXXFLAGS = -std=c++17 -g -O0 -Wall -Wextra -DDEBUG -D_DEBUG
BUILD_DIR = build/

OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv)
OPENCV_LIBS := $(shell pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv)
ifeq ($(strip $(OPENCV_LIBS)),)
  $(error OpenCV not found via pkg-config. Install libopencv-dev, or set OPENCV_CFLAGS / OPENCV_LIBS manually)
endif

HDF5_CFLAGS := $(shell pkg-config --cflags hdf5-serial 2>/dev/null || pkg-config --cflags hdf5 2>/dev/null)
HDF5_LIBS := $(shell pkg-config --libs hdf5-serial 2>/dev/null || pkg-config --libs hdf5 2>/dev/null)
GW = gesture_workflow
NLOHMANN_I = -I../../3rdparty
GW_I = -I$(GW) $(NLOHMANN_I) $(HDF5_CFLAGS)
RECORD_GESTURE_EXE = $(BUILD_DIR)/sharpa_sample_record_gesture
ifneq ($(strip $(HDF5_LIBS)),)
  GW_TARGETS = $(BUILD_DIR)/sharpa_json_to_h5 $(BUILD_DIR)/sharpa_gesture_workflow_example
else
  GW_TARGETS =
  $(info HDF5 not found: skipping json_to_h5 + gesture_workflow_example. Install: sudo apt install libhdf5-dev)
endif

# Pick SDK layout: prefer build/install, else packaged lib/ + include/
ifeq ($(UNAME_S),Darwin)
    CHECK_RELEASE = $(shell if [ -f "../../lib/libSharpaWaveSDKWrapper.dylib" ]; then echo "local"; fi)
    CHECK_BUILD = $(shell if [ -d "../../build/install/lib" ]; then echo "build"; fi)
else
    CHECK_RELEASE = $(shell if [ -f "../../lib/libSharpaWaveSDKWrapper.so" ]; then echo "local"; fi)
    CHECK_BUILD = $(shell if [ -d "../../build/install/lib" ]; then echo "build"; fi)
endif
# Determine paths based on what exists
ifeq ($(CHECK_BUILD),build)
    INCLUDE_DIR = ../../build/install/include
    LIB_DIR = ../../build/install/lib
    LIBS = -lSharpaWaveSDK -lSharpaHand -ltactile_sdk -lspdlog -lfmt -lyaml-cpp -lmsgpack11
    $(info Using ../../build/install as root directory)
else ifeq ($(CHECK_RELEASE),local)
    INCLUDE_DIR = ../../include
    LIB_DIR = ../../lib
    LIBS = -lSharpaWaveSDKWrapper -ldl
    $(info Using local lib/ and include/ directories)
else
    $(error Error: Could not find Sharpa libraries and headers. Please ensure: 1. SharpaRT is installed in release folder OR 2. ../../build/install directory exists with lib/ and include/)
endif

# Convert relative paths to absolute paths
ABS_INCLUDE_DIR = $(shell realpath $(INCLUDE_DIR))
ABS_LIB_DIR = $(shell realpath $(LIB_DIR))

# Display configuration
$(info Build Configuration:)
$(info   Include Directory: $(INCLUDE_DIR) -> $(ABS_INCLUDE_DIR))
$(info   Library Directory: $(LIB_DIR) -> $(ABS_LIB_DIR))

TACTILE_COMMON = tactile_sample_common.hpp
TARGET_FETCH = $(BUILD_DIR)/sharpa_tactile_fetch
SOURCE_FETCH = sharpa_tactile_fetch.cc
TARGET_CB = $(BUILD_DIR)/sharpa_tactile_callback
SOURCE_CB = sharpa_tactile_callback.cc
TARGET2 = $(BUILD_DIR)/sharpa_wave_example
SOURCE2 = sharpa_wave_example.cc

# Linker flags with RPATH to remember absolute library path
LDFLAGS = -Wl,--allow-multiple-definition -static-libgcc -static-libstdc++ -rdynamic -Wl,-rpath,$(ABS_LIB_DIR)

# Default target - build all examples
all: $(BUILD_DIR) $(TARGET_FETCH) $(TARGET_CB) $(TARGET2) $(RECORD_GESTURE_EXE) $(GW_TARGETS)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TARGET_FETCH): $(SOURCE_FETCH) $(TACTILE_COMMON) | $(BUILD_DIR)
	@echo "Building sharpa_tactile_fetch..."
	$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) $(SOURCE_FETCH) \
		-L$(ABS_LIB_DIR) $(LIBS) $(OPENCV_LIBS) \
		-I$(ABS_INCLUDE_DIR) \
		$(LDFLAGS) \
		-o $(TARGET_FETCH)
	@echo "✓ sharpa_tactile_fetch built"

$(TARGET_CB): $(SOURCE_CB) $(TACTILE_COMMON) | $(BUILD_DIR)
	@echo "Building sharpa_tactile_callback..."
	$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) $(SOURCE_CB) \
		-L$(ABS_LIB_DIR) $(LIBS) $(OPENCV_LIBS) \
		-I$(ABS_INCLUDE_DIR) \
		$(LDFLAGS) \
		-o $(TARGET_CB)
	@echo "✓ sharpa_tactile_callback built"

# Build sharpa_wave_example
$(TARGET2): $(SOURCE2) | $(BUILD_DIR)
	@echo "Building sharpa_wave_example..."
	@echo "  Using absolute library path: $(ABS_LIB_DIR)"
	$(CXX) $(CXXFLAGS) $(SOURCE2) \
		-L$(ABS_LIB_DIR) $(LIBS) \
		-I$(ABS_INCLUDE_DIR) \
		$(LDFLAGS) \
		-o $(TARGET2)
	@echo "✓ sharpa_wave_example built successfully"
	@echo "  RPATH set to: $(ABS_LIB_DIR)"

$(BUILD_DIR)/sharpa_json_to_h5: $(GW)/gesture_io.cpp $(GW)/hdf5_gestures.cpp $(GW)/json_to_h5_main.cpp | $(BUILD_DIR)
	@echo "Building sharpa_json_to_h5..."
	$(CXX) $(CXXFLAGS) $(GW_I) $(GW)/gesture_io.cpp $(GW)/hdf5_gestures.cpp $(GW)/json_to_h5_main.cpp \
		$(HDF5_LIBS) $(LDFLAGS) -o $@

$(RECORD_GESTURE_EXE): $(GW)/gesture_io.cpp $(GW)/gesture_device.cpp $(GW)/sample_record_gesture_main.cpp | $(BUILD_DIR)
	@echo "Building sharpa_sample_record_gesture..."
	$(CXX) $(CXXFLAGS) -I$(GW) $(NLOHMANN_I) $(GW)/gesture_io.cpp $(GW)/gesture_device.cpp $(GW)/sample_record_gesture_main.cpp \
		-L$(ABS_LIB_DIR) $(LIBS) $(LDFLAGS) \
		-I$(ABS_INCLUDE_DIR) -pthread -o $@

$(BUILD_DIR)/sharpa_gesture_workflow_example: $(GW)/gesture_io.cpp $(GW)/hdf5_gestures.cpp $(GW)/gesture_device.cpp $(GW)/h5_replay.cpp $(GW)/gesture_workflow_example_main.cpp | $(BUILD_DIR)
	@echo "Building sharpa_gesture_workflow_example..."
	$(CXX) $(CXXFLAGS) $(GW_I) $(GW)/gesture_io.cpp $(GW)/hdf5_gestures.cpp $(GW)/gesture_device.cpp $(GW)/h5_replay.cpp $(GW)/gesture_workflow_example_main.cpp \
		-L$(ABS_LIB_DIR) $(LIBS) $(HDF5_LIBS) $(LDFLAGS) \
		-I$(ABS_INCLUDE_DIR) -pthread -o $@

# Build only tactile examples
tactile: $(BUILD_DIR) $(TARGET_FETCH) $(TARGET_CB)

# Build only wave example
wave: $(BUILD_DIR) $(TARGET2)

# Gesture workflow: record always; HDF5 tools when libhdf5-dev is installed
gesture: $(BUILD_DIR) $(RECORD_GESTURE_EXE) $(GW_TARGETS)

# Clean target
clean:
	@echo "Cleaning build directory..."
	@rm -f $(TARGET_FETCH) $(TARGET_CB) $(TARGET2) $(RECORD_GESTURE_EXE) $(GW_TARGETS)
	@rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

# Help target
help:
	@echo "Sharpa Sample Makefile"
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all      Build all sample programs (default)"
	@echo "  tactile  Build sharpa_tactile_fetch + sharpa_tactile_callback"
	@echo "  wave     Build only sharpa_wave_example"
	@echo "  gesture  Build sharpa_json_to_h5 + sharpa_sample_record_gesture + sharpa_gesture_workflow_example (needs HDF5)"
	@echo "  clean    Clean build directory"
	@echo "  help     Show this help message"
	@echo ""
	@echo "Requires: libopencv-dev for tactile; libhdf5-dev for sharpa_json_to_h5 and sharpa_gesture_workflow_example."
	@echo ""
	@echo "This Makefile automatically detects library and include paths:"
	@echo "  1. Checks for system-wide installation (/usr/include/sharpa and /usr/lib/sharpa-wave-sdk)"
	@echo "  2. Falls back to ../../build/install"
	@echo ""
	@echo "Current configuration:"
	@echo "  Include Directory: $(INCLUDE_DIR) -> $(ABS_INCLUDE_DIR)"
	@echo "  Library Directory: $(LIB_DIR) -> $(ABS_LIB_DIR)"
	@echo ""
	@echo "Note: Targets are built with RPATH to remember absolute library paths"

.PHONY: all tactile wave gesture clean help
