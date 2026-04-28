PYTHON     := python/.venv/bin/python
BUILD_DIR  := analyzing/build
BUILD_UI   := ui/build

.PHONY: all setup setup-python setup-cpp setup-ui run scrape analyze viz ui clean

all: run

# ── Setup ──────────────────────────────────────────────────────────────────

setup: setup-python setup-cpp
	@echo "Setup complete. Run 'make run' to run the pipeline and open the UI."

setup-python: python/.venv/bin/python
python/.venv/bin/python:
	python3 -m venv python/.venv
	python/.venv/bin/pip install --quiet -r python/requirements.txt
	python/.venv/bin/playwright install chromium

setup-cpp: $(BUILD_DIR)/analyze
$(BUILD_DIR)/analyze: analyzing/CMakeLists.txt analyzing/analyze.cpp
	mkdir -p $(BUILD_DIR)
	cmake -S analyzing -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR)

# ── Pipeline steps ─────────────────────────────────────────────────────────

scrape: setup-python
	$(PYTHON) python/scraping/fetch_balance.py

dev-scrape: setup-python
	$(PYTHON) python/scraping/fetch_balance_dev.py

analyze: setup-cpp
	$(BUILD_DIR)/analyze

viz: setup-python
	$(PYTHON) python/visualizing/vizualize.py

# ── Qt UI ──────────────────────────────────────────────────────────────────

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  UI_BIN := $(BUILD_UI)/dining_ui.app/Contents/MacOS/dining_ui
  UI_LAUNCH := open $(BUILD_UI)/dining_ui.app
else
  UI_BIN := $(BUILD_UI)/dining_ui
  UI_LAUNCH := $(UI_BIN)
endif

run: scrape analyze viz setup-ui
	$(UI_LAUNCH)

setup-ui: $(UI_BIN)
$(UI_BIN): ui/CMakeLists.txt $(wildcard ui/*.cpp) $(wildcard ui/*.h) ui/styles.qss ui/resources.qrc
	cmake -S ui -B $(BUILD_UI) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_UI)

ui: setup-ui
	$(UI_LAUNCH)

# ── Cleanup ────────────────────────────────────────────────────────────────

clean:
	rm -rf $(BUILD_DIR) $(BUILD_UI)
	rm -f jsons/history.json jsons/balances.json jsons/rawHistory.json
	rm -f python/visualizing/*.png
