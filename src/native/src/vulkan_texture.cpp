#include "vulkan_texture.h"
#include <iostream>
#include <cmath>
#include <algorithm>

VulkanTexture::VulkanTexture(VulkanContext* context)
    : m_context(context) {}

VulkanTexture::~VulkanTexture() {
    destroy();
}

bool VulkanTexture::create(const uint8_t* pixelData, int width, int height, int mipLevels) {
    m_width = width;
    m_height = height;
    m_mipLevels = (mipLevels > 0) ? mipLevels :
            static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
    m_format = VK_FORMAT_R8G8B8A8_UNORM;

    // 创建暂存缓冲区
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

    VulkanBuffer stagingBuffer(m_context);
    if (!stagingBuffer.create(VulkanBuffer::Type::Storage, imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST)) {
        std::cerr << "[MTR-Vulkan] Failed to create staging buffer for texture" << std::endl;
        return false;
    }
    stagingBuffer.upload(pixelData, imageSize);

    // 创建图像
    if (!createImage(width, height, m_mipLevels, m_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT)) {
        return false;
    }

    // 使用临时命令缓冲区来执行图像操作
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = VK_NULL_HANDLE; // 需要命令池
    allocInfo.commandBufferCount = 1;

    // 使用上下文中的命令池
    VkCommandBuffer cmd;
    VkCommandPool tempPool;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_context->getGraphicsFamily();

    vkCreateCommandPool(m_context->getDevice(), &poolInfo, nullptr, &tempPool);
    allocInfo.commandPool = tempPool;
    vkAllocateCommandBuffers(m_context->getDevice(), &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // 转换图像布局为传输目标
    transitionImageLayout(cmd, m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels);

    // 从缓冲区复制到图像
    copyBufferToImage(cmd, stagingBuffer.getBuffer(), m_image, width, height);

    // 生成MIP映射
    if (m_mipLevels > 1) {
        generateMipmaps(cmd);
    } else {
        transitionImageLayout(cmd, m_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
    }

    vkEndCommandBuffer(cmd);

    // 提交并等待
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->getGraphicsQueue());

    // 清理临时资源
    vkFreeCommandBuffers(m_context->getDevice(), tempPool, 1, &cmd);
    vkDestroyCommandPool(m_context->getDevice(), tempPool, nullptr);

    stagingBuffer.destroy();

    // 创建图像视图和采样器
    if (!createImageView()) return false;
    if (!createSampler()) return false;

    return true;
}

bool VulkanTexture::createFromGL(int glTextureId, int width, int height) {
    m_width = width;
    m_height = height;
    m_mipLevels = 1;
    m_format = VK_FORMAT_R8G8B8A8_UNORM;

    // VulkanMod提供了GL纹理到Vulkan图像的互操作功能
    // 这里我们需要通过VulkanMod的API来获取Vulkan图像
    // 由于VulkanMod使用VK_EXT_external_memory或类似机制
    // 实际实现需要调用VulkanMod的互操作接口

    // 临时方案：从GL纹理读取像素数据，然后在Vulkan中创建新纹理
    // 这不是最优方案，但可以工作

    // 注意：在实际实现中，应该通过JNI回调Java侧的VulkanMod互操作API
    // 例如：VulkanInterop.getVulkanImageFromGLTexture(glTextureId)

    std::cerr << "[MTR-Vulkan] GL texture interop not yet implemented for texture "
              << glTextureId << ", falling back to pixel readback" << std::endl;

    // 创建空纹理作为占位
    std::vector<uint8_t> placeholder(width * height * 4, 255);
    return create(placeholder.data(), width, height, 1);
}

void VulkanTexture::update(const uint8_t* data, int x, int y, int width, int height) {
    // 使用临时命令缓冲区更新纹理区域
    VkDeviceSize dataSize = static_cast<VkDeviceSize>(width) * height * 4;

    VulkanBuffer stagingBuffer(m_context);
    if (!stagingBuffer.create(VulkanBuffer::Type::Storage, dataSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST)) {
        return;
    }
    stagingBuffer.upload(data, dataSize);

    // 创建命令缓冲区并执行更新
    VkCommandPool tempPool;
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_context->getGraphicsFamily();
    vkCreateCommandPool(m_context->getDevice(), &poolInfo, nullptr, &tempPool);

    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = tempPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(m_context->getDevice(), &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // 转换布局
    transitionImageLayout(cmd, m_image,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);

    // 复制区域
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { x, y, 0 };
    region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    vkCmdCopyBufferToImage(cmd, stagingBuffer.getBuffer(), m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 恢复布局
    transitionImageLayout(cmd, m_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(m_context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_context->getGraphicsQueue());

    vkFreeCommandBuffers(m_context->getDevice(), tempPool, 1, &cmd);
    vkDestroyCommandPool(m_context->getDevice(), tempPool, nullptr);

    stagingBuffer.destroy();
}

bool VulkanTexture::createImage(int width, int height, int mipLevels,
                                 VkFormat format, VkImageTiling tiling,
                                 VkImageUsageFlags usage) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    VkResult result = vmaCreateImage(m_context->getAllocator(),
            &imageInfo, &allocInfo, &m_image, &m_allocation, nullptr);

    if (result != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create image" << std::endl;
        return false;
    }
    return true;
}

bool VulkanTexture::createImageView() {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_context->getDevice(), &viewInfo, nullptr, &m_imageView) != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create image view" << std::endl;
        return false;
    }
    return true;
}

bool VulkanTexture::createSampler(VkFilter filter, VkSamplerAddressMode addressMode,
                                    float maxAnisotropy) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = (maxAnisotropy > 1.0f) ? VK_TRUE : VK_FALSE;
    samplerInfo.maxAnisotropy = maxAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_mipLevels);

    if (vkCreateSampler(m_context->getDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create sampler" << std::endl;
        return false;
    }
    return true;
}

