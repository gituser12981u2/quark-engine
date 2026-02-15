#include "backend/vulkan/vulkan_context.hpp"

#include <exception>
#include <iostream>

int main() {
  try {
    quark::vk::VulkanContext context;
    std::cout << "Vulkan initialised successfully.\n";
    context.run();
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  }
}
