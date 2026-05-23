#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstddef>
#include <cstdint>
#include <new>
#include <quark/platform/window/IWindow.hpp>
#include <quark/platform/window/glfw_window.hpp>
#include <quark/vk/surface_source.hpp>
#include <stdexcept>
#include <vector>

// TODO: make quark::headers more specific

namespace quark::platform {

namespace {

class GlfwVulkanSurfaceSource final : public vk::IVulkanSurfaceSource {
public:
  explicit GlfwVulkanSurfaceSource(GLFWwindow *window) : window_(window) {}

  [[nodiscard]] std::vector<const char *>
  required_instance_extensions() const override {
    uint32_t count = 0;
    const char **exts = glfwGetRequiredInstanceExtensions(&count);

    if (exts == nullptr || count == 0) {
      throw std::runtime_error("glfwGetRequiredInstanceExtensions failed");
    }

    return {exts, exts + count};
  }

  // TODO: update to non vk specific
  [[nodiscard]] VkSurfaceKHR
  create_surface(VkInstance instance) const override {
    if (window_ == nullptr) {
      throw std::runtime_error("create_surface called with null GLFWwindow");
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    const VkResult r =
        glfwCreateWindowSurface(instance, window_, nullptr, &surface);
    if (r != VK_SUCCESS) {
      throw std::runtime_error("glfwCreateWindowSurface failed");
    }

    return surface;
  }

private:
  GLFWwindow *window_ = nullptr;
};

void delete_vulkan_surface_source(void *p) noexcept {
  delete static_cast<GlfwVulkanSurfaceSource *>(p);
}

} // namespace

GlfwWindow::~GlfwWindow() { destroy(); }

void GlfwWindow::create(const CreateInfo &ci) {
  destroy();

  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("glfwInit failed");
  }
  glfw_initialized_ = true;

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, ci.resizable ? GLFW_TRUE : GLFW_FALSE);

  window_ = glfwCreateWindow(ci.width, ci.height, ci.title, /*monitor=*/nullptr,
                             /*share=*/nullptr);
  if (window_ == nullptr) {
    throw std::runtime_error("glfwCreateWindow failed");
  }

  // Track resizes
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_,
                                 &GlfwWindow::framebuffer_size_callback);

  resized_ = false;
}

void GlfwWindow::destroy() noexcept {
  iface_.clear();

  if (window_ != nullptr) {
    glfwSetFramebufferSizeCallback(window_, /*callback=*/nullptr);
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }

  if (glfw_initialized_) {
    glfwTerminate();
    glfw_initialized_ = false;
  }

  iface_.clear();
  resized_ = false;
}

bool GlfwWindow::valid() const noexcept { return window_ != nullptr; }

bool GlfwWindow::should_close() const noexcept {
  return (window_ == nullptr) || (glfwWindowShouldClose(window_) != 0);
}

void GlfwWindow::poll_events() noexcept { glfwPollEvents(); }
void GlfwWindow::wait_events() noexcept { glfwWaitEvents(); }

void GlfwWindow::framebuffer_size(int &out_width,
                                  int &out_height) const noexcept {
  out_width = 0;
  out_height = 0;

  if (window_ != nullptr) {
    glfwGetFramebufferSize(window_, &out_width, &out_height);
  }
}

bool GlfwWindow::was_resized() const noexcept { return resized_; }

void GlfwWindow::clear_resized() noexcept { resized_ = false; }

void *GlfwWindow::query_interface(InterfaceId id) noexcept {
  if (void *cached = iface_.find(id)) {
    return cached;
  }

  const InterfaceId vk_id =
      platform::interface_id<quark::vk::IVulkanSurfaceSource>();
  if (id == vk_id) {
    auto *p = new (std::nothrow) GlfwVulkanSurfaceSource(window_);

    if (p == nullptr) {
      return nullptr;
    }

    if (!iface_.add(id, p, &delete_vulkan_surface_source)) {
      delete p;
      return nullptr;
    }

    return p;
  }

  return nullptr;
}

const void *GlfwWindow::query_interface(InterfaceId id) const noexcept {
  if (const void *cached = iface_.find(id)) {
    return cached;
  }

  const InterfaceId vk_id =
      platform::interface_id<quark::vk::IVulkanSurfaceSource>();
  if (id == vk_id) {
    auto *p = new (std::nothrow) GlfwVulkanSurfaceSource(window_);

    if (p == nullptr) {
      return nullptr;
    }

    if (!iface_.add(id, p, &delete_vulkan_surface_source)) {
      delete p;
      return nullptr;
    }

    return p;
  }

  return nullptr;
}

void GlfwWindow::framebuffer_size_callback(GLFWwindow *window, int /*width*/,
                                           int /*height*/) {
  auto *self = static_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));

  if (self != nullptr) {
    self->resized_ = true;
  }
}

} // namespace quark::platform
