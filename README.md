# Sharpa Wave SDK

Sharpa Wave SDK provides libraries and APIs for integrating Sharpa Wave into custom applications and robotics software stacks.

This document covers **x86_64 (amd64)** and **ARM64 (aarch64)**. Paths under `/usr/...` are the same on the target machine; only the package name / architecture tag and the native module filename differ.

---

## Download

Download the latest package from Releases:

[https://github.com/sharpa-robotics/sharpa-wave-sdk/releases](https://github.com/sharpa-robotics/sharpa-wave-sdk/releases)

Choose the build that matches your **CPU architecture** (e.g. `amd64` for PC, `arm64` for many embedded Linux boards).

---

## Architectures at a glance

| Arch            | Debian/Ubuntu package suffix | Python extension name pattern (typical)        |
| --------------- | ------------------------------ | ---------------------------------------------- |
| **x86_64**      | `amd64`                        | `sharpa.cpython-3xx-x86_64-linux-gnu.so`       |
| **ARM64**       | `arm64`                        | `sharpa.cpython-3xx-aarch64-linux-gnu.so`     |

On the device, layout under `/usr` is the same; only the `.so` files are built for the correct ISA.

---

## Installation (Ubuntu, `.deb`)

### x86_64 (amd64)

```bash
sudo dpkg -i sharpa-wave-sdk_<version>_amd64.deb
```

### ARM64 (aarch64)

Use the **arm64** (or your release’s equivalent) package on the **ARM64 device** (or rootfs prepared for that arch):

```bash
sudo dpkg -i sharpa-wave-sdk_<version>_arm64.deb
```

If dependencies are missing:

```bash
sudo apt-get install -f
```

**Note:** Do not install an `amd64` `.deb` on an ARM64 system (and vice versa). The package architecture must match `uname -m` (e.g. `x86_64` vs `aarch64`).

---

## Installed layout (FHS)

After installation, files are placed as follows. **Use the on-disk paths under `/usr/...`**; you do not need to mirror a local `dist/linux/` or `dist/arm/` tree on the target.

| What                      | Where                                                                                                                                                                                             |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **C++ headers**           | `/usr/include/sharpa-wave-sdk/` — main entry: `SharpaWaveSDK.h` (and any additional headers shipped with that release)                                                                             |
| **Shared library**        | `/usr/lib/sharpa-wave-sdk/libsharpa-wave-sdk.so`                                                                                                                                                 |
| **Python package**        | `/usr/lib/sharpa-wave-sdk/python/sharpa/` — `__init__.py` and versioned `sharpa.cpython-3xx-<arch>-linux-gnu.so` for **Python 3.10–3.13** (see architecture table above for `<arch>`)            |
| **Version and resources** | `/usr/share/sharpa-wave-sdk/` — this `README`, `VERSION`, and `config/` (including **static** assets such as default tactile/mapping data)                                                        |
| **Samples**               | `/usr/share/sharpa-wave-sdk/sample/` — `c++/`, `python/` (e.g. `gesture_workflow/`), `ROS/`, each with its own `README` or `requirements.txt` where applicable                                    |

**Tips**

- If the dynamic linker does not find the merged SDK library, set:  
  `export LD_LIBRARY_PATH=/usr/lib/sharpa-wave-sdk:$LD_LIBRARY_PATH`
- Python samples often add:  
  `sys.path.insert(0, '/usr/lib/sharpa-wave-sdk/python')`  
  so the `sharpa` extension that matches your **`python3` major.minor** and **CPU arch** is loaded.

---

## ARM64: cross-build / developer bundle (optional)

Some release or internal flows produce a **tarball/zip** (e.g. from a cross-compiled `dist/arm/` tree) with **C++** libraries, headers, and samples for embedded bring-up. That layout is **not** the same as a full `.deb` and may **omit** the Python extension or ship it separately—check the files listed in that archive. If Python is not included, build or install the matching `sharpa` module on the **target** architecture when your product requires it.

---

## Quick start

### C++ projects

- Include: `-I/usr/include/sharpa-wave-sdk`
- Link: `-L/usr/lib/sharpa-wave-sdk -lsharpa-wave-sdk` (ensure the loader can find the `.so` at runtime; see `LD_LIBRARY_PATH` or rpath as above)

### Python

- Add `'/usr/lib/sharpa-wave-sdk/python'` to `sys.path`, then `import sharpa` (same pattern as the shipped samples).
- Use one of **Python 3.10, 3.11, 3.12, or 3.13** that matches a bundled `cpython-3xx-*.so` for **your** machine’s architecture. A mismatch of **Python version** or **CPU arch** will fail to import the extension.

### Configuration and static assets

- `config/VERSION` and `config/static/` (and related files) supply default data for tactile and runtime use; the exact set follows the release you install.

---

## Samples and further reading

- **C++** — `/usr/share/sharpa-wave-sdk/sample/c++/` (e.g. `Makefile`, `sharpa_wave_example.cc`, tactile examples). On ARM, use makefiles or flags appropriate for `aarch64` if the sample differentiates.
- **Python** — `/usr/share/sharpa-wave-sdk/sample/python/` and subfolders; see each `requirements.txt`.
- **ROS** — `/usr/share/sharpa-wave-sdk/sample/ROS/`.

For build steps, tactile framerate notes, and hands-on details, see the **developer-oriented** `sample/demo.md` in the source repository if you have it, or any extra documentation supplied with your release.

---

## Update

To upgrade to a newer package (use the correct `.deb` for your arch):

```bash
sudo dpkg -i sharpa-wave-sdk_<new-version>_<arch>.deb
sudo apt-get install -f
```

Replace `<arch>` with `amd64`, `arm64`, or the suffix provided in the release.

---

## Uninstall

```bash
sudo dpkg -r sharpa-wave-sdk
```

If the package name differs, use the name shown by `dpkg -l | grep sharpa`.

---

## Requirements

- **OS:** Ubuntu 20.04 / 22.04 (or as stated in the release notes for your build).
- **Architecture**
  - **x86_64** — package `amd64` unless otherwise published.
  - **ARM64** — package `arm64` (or as published); other ARM ISAs only if explicitly provided.
- **C++:** C++17 or later.
- **Python:** 3.10, 3.11, 3.12, or 3.13, matching a shipped `sharpa` extension **for the same architecture as the running interpreter**.

---

## Notes

- Connect and authorize the device according to your deployment policy before use.

---

## Support

For issues or product questions, contact **Sharpa Robotics** through your support channel or the address provided with your license or purchase.