bool VulkanTexture::transitionImageLayout(VkCommandBuffer cmd, VkImage image,
                                            VkImageLayout oldLayout, VkImageLayout newLayout,
                                            int mipLevels) {
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0,
            0, nullptr, 0, nullptr, 1, &barrier);
    return true;
}

bool VulkanTexture::copyBufferToImage(VkCommandBuffer cmd, VkBuffer buffer,
                                        VkImage image, int width, int height) {
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    vkCmdCopyBufferToImage(cmd, buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    return true;
}

void VulkanTexture::generateMipmaps(VkCommandBuffer cmd) {
    int mipWidth = m_width;
    int mipHeight = m_height;

    for (int i = 1; i < m_mipLevels; i++) {
        // 将上一级MIP从TRANSFER_DST转换为TRANSFER_SRC
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr, 0, nullptr, 1, &barrier);

        // Blit上一级MIP到当前级
        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = {
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(cmd, m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_LINEAR);

        // 将上一级MIP转换为SHADER_READ_ONLY
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // 将最后一级MIP转换为SHADER_READ_ONLY
    VkImageMemoryBarrier finalBarrier = {};
    finalBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    finalBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    finalBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    finalBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    finalBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    finalBarrier.image = m_image;
    finalBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    finalBarrier.subresourceRange.baseMipLevel = m_mipLevels - 1;
    finalBarrier.subresourceRange.levelCount = 1;
    finalBarrier.subresourceRange.baseArrayLayer = 0;
    finalBarrier.subresourceRange.layerCount = 1;
    finalBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    finalBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &finalBarrier);
}

void VulkanTexture::destroy() {
    VkDevice device = m_context ? m_context->getDevice() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE) return;

    if (m_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    if (m_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
    if (m_image != VK_NULL_HANDLE) {
        vmaDestroyImage(m_context->getAllocator(), m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
    }
}