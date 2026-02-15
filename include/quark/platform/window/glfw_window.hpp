#pragma once

#include <cstddef>
#include <quark/platform/window/IWindow.hpp>
#include <quark/platform/window/interface_cache.hpp>
#include <quark/utils/raii.hpp>

struct GLFWwindow;

namespace quark::platform {

class GlfwWindow final : public IWindow {
public:
  GlfwWindow() = default;
  explicit GlfwWindow(const CreateInfo &ci) { create(ci); }

  ~GlfwWindow() override;

  QUARK_NO_COPY_NO_MOVE(GlfwWindow);

  void create(const CreateInfo &ci) override;
  void destroy() noexcept override;
  [[nodiscard]] bool valid() const noexcept override;

  [[nodiscard]] bool should_close() const noexcept override;
  void poll_events() noexcept override;
  void wait_events() noexcept override;

  // Returns current framebuffer size in pixels.
  void framebuffer_size(int &out_width,
                        int &out_height) const noexcept override;

  [[nodiscard]] bool was_resized() const noexcept override;
  void clear_resized() noexcept override;

  [[nodiscard]] void *query_interface(InterfaceId id) noexcept override;
  [[nodiscard]] const void *
  query_interface(InterfaceId id) const noexcept override;

private:
  static void framebuffer_size_callback(GLFWwindow *window, int width,
                                        int height);

  mutable platform::InterfaceCache iface_;
  GLFWwindow *window_ = nullptr;
  bool glfw_initialized_ = false;
  bool resized_ = false;
};

} // namespace quark::platform
