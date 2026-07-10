#include "vulkan_context.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>

VulkanContext::VulkanContext() {}

VulkanContext::~VulkanContext() {
    cleanup();
}

bool VulkanContext::initialize() {
    // 初始化volk (Vulkan meta-loader)
    VkResult result = volkInitialize();
    if (result != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to initialize volk" << std::endl;
        return false;
    }

    // 检查是否VulkanMod已经初始化了Vulkan
    // 如果是，我们将复用其VkInstance和VkDevice
    // 否则创建独立的Vulkan上下文

    if (m_instance == VK_NULL_HANDLE) {
        // 创建独立的Vulkan实例
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "MTR Vulkan Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "MTR-Vulkan";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(__linux__)
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if (result != VK_SUCCESS) {
            std::cerr << "[MTR-Vulkan] Failed to create Vulkan instance" << std::endl;
            return false;
        }

        volkLoadInstance(m_instance);
        m_ownsVulkan = true;

        // 选择物理设备
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            std::cerr << "[MTR-Vulkan] No Vulkan-capable GPU found" << std::endl;
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        // 选择最好的GPU（优先独立显卡）
        m_physicalDevice = devices[0];
        for (auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                m_physicalDevice = device;
                break;
            }
        }

        // 查找图形队列族
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                m_graphicsFamily = i;
                break;
            }
        }

        // 创建逻辑设备
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_graphicsFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
        };

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.geometryShader = VK_FALSE;
        deviceFeatures.multiDrawIndirect = VK_TRUE;
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
        if (result != VK_SUCCESS) {
            std::cerr << "[MTR-Vulkan] Failed to create logical device" << std::endl;
            return false;
        }

        volkLoadDevice(m_device);
        vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
    }

    // 创建VMA分配器
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;

    result = vmaCreateAllocator(&allocatorInfo, &m_allocator);
    if (result != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create VMA allocator" << std::endl;
        return false;
    }

    // 创建命令池和同步对象
    if (!createCommandPools()) return false;
    if (!createDescriptorPool()) return false;
    if (!createSyncObjects()) return false;

    std::cout << "[MTR-Vulkan] Vulkan context initialized: " << getGPUName() << std::endl;
    return true;
}

void VulkanContext::setExternalVulkanHandles(VkInstance instance, VkPhysicalDevice physicalDevice,
                                               VkDevice device, VkQueue graphicsQueue,
                                               uint32_t graphicsFamily) {
    m_instance = instance;
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_graphicsQueue = graphicsQueue;
    m_graphicsFamily = graphicsFamily;
    m_ownsVulkan = false;

    volkLoadInstance(m_instance);
    volkLoadDevice(m_device);
}

int VulkanContext::beginFrame() {
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    // 等待上一帧完成
    vkWaitForFences(m_device, 1, &m_frames[m_currentFrame].inFlightFence,
                    VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_frames[m_currentFrame].inFlightFence);

    // 重置命令缓冲区
    vkResetCommandBuffer(m_frames[m_currentFrame].commandBuffer, 0);

    // 开始记录命令
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(m_frames[m_currentFrame].commandBuffer, &beginInfo);

    return m_currentFrame;
}

VkCommandBuffer VulkanContext::getCurrentCommandBuffer() {
    if (m_currentFrame < 0) return VK_NULL_HANDLE;
    return m_frames[m_currentFrame].commandBuffer;
}

void VulkanContext::endFrame() {
    if (m_currentFrame < 0) return;

    vkEndCommandBuffer(m_frames[m_currentFrame].commandBuffer);

    // 提交命令缓冲区
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_frames[m_currentFrame].commandBuffer;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitStage;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_frames[m_currentFrame].renderCompleteSemaphore;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_frames[m_currentFrame].inFlightFence);
}

void VulkanContext::waitFrame() {
    vkWaitForFences(m_device, MAX_FRAMES_IN_FLIGHT,
                    &m_frames[0].inFlightFence, VK_TRUE, UINT64_MAX);
}

VkShaderModule VulkanContext::loadShaderModule(const char* path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[MTR-Vulkan] Failed to open shader: " << path << std::endl;
        return VK_NULL_HANDLE;
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = buffer.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create shader module" << std::endl;
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

std::string VulkanContext::getGPUName() const {
    if (m_physicalDevice == VK_NULL_HANDLE) return "No GPU";
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
    return std::string(props.deviceName);
}

bool VulkanContext::createCommandPools() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsFamily;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_frames[i].commandPool) != VK_SUCCESS) {
            return false;
        }

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_frames[i].commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_frames[i].commandBuffer) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

bool VulkanContext::createRenderPass() {
    // 简化版 - 实际实现需要与Minecraft的渲染流程匹配
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef = {};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef = {};
    depthRef.attachment = 1;
    depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkAttachmentDescription attachments[] = { colorAttachment, depthAttachment };

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    return vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS;

    // 子pass依赖
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    return vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS;
}

bool VulkanContext::createDescriptorPool() {
    // 创建大型描述符池以支持大量网格和材质
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 512 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 8192;
    poolInfo.poolSizeCount = 4;
    poolInfo.pPoolSizes = poolSizes;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create descriptor pool" << std::endl;
        return false;
    }
    return true;
}

bool VulkanContext::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // 初始已发出

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr,
                &m_frames[i].renderCompleteSemaphore) != VK_SUCCESS) {
            std::cerr << "[MTR-Vulkan] Failed to create semaphore for frame " << i << std::endl;
            return false;
        }
        if (vkCreateFence(m_device, &fenceInfo, nullptr,
                &m_frames[i].inFlightFence) != VK_SUCCESS) {
            std::cerr << "[MTR-Vulkan] Failed to create fence for frame " << i << std::endl;
            return false;
        }
    }
    return true;
}

VkFramebuffer VulkanContext::getCurrentFramebuffer() const {
    // 在实际实现中，这需要从VulkanMod获取当前的交换链帧缓冲
    // 暂时返回VK_NULL_HANDLE
    return VK_NULL_HANDLE;
}

void VulkanContext::cleanup() {
    if (m_device == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(m_device);

    // 清理同步对象
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_frames[i].renderCompleteSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_frames[i].renderCompleteSemaphore, nullptr);
            m_frames[i].renderCompleteSemaphore = VK_NULL_HANDLE;
        }
        if (m_frames[i].inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_frames[i].inFlightFence, nullptr);
            m_frames[i].inFlightFence = VK_NULL_HANDLE;
        }
        if (m_frames[i].commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device, m_frames[i].commandPool, nullptr);
            m_frames[i].commandPool = VK_NULL_HANDLE;
        }
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }

    // 如果我们自己创建了Vulkan资源，需要清理
    if (m_ownsVulkan) {
        if (m_device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;
        }
        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }

    m_physicalDevice = VK_NULL_HANDLE;
    m_graphicsQueue = VK_NULL_HANDLE;

    std::cout << "[MTR-Vulkan] Vulkan context cleaned up" << std::endl;
}