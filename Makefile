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
  else ifeq ($(UNAME_M),x86_64)
    CMAKE_CONFIGURE_PRESET := macos-x64-$(SUFFIX)
  else
    $(error Unsupported macOS arch '$(UNAME_M)')
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

  ifeq ($(SUFFIX),asan-ubsan)
    $(error CONFIG=asan-ubsan is not supported for windows-x64-msvc presets)
  endif
  ifeq ($(SUFFIX),tsan)
    $(error CONFIG=tsan is not supported for windows-x64-msvc presets)
  endif
endif

.PHONY: deps configure build run clean

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


configure:
	cmake --preset $(CMAKE_CONFIGURE_PRESET)
	./scripts/sync_compile_commands.sh $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR)

run: build
	cmake --build $(BUILD_DIR) --target run

clean:
	rm -rf build
