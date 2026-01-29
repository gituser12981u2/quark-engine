CONFIG ?= debug

UNAME_S := $(shell uname -s 2>/dev/null)
UNAME_M := $(shell uname -m 2>/dev/null)

ifeq ($(CONFIG),debug)
  SUFFIX := debug
else ifeq ($(CONFIG),release)
  SUFFIX := release
else ifeq ($(CONFIG),asan-ubsan)
  SUFFIX := asan-ubsan
else ifeq ($(CONFIG),tsan)
  SUFFIX := tsan
else ifeq ($(CONFIG),release-lto)
  SUFFIX := release-lto
else ifeq ($(CONFIG),debug-gcc)
  SUFFIX := debug
  LINUX_TOOLCHAIN := gcc
else ifeq ($(CONFIG),debug-clang)
  SUFFIX := debug
  LINUX_TOOLCHAIN := clang
else
  $(error Unknown CONFIG '$(CONFIG)'. Use one of: debug release asan-ubsan tsan release-lto debug-gcc debug-clang)
endif

ifeq ($(UNAME_S),Darwin)
  ifeq ($(UNAME_M),arm64)
    CMAKE_CONFIGURE_PRESET := macos-arm64-$(SUFFIX)
  else
    $(error Unsupported macOS arch '$(UNAME_M)'. Add macos-x64-* presets if needed.)
  endif
  BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)

else ifeq ($(UNAME_S),Linux)
  ifndef LINUX_TOOLCHAIN
    LINUX_TOOLCHAIN := gcc
  endif
  CMAKE_CONFIGURE_PRESET := linux-x64-$(LINUX_TOOLCHAIN)-$(SUFFIX)
  BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)

else
  # Windows (i.e. Git Bash/MSYS) often reports uname -s like MINGW64_NT-*
  # For native Windows usage, recommend running from a shell where `cmake` works.
  ifneq (,$(findstring MINGW,$(UNAME_S)))
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)
  else ifneq (,$(findstring MSYS,$(UNAME_S)))
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)
  else ifneq (,$(findstring CYGWIN,$(UNAME_S)))
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)
  else
    # If uname isn't available, assume Windows.
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)
  endif
endif

.PHONY: deps configure build run clean 

deps:
	git submodule update --init --recursive
	@if [ -f "vcpkg/bootstrap-vcpkg.sh" ]; then \
		if [ ! -f "vcpkg/vcpkg" ]; then \
			echo "Bootstrapping vcpkg (Unix)..."; \
			cd vcpkg && ./bootstrap-vcpkg.sh; \
		else \
			echo "vcpkg already bootstrapped"; \
		fi; \
	elif [ -f "vcpkg/bootstrap-vcpkg.bat" ]; then \
		if [ ! -f "vcpkg/vcpkg.exe" ]; then \
			echo "Bootstrapping vcpkg (Windows)..."; \
			cd vcpkg && ./bootstrap-vcpkg.bat; \
		else \
			echo "vcpkg already bootstrapped"; \
		fi; \
	else \
		echo "error: vcpkg bootstrap script not found"; \
		exit 1; \
	fi


configure:
	cmake --preset $(CMAKE_CONFIGURE_PRESET) 
	./scripts/sync_compile_commands.sh $(BUILD_DIR) 

build: configure
	cmake --build $(BUILD_DIR) 

run: build 
	cmake --build $(BUILD_DIR) --target run

clean:
	rm -rf build
