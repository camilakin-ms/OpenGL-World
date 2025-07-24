
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>  // GLM is an optimized math library with syntax similar to OpenGL Shading Language
#include <glm/gtc/matrix_transform.hpp> // include this to create transformation matrices
#include <glm/common.hpp>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>

std::string loadShader(const char*);
int compileAndLinkShaders(const char* , const char*);
void setProjectionMatrix(int, glm::mat4);
void setWorldMatrix(int, glm::mat4);
void setViewMatrix(int, glm::mat4);
int createTexturedTerrainVAO();

// Constants for control point and terrain resolution
const int controlSize = 20;
const int fineSize = 200;

// 2D arrays for control points and resulting height map
float controlPoints[controlSize][controlSize];
float heightMap[fineSize][fineSize];

// creating VAO for terrain
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};


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

float getHeightAt(float worldX, float worldZ) {

    // convert back from world coordinates to height map coordinates
    float offset = fineSize / 2.0f;

    float x = worldX + offset;
    float z = -worldZ + offset;

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

    return 0.0f; // outside bounds
}


// Renders the heightMap as a textured mesh using triangle strips
void drawTerrain(GLuint shaderProgram, int terrainVAO, GLuint texture) {
    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(terrainVAO);

    for (int z = 0; z < fineSize - 1; ++z) {
        int verticesPerStrip = fineSize * 2;
        glDrawArrays(GL_TRIANGLE_STRIP, z * verticesPerStrip, verticesPerStrip);
    }

    glBindVertexArray(0);
}

GLuint loadTexture(const char* path) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Error::Texture could not load texture file: " << path << std::endl;
        return 0;
    }

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    if (textureId == 0) {
        std::cerr << "Error::Failed to generate texture ID\n";
        stbi_image_free(data);
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);

    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // Set texture parameters (wrap & filter)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return textureId;
}


// Main entry point
int main() {
    // generate terrain
    srand(static_cast<unsigned int>(time(0)));
    generateControlPoints();
    generateHeightMap();

    // initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // set up window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Sand Dunes", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;

    // initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    // load textures
    GLuint sandTexture = loadTexture("sand/Ground080_1K-PNG_Color.png");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.95f, 0.87f, 0.72f, 1.0f); // background sky tint

    // compile and link shaders
    const std::string vertexShaderSource = loadShader("shaders/vertexShader.glsl");
    const std::string fragmentShaderSource = loadShader("shaders/fragmentShader.glsl");
    const std::string textureVertexShaderSource = loadShader("shaders/texturedVertexShader.glsl");
    const std::string textureFragmentShaderSource = loadShader("shaders/texturedFragmentShader.glsl");
    const char* vShaderCode = vertexShaderSource.c_str();
    const char* fShaderCode = fragmentShaderSource.c_str();
    const char* tvShaderCode = textureVertexShaderSource.c_str();
    const char* tfShaderCode = textureFragmentShaderSource.c_str();

    int colorShaderProgram = compileAndLinkShaders(vShaderCode, fShaderCode);
    int textureShaderProgram = compileAndLinkShaders(tvShaderCode, tfShaderCode);

    // lookAt() parameters for view transform
    glm::vec3 cameraPosition(0.6f,15.0f,0.0f);
    glm::vec3 cameraLookAt(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

    // camera parameters for calculation
    float cameraSpeed = 4.0f;
    float cameraFastSpeed = 2.0f * cameraSpeed;
    float cameraHorizontalAngle = 90.0f;
    float cameraVerticalAngle = 0.0f;
    float lastFrameTime = glfwGetTime();
    double lastMousePosX, lastMousePosY;
    glfwGetCursorPos(window, &lastMousePosX, &lastMousePosY);

    // set up default view matrix
    glm::mat4 viewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp);
    setViewMatrix(colorShaderProgram, viewMatrix );
    setViewMatrix(textureShaderProgram, viewMatrix );

    // set up default projection matrix
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), 800.0f/600.0f, 0.01f, 1000.0f);
    setProjectionMatrix(colorShaderProgram, projectionMatrix);
    setProjectionMatrix(textureShaderProgram, projectionMatrix);

    // create terrain VAO
    int terrainVAO = createTexturedTerrainVAO();
    glBindVertexArray(terrainVAO);

    // Game loop
    while (!glfwWindowShouldClose(window)) {

        float dt = glfwGetTime() - lastFrameTime;
        lastFrameTime += dt;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(textureShaderProgram);
        int samplerLocation = glGetUniformLocation(textureShaderProgram, "textureSampler");
        if (samplerLocation == -1) {
            std::cerr << "Uniform 'textureSampler' not found in shader!\n";
        }
        glUniform1i(samplerLocation, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sandTexture);


        setWorldMatrix(textureShaderProgram, glm::mat4(1.0f));

        // generate and bind terrain VAO & VBO
        drawTerrain(textureShaderProgram, terrainVAO, sandTexture);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // -------------------- input handler

        // ESC for closing window
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { // close window with ESC
            glfwSetWindowShouldClose(window, true);
        }

        // space bar for starting

        // SHIFT for fast speed
        bool fastCam = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        float currentCameraSpeed = (fastCam) ? cameraFastSpeed : cameraSpeed;

        // mouse for turning left and right
        double currentMouseX, currentMouseY;
        glfwGetCursorPos(window, &currentMouseX, &currentMouseY);
        double dx = currentMouseX - lastMousePosX;
        double dy = currentMouseY - lastMousePosY;
        lastMousePosX = currentMouseX;
        lastMousePosY = currentMouseY;

        const float cameraAngularSpeed = 60.0f;
        cameraHorizontalAngle -= dx * cameraAngularSpeed * dt;
        cameraVerticalAngle   -= dy * cameraAngularSpeed * dt;

        // clamp vertical angle to [-30, 85] degrees
        cameraVerticalAngle = std::max(-30.0f, std::min(85.0f, cameraVerticalAngle));
        if (cameraHorizontalAngle > 360)
            cameraHorizontalAngle -= 360;
        else if (cameraHorizontalAngle < -360)
            cameraHorizontalAngle += 360;

        float theta = glm::radians(cameraHorizontalAngle);
        float phi = glm::radians(cameraVerticalAngle);

        // dx and dy affect the lookAt of the camera (so user can look up and down)
        // but only dx affects position (cannot move in y, delimited by terrain)
        cameraLookAt = glm::vec3(cosf(phi)*cosf(theta), sinf(phi), -cosf(phi)*sinf(theta));
        glm::vec3 movementDirection = glm::vec3(cosf(theta), 0.0f, -sinf(theta));

        // update x and z with constant movement forward
        // W for moving forward
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraPosition += movementDirection * currentCameraSpeed * dt;
        }
        // cameraPosition += movementDirection * currentCameraSpeed * dt;

        // get y from new x and z position, to stay on terrain
        cameraPosition.y = getHeightAt(cameraPosition.x, cameraPosition.z) +2.0f;

        viewMatrix = glm::lookAt(cameraPosition, cameraPosition + cameraLookAt, cameraUp );
        setViewMatrix(textureShaderProgram, viewMatrix );
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


