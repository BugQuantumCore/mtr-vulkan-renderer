package com.mtr.vulkanrenderer.vulkan;

import com.mtr.vulkanrenderer.bridge.VulkanBridge;

/**
 * 管理Vulkan缓冲区（顶点、索引、Uniform）
 */
public class VulkanBuffer {

    public enum Type {
        VERTEX, INDEX, UNIFORM, STORAGE
    }

    private final long handle;
    private final Type type;
    private final int sizeBytes;
    private final VulkanBridge bridge;
    private int usedBytes = 0;

    public VulkanBuffer(Type type, int sizeBytes, VulkanBridge bridge) {
        this.type = type;
        this.sizeBytes = sizeBytes;
        this.bridge = bridge;

        switch (type) {
            case VERTEX:
                this.handle = bridge.createVertexBuffer(sizeBytes);
                break;
            case INDEX:
                this.handle = bridge.createIndexBuffer(sizeBytes);
                break;
            case UNIFORM:
            case STORAGE:
                this.handle = bridge.createUniformBuffer(sizeBytes);
                break;
            default:
                this.handle = 0;
        }
    }

    public void upload(float[] data, int offsetBytes) {
        bridge.uploadVertexData(handle, data, offsetBytes);
        usedBytes = Math.max(usedBytes, offsetBytes + data.length * 4);
    }

    public void upload(int[] data, int offsetBytes) {
        bridge.uploadIndexData(handle, data, offsetBytes);
        usedBytes = Math.max(usedBytes, offsetBytes + data.length * 4);
    }

    public void bind() {
        switch (type) {
            case VERTEX:
                bridge.bindVertexBuffer(handle);
                break;
            case INDEX:
                bridge.bindIndexBuffer(handle);
                break;
        }
    }

    public void destroy() {
        bridge.destroyBuffer(handle);
    }

    public long getHandle() { return handle; }
    public Type getType() { return type; }
    public int getSizeBytes() { return sizeBytes; }
    public int getUsedBytes() { return usedBytes; }
    public float getUsagePercent() { return (float) usedBytes / sizeBytes * 100; }
}