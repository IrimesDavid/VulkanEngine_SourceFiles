#pragma once

#include "skybox_render_system.h"

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace lve {

    SkyboxRenderSystem::SkyboxRenderSystem(
        LveDevice& device,
        VkRenderPass renderPass,
        VkDescriptorSetLayout globalSetLayout)
        : lveDevice{ device } {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    SkyboxRenderSystem::~SkyboxRenderSystem() {
        vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
    }

    void SkyboxRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create skybox pipeline layout!");
        }
    }

    void SkyboxRenderSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LvePipeline::defaultPipelineConfigInfo(pipelineConfig);

        // Clear vertex input since skybox uses procedural geometry
        pipelineConfig.bindingDescriptions.clear();
        pipelineConfig.attributeDescriptions.clear();

        // Set up depth testing for skybox (render at far plane)
        pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

        // Disable face culling for skybox
        pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;

        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipelineConfig.multisampleInfo.rasterizationSamples = lveDevice.getMsaaSampleCount();

        lvePipeline = std::make_unique<LvePipeline>(
            lveDevice,
            "Shaders/skybox.vert.spv",
            "Shaders/skybox.frag.spv",
            pipelineConfig);
    }

    void SkyboxRenderSystem::render(FrameInfo& frameInfo) {
        lvePipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr);

        // Draw skybox cube (36 vertices for 12 triangles)
        vkCmdDraw(frameInfo.commandBuffer, 36, 1, 0, 0);
    }

}  // namespace lve