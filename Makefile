BUILD_DIR := build
BUILD_CONF := Release

.PHONY: all configure build clean

all: build

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake ..

configure: $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) --config $(BUILD_CONF)

clean:
	rm -rf $(BUILD_DIR)
