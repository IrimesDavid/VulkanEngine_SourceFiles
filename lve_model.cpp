#include "lve_model.h"
#include "lve_utils.h"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include "Externals/tiny_obj_loader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// std
#include <cassert>
#include <iostream>
#include <cstring>
#include <unordered_map>

namespace std {
    template <>
    struct hash<lve::LveModel::Vertex> {
        size_t operator()(const lve::LveModel::Vertex& vertex) const noexcept {
            size_t seed = 0;
            // Combine all fields for hashing
            lve::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
            return seed;
        }
    };
}

namespace lve {
    uint32_t LveModel::nextMeshId = 1;

    // Mesh methods
    LveModel::Mesh::Mesh() {}
    LveModel::Mesh::~Mesh() {}

    void LveModel::Mesh::createVertexBuffers(LveDevice& device) {
        vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3\n");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
        uint32_t vertexSize = sizeof(vertices[0]);

        LveBuffer stagingBuffer{
            device,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)vertices.data());

        vertexBuffer = std::make_unique<LveBuffer>(
            device,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }

    void LveModel::Mesh::createIndexBuffers(LveDevice& device) {
        indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = indexCount > 0;

        if (!hasIndexBuffer) {
            return;
        }

        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);

        LveBuffer stagingBuffer{
            device,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)indices.data());

        indexBuffer = std::make_unique<LveBuffer>(
            device,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    void LveModel::Mesh::bind(VkCommandBuffer commandBuffer) {
        VkBuffer buffers[] = { vertexBuffer->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer) {
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void LveModel::Mesh::draw(VkCommandBuffer commandBuffer) {
        if (hasIndexBuffer) {
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        }
        else {
            vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    // LveModel methods
    LveModel::LveModel(LveDevice& device) : lveDevice{ device } {}
    LveModel::~LveModel() {}

    std::unique_ptr<LveModel> LveModel::createModelFromFile(LveDevice& device, const std::string& filepath) {
        auto model = std::make_unique<LveModel>(device);

        std::string directory = filepath.substr(0, filepath.find_last_of('/'));
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), directory.c_str());

        std::cout << warn << std::endl;
        std::cout << err << std::endl;

        std::cout << "Material Count: " << materials.size() << std::endl;

        for (size_t s = 0; s < shapes.size(); ++s) {
            Mesh mesh;
            std::unordered_map<Vertex, uint32_t> uniqueVertices{};

            for (const auto& index : shapes[s].mesh.indices) {
                Vertex vertex{};

                if (index.vertex_index >= 0) {
                    // Apply coordinate system transformation:
                    // +X = right, +Z = forward, -Y = up
                    // Remove horizontal flip by not negating X
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],     // Keep X as is (no horizontal flip)
                        -attrib.vertices[3 * index.vertex_index + 1],    // Negate Y for vertical correction
                        -attrib.vertices[3 * index.vertex_index + 2],    // Negate Z for coordinate system
                    };

                    vertex.color = {
                        -attrib.colors[3 * index.vertex_index + 0],
                        -attrib.colors[3 * index.vertex_index + 1],
                        -attrib.colors[3 * index.vertex_index + 2],
                    };
                }

                if (index.normal_index >= 0) {
                    // Apply the same transformation to normals as positions
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],      // Keep X normal as is
                        -attrib.normals[3 * index.normal_index + 1],     // Negate Y normal
                        -attrib.normals[3 * index.normal_index + 2],     // Negate Z normal
                    };
                }

                if (index.texcoord_index >= 0) {
                    // Keep texture coordinates properly oriented
                    vertex.uv = {
                       attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
                    mesh.vertices.push_back(vertex);
                }
                mesh.indices.push_back(uniqueVertices[vertex]);
            }

            // Reverse triangle winding order to fix inside-out lighting
            // This is necessary because the coordinate system transformation changes handedness
            for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
            }

            // Assign texture from material if available
            int matId = shapes[s].mesh.material_ids.empty() ? -1 : shapes[s].mesh.material_ids[0];
            if (matId >= 0 && matId < materials.size()) {
                const auto& mat = materials[matId];
                if (!mat.diffuse_texname.empty()) {
                    std::string fullPath = directory + "/" + mat.diffuse_texname;
                    mesh.fragmentBuffer.diffuseTexture = LveTexture::createFromFile(device, fullPath);
                }
            }

            mesh.createVertexBuffers(model->lveDevice);
            mesh.createIndexBuffers(model->lveDevice);

            model->meshes[LveModel::nextMeshId++] = std::move(mesh);
        }

        std::cout << "Loaded: " << filepath << "\n";
        std::cout << "Mesh count: " << model->meshes.size() << "\n";

        return model;
    }

    // Vertex attribute/binding methods
    std::vector<VkVertexInputBindingDescription> LveModel::Vertex::getBindingDescriptions() {
        std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(Vertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescriptions;
    }

    std::vector<VkVertexInputAttributeDescription> LveModel::Vertex::getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

        attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
        attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
        attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
        attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

        return attributeDescriptions;
    }
}