package com.mtr.vulkanrenderer.bridge;

/**
 * 表示一个待执行的渲染命令。
 * 用于在Java侧收集渲染命令，然后批量提交给C++侧。
 */
public class RenderCommand {

    public enum Type {
        BIND_PIPELINE,
        BIND_VERTEX_BUFFER,
        BIND_INDEX_BUFFER,
        BIND_TEXTURE,
        SET_UNIFORMS,
        DRAW_INDEXED,
        DRAW_ARRAYS,
        DRAW_INDIRECT,
        PUSH_DEBUG_MARKER,
        POP_DEBUG_MARKER
    }

    public final Type type;
    public final long handle;       // 管线/缓冲区/纹理句柄
    public final int intParam1;     // indexCount / slot / etc.
    public final int intParam2;     // instanceCount
    public final float[] floatData; // MVP矩阵 / 颜色 / etc.
    public final String stringParam; // debug marker name

    private RenderCommand(Type type, long handle, int p1, int p2, float[] floats, String str) {
        this.type = type;
        this.handle = handle;
        this.intParam1 = p1;
        this.intParam2 = p2;
        this.floatData = floats;
        this.stringParam = str;
    }

    public static RenderCommand bindPipeline(long pipeline) {
        return new RenderCommand(Type.BIND_PIPELINE, pipeline, 0, 0, null, null);
    }

    public static RenderCommand bindVertexBuffer(long buffer) {
        return new RenderCommand(Type.BIND_VERTEX_BUFFER, buffer, 0, 0, null, null);
    }

    public static RenderCommand bindIndexBuffer(long buffer) {
        return new RenderCommand(Type.BIND_INDEX_BUFFER, buffer, 0, 0, null, null);
    }

    public static RenderCommand bindTexture(long texture, int slot) {
        return new RenderCommand(Type.BIND_TEXTURE, texture, slot, 0, null, null);
    }

    public static RenderCommand setUniforms(float[] mvp, float[] model, float[] lightDir, float ambient) {
        float[] combined = new float[16 + 16 + 3 + 1];
        System.arraycopy(mvp, 0, combined, 0, 16);
        System.arraycopy(model, 0, combined, 16, 16);
        System.arraycopy(lightDir, 0, combined, 32, 3);
        combined[35] = ambient;
        return new RenderCommand(Type.SET_UNIFORMS, 0, 0, 0, combined, null);
    }

    public static RenderCommand drawIndexed(int indexCount, int instanceCount) {
        return new RenderCommand(Type.DRAW_INDEXED, 0, indexCount, instanceCount, null, null);
    }

    public static RenderCommand drawArrays(int vertexCount, int instanceCount) {
        return new RenderCommand(Type.DRAW_ARRAYS, 0, vertexCount, instanceCount, null, null);
    }

    public static RenderCommand debugMarker(String name, float r, float g, float b) {
        return new RenderCommand(Type.PUSH_DEBUG_MARKER, 0, 0, 0, new float[]{r, g, b}, name);
    }

    /**
     * 执行此命令
     */
    public void execute(VulkanBridge bridge) {
        switch (type) {
            case BIND_PIPELINE:
                bridge.bindPipeline(handle);
                break;
            case BIND_VERTEX_BUFFER:
                bridge.bindVertexBuffer(handle);
                break;
            case BIND_INDEX_BUFFER:
                bridge.bindIndexBuffer(handle);
                break;
            case BIND_TEXTURE:
                bridge.bindTexture(handle, intParam1);
                break;
            case SET_UNIFORMS:
                float[] mvp = new float[16];
                float[] model = new float[16];
                float[] lightDir = new float[3];
                System.arraycopy(floatData, 0, mvp, 0, 16);
                System.arraycopy(floatData, 16, model, 0, 16);
                System.arraycopy(floatData, 32, lightDir, 0, 3);
                bridge.setUniforms(mvp, model, lightDir, floatData[35]);
                break;
            case DRAW_INDEXED:
                bridge.drawIndexed(intParam1, intParam2);
                break;
            case DRAW_ARRAYS:
                bridge.drawArrays(intParam1, intParam2);
                break;
            case PUSH_DEBUG_MARKER:
                bridge.debugMarker(stringParam, floatData[0], floatData[1], floatData[2]);
                break;
        }
    }
}