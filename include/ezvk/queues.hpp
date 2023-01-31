/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <tsimmerman.ss@phystech.edu>, <alex.rom23@mail.ru> wrote this file.  As long as you
 * retain this notice you can do whatever you want with this stuff. If we meet
 * some day, and you think this stuff is worth it, you can buy us a beer in
 * return.
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "utils.hpp"
#include "vulkan_hpp_include.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace ezvk {

using queue_family_index_type = uint32_t;
struct queue_family_indices {
  queue_family_index_type graphics, present;
};

inline std::vector<queue_family_index_type>
find_family_indices_with_queue_type(const vk::raii::PhysicalDevice &p_device, vk::QueueFlagBits queue_bits) {
  auto                                 properties = p_device.getQueueFamilyProperties();
  std::vector<queue_family_index_type> graphics_indices;

  for (queue_family_index_type i = 0; const auto &qfp : properties) {
    if (qfp.queueFlags & queue_bits) graphics_indices.push_back(i);
    ++i;
  }

  return graphics_indices;
}

inline std::vector<queue_family_index_type> find_graphics_family_indices(const vk::raii::PhysicalDevice &p_device) {
  return find_family_indices_with_queue_type(p_device, vk::QueueFlagBits::eGraphics);
}

inline std::vector<queue_family_index_type> find_present_family_indices(const vk::raii::PhysicalDevice &p_device,
                                                                        const vk::raii::SurfaceKHR     &surface) {
  auto                                 size = p_device.getQueueFamilyProperties().size();
  std::vector<queue_family_index_type> present_indices;

  for (queue_family_index_type i = 0; i < size; ++i) {
    if (p_device.getSurfaceSupportKHR(i, *surface)) present_indices.push_back(i);
  }

  return present_indices;
}

class i_graphics_present_queues {
public:
  virtual const vk::raii::Queue &graphics() const = 0;
  virtual const vk::raii::Queue &present() const = 0;
  virtual ~i_graphics_present_queues() {}
};

using queue_index_type = uint32_t;

class device_queue {
  vk::raii::Queue         m_queue = nullptr;
  queue_index_type        m_queue_index;
  queue_family_index_type m_queue_family_index;

public:
  device_queue() = default;

  device_queue(const vk::raii::Device &l_device, queue_family_index_type queue_family, queue_index_type index) {
    m_queue = l_device.getQueue(queue_family, index);
    m_queue_index = index;
    m_queue_family_index = queue_family;
  }

  auto family_index() const { return m_queue_family_index; }
  auto queue_index() const { return m_queue_index; }

  auto       &queue() { return m_queue; }
  const auto &queue() const { return m_queue; }
};

namespace detail {

class separate_graphics_present_queues : public i_graphics_present_queues {
  device_queue m_graphics, m_present;

public:
  separate_graphics_present_queues(const vk::raii::Device &l_device, queue_family_index_type graphics_family,
                                   queue_index_type graphics, queue_family_index_type present_family,
                                   queue_index_type present) {
    m_graphics = {l_device, graphics_family, graphics};
    m_present = {l_device, present_family, present};
  }

  const vk::raii::Queue &graphics() const override { return m_graphics.queue(); }
  const vk::raii::Queue &present() const override { return m_present.queue(); }
};

class single_graphics_present_queues : i_graphics_present_queues {
  device_queue m_queue;

public:
  single_graphics_present_queues(const vk::raii::Device &l_device, queue_family_index_type family,
                                 queue_index_type index) {
    m_queue = {l_device, family, index};
  }

  const vk::raii::Queue &graphics() const override { return m_queue.queue(); }
  const vk::raii::Queue &present() const override { return m_queue.queue(); }
};

} // namespace detail

inline vk::raii::CommandPool create_command_pool(const vk::raii::Device &device, queue_family_index_type queue) {
  return device.createCommandPool(vk::CommandPoolCreateInfo{.queueFamilyIndex = queue});
}

} // namespace ezvk