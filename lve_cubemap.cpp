#pragma once

#include "lve_cubemap.h"
#include "lve_buffer.h"

// stb_image for loading pixels
#include "Externals/stb_image.h"

#include <stdexcept>
#include <iostream>

namespace lve {

    // Helper function to flip pixels vertically (Y flip)
    void flipPixelsVertically(stbi_uc* pixels, int width, int height) {
        int stride = width * 4; // 4 bytes per pixel (RGBA)
        stbi_uc* temp_row = new stbi_uc[stride];

        for (int y = 0; y < height / 2; ++y) {
            stbi_uc* row1 = pixels + y * stride;
            stbi_uc* row2 = pixels + (height - 1 - y) * stride;

            memcpy(temp_row, row1, stride);
            memcpy(row1, row2, stride);
            memcpy(row2, temp_row, stride);
        }

        delete[] temp_row;
    }

    // Helper function to flip pixels horizontally (X flip)
    void flipPixelsHorizontally(stbi_uc* pixels, int width, int height) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width / 2; ++x) {
                int idx1 = (y * width + x) * 4;
                int idx2 = (y * width + (width - 1 - x)) * 4;

                // Swap RGBA values
                for (int c = 0; c < 4; ++c) {
                    stbi_uc temp = pixels[idx1 + c];
                    pixels[idx1 + c] = pixels[idx2 + c];
                    pixels[idx2 + c] = temp;
                }
            }
        }
    }

    std::shared_ptr<LveCubemap> LveCubemap::createFromFiles(LveDevice& device, const std::array<std::string, 6>& filepaths) {
        return std::make_shared<LveCubemap>(device, filepaths);
    }

    LveCubemap::LveCubemap(LveDevice& device, const std::array<std::string, 6>& filepaths) : lveDevice{ device } {
        createCubemapImage(filepaths);
        createCubemapImageView();
        createCubemapSampler();
    }

    LveCubemap::~LveCubemap() {
        vkDestroySampler(lveDevice.device(), sampler, nullptr);
        vkDestroyImageView(lveDevice.device(), imageView, nullptr);
        vkDestroyImage(lveDevice.device(), image, nullptr);
        vkFreeMemory(lveDevice.device(), memory, nullptr);
    }

    void LveCubemap::createCubemapImage(const std::array<std::string, 6>& filepaths) {
        // Load all 6 faces and validate they're the same size
        std::vector<stbi_uc*> facePixels(6);
        int width = 0, height = 0, channels = 0;

        for (int i = 0; i < 6; ++i) {
            int w, h, c;
            facePixels[i] = stbi_load(filepaths[i].c_str(), &w, &h, &c, STBI_rgb_alpha);
            if (!facePixels[i]) {
                // Clean up already loaded faces
                for (int j = 0; j < i; ++j) {
                    stbi_image_free(facePixels[j]);
                }
                printf("failed to load cubemap face: %s", filepaths[i]);
            }

            if (i == 0) {
                width = w;
                height = h;
                channels = c;
                texWidth = width;
                texHeight = height;
                texChannels = channels;
            }
            else if (w != width || h != height) {
                // Clean up all loaded faces
                for (int j = 0; j <= i; ++j) {
                    stbi_image_free(facePixels[j]);
                }
                printf("cubemap faces must have the same dimensions");
            }
        }

        // Apply coordinate system fixes for +X=right, +Z=forward, -Y=up system
		// Face order: left, right, top, bottom, front, back (indices 0-5) - but for my system top is bottom and bottom is top because of the Y flip that we need to do

        // Apply Y flip to all faces (equivalent to texCoord.y = -texCoord.y in shader)
        for (int i = 0; i < 6; ++i) {
            flipPixelsVertically(facePixels[i], width, height);
        }

        // Apply additional flips for the top face (index 3)
        flipPixelsHorizontally(facePixels[3], width, height); // X flip for top face
        flipPixelsVertically(facePixels[3], width, height);   // Z flip for top face (additional Y flip)

        VkDeviceSize layerSize = width * height * 4; // 4 bytes per pixel (RGBA)
        VkDeviceSize totalImageSize = layerSize * 6;

        // Create staging buffer for all 6 faces
        LveBuffer stagingBuffer{
            lveDevice,
            4,
            static_cast<uint32_t>(width * height * 6),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        stagingBuffer.map();

        // Copy all face data to staging buffer
        char* mappedData = static_cast<char*>(stagingBuffer.getMappedMemory());
        for (int i = 0; i < 6; ++i) {
            memcpy(mappedData + i * layerSize, facePixels[i], layerSize);
            stbi_image_free(facePixels[i]);
        }

        // Create VkImage for cubemap
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // Important for cubemaps
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6; // 6 faces for cubemap
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        lveDevice.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

        // Transition image layout for transfer
        lveDevice.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copy buffer to image (all 6 layers)
        lveDevice.copyBufferToImage(
            stagingBuffer.getBuffer(),
            image,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            6); // 6 layers for cubemap

        // Transition to shader read optimal
        lveDevice.transitionImageLayout(
            image,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void LveCubemap::createCubemapImageView() {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE; // Cube view type
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6; // All 6 faces

        if (vkCreateImageView(lveDevice.device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            printf("failed to create cubemap image view!");
        }
    }

    void LveCubemap::createCubemapSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // Important for cubemaps
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        if (vkCreateSampler(lveDevice.device(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            printf("failed to create cubemap sampler!");
        }
    }

}  // namespace lve