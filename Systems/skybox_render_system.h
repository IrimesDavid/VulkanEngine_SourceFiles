#pragma once

#include "../lve_device.h"
#include "../lve_pipeline.h"
#include "../lve_game_object.h"
#include "../lve_camera.h"
#include "../lve_frame_info.h"
#include "../lve_cubemap.h"

#include <memory>
#include <vector>

namespace lve {

    class SkyboxRenderSystem {
    public:
        SkyboxRenderSystem(
            LveDevice& device,
            VkRenderPass renderPass,
            VkDescriptorSetLayout globalSetLayout);
        ~SkyboxRenderSystem();

        SkyboxRenderSystem(const SkyboxRenderSystem&) = delete;
        SkyboxRenderSystem& operator=(const SkyboxRenderSystem&) = delete;

        void render(FrameInfo& frameInfo);

    private:
        void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
        void createPipeline(VkRenderPass renderPass);

        LveDevice& lveDevice;
        std::unique_ptr<LvePipeline> lvePipeline;
        VkPipelineLayout pipelineLayout{};
    };

}  // namespace lve