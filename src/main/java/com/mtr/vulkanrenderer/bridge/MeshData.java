package com.mtr.vulkanrenderer.bridge;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * 用于在Java和C++之间传输网格数据的类。
 * 支持交错顶点格式: Position(3f) + Normal(3f) + UV(2f) + Color(4f)
 */
public class MeshData {

    // 每个顶点的浮点数数量 (3 pos + 3 normal + 2 uv + 4 color = 12)
    public static final int FLOATS_PER_VERTEX = 12;
    public static final int BYTES_PER_VERTEX = FLOATS_PER_VERTEX * 4;

    private final List<float[]> vertices = new ArrayList<>();
    private final List<Integer> indices = new ArrayList<>();

    /**
     * 添加一个顶点
     */
    public void addVertex(float px, float py, float pz,
                          float nx, float ny, float nz,
                          float u, float v,
                          float r, float g, float b, float a) {
        vertices.add(new float[]{px, py, pz, nx, ny, nz, u, v, r, g, b, a});
    }

    /**
     * 添加一个三角形（3个索引）
     */
    public void addTriangle(int i0, int i1, int i2) {
        indices.add(i0);
        indices.add(i1);
        indices.add(i2);
    }

    /**
     * 添加一个四边形（自动拆分为两个三角形）
     */
    public void addQuad(int i0, int i1, int i2, int i3) {
        addTriangle(i0, i1, i2);
        addTriangle(i0, i2, i3);
    }

    /**
     * 获取交错的顶点数据
     */
    public float[] getVertexData() {
        float[] data = new float[vertices.size() * FLOATS_PER_VERTEX];
        for (int i = 0; i < vertices.size(); i++) {
            float[] v = vertices.get(i);
            System.arraycopy(v, 0, data, i * FLOATS_PER_VERTEX, FLOATS_PER_VERTEX);
        }
        return data;
    }

    /**
     * 获取索引数据
     */
    public int[] getIndexData() {
        int[] data = new int[indices.size()];
        for (int i = 0; i < indices.size(); i++) {
            data[i] = indices.get(i);
        }
        return data;
    }

    public int getVertexCount() {
        return vertices.size();
    }

    public int getIndexCount() {
        return indices.size();
    }

    public int getTriangleCount() {
        return indices.size() / 3;
    }

    public void clear() {
        vertices.clear();
        indices.clear();
    }

    /**
     * 从Minecraft的BufferBuilder提取数据
     * 用于拦截MTR的渲染调用并转换数据
     */
    public static MeshData fromMinecraftBuffer(FloatBuffer vertexBuffer, IntBuffer indexBuffer,
                                                int vertexStride) {
        MeshData mesh = new MeshData();

        // 解析Minecraft的顶点数据并转换为我们的格式
        vertexBuffer.rewind();
        while (vertexBuffer.hasRemaining()) {
            float px = vertexBuffer.get();
            float py = vertexBuffer.get();
            float pz = vertexBuffer.get();
            // 跳过Minecraft额外的数据，提取我们需要的
            float u = 0, v = 0;
            float r = 1, g = 1, b = 1, a = 1;
            float nx = 0, ny = 1, nz = 0;

            // 根据stride读取更多数据...
            for (int i = 3; i < vertexStride / 4; i++) {
                if (vertexBuffer.hasRemaining()) {
                    vertexBuffer.get(); // skip
                }
            }

            mesh.addVertex(px, py, pz, nx, ny, nz, u, v, r, g, b, a);
        }

        indexBuffer.rewind();
        while (indexBuffer.hasRemaining()) {
            int i0 = indexBuffer.get();
            int i1 = indexBuffer.get();
            int i2 = indexBuffer.get();
            mesh.addTriangle(i0, i1, i2);
        }

        return mesh;
    }
}