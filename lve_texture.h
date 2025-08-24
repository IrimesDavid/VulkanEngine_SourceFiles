#pragma once

#include "lve_device.h"
#include <string>

//standard
#include <memory>

namespace lve {

    class LveTexture {
    public:
        LveTexture(LveDevice& device, const std::string& filepath);
        ~LveTexture();

        LveTexture(const LveTexture&) = delete;
        LveTexture& operator=(const LveTexture&) = delete;

        VkImageView getImageView() const { return imageView; }
        VkSampler getSampler() const { return sampler; }

        static std::shared_ptr<LveTexture> createFromFile(LveDevice& device, const std::string& filepath);

    private:
        void createTextureImage(const std::string& filepath);
        void createTextureImageView();
        void createTextureSampler();

        LveDevice& lveDevice;

        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView imageView{};
        VkSampler sampler{};
        int texWidth{}, texHeight{}, texChannels{};
    };

}  // namespace lve
