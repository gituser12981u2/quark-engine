#include "quark/utils/result.hpp"
#include <memory_resource>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/instance/details/debug_messenger.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <quark/vk/instance/details/instance_handle.hpp>
#include <quark/vk/instance/details/instance_registry.hpp>
#include <quark/vk/instance/instance_bundle.hpp>
#include <vulkan/vulkan_core.h>

namespace quark::vk {

InstanceBundle::InstanceBundle(std::pmr::memory_resource *memory_resource)
    : registry_(memory_resource) {}

util::Status InstanceBundle::create(const Instance::CreateInfo &ci) {
  destroy();

  QUARK_TRY_ASSIGN(handle_, registry_.create(ci));
  return {};
}

void InstanceBundle::destroy() noexcept {
  registry_.destroy(handle_);
  handle_ = InstanceHandle{};
}

VkInstance InstanceBundle::vk_instance() const noexcept {
  const Instance *const instance = registry_.get(handle_);
  return (instance != nullptr) ? instance->handle() : VK_NULL_HANDLE;
}

const DebugMessenger *InstanceBundle::debug_messenger() const noexcept {
  const Instance *const instance = registry_.get(handle_);
  if (instance == nullptr) {
    return nullptr;
  }

  return &instance->debug_messenger();
}

} // namespace quark::vk
