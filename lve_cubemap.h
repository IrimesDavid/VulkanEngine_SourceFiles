#pragma once

#include "lve_device.h"
#include <string>
#include <vector>
#include <array>

//standard
#include <memory>

namespace lve {

    class LveCubemap {
    public:
        // Constructor takes 6 texture file paths in order: +X, -X, +Y, -Y, +Z, -Z
        LveCubemap(LveDevice& device, const std::array<std::string, 6>& filepaths);
        ~LveCubemap();

        LveCubemap(const LveCubemap&) = delete;
        LveCubemap& operator=(const LveCubemap&) = delete;

        VkImageView getImageView() const { return imageView; }
        VkSampler getSampler() const { return sampler; }

        static std::shared_ptr<LveCubemap> createFromFiles(LveDevice& device, const std::array<std::string, 6>& filepaths);

    private:
        void createCubemapImage(const std::array<std::string, 6>& filepaths);
        void createCubemapImageView();
        void createCubemapSampler();

        LveDevice& lveDevice;

        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView imageView{};
        VkSampler sampler{};
        int texWidth{}, texHeight{}, texChannels{};
    };

}  // namespace lve