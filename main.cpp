// main.cpp

// Include implementation of stb_image before OpenGL headers
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// OpenGL and GLFW headers
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <cmath>

// Constants for control point and terrain resolution
const int controlSize = 20;
const int fineSize = 200;

// 2D arrays for control points and resulting height map
float controlPoints[controlSize][controlSize];
float heightMap[fineSize][fineSize];

// Camera position and control variables
float camX = 100.0f, camZ = 100.0f, camY = 0.0f;
float camSpeed = 0.5f;
float yaw = 0.0f; // Angle for direction camera is facing

// Texture ID for sand
GLuint sandTexture;

// Catmull-Rom spline interpolation to smooth terrain between points
float catmullRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * (
        2.0f * p1 +
        (-p0 + p2) * t +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
    );
}

// Fill control points with random values for generating dune heights
void generateControlPoints() {
    for (int z = 0; z < controlSize; ++z) {
        for (int x = 0; x < controlSize; ++x) {
            controlPoints[z][x] = static_cast<float>((rand() % 10) + 3);
        }
    }
}

// Create fine height map from control points using Catmull-Rom splines
void generateHeightMap() {
    for (int z = 0; z < fineSize; ++z) {
        float zRatio = (float)z / (fineSize - 1) * (controlSize - 3);
        int zIndex = (int)zRatio;
        float tz = zRatio - zIndex;

        for (int x = 0; x < fineSize; ++x) {
            float xRatio = (float)x / (fineSize - 1) * (controlSize - 3);
            int xIndex = (int)xRatio;
            float tx = xRatio - xIndex;

            float col[4];
            for (int i = 0; i < 4; ++i) {
                float p0 = controlPoints[zIndex + i][xIndex];
                float p1 = controlPoints[zIndex + i][xIndex + 1];
                float p2 = controlPoints[zIndex + i][xIndex + 2];
                float p3 = controlPoints[zIndex + i][xIndex + 3];
                col[i] = catmullRom(p0, p1, p2, p3, tx);
            }

            float height = catmullRom(col[0], col[1], col[2], col[3], tz);
            heightMap[z][x] = height;
        }
    }
}

// Interpolates terrain height at floating point world coordinates
float getHeightAt(float x, float z) {
    int ix = static_cast<int>(x);
    int iz = static_cast<int>(z);
    if (ix >= 0 && ix < fineSize - 1 && iz >= 0 && iz < fineSize - 1) {
        float fx = x - ix;
        float fz = z - iz;

        float h00 = heightMap[iz][ix];
        float h10 = heightMap[iz][ix + 1];
        float h01 = heightMap[iz + 1][ix];
        float h11 = heightMap[iz + 1][ix + 1];

        float hx0 = h00 + fx * (h10 - h00);
        float hx1 = h01 + fx * (h11 - h01);

        return hx0 + fz * (hx1 - hx0);
    }
    return 0.0f;
}

// Renders the heightMap as a textured mesh using triangle strips
void drawTerrain() {
    glBindTexture(GL_TEXTURE_2D, sandTexture);
    for (int z = 0; z < fineSize - 1; ++z) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x = 0; x < fineSize; ++x) {
            float u = x / (float)(fineSize - 1) * 10.0f;
            float v1 = z / (float)(fineSize - 1) * 10.0f;
            float v2 = (z + 1) / (float)(fineSize - 1) * 10.0f;

            glTexCoord2f(u, v1);
            glVertex3f((float)x, heightMap[z][x], (float)z);

            glTexCoord2f(u, v2);
            glVertex3f((float)x, heightMap[z + 1][x], (float)(z + 1));
        }
        glEnd();
    }
}

// Sets up OpenGL projection and texture options
void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glClearColor(0.95f, 0.87f, 0.72f, 1.0f); // background sky tint

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float fov = 60.0f;
    float aspect = 800.0f / 600.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float top = tan(fov * 0.5f * 3.14159f / 180.0f) * nearPlane;
    float bottom = -top;
    float right = top * aspect;
    float left = -right;
    glFrustum(left, right, bottom, top, nearPlane, farPlane);
}

// Loads an image file and sets it as an OpenGL texture
bool loadTexture(const char* path) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        glGenTextures(1, &sandTexture);
        glBindTexture(GL_TEXTURE_2D, sandTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, nrChannels == 4 ? GL_RGBA : GL_RGB, width, height, 0, nrChannels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        return true;
    }
    return false;
}

// Main entry point
int main() {
    srand(static_cast<unsigned int>(time(0)));
    generateControlPoints();
    generateHeightMap();

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Sand Dunes", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    if (!loadTexture("sand/Ground080_1K-PNG_Color.png")) {
        std::cerr << "Failed to load sand texture\n";
        return -1;
    }

    initOpenGL();

    // Game loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // WASD for movement, QE for camera rotation
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camX += sinf(yaw) * camSpeed;
            camZ -= cosf(yaw) * camSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camX -= sinf(yaw) * camSpeed;
            camZ += cosf(yaw) * camSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            camX -= cosf(yaw) * camSpeed;
            camZ -= sinf(yaw) * camSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            camX += cosf(yaw) * camSpeed;
            camZ += sinf(yaw) * camSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            yaw -= 0.03f;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            yaw += 0.03f;

        // Adjust camera height to stay on terrain
        camY = getHeightAt(camX, camZ) + 2.0f;

        // Compute look direction from yaw
        float lookX = camX + sinf(yaw);
        float lookZ = camZ - cosf(yaw);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(camX, camY, camZ, lookX, camY, lookZ, 0.0f, 1.0f, 0.0f);

        drawTerrain();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
