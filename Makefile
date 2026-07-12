CONFIG ?= debug
HEADLESS ?= OFF
CMAKE ?= cmake

ifeq ($(OS),Windows_NT)
	HOST_OS := Windows
	HOST_ARCH := x86_64
else
	HOST_OS := $(shell uname -s 2>/dev/null)
	HOST_ARCH := $(shell uname -m 2>/dev/null)
endif

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

ifeq ($(HEADLESS),ON)
	HEADLESS_SUFFIX := -headless 
else ifeq ($(HEADLESS),OFF)
	HEADLESS_SUFFIX :=
else
	$(error Unknown HEADLESS '$(HEADLESS)'. Use ON or OFF)
endif

ifeq ($(HOST_OS),Darwin)
	ifeq ($(HOST_ARCH),arm64)
    CMAKE_CONFIGURE_PRESET := macos-arm64-$(SUFFIX)
	else ifeq ($(HOST_ARCH),x86_64)
    CMAKE_CONFIGURE_PRESET := macos-x64-$(SUFFIX)
  else
		$(error Unsupported macOS arch '$(HOST_ARCH)')
  endif
  BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)$(HEADLESS_SUFFIX)

else ifeq ($(HOST_OS),Linux)
  ifndef LINUX_TOOLCHAIN
    LINUX_TOOLCHAIN := gcc
  endif
  CMAKE_CONFIGURE_PRESET := linux-x64-$(LINUX_TOOLCHAIN)-$(SUFFIX)
  BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)$(HEADLESS_SUFFIX)

else
	# Windows (native, Git Bash/MSYS, or Cygwin)
	ifneq (,$(findstring MINGW,$(HOST_OS)))
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)$(HEADLESS_SUFFIX)
	else ifneq (,$(findstring MSYS,$(HOST_OS)))
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)$(HEADLESS_SUFFIX)
	else ifneq (,$(findstring CYGWIN,$(HOST_OS)))
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)$(HEADLESS_SUFFIX)
  else
    CMAKE_CONFIGURE_PRESET := windows-x64-msvc-$(SUFFIX)
    BUILD_DIR := build/$(CMAKE_CONFIGURE_PRESET)$(HEADLESS_SUFFIX)
  endif

  ifeq ($(SUFFIX),asan-ubsan)
    $(error CONFIG=asan-ubsan is not supported for windows-x64-msvc presets)
  endif
  ifeq ($(SUFFIX),tsan)
    $(error CONFIG=tsan is not supported for windows-x64-msvc presets)
  endif
endif

# Derive the vcpkg triplet from platform to match CMakePresets.json
ifeq ($(HOST_OS),Darwin)
	ifeq ($(HOST_ARCH),arm64)
    VCPKG_TRIPLET := arm64-osx
	else ifeq ($(HOST_ARCH),x86_64)
    VCPKG_TRIPLET := x64-osx
  else
		$(error Unsupported macOS arch '$(HOST_ARCH)')
  endif
else ifeq ($(HOST_OS),Linux)
  VCPKG_TRIPLET := x64-linux
else
  # Windows/MSYS/Git Bash/Cygwin (probably)
  VCPKG_TRIPLET := x64-windows
endif

ifeq ($(OS),Windows_NT)
	VCPKG_CMD := vcpkg/vcpkg.exe
else
	VCPKG_CMD := ./vcpkg/vcpkg
endif



.PHONY: deps vcpkg-install configure build run clean bench-noop bench-touch

###  testing this for non-gh actions
deps:
	@set -e; \
	if ! command -v ninja >/dev/null 2>&1; then \
		echo "ninja not found; attempting to install..."; \
		if command -v apt-get >/dev/null 2>&1; then \
			if command -v sudo >/dev/null 2>&1; then SUDO=sudo; else SUDO=; fi; \
			$$SUDO apt-get update; \
			$$SUDO apt-get install -y ninja-build; \
		elif command -v dnf >/dev/null 2>&1; then \
			if command -v sudo >/dev/null 2>&1; then SUDO=sudo; else SUDO=; fi; \
			$$SUDO dnf install -y ninja-build; \
		elif command -v pacman >/dev/null 2>&1; then \
			if command -v sudo >/dev/null 2>&1; then SUDO=sudo; else SUDO=; fi; \
			$$SUDO pacman -S --noconfirm ninja; \
		elif command -v zypper >/dev/null 2>&1; then \
			if command -v sudo >/dev/null 2>&1; then SUDO=sudo; else SUDO=; fi; \
			$$SUDO zypper --non-interactive install ninja; \
		elif command -v brew >/dev/null 2>&1; then \
			brew install ninja; \
		elif command -v winget >/dev/null 2>&1 || command -v winget.exe >/dev/null 2>&1; then \
			if command -v winget >/dev/null 2>&1; then WINGET=winget; else WINGET=winget.exe; fi; \
			$$WINGET install --id Ninja-build.Ninja --exact --accept-package-agreements --accept-source-agreements; \
		else \
			echo "error: ninja is missing and no supported package manager was found"; \
			exit 1; \
		fi; \
	else \
		echo "ninja already installed"; \
	fi; \
	if [ ! -f "vcpkg/bootstrap-vcpkg.sh" ] && [ ! -f "vcpkg/bootstrap-vcpkg.bat" ]; then \
		echo "vcpkg checkout not found; cloning..."; \
		rm -rf vcpkg; \
		git clone https://github.com/microsoft/vcpkg.git vcpkg; \
	else \
		git submodule update --init --recursive; \
	fi; \
	if [ -f "vcpkg/bootstrap-vcpkg.sh" ]; then \
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

vcpkg-install: deps
	$(VCPKG_CMD) install --triplet $(VCPKG_TRIPLET)

configure:
	$(CMAKE) --preset $(CMAKE_CONFIGURE_PRESET) \
		-DQUARK_HEADLESS=$(HEADLESS) \
		-DVCPKG_MANIFEST_FEATURES=window
	$(CMAKE) -E copy_if_different $(BUILD_DIR)/compile_commands.json compile_commands.json

build: configure
	$(CMAKE) --build $(BUILD_DIR)

run: build
	$(CMAKE) --build $(BUILD_DIR) --target run

clean:
	$(CMAKE) -E rm -rf build compile_commands.json
