#include <cstddef>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/instance/details/debug_messenger.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <quark/vk/instance/instance_bundle.hpp>

namespace quark::vk {

util::Status InstanceBundle::create(const CreateInfo &ci) {
  return instance_.create(ci.instance);
}

void InstanceBundle::destroy() noexcept { instance_.destroy(); }

VkInstance InstanceBundle::vk_instance() const noexcept {
  return instance_.handle();
}

const details::DebugMessenger *
InstanceBundle::debug_messenger() const noexcept {
  return &instance_.debug_messenger();
}

} // namespace quark::vk
