#pragma once

#include "lve_device.h"
#include "lve_game_object.h"
#include "lve_renderer.h"
#include "lve_window.h"
#include "lve_descriptors.h"
#include "Systems/simple_render_system.h"
#include "Systems/light_system.h"
#include "Systems/skybox_render_system.h"
#include "lve_texture.h"
#include "lve_cubemap.h"
#include "lve_utils.h"

// std
#include <memory>
#include <vector>

namespace lve {
	
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();

	private:
		void loadGameObjects();
		void handleStatusBar();
		void resetSystem();
		void createSystemsAndDescriptorLayouts();
		void createDescriptorSets();

		LveWindow lveWindow{ WIDTH, HEIGHT, "Vulkan Engine" };
		LveDevice lveDevice{ lveWindow };
		LveRenderer lveRenderer{ lveWindow, lveDevice };

		//note: order of declarations matters
		std::unique_ptr<LveDescriptorPool> globalPool{};
		LveGameObject::Map gameObjects;
		std::shared_ptr<LveTexture> defaultTexture; //fallback texture
		std::shared_ptr<LveCubemap> skyboxCubemap; //skybox cubemap
		StatusBar statusBar;

		std::vector<std::unique_ptr<LveBuffer>> uboBuffers;
		std::vector<VkDescriptorSet> globalDescriptorSets;
		std::unique_ptr<LveDescriptorSetLayout> globalSetLayout;
		std::unique_ptr<LveDescriptorSetLayout> textureSetLayout;
		std::unordered_map<id_t, VkDescriptorSet> textureDescriptorSets;
		std::unique_ptr<SimpleRenderSystem> simpleRenderSystem{};
		std::unique_ptr<LightSystem> lightSystem{};
		std::unique_ptr<SkyboxRenderSystem> skyboxRenderSystem{};
	};
}  // namespace lve