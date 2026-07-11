#include "vulkan_shader.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <bits/stdc++.h>

bool VulkanShader::loadSPIRV(const std::string& path, std::vector<char>& outCode) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[MTR-Vulkan] Failed to open shader file: " << path << std::endl;
        return false;
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    if (fileSize == 0 || fileSize % 4 != 0) {
        std::cerr << "[MTR-Vulkan] Invalid SPIR-V file size: " << fileSize << std::endl;
        return false;
    }

    outCode.resize(fileSize);
    file.seekg(0);
    file.read(outCode.data(), fileSize);
    file.close();

    // 验证SPIR-V魔数
    if (fileSize >= 4) {
        uint32_t magic = *reinterpret_cast<const uint32_t*>(outCode.data());
        if (magic != 0x07230203) {
            std::cerr << "[MTR-Vulkan] Invalid SPIR-V magic number in: " << path << std::endl;
            return false;
        }
    }

    return true;
}

VkShaderModule VulkanShader::createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "[MTR-Vulkan] Failed to create shader module" << std::endl;
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

VkShaderModule VulkanShader::loadAndCreate(VkDevice device, const std::string& path) {
    std::vector<char> code;
    if (!loadSPIRV(path, code)) {
        return VK_NULL_HANDLE;
    }
    return createShaderModule(device, code);
}