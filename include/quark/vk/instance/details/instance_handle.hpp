#pragma once

#include <quark/utils/generic_handle.hpp>

namespace quark::vk {

/**
 * @brief Opaque handle to a registry-managed InstanceBundle.
 *
 * Generation prevents use-after-free when slots are reused.
 */
struct InstanceHandleTag;

using InstanceHandle = util::GenericHandle<InstanceHandleTag>;

} // namespace quark::vk
