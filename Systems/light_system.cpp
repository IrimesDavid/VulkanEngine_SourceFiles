#include "light_system.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <map>
#include <stdexcept>

namespace lve {

    struct LightPushConstants {
        glm::vec4 position{};
        glm::vec4 color{};
        float radius = .1f;
    };

    LightSystem::LightSystem(LveDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
        : lveDevice{ device } {
        createPipelineLayout(globalSetLayout);
        createPipeline(renderPass);
    }

    LightSystem::~LightSystem() {
        vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
    }

    void LightSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(LightPushConstants);

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
        if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }

    void LightSystem::createPipeline(VkRenderPass renderPass) {
        assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

        PipelineConfigInfo pipelineConfig{};
        LvePipeline::defaultPipelineConfigInfo(pipelineConfig);
		LvePipeline::enableAlphaBlending(pipelineConfig); // Enable alpha blending for light rendering
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.attributeDescriptions.clear();
        pipelineConfig.renderPass = renderPass;
        pipelineConfig.pipelineLayout = pipelineLayout;
        pipelineConfig.multisampleInfo.rasterizationSamples = lveDevice.getMsaaSampleCount();
        lvePipeline = std::make_unique<LvePipeline>(
            lveDevice,
            "shaders/light.vert.spv",
            "shaders/light.frag.spv",
            pipelineConfig);
    }

    void LightSystem::update(FrameInfo& frameInfo, GlobalUbo& ubo) {
        int lightIndex = 0;
        for(auto& kv: frameInfo.gameObjects) {
            auto& obj= kv.second;
            if (obj.lightComponent == nullptr) continue;
            
            //copy light to ubo
            ubo.lights[lightIndex].position = glm::vec4(obj.transform.translation, 1.0f);
			ubo.lights[lightIndex].color = glm::vec4(obj.color, obj.lightComponent->lightIntensity);

            lightIndex++;
		}
        ubo.numLights = lightIndex;
    }

    void LightSystem::render(FrameInfo& frameInfo) {
        //sort lights
		std::map<float, LveGameObject::id_t> sortedLights;

        for (auto& kv : frameInfo.gameObjects) {
            auto& obj = kv.second;
            if (obj.lightComponent == nullptr) continue;
			auto offset = frameInfo.camera.getPosition() - obj.transform.translation;
			float distanceSquared = glm::dot(offset, offset);
			sortedLights[distanceSquared] = obj.getId();

        }
        lvePipeline->bind(frameInfo.commandBuffer);

        vkCmdBindDescriptorSets(
            frameInfo.commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            0, 1,
            &frameInfo.globalDescriptorSet,
            0, nullptr);

		// iterate through sorted lights in reverse order (furthest to closest)
        for (auto it = sortedLights.rbegin(); it != sortedLights.rend(); it++) {
            auto& obj = frameInfo.gameObjects.at(it->second);
 
            LightPushConstants push{};
			push.position = glm::vec4(obj.transform.translation, 1.0f);
			push.color = glm::vec4(obj.color, obj.lightComponent->lightIntensity);
			push.radius = obj.transform.scale.x;

            vkCmdPushConstants(
                frameInfo.commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(LightPushConstants),
                &push
            );
		    vkCmdDraw(frameInfo.commandBuffer, 6, 1, 0, 0);
        }
    }

}  // namespace lve