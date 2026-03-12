#pragma once

#include <quark/utils/raii.hpp>

namespace quark::platform {

/**
 * @brief Opaque identifier used by IWindow::query_interface().
 *
 * Two InterfaceId values are considered equal if they refer to the same
 * unique program address. Use interface_id<T>() to obtain an ID for a
 * specific capability interface type.
 *
 * @warning This ID mechanism assumes a single linked binary image.
 */
struct InterfaceId {
  const void *value = nullptr;
  friend constexpr bool operator==(InterfaceId a, InterfaceId b) noexcept {
    return a.value == b.value;
  }
};

/**
 * @brief Returns a unique InterfaceId token for a capability interface type T.
 *
 * This implementation does not require RTTI and is O(1). The returned id is
 * unique for each distinct T within the current program image.
 */
template <class T> inline InterfaceId interface_id() noexcept {
  // TODO: make workable with shared libraries for plugin DLL
  static const int tag = 0;
  return InterfaceId{&tag};
}

/**
 * @brief Window interface for platform backends (GLFW, SDL, etc.).
 *
 * This interface is graphics-backend-agnostic. Graphics backends request
 * optional capabilities (e.g, Vulkan surface creation) using query_interface().
 */
class IWindow {
public:
  struct CreateInfo {
    int width = 1280; ///< Window client width in screen coordinates.
    int height = 720; ///< Window client height in screen coordinates.
    const char *title = "quark-engine"; ///< UTF-8 title string.
    bool resizable = true;              ///< Whether user resizing is permitted.
  };

  virtual ~IWindow() = default;

  QUARK_NO_COPY_NO_MOVE(IWindow);

  virtual void create(const CreateInfo &ci) = 0;
  virtual void destroy() noexcept = 0;
  [[nodiscard]] virtual bool valid() const noexcept = 0;

  [[nodiscard]] virtual bool should_close() const noexcept = 0;

  /**
   * @brief Processes pending windowing events without blocking.
   */
  virtual void poll_events() noexcept = 0;

  /**
   * @brief Blocks until at least one windowing event is available, then
   * processes events.
   */
  virtual void wait_events() noexcept = 0;

  /**
   * @brief Gets the current framebuffer size in pixels.
   *
   * @note On HiDPI displays, framebuffer size may differ from logical window
   * size.
   */
  virtual void framebuffer_size(int &out_width,
                                int &out_height) const noexcept = 0;

  /**
   * @brief Returns whether the window has been resized since the last
   * clear_resized().
   *
   * This is intended for swapchain recreation triggers.
   */
  [[nodiscard]] virtual bool was_resized() const noexcept = 0;
  virtual void clear_resized() noexcept = 0;

  /**
   * @brief Queries an optional capability interface by InterfaceId.
   *
   * Backends (Vulkan, D3D12, etc.) use this to obtain window-system
   * integration objects.
   *
   * @param id Capability interface identifier
   *
   * @return Pointer to capability interface, or nullptr if unsupported
   *
   * @note The returned pointer is owned by the window implementation and
   * remains valid until destroy() is called.
   */
  [[nodiscard]] virtual void *query_interface(InterfaceId id) noexcept = 0;

  /**
   * @brief Const overlpad of query_interface().
   *
   * Implementations may lazily create and cache adapters in const context.
   */
  [[nodiscard]] virtual const void *
  query_interface(InterfaceId id) const noexcept = 0;

protected:
  IWindow() = default;
};

} // namespace quark::platform
