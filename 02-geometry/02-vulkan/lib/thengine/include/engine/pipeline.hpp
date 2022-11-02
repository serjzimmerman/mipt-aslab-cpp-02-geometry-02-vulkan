/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <tsimmerman.ss@phystech.edu>, <alex.rom23@mail.ru> wrote this file.  As long as you
 * retain this notice you can do whatever you want with this stuff. If we meet
 * some day, and you think this stuff is worth it, you can buy me a beer in
 * return.
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "shaders.hpp"
#include "vertex.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <numeric>

namespace throttle {
namespace graphics {

struct descriptor_set_data {
public:
  vk::raii::DescriptorSetLayout m_layout{nullptr};
  vk::raii::DescriptorPool      m_pool{nullptr};
  vk::raii::DescriptorSet       m_descriptor_set{nullptr};

  descriptor_set_data(std::nullptr_t) {}
  descriptor_set_data(const vk::raii::Device &p_device)
      : m_layout{create_decriptor_set_layout(
            p_device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex},
                       {vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})},
        m_pool{std::move(create_descriptor_pool(
            p_device, {{vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eCombinedImageSampler, 1}}))},
        m_descriptor_set{std::move((vk::raii::DescriptorSets{p_device, {*m_pool, *m_layout}}).front())} {}

private:
  static vk::raii::DescriptorSetLayout create_decriptor_set_layout(
      const vk::raii::Device                                                            &p_device,
      const std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> &p_binding_data,
      vk::DescriptorSetLayoutCreateFlags                                                 p_flags = {}) {
    std::vector<vk::DescriptorSetLayoutBinding> bindings{static_cast<uint32_t>(p_binding_data.size())};
    for (uint32_t i = 0; i < p_binding_data.size(); ++i)
      bindings.push_back(vk::DescriptorSetLayoutBinding{
          i, std::get<0>(p_binding_data[i]), std::get<1>(p_binding_data[i]), std::get<2>(p_binding_data[i])});
    vk::DescriptorSetLayoutCreateInfo descriptor_set_info{p_flags, bindings};
    return vk::raii::DescriptorSetLayout{p_device, descriptor_set_info};
  }

  static vk::raii::DescriptorPool create_descriptor_pool(const vk::raii::Device                    &p_device,
                                                         const std::vector<vk::DescriptorPoolSize> &p_pool_sizes) {
    uint32_t max_sets =
        std::accumulate(p_pool_sizes.begin(), p_pool_sizes.end(), 0,
                        [](uint32_t sum, vk::DescriptorPoolSize const &dps) { return sum + dps.descriptorCount; });
    vk::DescriptorPoolCreateInfo descriptor_info{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, max_sets,
                                                 p_pool_sizes};
    return vk::raii::DescriptorPool(p_device, descriptor_info);
  }
};

struct pipeline_data {
public:
  vk::raii::RenderPass     m_render_pass{nullptr};
  vk::raii::PipelineLayout m_layout{nullptr};
  vk::raii::Pipeline       m_pipeline{nullptr};

  pipeline_data(std::nullptr_t) {}

