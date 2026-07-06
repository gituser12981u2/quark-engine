#pragma once

#include "quark/engine/registry/backend_registry.hpp"
#include "quark/rhi/descriptor/descriptor_set_layout_desc.hpp"
#include "quark/rhi/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/utils/raii.hpp"
#include "quark/utils/result.hpp"

#include "quark/vk/descriptor/descriptor_set_layout_backend.hpp"
#include "quark/vk/device/device_view.hpp"

namespace quark {

namespace engine {
class RetirementQueue;
}

namespace rhi {

class DescriptorSetLayout final {
public:
  struct CreateInfo {
    vk::DeviceView device{};
    engine::RetirementQueue *retire_queue{nullptr};
    const DescriptorSetLayoutDesc *desc{nullptr};
    // VkDescriptorSetLayoutCreateFlags flags{};
    // const VkAllocationCallbacks *allocator{nullptr};
  };

  DescriptorSetLayout() = default;
  ~DescriptorSetLayout() { destroy(); }

  QUARK_MOVE_ONLY(DescriptorSetLayout);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept;

  [[nodiscard]] VkDescriptorSetLayout vk_handle() const noexcept;

private:
  engine::BackendRegistry<details::DescriptorSetLayoutHandle,
                          vk::DescriptorSetLayoutBackend>
      registry_;
  details::DescriptorSetLayoutHandle handle_{};
  const vk::DescriptorSetLayoutBackend *backend_{nullptr};
};

} // namespace rhi

} // namespace quark
