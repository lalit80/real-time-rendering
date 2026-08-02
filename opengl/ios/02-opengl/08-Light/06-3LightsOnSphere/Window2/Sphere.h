#pragma once
#include <math.h>

static unsigned int gNumVertices = 0;
static unsigned int gNumElements = 0;

static void getSphereVertexData(
    float* vertices,
    float* normals,
    float* texcoords,
    unsigned short* elements)
{
    const int stacks = 30;
    const int slices = 30;
    int v = 0, n = 0, t = 0, e = 0;

    for (int i = 0; i <= stacks; i++) {
        float V = (float)i / stacks;
        float phi = V * M_PI;

        for (int j = 0; j <= slices; j++) {
            float U = (float)j / slices;
            float theta = U * (M_PI * 2);

            float x = cosf(theta) * sinf(phi);
            float y = cosf(phi);
            float z = sinf(theta) * sinf(phi);

            vertices[v++] = x;
            vertices[v++] = y;
            vertices[v++] = z;

            normals[n++] = x;
            normals[n++] = y;
            normals[n++] = z;

            texcoords[t++] = U;
            texcoords[t++] = V;
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            unsigned short first = (i * (slices + 1)) + j;
            unsigned short second = first + slices + 1;

            elements[e++] = first;
            elements[e++] = second;
            elements[e++] = first + 1;

            elements[e++] = second;
            elements[e++] = second + 1;
            elements[e++] = first + 1;
        }
    }

    gNumVertices = (stacks + 1) * (slices + 1);
    gNumElements = e;
}

static unsigned int getNumberOfSphereVertices(void) {
    return gNumVertices;
}

static unsigned int getNumberOfSphereElements(void) {
    return gNumElements;
}
