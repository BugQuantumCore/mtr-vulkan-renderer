package com.mtr.vulkanrenderer.bridge;

import com.mtr.vulkanrenderer.MTRVulkanRenderer;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;

/**
 * Java和C++ Vulkan渲染器之间的核心桥接类。
 * 所有native方法都在jni_bridge.cpp中实现。
 */
public class VulkanBridge {

    // ==================== 初始化/清理 ====================

    /**
     * 初始化Vulkan渲染器。利用VulkanMod已经创建的VkInstance和VkDevice。
     * @return 是否初始化成功
     */
    public native boolean initializeVulkan();

    /**
     * 清理所有Vulkan资源
     */
    public native void cleanup();

    /**
     * 在窗口大小改变时调用
     */
    public native void onWindowResize(int width, int height);

    // ==================== 帧管理 ====================

    /**
     * 开始新的一帧渲染
     * @return 当前帧索引
     */
    public native int beginFrame();

    /**
     * 结束当前帧并提交
     */
    public native void endFrame();

    /**
     * 等待GPU完成当前帧
     */
    public native void waitForFrame();

    // ==================== 管线管理 ====================

    /**
     * 创建渲染管线
     * @param vertexShaderPath 顶点着色器SPIR-V文件路径
     * @param fragmentShaderPath 片段着色器SPIR-V文件路径
     * @param vertexFormat 顶点格式描述 (位置:3f, 法线:3f, UV:2f, 颜色:4f等)
     * @param topology 图元拓扑 (0=三角形列表, 1=线条列表等)
     * @param blendEnabled 是否启用混合
     * @param depthTestEnabled 是否启用深度测试
     * @return 管线句柄
     */
    public native long createPipeline(String vertexShaderPath, String fragmentShaderPath,
                                       String vertexFormat, int topology,
                                       boolean blendEnabled, boolean depthTestEnabled);

    /**
     * 销毁管线
     */
    public native void destroyPipeline(long pipelineHandle);

    // ==================== 缓冲区管理 ====================

    /**
     * 创建顶点缓冲区
     * @param sizeBytes 缓冲区大小（字节）
     * @return 缓冲区句柄
     */
    public native long createVertexBuffer(int sizeBytes);

    /**
     * 创建索引缓冲区
     */
    public native long createIndexBuffer(int sizeBytes);

    /**
     * 创建Uniform缓冲区
     */
    public native long createUniformBuffer(int sizeBytes);

    /**
     * 上传顶点数据到GPU
     * @param bufferHandle 缓冲区句柄
     * @param data 顶点数据 (交错格式: pos.x, pos.y, pos.z, nx, ny, nz, u, v, ...)
     * @param offset 偏移量（字节）
     */
    public native void uploadVertexData(long bufferHandle, float[] data, int offset);

    /**
     * 上传索引数据到GPU
     */
    public native void uploadIndexData(long bufferHandle, int[] data, int offset);

    /**
     * 上传Uniform数据
     * @param data Uniform数据（字节缓冲）
     */
    public native void uploadUniformData(long bufferHandle, byte[] data, int offset);

    /**
     * 销毁缓冲区
     */
    public native void destroyBuffer(long bufferHandle);

    // ==================== 纹理管理 ====================

    /**
     * 从Minecraft纹理创建Vulkan纹理
     * @param pixelData RGBA像素数据
     * @param width 宽度
     * @param height 高度
     * @param mipLevels MIP等级
     * @return 纹理句柄
     */
    public native long createTexture(byte[] pixelData, int width, int height, int mipLevels);

    /**
     * 从Minecraft的NativeImage/GL纹理ID创建Vulkan纹理
     * 这利用了VulkanMod提供的OpenGL互操作功能
     */
    public native long createTextureFromGL(int glTextureId, int width, int height);

    /**
     * 更新纹理数据
     */
    public native void updateTexture(long textureHandle, byte[] pixelData, int x, int y, int width, int height);

    /**
     * 销毁纹理
     */
    public native void destroyTexture(long textureHandle);

    // ==================== 渲染命令 ====================

    /**
     * 绑定管线用于后续绘制
     */
    public native void bindPipeline(long pipelineHandle);

    /**
     * 绑定顶点缓冲区
     */
    public native void bindVertexBuffer(long bufferHandle);

    /**
     * 绑定索引缓冲区
     */
    public native void bindIndexBuffer(long bufferHandle);

    /**
     * 绑定纹理到指定槽位
     */
    public native void bindTexture(long textureHandle, int slot);

    /**
     * 设置Uniform数据（MVP矩阵等）
     * @param mvpMatrix 4x4 MVP矩阵 (列主序)
     * @param modelMatrix 4x4 模型矩阵
     * @param lightDirection 光源方向 (x, y, z)
     * @param ambientLight 环境光强度
     */
    public native void setUniforms(float[] mvpMatrix, float[] modelMatrix,
                                    float[] lightDirection, float ambientLight);

    /**
     * 绘制索引几何体
     * @param indexCount 索引数量
     * @param instanceCount 实例数量
     */
    public native void drawIndexed(int indexCount, int instanceCount);

    /**
     * 绘制非索引几何体
     */
    public native void drawArrays(int vertexCount, int instanceCount);

    /**
     * 批量绘制（GPU驱动渲染）
     * @param commands 绘制命令数组 [indexCount, instanceCount, firstIndex, vertexOffset, firstInstance, ...]
     */
    public native void drawIndirect(int[] commands);

    // ==================== 网格管理 ====================

    /**
     * 注册一个静态网格（如列车模型部件）
     * @param meshId 唯一标识
     * @param vertices 顶点数据
     * @param indices 索引数据
     * @param textureHandle 纹理句柄
     */
    public native void registerMesh(String meshId, float[] vertices, int[] indices, long textureHandle);

    /**
     * 渲染已注册的网格
     * @param meshId 网格标识
     * @param mvpMatrix MVP矩阵
     * @param color 颜色叠加 (r, g, b, a)
     */
    public native void renderMesh(String meshId, float[] mvpMatrix, float[] color);

    /**
     * 注销网格
     */
    public native void unregisterMesh(String meshId);

    // ==================== 调试 ====================

    /**
     * 获取GPU信息
     */
    public native String getGPUInfo();

    /**
     * 获取渲染统计
     * @return [drawCalls, triangles, frameTimeMs, gpuMemoryUsedMB]
     */
    public native float[] getRenderStats();

    /**
     * 插入调试标记
     */
    public native void debugMarker(String name, float r, float g, float b);
}