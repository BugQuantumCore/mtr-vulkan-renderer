#pragma once

#include "vulkan_context.h"
#include <cstdint>

/**
 * Vulkan纹理管理
 * 管理2D纹理的创建、上传和采样
 */
class VulkanTexture {
public:
    VulkanTexture(VulkanContext* context);
    ~VulkanTexture();

    /**
     * 从像素数据创建纹理
     */
    bool create(const uint8_t* pixelData, int width, int height, int mipLevels);

    /**
     * 从OpenGL纹理创建（通过VulkanMod的GL互操作）
     */
    bool createFromGL(int glTextureId, int width, int height);

    /**
     * 更新纹理的部分区域
     */
    void update(const uint8_t* data, int x, int y, int width, int height);

    /**
     * 创建采样器
     */
    bool createSampler(VkFilter filter = VK_FILTER_LINEAR,
                       VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                       float maxAnisotropy = 16.0f);

    /**
     * 创建图像视图
     */
    bool createImageView();

    /**
     * 生成MIP映射
     */
    void generateMipmaps(VkCommandBuffer cmd);

    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_sampler; }
    VkImage getImage() const { return m_image; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getMipLevels() const { return m_mipLevels; }

    void destroy();

private:
    bool createImage(int width, int height, int mipLevels,
                     VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage);
    bool transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                VkImageLayout oldLayout, VkImageLayout newLayout,
                                int mipLevels);
    bool copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer,
                            VkImage image, int width, int height);

    VulkanContext* m_context;
    VkImage m_image = VK_NULL_HANDLE;
    VmaAllocation m_allocation = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    int m_width = 0;
    int m_height = 0;
    int m_mipLevels = 1;
    VkFormat m_format = VK_FORMAT_R8G8B8A8_UNORM;
};