#pragma once

#include "quark/rhi/backend/native_backend_ref.hpp"
#include "quark/rhi/descriptor/descriptor_set_layout_desc.hpp"
#include "quark/rhi/descriptor/details/descriptor_set_layout_handle.hpp"
#include "quark/rhi/device/device_view.hpp"
#include "quark/utils/result.hpp"

#include <cstddef>
#include <cstdint>

namespace quark {

namespace engine {
class RetirementQueue;
};

namespace rhi {

class PipelineLayout;

class DescriptorSetLayout final {
public:
  struct CreateInfo {
    DeviceView device;
    engine::RetirementQueue *retire_queue{nullptr};
    const DescriptorSetLayoutDesc *desc{nullptr};
  };

  DescriptorSetLayout() noexcept;
  ~DescriptorSetLayout();

  DescriptorSetLayout(const DescriptorSetLayout &) = delete;
  DescriptorSetLayout &operator=(DescriptorSetLayout &&other) noexcept;
  DescriptorSetLayout(DescriptorSetLayout &&other) noexcept;

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  void retire(uint64_t retire_at) noexcept;

  [[nodiscard]] bool valid() const noexcept;

  [[nodiscard]] details::DescriptorSetLayoutHandle handle() const noexcept {
    return handle_;
  }

private:
  friend class PipelineLayout;

  [[nodiscard]] NativeBackendRef native_backend() const noexcept;

  DeviceView device_;
  engine::RetirementQueue *retire_queue_{nullptr};

  details::DescriptorSetLayoutHandle handle_{};
};

} // namespace rhi

} // namespace quark
