package com.mtr.vulkanrenderer.vulkan;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

/**
 * 管理Vulkan着色器（SPIR-V格式）
 */
public class VulkanShaderManager {

    /**
     * 将GLSL着色器编译为SPIR-V（在开发时使用）
     * 实际发布时，SPIR-V文件预编译并打包在模组JAR中
     */
    public static byte[] loadSpirV(String resourcePath) {
        try (InputStream is = VulkanShaderManager.class.getResourceAsStream(resourcePath)) {
            if (is == null) {
                MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Shader not found: {}", resourcePath);
                return null;
            }
            return is.readAllBytes();
        } catch (IOException e) {
            MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to load shader: {}", resourcePath, e);
            return null;
        }
    }

    /**
     * 提取着色器文件到临时目录供原生库使用
     */
    public static Path extractShader(String resourcePath, Path tempDir) {
        try (InputStream is = VulkanShaderManager.class.getResourceAsStream(resourcePath)) {
            if (is == null) return null;
            Path target = tempDir.resolve(resourcePath.substring(resourcePath.lastIndexOf('/') + 1));
            Files.copy(is, target, StandardCopyOption.REPLACE_EXISTING);
            return target;
        } catch (IOException e) {
            MTRVulkanRenderer.LOGGER.error("[MTR-Vulkan] Failed to extract shader", e);
            return null;
        }
    }
}