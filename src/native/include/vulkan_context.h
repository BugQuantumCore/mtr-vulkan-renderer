#pragma once

#define VOLK_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>

#include <vector>
#include <string>

/**
 * Vulkan上下文 - 管理Vulkan实例、设备、交换链等核心资源。
 *
 * 重要：VulkanMod已经创建了VkInstance和VkDevice。
 * 我们通过外部句柄获取这些资源，而不是重新创建。
 * 这样我们可以与VulkanMod共享同一个Vulkan上下文。
 */
class VulkanContext {
public:
    struct FrameData {
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;
    };

    VulkanContext();
    ~VulkanContext();

    /**
     * 初始化Vulkan上下文。
     * 如果VulkanMod已初始化，则复用其VkInstance/VkDevice。
     * 否则创建独立的Vulkan上下文。
     */
    bool initialize();

    /**
     * 从VulkanMod获取Vulkan句柄
     * 这些值通过JNI从Java侧传入
     */
    void setExternalVulkanHandles(VkInstance instance, VkPhysicalDevice physicalDevice,
                                   VkDevice device, VkQueue graphicsQueue,
                                   uint32_t graphicsFamily);

    // 帧管理
    int beginFrame();
    VkCommandBuffer getCurrentCommandBuffer();
    void endFrame();
    void waitFrame();

    // 资源创建辅助
    VkShaderModule loadShaderModule(const char* path);
    VkRenderPass getMainRenderPass() const { return m_renderPass; }
    VkFramebuffer getCurrentFramebuffer() const;

    // 获取器
    VkDevice getDevice() const { return m_device; }
    VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
    VmaAllocator getAllocator() const { return m_allocator; }
    VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
    uint32_t getGraphicsFamily() const { return m_graphicsFamily; }
    std::string getGPUName() const;

    void cleanup();

private:
    bool createCommandPools();
    bool createRenderPass();
    bool createDescriptorPool();
    bool createSyncObjects();

    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsFamily = 0;

    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    FrameData m_frames[MAX_FRAMES_IN_FLIGHT];
    int m_currentFrame = 0;

    bool m_ownsVulkan = false; // 是否由我们创建Vulkan（vs 复用VulkanMod的）
};