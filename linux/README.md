## Software Prerequisites
### System Requirements
- **Architecture**: x86_64 (64-bit) only
- **Operating System**: Ubuntu 20.04 LTS, Ubuntu 22.04 LTS, or Ubuntu 24.04 LTS
- **C++ Standard**: C++17 or later
- **Python Support**: Python 3.10, Python 3.11, Python 3.12, Python 3.13

### System Dependencies
- **glibc**: Version 2.30 or later (Free Software Foundation)
- **CMake**: Version 3.11.0 or later


## SDK-based Demo 

The tactile output corresponds to tactile data at 30 Hz. Since tactile inference is performed inside the hand, there is no requirement on the computer's performance.

To obtain high-frame-rate tactile data at 180 Hz:

- The computer must be equipped with a high-performance graphics card (e.g., NVIDIA RTX 3060 or higher).

- Use SharpaWaveSDK_cuda instead. For details, refer to Steps to Acquire 180 Hz High-Frame-Rate Tactile Information.

### C++ Examples

1. install g++
```bash
sudo apt update
sudo apt install g++
```
2. Build the project:

#please run the c++ examples in the 'sample/c++' folder
```bash
cd sample/c++/
make
```
4. Run Gesture Demo:
```bash
LD_LIBRARY_PATH=../../lib && ./build/sharpa_wave_example
```

5. Install OpenCV development headers (tactile C++ demos):
```bash
sudo apt install libopencv-dev
```

6. Run tactile demos (same roles as `sample/python/sharpa_tactile_fetch.py` and `sharpa_tactile_callback.py`):
```bash
LD_LIBRARY_PATH=../../lib && ./build/sharpa_tactile_fetch
LD_LIBRARY_PATH=../../lib && ./build/sharpa_tactile_callback
```
Each program opens a combined window (LEFT/RIGHT hands, RAW + DEFORM). Keys: **t** calibrate, **j** enable JPEG, **s** disable JPEG, **ESC** exit.

The callback example also prints `tactile_summary()` before shutdown. Latency lines appear in the terminal periodically.



### Python Examples
Currently we only support python310, python311, python312
Please run the python examples in the SharpaWaveSDK folder

1. Set Environment Variables:
```bash
export LD_LIBRARY_PATH=lib:$LD_LIBRARY_PATH
```
2. Run Gesture Demo:
```bash
python3 sample/python/sharpa_wave_example.py
```
3. Run Tactile Sensing Demo (choose either fetch or callback method):

```bash
python3 sample/python/sharpa_tactile_fetch.py
python3 sample/python/sharpa_tactile_callback.py
```
The script will output a raw image and a deformation depth map for each finger

The frame rate and latency statistics will be displayed in terminal:
```
sdk got frame: 540 in 3.015814781188965 s, frame rate: 30.05608904374049
user got frame: 539 in 3.015814781188965 s, frame rate: 29.72450369365947
```
Press the Esc key to exit the script. 