  pipeline_data(const vk::raii::Device &p_device, const std::string &p_vertex_file_path,
                const std::string &p_fragment_file_path, const vk::Extent2D &p_extent,
                const descriptor_set_data &p_descriptor_set_data)
      : m_render_pass{create_render_pass(p_device)}, m_layout{create_pipeline_layout(p_device,
                                                                                     p_descriptor_set_data.m_layout)},
        m_pipeline{create_pipeline(p_device, p_vertex_file_path, p_fragment_file_path, p_extent)} {}

private:
  vk::raii::Pipeline create_pipeline(const vk::raii::Device &p_device, const std::string &p_vertex_file_path,
                                     const std::string &p_fragment_file_path, const vk::Extent2D &p_extent) {
    // vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.flags = vk::PipelineVertexInputStateCreateFlags();
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    auto binding_description = vertex::get_binding_description();
    auto attribute_description = vertex::get_attribute_description();
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.vertexAttributeDescriptionCount = attribute_description.size();
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();

    // input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_asm_info{};
    input_asm_info.flags = vk::PipelineInputAssemblyStateCreateFlags();
    input_asm_info.topology = vk::PrimitiveTopology::eTriangleList;

    // viewport and scissor
    vk::Viewport viewport = {0.0f, 0.0f, static_cast<float>(p_extent.width), static_cast<float>(p_extent.height),
                             0.0f, 1.0f};
    vk::Rect2D   scissor = {vk::Offset2D{0, 0}, p_extent};
    vk::PipelineViewportStateCreateInfo viewport_info{vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1,
                                                      &scissor};

    // rasterisation
    vk::PipelineRasterizationStateCreateInfo rasterization_info = {
        vk::PipelineRasterizationStateCreateFlags(),
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        VK_FALSE,
    };
    rasterization_info.lineWidth = 1.0f;

    // multisampling
    vk::PipelineMultisampleStateCreateInfo multisampling = {vk::PipelineMultisampleStateCreateFlags(),
                                                            vk::SampleCountFlagBits::e1, VK_FALSE};

    // color blend
    vk::PipelineColorBlendAttachmentState color_blend_attachments = {};
    color_blend_attachments.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                             vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachments.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.flags = vk::PipelineColorBlendStateCreateFlags();
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = vk::LogicOp::eCopy;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachments;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;

    // shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;

    // vertex shader
    auto                              vertex_shader = create_module(p_vertex_file_path, p_device);
    vk::PipelineShaderStageCreateInfo vertex_shader_info(vk::PipelineShaderStageCreateFlags(),
                                                         vk::ShaderStageFlagBits::eVertex, *vertex_shader, "main");
    shader_stages.push_back(vertex_shader_info);

    // fragment shader
    auto                              fragment_shader = create_module(p_fragment_file_path, p_device);
    vk::PipelineShaderStageCreateInfo fragment_shader_info(
        vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, *fragment_shader, "main");
    shader_stages.push_back(fragment_shader_info);

    vk::GraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.flags = vk::PipelineCreateFlags{};
    pipeline_info.stageCount = shader_stages.size();
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_asm_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterization_info;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = nullptr;
    pipeline_info.renderPass = *m_render_pass;
    pipeline_info.layout = *m_layout;
    return p_device.createGraphicsPipeline(nullptr, pipeline_info);
  }

  static vk::raii::PipelineLayout create_pipeline_layout(const vk::raii::Device              &device,
                                                         const vk::raii::DescriptorSetLayout &p_descriptor_set_layout) {
    vk::PipelineLayoutCreateInfo layout_info{};
    layout_info.flags = vk::PipelineLayoutCreateFlags();
    layout_info.setLayoutCount = 1;
    vk::DescriptorSetLayout set_layouts[] = {*p_descriptor_set_layout};
    layout_info.pSetLayouts = set_layouts;
    layout_info.pushConstantRangeCount = 0;
    return device.createPipelineLayout(layout_info);
  }

  static vk::raii::RenderPass create_render_pass(const vk::raii::Device &p_device) {
    vk::AttachmentDescription color_attachment = {};
    color_attachment.flags = vk::AttachmentDescriptionFlags();
    color_attachment.format = vk::Format::eB8G8R8A8Unorm;
    color_attachment.samples = vk::SampleCountFlagBits::e1;
    color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
    color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
    color_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_attachment.initialLayout = vk::ImageLayout::eUndefined;
    color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.flags = vk::SubpassDescriptionFlags();
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    vk::RenderPassCreateInfo renderpass_info = {};
    renderpass_info.flags = vk::RenderPassCreateFlags();
    renderpass_info.attachmentCount = 1;
    renderpass_info.pAttachments = &color_attachment;
    renderpass_info.subpassCount = 1;
    renderpass_info.pSubpasses = &subpass;

    return p_device.createRenderPass(renderpass_info);
  }
};
} // namespace graphics
} // namespace throttle