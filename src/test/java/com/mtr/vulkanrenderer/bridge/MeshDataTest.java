package com.mtr.vulkanrenderer.bridge;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class MeshDataTest {

    @Test
    void testAddVertex() {
        MeshData mesh = new MeshData();

        mesh.addVertex(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1);
        mesh.addVertex(1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1);
        mesh.addVertex(0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1);

        assertEquals(3, mesh.getVertexCount());
    }

    @Test
    void testAddTriangle() {
        MeshData mesh = new MeshData();

        mesh.addVertex(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1);
        mesh.addVertex(1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1);
        mesh.addVertex(0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1);

        mesh.addTriangle(0, 1, 2);

        assertEquals(3, mesh.getIndexCount());
        assertEquals(1, mesh.getTriangleCount());
    }

    @Test
    void testAddQuad() {
        MeshData mesh = new MeshData();

        mesh.addVertex(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1);
        mesh.addVertex(1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1);
        mesh.addVertex(1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1);
        mesh.addVertex(0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1);

        mesh.addQuad(0, 1, 2, 3);

        assertEquals(6, mesh.getIndexCount());
        assertEquals(2, mesh.getTriangleCount());
    }

    @Test
    void testGetVertexData() {
        MeshData mesh = new MeshData();

        mesh.addVertex(1, 2, 3, 0, 1, 0, 0.5f, 0.5f, 1, 0, 0, 1);

        float[] data = mesh.getVertexData();

        assertEquals(12, data.length);
        assertEquals(1.0f, data[0]);
        assertEquals(2.0f, data[1]);
        assertEquals(3.0f, data[2]);
        assertEquals(0.0f, data[3]);
        assertEquals(1.0f, data[4]);
        assertEquals(0.0f, data[5]);
        assertEquals(0.5f, data[6]);
        assertEquals(0.5f, data[7]);
        assertEquals(1.0f, data[8]);
        assertEquals(0.0f, data[9]);
        assertEquals(0.0f, data[10]);
        assertEquals(1.0f, data[11]);
    }

    @Test
    void testGetIndexData() {
        MeshData mesh = new MeshData();

        mesh.addVertex(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1);
        mesh.addVertex(1, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1);
        mesh.addVertex(0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1);

        mesh.addTriangle(0, 1, 2);

        int[] data = mesh.getIndexData();

        assertEquals(3, data.length);
        assertEquals(0, data[0]);
        assertEquals(1, data[1]);
        assertEquals(2, data[2]);
    }

    @Test
    void testClear() {
        MeshData mesh = new MeshData();

        mesh.addVertex(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1);
        mesh.addTriangle(0, 0, 0);

        mesh.clear();

        assertEquals(0, mesh.getVertexCount());
        assertEquals(0, mesh.getIndexCount());
    }
}