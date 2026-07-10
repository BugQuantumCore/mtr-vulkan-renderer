#pragma once

#include "vulkan_context.h"
#include <string>
#include <vector>

/**
 * Vulkan着色器管理
 * 负责加载SPIR-V着色器并创建VkShaderModule
 */
class VulkanShader {
public:
    /**
     * 从文件加载SPIR-V字节码
     */
    static bool loadSPIRV(const std::string& path, std::vector<char>& outCode);

    /**
     * 从SPIR-V字节码创建VkShaderModule
     */
    static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

    /**
     * 加载并创建着色器模块的便捷方法
     */
    static VkShaderModule loadAndCreate(VkDevice device, const std::string& path);
};