std::string loadShader(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file " << filepath << "\n";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int compileAndLinkShaders(const char* vertexShaderSource, const char* fragmentShaderSource) {

    // create and compile vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // check for vertex shader compilation success
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Error compiling vertex shader\n" << infoLog << std::endl;
    }

    // create and compile fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // check for fragment shader compilation success
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Error compiling fragment shader\n" << infoLog << std::endl;
    }

    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Error linking shader program\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void setProjectionMatrix(int shaderProgram, glm::mat4 projectionMatrix){
    glUseProgram(shaderProgram);
    GLuint projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

void setViewMatrix(int shaderProgram, glm::mat4 viewMatrix){
    glUseProgram(shaderProgram);
    GLuint viewMatrixLocation = glGetUniformLocation(shaderProgram, "viewMatrix");
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
}

void setWorldMatrix(int shaderProgram, glm::mat4 worldMatrix){
    glUseProgram(shaderProgram);
    GLuint worldMatrixLocation = glGetUniformLocation(shaderProgram, "worldMatrix");
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &worldMatrix[0][0]);
}

int createTexturedTerrainVAO() {
    GLuint terrainVAO, terrainVBO;
    std::vector<Vertex> terrainVertices;

    // create vertex array for terrain, centered around (0,0,0)
    float offset = fineSize / 2.0f;

    for (int z = 0; z < fineSize - 1; ++z) {
        for (int x = 0; x < fineSize; ++x) {
            float u = x / (float)(fineSize - 1) * 10.0f;
            float v1 = z / (float)(fineSize - 1) * 10.0f;
            float v2 = (z + 1) / (float)(fineSize - 1) * 10.0f;

            // Flip z and offset both x and z to center terrain around origin
            terrainVertices.push_back({
                glm::vec3(x - offset, heightMap[z][x], -(z - offset)),
                glm::vec2(u, v1)
            });

            terrainVertices.push_back({
                glm::vec3(x - offset, heightMap[z + 1][x], -(z + 1 - offset)),
                glm::vec2(u, v2)
            });
        }
    }

    // create VAO
    glGenVertexArrays(1, &terrainVAO);
    glBindVertexArray(terrainVAO);

    // create and bind VBO
    glGenBuffers(1, &terrainVBO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(Vertex), terrainVertices.data(), GL_STATIC_DRAW);

    // vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return terrainVAO;
}


