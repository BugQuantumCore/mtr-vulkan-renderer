#pragma once

#include "vulkan_context.h"
#include <string>
#include <vector>

/**
 * Vulkan图形管线
 */
class VulkanPipeline {
public:
    struct CreateInfo {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;
        std::string vertexFormat;  // "3f,3f,2f,4f" 格式
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        bool blendEnabled = false;
        bool depthTestEnabled = true;
        bool depthWriteEnabled = true;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    };

    VulkanPipeline(VulkanContext* context);
    ~VulkanPipeline();

    bool create(const CreateInfo& info);
    void destroy();

    void bind(VkCommandBuffer cmd);

    VkPipeline getPipeline() const { return m_pipeline; }
    VkPipelineLayout getLayout() const { return m_pipelineLayout; }

private:
    bool createDescriptorSetLayout();
    bool createPipelineLayout();
    bool createGraphicsPipeline(const CreateInfo& info);
    std::vector<VkVertexInputBindingDescription> parseVertexBindings(const std::string& format);
    std::vector<VkVertexInputAttributeDescription> parseVertexAttributes(const std::string& format);

    VulkanContext* m_context;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

    // Uniform缓冲区的描述符集
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
};