package com.mtr.vulkanrenderer.vulkan;

import com.mtr.vulkanrenderer.bridge.VulkanBridge;

/**
 * 封装一个Vulkan渲染管线及其关联资源
 */
public class VulkanPipeline {

    private final long handle;
    private final String name;
    private final VulkanBridge bridge;
    private long vertexBuffer;
    private long indexBuffer;
    private long uniformBuffer;

    public VulkanPipeline(long handle, String name, VulkanBridge bridge) {
        this.handle = handle;
        this.name = name;
        this.bridge = bridge;
    }

    public void createBuffers(int vertexSizeBytes, int indexSizeBytes, int uniformSizeBytes) {
        if (vertexSizeBytes > 0) {
            vertexBuffer = bridge.createVertexBuffer(vertexSizeBytes);
        }
        if (indexSizeBytes > 0) {
            indexBuffer = bridge.createIndexBuffer(indexSizeBytes);
        }
        if (uniformSizeBytes > 0) {
            uniformBuffer = bridge.createUniformBuffer(uniformSizeBytes);
        }
    }

    public void bind() {
        bridge.bindPipeline(handle);
        if (vertexBuffer != 0) bridge.bindVertexBuffer(vertexBuffer);
        if (indexBuffer != 0) bridge.bindIndexBuffer(indexBuffer);
    }

    public void uploadVertices(float[] data, int offset) {
        bridge.uploadVertexData(vertexBuffer, data, offset);
    }

    public void uploadIndices(int[] data, int offset) {
        bridge.uploadIndexData(indexBuffer, data, offset);
    }

    public void uploadUniform(byte[] data, int offset) {
        bridge.uploadUniformData(uniformBuffer, data, offset);
    }

    public void drawIndexed(int indexCount, int instanceCount) {
        bridge.drawIndexed(indexCount, instanceCount);
    }

    public void destroy() {
        if (vertexBuffer != 0) bridge.destroyBuffer(vertexBuffer);
        if (indexBuffer != 0) bridge.destroyBuffer(indexBuffer);
        if (uniformBuffer != 0) bridge.destroyBuffer(uniformBuffer);
        bridge.destroyPipeline(handle);
    }

    public long getHandle() { return handle; }
    public String getName() { return name; }
    public long getVertexBuffer() { return vertexBuffer; }
    public long getIndexBuffer() { return indexBuffer; }
}