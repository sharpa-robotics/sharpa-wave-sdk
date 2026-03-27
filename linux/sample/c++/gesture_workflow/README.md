# Gesture workflow (C++)

C++ counterparts of `sample/python/gesture_workflow/`. Tracked sources live under `gesture_workflow/`; **do not commit** `../exports/`, `../output/`, or `../build/` (see `backend/sharpaRT/.gitignore`).

## Quick check: why is `sharpa_json_to_h5` missing?

If `./build/sharpa_json_to_h5` does not exist, HDF5 was not detected at `make` time. Install **`libhdf5-dev`** (Ubuntu/Debian), then:

```bash
cd sample/c++
make clean && make
ls -l build/sharpa_json_to_h5
```

## Program mapping

| Python | C++ binary (built into `sample/c++/build/`) |
|--------|---------------------------------------------|
| `json_to_h5.py` | `sharpa_json_to_h5` |
| `sample_record_gesture.py` | `sharpa_sample_record_gesture` |
| `gesture_workflow_example.py` | `sharpa_gesture_workflow_example` |

`h5_replay.py` is implemented as a class in `h5_replay.cpp` / `h5_replay.hpp` (used by the workflow example, not a separate CLI).

## Dependencies

- **All gesture tools**: nlohmann JSON (vendored in this repo at `sharpaRT/3rdparty/nlohmann`).
- **`sharpa_sample_record_gesture`**, **`sharpa_gesture_workflow_example`**: `libSharpaWaveSDK` (same as other `sample/c++` demos).
- **`sharpa_json_to_h5`**, **`sharpa_gesture_workflow_example`**: HDF5 C library (`sudo apt install libhdf5-dev`). If HDF5 is missing, `make` still builds the record tool; JSON→H5 and full workflow binaries are skipped.

## Build

From `sample/c++/`:

```bash
make              # includes gesture record + tactile + wave; HDF5 tools if libhdf5-dev is installed
make gesture      # record + HDF5 tools (when available)
```

## Run (working directory: `sample/c++`)

Default data files live under `gesture_workflow/`:

- `predefined_gestures.json` — same content as Python `GESTURE_DATA`
- `example_gesture_data.txt` — same as Python example

```bash
export LD_LIBRARY_PATH=../../build/install/lib

# Export predefined poses to exports/Gesture_*.json
./build/sharpa_sample_record_gesture

# Record one pose from device
./build/sharpa_sample_record_gesture --from-device --hand-side RIGHT

# Convert JSON folder to HDF5
./build/sharpa_json_to_h5 --json-folder exports --output output/demo.hdf5

# Workflows (mirror Python --workflow)
./build/sharpa_gesture_workflow_example --workflow predefined
./build/sharpa_gesture_workflow_example --workflow grouped
./build/sharpa_gesture_workflow_example --workflow replay-existing --h5-file output/grouped_gestures.hdf5

# Grouped workflow: generate H5 only (no device replay)
./build/sharpa_gesture_workflow_example --workflow grouped --no-replay
```

## Generated paths (gitignored)

When you run tools with working directory `sample/c++/`:

| Path | Contents |
|------|----------|
| `build/` | `sharpa_*` executables |
| `exports/` | `Gesture_*.json`, `GroupedGesture_*.json`, etc. |
| `output/` | `*.hdf5` (e.g. `predefined_gestures.hdf5`, `grouped_gestures.hdf5`) |

Use `--output-dir` / `--output` if you want other locations.

## Notes

- HDF5 layout matches Python: datasets `/stamp`, `/action/position`, `/state/position` (2D float32, 22 joints per row).
- Replay drives `SharpaWave::set_joint_position` on a background thread using stamp deltas as sleep intervals (same idea as Python `h5_replay.py`).
- CLI coverage is slightly smaller than Python (`--all`, `interactive` menu, and some edge cases are trimmed); core paths above are aligned.
