#version 330 core

    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aUV;

    uniform mat4 worldMatrix;
    uniform mat4 viewMatrix = mat4(1.0);
    uniform mat4 projectionMatrix = mat4(1.0);

    out vec2 vertexUV;

    void main(){
        vertexUV = aUV;
        mat4 modelViewProjection = projectionMatrix * viewMatrix * worldMatrix;
        gl_Position = modelViewProjection * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    }
