#pragma once

#include "lve_device.h"
#include "lve_buffer.h"
#include "lve_texture.h"

//libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

//std
#include <memory>
#include <vector>
#include <unordered_map>

namespace lve {

    struct FragmentBuffer {
        std::shared_ptr<LveTexture> diffuseTexture;
    };

    class LveModel {
    public:
        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex& other) const {
                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
            }
        };

        struct Mesh {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            FragmentBuffer fragmentBuffer;

            std::unique_ptr<LveBuffer> vertexBuffer;
            std::unique_ptr<LveBuffer> indexBuffer;
            uint32_t vertexCount;
            uint32_t indexCount;
            bool hasIndexBuffer = false;

            Mesh();
            ~Mesh();
            Mesh(Mesh&&) = default;
            Mesh& operator=(Mesh&&) = default;
            void createVertexBuffers(LveDevice& device);
            void createIndexBuffers(LveDevice& device);
            void bind(VkCommandBuffer commandBuffer);
            void draw(VkCommandBuffer commandBuffer);
        };

        LveModel(LveDevice& device);
        ~LveModel();

        LveModel(const LveModel&) = delete;
        LveModel& operator=(const LveModel&) = delete;

        static std::unique_ptr<LveModel> createModelFromFile(LveDevice& device, const std::string& filepath);

        LveDevice& lveDevice;

        std::unordered_map<uint32_t, Mesh> meshes;
        static uint32_t nextMeshId;
    };

}