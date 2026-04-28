# 240_final_project

Scrapes University of Richmond One Card spending history, processes it with C++,
and visualizes balance and daily spending over time.

## Requirements

- Python 3 + `make`
- CMake 3.14+ and a C++17 compiler (install via `brew install cmake`)

## Setup (one time)

```
make setup
```

This creates a Python virtual environment, installs all dependencies,
installs the Playwright Chromium browser, and compiles the C++ analyzer.

## Run the pipeline

```
make run
```

Steps in order:
1. **Scrape** — launches Chromium, logs in to `onecardweb.richmond.edu`, and writes `jsons/rawHistory.json`
2. **Analyze** — C++ binary parses the JSON and writes `jsons/history.json`
3. **Visualize** — Python reads the processed data and saves PNGs to `python/visualizing/`

## Individual steps

```
make scrape    # step 1 only
make analyze   # step 2 only
make viz       # step 3 only
```

## Desktop UI (Qt 6)

A native dashboard built on Qt 6 + QtCharts surfaces the same data as the
matplotlib step, plus a sortable transaction table, color-coded pace banner,
per-shop recommendations, and native system-tray notifications when your
spending pace drifts.

### Install Qt 6 (one-time, per machine)

The dashboard depends on **Qt 6** with the **Qt Charts** module. Install it
once on each machine that will build the app:

#### macOS

```
brew install qt
```

If you don't have Homebrew, install it from <https://brew.sh> first. The
build picks up `brew --prefix qt` automaticallys

#### Windows

1. Download the **Qt Online Installer** from
   <https://www.qt.io/download-qt-installer> (free for open-source use; sign
   in with a free Qt Account when prompted).
2. In the installer, select:
   - **Qt → Qt 6.7** (or any 6.x ≥ 6.5)
     - **MSVC 2019 64-bit** (or **MinGW 64-bit** if you don't have Visual Studio)
     - **Qt Charts** (under *Additional Libraries*)
   - **Developer and Designer Tools → CMake** and **Ninja** (only if you
     don't already have them).
3. Also install **Visual Studio 2019/2022 Build Tools** (free) for the MSVC
   compiler if you chose the MSVC kit.
4. Note the install path, e.g. `C:\Qt\6.7.0\msvc2019_64`.

Then build from a **Developer Command Prompt** (or Git Bash):

```
cmake -S ui -B ui\build -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64" -DCMAKE_BUILD_TYPE=Release
cmake --build ui\build --config Release
ui\build\Release\dining_ui.exe
```

(`make ui` works on Windows too if you run it inside Git Bash, MSYS2, or
WSL.)

### Build and launch

Once Qt is installed:

```
make ui                  # builds + launches the dashboard
```

The build is wired into the root `CMakeLists.txt`, so the project also opens
directly in VSCode (with the **CMake Tools** extension): `Cmd+Shift+P → CMake:
Configure → Build → Run` (or `F5` to debug).

The UI reads `jsons/history.json` and `jsons/status.json` produced by the
existing pipeline. Run `make run` first so those files exist; then `make ui`.

### Notifications

The dashboard installs a small `$` icon in your menu bar (macOS) or system
tray (Windows). It re-checks `status.json` every 5 minutes and fires a native
banner notification when your pace classification changes (on track →
overspending, etc.). On first launch, allow notifications when the OS prompts
you — otherwise alerts will only appear in-window.

## Clean up generated files

```
make clean
rm -rf .claude build ui/build analyzing/build python/.venv ~/Library/Caches/ms-playwright
```
