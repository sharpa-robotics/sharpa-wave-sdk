# Sharpa Wave SDK

Sharpa Wave SDK provides libraries and APIs for integrating Sharpa Wave into custom applications and robotics software stacks.

This document covers **x86_64 (amd64)** and **ARM64 (aarch64)**. On disk, the SDK tree is **`/opt/sharpa-wave-sdk/`** with the same folder names on both architectures; **x86_64** is shipped as a **`.deb`**, while **ARM64** is shipped as a **`.zip`** (extract to `/opt/sharpa-wave-sdk/`). Only the archive name and the native module filename differ.

---

## 📥 Download

Download the latest package from Releases:

👉[https://github.com/sharpa-robotics/sharpa-wave-sdk/releases](https://github.com/sharpa-robotics/sharpa-wave-sdk/releases)

Choose the build that matches your **CPU architecture**: **`.deb` (amd64)** for typical PC Ubuntu/Debian, **`.zip` (ARM64 / aarch64)** for embedded or board images (exact filename follows the release).

---

## 📦 Architectures at a glance

| Arch            | Linux distribution on Releases              | Python extension name pattern (typical)        |
| --------------- | ------------------------------------------- | ---------------------------------------------- |
| **x86_64**      | **`.deb`**, architecture suffix `amd64`   | `sharpa.cpython-3xx-x86_64-linux-gnu.so`       |
| **ARM64**       | **`.zip`** (aarch64 bundle, not a `.deb`)   | `sharpa.cpython-3xx-aarch64-linux-gnu.so`     |

Here `3xx` means **310, 311, or 312** (matching a supported Python minor); there is **no** `cpython-313-…` build in supported release lines.

On the device, layout under `/opt/sharpa-wave-sdk/` is the same; only the `.so` files are built for the correct ISA.

---

## 🚀 Installation (Linux)

### x86_64 (amd64) — `.deb` (Ubuntu / Debian)

```bash
sudo dpkg -i sharpa-wave-sdk_<version>_amd64.deb
```

If dependencies are missing:

```bash
sudo apt-get install -f
```

### ARM64 (aarch64) — `.zip` (not distributed as `.deb`)

On the **ARM64** device (or an `aarch64` rootfs), download the **ARM64 `.zip`** from Releases and extract it so the SDK contents end up under **`/opt/sharpa-wave-sdk/`** (same `include/`, `lib/`, `python/`, `config/`, `sample/`, `pilot_sdk`, etc. as in the table below). Example:

```bash
sudo mkdir -p /opt/sharpa-wave-sdk
sudo unzip -o sharpa-wave-sdk_<version>_linux_aarch64.zip -d /opt/sharpa-wave-sdk
```

Adjust the zip filename to match your release; if the archive contains a single top-level directory, extract/move so that **`/opt/sharpa-wave-sdk/lib`** and **`/opt/sharpa-wave-sdk/include`** exist (not an extra nesting level).

**Note:** Use the **amd64 `.deb`** only on **x86_64**; use the **ARM64 `.zip`** only on **`aarch64`**. Do not install the PC `.deb` on ARM boards.

---

## 💼 Installed layout (`/opt/sharpa-wave-sdk`)

After installation, files are placed as follows. **Use the on-disk paths under `/opt/sharpa-wave-sdk/`**; you do not need to mirror a local `dist/linux/` or `dist/arm/` tree on the target. **x86_64:** the current **`.deb`** installs here and removes legacy `/usr/...` trees on upgrade. **ARM64:** unpack the **`.zip`** to this path yourself (there is no `dpkg` metadata for the zip bundle).

| What                      | Where                                                                                                                                                                                             |
| ------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **C++ headers**           | `/opt/sharpa-wave-sdk/include/` — main entry: `SharpaWaveSDK.h` (and any additional headers shipped with that release)                                                                           |
| **Shared library**        | `/opt/sharpa-wave-sdk/lib/libsharpa-wave-sdk.so`                                                                                                                                                  |
| **Python package**        | `/opt/sharpa-wave-sdk/python/sharpa/` — `__init__.py` and versioned `sharpa.cpython-3xx-<arch>-linux-gnu.so` for **Python 3.10, 3.11, or 3.12 only** (`<arch>` as in the table above). **Python 3.13 is not supported.** |
| **Runtime binary**        | `/opt/sharpa-wave-sdk/pilot_sdk` — standalone SDK runtime (used by integrations such as Sharpa Pilot on Linux)                                                                                    |
| **Version and resources** | `/opt/sharpa-wave-sdk/` — `README.md`, `VERSION`, `config.yaml`, and `config/` (including **static** assets such as default tactile/mapping data)                                                    |
| **Samples**               | `/opt/sharpa-wave-sdk/sample/` — `c++/`, `python/` (e.g. `gesture_workflow/`), `ROS/`, each with its own `README` or `requirements.txt` where applicable                                           |

**Tips**

- **amd64 `.deb` only:** `postinst` registers `/opt/sharpa-wave-sdk/lib` in **`/etc/ld.so.conf.d/sharpa-wave-sdk.conf`** and runs **`ldconfig`**, so `libsharpa-wave-sdk.so` is normally found without extra env vars.
- **ARM64 `.zip`:** there is no automatic `ldconfig` step; either add the same **`ld.so.conf.d`** snippet and run **`sudo ldconfig`**, or set **`export LD_LIBRARY_PATH=/opt/sharpa-wave-sdk/lib:$LD_LIBRARY_PATH`** (also useful on any arch if the loader still misses the library).
- Python samples often add:  
  `sys.path.insert(0, '/opt/sharpa-wave-sdk/python')`  
  so the `sharpa` extension that matches your **`python3` major.minor** (**3.10 / 3.11 / 3.12 only**) and **CPU arch** is loaded.


### ARM64: what is in the `.zip`

The **ARM64 release artifact is a `.zip`**, not a Debian package. It is built for embedded / board bring-up and mirrors the same **`/opt/sharpa-wave-sdk/`** layout as the PC install when unpacked to that directory. Check the archive listing for your release: some builds may **omit** the Python extension or ship extra tooling—if `python/sharpa/*.so` for your Python minor is missing, obtain a build that includes it or build on the **target** `aarch64` host when your product requires Python.

---

## ▶️ Quick start

### C++ projects

- Include: `-I/opt/sharpa-wave-sdk/include`
- Link: `-L/opt/sharpa-wave-sdk/lib -lsharpa-wave-sdk` (ensure the loader can find the `.so` at runtime; see `LD_LIBRARY_PATH` or rpath as above)

### Python

- Add `'/opt/sharpa-wave-sdk/python'` to `sys.path`, then `import sharpa` (same pattern as the shipped samples).
- Use **Python 3.10, 3.11, or 3.12** with a bundled `cpython-3xx-*.so` for **your** machine’s architecture. **Python 3.13 is not supported** (no matching wheel/extension in this release line). A mismatch of **Python version** or **CPU arch** will fail to import the extension.

### Configuration and static assets

- Under `/opt/sharpa-wave-sdk/config/` (plus top-level `config.yaml` / `VERSION` as shipped), default data for tactile and runtime use; the exact set follows the release you install.

---

## 📖 Samples and further reading

- **C++** — `/opt/sharpa-wave-sdk/sample/c++/` (e.g. `Makefile`, `sharpa_wave_example.cc`, tactile examples). On ARM, use makefiles or flags appropriate for `aarch64` if the sample differentiates.
- **Python** — `/opt/sharpa-wave-sdk/sample/python/` and subfolders; see each `requirements.txt`.
- **ROS** — `/opt/sharpa-wave-sdk/sample/ROS/`.

For build steps, tactile framerate notes, and hands-on details, see the **developer-oriented** `sample/demo.md` in the source repository if you have it, or any extra documentation supplied with your release.

---

## 🔄 Update

### x86_64 (`.deb`)

```bash
sudo dpkg -i sharpa-wave-sdk_<new-version>_amd64.deb
sudo apt-get install -f
```

### ARM64 (`.zip`)

Replace the tree under `/opt/sharpa-wave-sdk/` with the contents of the newer **`.zip`** (backup any local edits to `config.yaml` first), e.g. remove the old directory and extract again as in [Installation](#installation-linux).

---

## ❌ Uninstall

### x86_64 (installed from `.deb`)

```bash
sudo dpkg -r sharpa-wave-sdk
```

If the package name differs, use the name shown by `dpkg -l | grep sharpa`.

### ARM64 (installed from `.zip`)

```bash
sudo rm -rf /opt/sharpa-wave-sdk
```

Remove any manual **`/etc/ld.so.conf.d/sharpa-wave-sdk.conf`** (and run **`sudo ldconfig`**) if you added one for the zip install.

---

## ⚠️ Requirements

- **OS:** Ubuntu 20.04 / 22.04 (or as stated in the release notes for your build).
- **Architecture**
  - **x86_64** — **`.deb`** with suffix `amd64` unless otherwise published.
  - **ARM64** — **`.zip`** for `aarch64` (not a `.deb`); other ARM ISAs only if explicitly provided.
- **C++:** C++17 or later.
- **Python:** 3.10, 3.11, or 3.12 only, with a shipped `sharpa` extension **for the same architecture as the running interpreter**. **Not compatible with Python 3.13.**

---

## 📌 Notes

- Connect and authorize the device according to your deployment policy before use.

---

## 📞 Support

For issues or product questions, contact **Sharpa Robotics** through your support channel or the address provided with your license or purchase.
