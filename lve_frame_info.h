#pragma once

#include "lve_camera.h"
#include "lve_game_object.h"
#include "lve_descriptors.h"
#include "lve_utils.h"

//lib
#include <vulkan/vulkan.h>

namespace lve {

	#define MAX_LIGHTS 10

	struct Light {
		glm::vec4 position{};
		glm::vec4 color{};
	};

	struct GlobalUbo {
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f }; //used for camera position extraction from the last column
		//glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f, -3.f, -1.f}); //this was for directional light
		glm::vec4 ambientLightColor{ 0.01f, 0.01f, 0.01f, 1.0f }; // w is for intensity

		Light lights[MAX_LIGHTS];
		int numLights; // Number of lights currently in use
		
	};

	struct FrameInfo {

		int frameIndex;
		float frameTime;

		VkCommandBuffer commandBuffer;
		LveCamera &camera;

		VkDescriptorSet globalDescriptorSet;
		std::unordered_map<id_t, VkDescriptorSet> textureDescriptorSets;

		LveGameObject::Map& gameObjects;
	};
}