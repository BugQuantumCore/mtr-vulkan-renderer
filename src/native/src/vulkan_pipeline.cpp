#include "vulkan_pipeline.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

VulkanPipeline::VulkanPipeline(VulkanContext* context)
    : m_context(context) {}

VulkanPipeline::~VulkanPipeline() {
    destroy();
}

bool VulkanPipeline::create(const CreateInfo& info) {
    if (!createDescriptorSetLayout()) return false;
    if (!createPipelineLayout()) return false;
    if (!createGraphicsPipeline(info)) return false;
    return true;
}

bool VulkanPipeline::createDescriptorSetLayout() {
    // Binding 0: Uniform Buffer (MVP矩阵, 模型矩阵, 光照参数)
    VkDescriptorSetLayoutBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboBinding.pImmutableSamplers = nullptr;

    // Binding 1: Combined Image Sampler (纹理)
    VkDescriptorSetLayoutBinding samplerBinding = {};
    samplerBinding.binding = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerBinding.pImmutableSamplers = nullptr;

    // Binding 2: Instance Data (Storage Buffer - 实例化数据)
    VkDescriptorSetLayoutBinding instanceBinding = {};
    instanceBinding.binding = 2;
    instanceBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    instanceBinding.descriptorCount = 1;
    instanceBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    instanceBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding bindings[] = { uboBinding, samplerBinding, instanceBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 3;
    layoutInfo.pBindings = bindings;

    if (vkCreateDescriptorSetLayout(m_context->getDevice(), &layoutInfo, nullptr,
            &m_descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create descriptor set layout" << std::endl;
        return false;
    }
    return true;
}

bool VulkanPipeline::createPipelineLayout() {
    // Push Constants 用于传递每实例的变换和颜色数据
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 20; // 4x4矩阵(16) + 颜色(4)

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(m_context->getDevice(), &layoutInfo, nullptr,
            &m_pipelineLayout) != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create pipeline layout" << std::endl;
        return false;
    }
    return true;
}

bool VulkanPipeline::createGraphicsPipeline(const CreateInfo& info) {
    // 加载着色器
    VkShaderModule vertModule = m_context->loadShaderModule(info.vertexShaderPath.c_str());
    VkShaderModule fragModule = m_context->loadShaderModule(info.fragmentShaderPath.c_str());

    if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) {
        std::cerr << "[MTR-Vulkan] Failed to load shader modules" << std::endl;
        return false;
    }

    VkPipelineShaderStageCreateInfo vertStage = {};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage = {};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

    // 解析顶点格式
    auto bindings = parseVertexBindings(info.vertexFormat);
    auto attributes = parseVertexAttributes(info.vertexFormat);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = info.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // 视口和裁剪（动态状态）
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 1920.0f;  // 将在运行时更新
    viewport.height = 1080.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { 1920, 1080 };

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // 动态状态 - 视口和裁剪区域在渲染时动态设置
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = info.cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    // 深度模板
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = info.depthTestEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = info.depthWriteEnabled ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = VK_FALSE;

    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    if (info.blendEnabled) {
        // Alpha混合: srcColor * srcAlpha + dstColor * (1 - srcAlpha)
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        colorBlendAttachment.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // 创建图形管线
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_context->getMainRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(m_context->getDevice(), VK_NULL_HANDLE, 1,
            &pipelineInfo, nullptr, &m_pipeline);

    // 着色器模块在管线创建后可以销毁
    vkDestroyShaderModule(m_context->getDevice(), vertModule, nullptr);
    vkDestroyShaderModule(m_context->getDevice(), fragModule, nullptr);

    if (result != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create graphics pipeline" << std::endl;
        return false;
    }

    return true;
}

void VulkanPipeline::bind(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

void VulkanPipeline::destroy() {
    VkDevice device = m_context->getDevice();
    if (device == VK_NULL_HANDLE) return;

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (m_descriptorSet != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, m_context->getDescriptorPool(), 1, &m_descriptorSet);
        m_descriptorSet = VK_NULL_HANDLE;
    }
}

/**
 * 解析顶点绑定描述
 * 顶点格式: "3f,3f,2f,4f" 表示 pos(3 float), normal(3 float), uv(2 float), color(4 float)
 * 所有属性交错在单个绑定中
 */
std::vector<VkVertexInputBindingDescription>
VulkanPipeline::parseVertexBindings(const std::string& format) {
    // 计算总的stride
    uint32_t stride = 0;
    std::istringstream ss(format);
    std::string token;

    while (std::getline(ss, token, ',')) {
        // 解析如 "3f" 这样的token
        int count = std::stoi(token.substr(0, token.size() - 1));
        stride += count * 4; // 每个float 4字节
    }

    VkVertexInputBindingDescription binding = {};
    binding.binding = 0;
    binding.stride = stride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return { binding };
}

/**
 * 解析顶点属性描述
 */
std::vector<VkVertexInputAttributeDescription>
VulkanPipeline::parseVertexAttributes(const std::string& format) {
    std::vector<VkVertexInputAttributeDescription> attributes;
    std::istringstream ss(format);
    std::string token;
    uint32_t location = 0;
    uint32_t offset = 0;

    while (std::getline(ss, token, ',')) {
        // 去除空格
        while (!token.empty() && token[0] == ' ') token.erase(0, 1);

        int count = std::stoi(token.substr(0, token.size() - 1));
        char type = token.back();

        VkFormat vkFormat;
        switch (count) {
            case 1: vkFormat = VK_FORMAT_R32_SFLOAT; break;
            case 2: vkFormat = VK_FORMAT_R32G32_SFLOAT; break;
            case 3: vkFormat = VK_FORMAT_R32G32B32_SFLOAT; break;
            case 4: vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
            default: vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        }

        VkVertexInputAttributeDescription attr = {};
        attr.location = location;
        attr.binding = 0;
        attr.format = vkFormat;
        attr.offset = offset;

        attributes.push_back(attr);

        offset += count * 4;
        location++;
    }

    return attributes;
}