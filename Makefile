# Build into the same directory as the nvim cmake-tools setup (out/<BuildType>)
# so command-line and editor builds share the same cache and object files.
BUILD_CONF := Debug
BUILD_DIR := out/$(BUILD_CONF)

.PHONY: all configure build clean

all: build

configure:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_CONF) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build: configure
	cmake --build $(BUILD_DIR) --config $(BUILD_CONF)

clean:
	rm -rf $(BUILD_DIR)
