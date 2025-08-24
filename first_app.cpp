#include "first_app.h"

#include "lve_camera.h"
#include "input_controller.h"
#include "lve_buffer.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <chrono>
#include <cassert>
#include <stdexcept>

namespace lve {

    FirstApp::FirstApp() { 
        defaultTexture = LveTexture::createFromFile(lveDevice, "Textures/white.png");
        assert(defaultTexture && "Default texture pointer is null!");

        loadGameObjects();

        globalPool = LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT + gameObjects.size())
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gameObjects.size())
            .build();
        createSystemsAndDescriptorLayouts();
        createDescriptorSets();
    }

    FirstApp::~FirstApp() {}

    void FirstApp::run() {

        LveCamera camera{};

        auto viewerObject = LveGameObject::createGameObject();
        viewerObject.transform.translation = { 0.0f, -2.0f, -12.5f }; // Set initial camera position (axis standard: X = right, -Y = up, Z = forward)
        InputController inputController{};
        inputController.bindStatusBar(&statusBar);
		inputController.bindWindow(lveWindow.getGLFWwindow());
        
        auto currentTime = std::chrono::high_resolution_clock::now();

        while (!lveWindow.shouldClose()) {
            glfwPollEvents();
            handleStatusBar();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period> (newTime - currentTime).count();
            currentTime = newTime;


            inputController.processMouseMovement(frameTime, viewerObject);
            inputController.moveInSpace(frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = lveRenderer.getAspectRatio();
            //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
            camera.setPerspectiveProjection(glm::radians(60.f), aspect, 0.1f, 100.f);

            if (auto commandBuffer = lveRenderer.beginFrame()) {
                int frameIndex = lveRenderer.getFrameIndex();
                FrameInfo frameInfo{
                    frameIndex,
                    frameTime,
                    commandBuffer,
                    camera,
                    globalDescriptorSets[frameIndex],
                    textureDescriptorSets,
                    gameObjects
                };

                //update
                GlobalUbo ubo{};
                ubo.projection = camera.getProjection();
                ubo.view = camera.getView();
				ubo.inverseView = camera.getInverseView();
				lightSystem->update(frameInfo, ubo);
                uboBuffers[frameIndex]->writeToBuffer(&ubo);
                uboBuffers[frameIndex]->flush();

                //render
                lveRenderer.beginSwapChainRenderPass(commandBuffer);
                simpleRenderSystem->renderGameObjects(frameInfo);
				lightSystem->render(frameInfo);
                lveRenderer.endSwapChainRenderPass(commandBuffer);
                lveRenderer.endFrame();
            }
        }

        vkDeviceWaitIdle(lveDevice.device());
    }

    void FirstApp::loadGameObjects() {

        //Obj1
        auto gameObj = LveGameObject::createGameObject();
        std::shared_ptr<LveModel> lveModel = LveModel::createModelFromFile(lveDevice, "C://Dev//Work//CODING//VULKAN_PROJECTS//VulkanProject1//VulkanProject1//Models//bricks//bricks.obj");
        gameObj.model = lveModel;
            //gameObj.transform.translation = { 0.0f, 0.0f, 2.5f };
        gameObjects.emplace(gameObj.getId(), std::move(gameObj));

        //Lights
        {
            auto lightObj = LveGameObject::makeLight(2.f, 0.1f, glm::vec3(1.f, 0.f, 0.f));
            lightObj.transform.translation = { 1.0f, -2.5f, -1.f };
            gameObjects.emplace(lightObj.getId(), std::move(lightObj));
        }

        {
            auto lightObj = LveGameObject::makeLight(2.f, 0.1f, glm::vec3(0.f, 1.f, 0.f));
            lightObj.transform.translation = { -1.0f, -2.5f, -1.f };
            gameObjects.emplace(lightObj.getId(), std::move(lightObj));
        }

        {
            auto lightObj = LveGameObject::makeLight(2.f, 0.1f, glm::vec3(0.f, 0.f, 1.f));
            lightObj.transform.translation = { 0.0f, -2.5f, 1.f };
            gameObjects.emplace(lightObj.getId(), std::move(lightObj));
        }
    }
    void FirstApp::createDescriptorSets() {
        for (auto& kv : gameObjects) {
            auto& obj = kv.second;
            auto id = kv.first;

            if (obj.model == nullptr) continue;
            if (!obj.model->fragmentBuffer.diffuseTexture) {
                obj.model->fragmentBuffer.diffuseTexture = defaultTexture;
            }

            VkDescriptorImageInfo diffuseInfo{};
            diffuseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            diffuseInfo.imageView = obj.model->fragmentBuffer.diffuseTexture->getImageView();
            diffuseInfo.sampler = obj.model->fragmentBuffer.diffuseTexture->getSampler();


            VkDescriptorSet descriptorSet;
            LveDescriptorWriter(*textureSetLayout, *globalPool)
                .writeImage(0, &diffuseInfo) //diffuse at binding 0
                .build(descriptorSet);

            textureDescriptorSets[id] = descriptorSet;
        }
	}

    void FirstApp::handleStatusBar() {
        if (statusBar.reloadResources) {
            statusBar.reloadResources = false;

            if (statusBar.command == "MSAA1") {
                lveDevice.setMsaaSampleCount(VK_SAMPLE_COUNT_1_BIT);
            }
            if (statusBar.command == "MSAA2") {
                lveDevice.setMsaaSampleCount(VK_SAMPLE_COUNT_2_BIT);
            }
            if (statusBar.command == "MSAA4") {
                lveDevice.setMsaaSampleCount(VK_SAMPLE_COUNT_4_BIT);
            }
            if (statusBar.command == "MSAA8") {
                lveDevice.setMsaaSampleCount(VK_SAMPLE_COUNT_8_BIT);
            }

			statusBar.command = "";
            resetSystem();
        }
    }

    void FirstApp::resetSystem() {
        vkDeviceWaitIdle(lveDevice.device());

        lveRenderer.recreateSwapChain();

        globalPool = LveDescriptorPool::Builder(lveDevice)
            .setMaxSets(LveSwapChain::MAX_FRAMES_IN_FLIGHT + gameObjects.size())
            .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, LveSwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gameObjects.size())
            .build();        
        createSystemsAndDescriptorLayouts();
        createDescriptorSets();

    }

    void FirstApp::createSystemsAndDescriptorLayouts() {
        uboBuffers = std::vector<std::unique_ptr<LveBuffer>>(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<LveBuffer>(
                lveDevice,
                sizeof(GlobalUbo),
                1,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        // - GLOBAL (UBO) layout (set=0) -
        globalSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
            .build();

        globalDescriptorSets = std::vector<VkDescriptorSet>(LveSwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            LveDescriptorWriter(*globalSetLayout, *globalPool)
                .writeBuffer(0, &bufferInfo)
                .build(globalDescriptorSets[i]);
        }

        // - TEXTURE layout (set=1) -
        textureSetLayout = LveDescriptorSetLayout::Builder(lveDevice)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) //for now only 1 texture type
            .build();
		// NOTE: will write to the texture set when creating the actual textures

		// - Systems -
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
            globalSetLayout->getDescriptorSetLayout(),
            textureSetLayout->getDescriptorSetLayout()
        };

        simpleRenderSystem = std::make_unique<SimpleRenderSystem>(
            lveDevice,
            lveRenderer.getSwapChainRenderPass(),
            descriptorSetLayouts
        );

        lightSystem = std::make_unique<LightSystem>(
            lveDevice,
            lveRenderer.getSwapChainRenderPass(),
            globalSetLayout->getDescriptorSetLayout()
        );
    }

}  // namespace lve