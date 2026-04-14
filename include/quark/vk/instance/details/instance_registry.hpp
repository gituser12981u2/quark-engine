#pragma once

#include <quark/utils/generational_registry.hpp>
#include <quark/vk/instance/details/instance.hpp>
#include <quark/vk/instance/details/instance_handle.hpp>

namespace quark::vk {

using InstanceRegistry = util::GenerationalRegistry<Instance, InstanceHandle>;

} // namespace quark::vk
