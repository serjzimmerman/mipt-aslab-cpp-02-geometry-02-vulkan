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

#include "debug.hpp"
#include "error.hpp"
#include "utils.hpp"
#include "vulkan_hpp_include.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>

namespace ezvk {

class unsupported_error : public ezvk::error {
  std::vector<std::string> m_missing;

public:
  unsupported_error(std::string msg, auto start, auto finish) : ezvk::error{msg}, m_missing{start, finish} {}

  const auto &missing() const { return m_missing; }
};

// clang-format off
class i_instance {
public:
  virtual vk::raii::Instance       &operator()()       & = 0;
  virtual const vk::raii::Instance &operator()() const & = 0;
  virtual ~i_instance() {}
};
// clang-format on

class instance : public i_instance {
  vk::raii::Instance m_instance = nullptr;

public:
  instance() = default;

  instance(const vk::raii::Context &ctx, vk::ApplicationInfo app_info, auto ext_start, auto ext_finish,
           auto layers_start, auto layers_finish) {
    auto [ext_ok, missing_ext] = supports_extensions(ext_start, ext_finish, ctx);
    auto [layers_ok, missing_layers] = supports_layers(layers_start, layers_finish, ctx);

    if (!ext_ok || !layers_ok) {
      std::move(missing_layers.begin(), missing_layers.end(), std::back_inserter(missing_ext));
      throw unsupported_error{"Vulkan does not support some required extensions/layers", missing_ext.begin(),
                              missing_ext.end()};
    }

    std::vector<const char *> extensions, layers;
    for (; ext_start != ext_finish; ++ext_start) {
      extensions.push_back(ext_start->c_str());
    }
    for (; layers_start != layers_finish; ++layers_start) {
      layers.push_back(layers_start->c_str());
    }

    vk::InstanceCreateInfo create_info = {.pApplicationInfo = &app_info,
                                          .enabledLayerCount = static_cast<uint32_t>(layers.size()),
                                          .ppEnabledLayerNames = layers.data(),
                                          .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
                                          .ppEnabledExtensionNames = extensions.data()};

    m_instance = vk::raii::Instance{ctx, create_info};
  }

  using supports_result = std::pair<bool, std::vector<std::string>>;

  [[nodiscard]] static supports_result supports_extensions(auto start, auto finish, const vk::raii::Context &ctx) {
    const auto supported_extensions = ctx.enumerateInstanceExtensionProperties();
    auto missing_extensions = utils::find_all_missing(supported_extensions.begin(), supported_extensions.end(), start,
                                                      finish, [](auto a) { return a.extensionName; });
    return std::make_pair(missing_extensions.empty(), missing_extensions);
  }

  [[nodiscard]] static supports_result supports_layers(auto start, auto finish, const vk::raii::Context &ctx) {
    const auto supported_layers = ctx.enumerateInstanceLayerProperties();
    auto       missing_layers = utils::find_all_missing(supported_layers.begin(), supported_layers.end(), start, finish,
                                                        [](auto a) { return a.layerName; });
    return std::make_pair(missing_layers.empty(), missing_layers);
  }

  vk::raii::Instance       &operator()() & override { return m_instance; }
  const vk::raii::Instance &operator()() const & override { return m_instance; }
};

} // namespace ezvk