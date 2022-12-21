#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in float brightness;

layout (location = 0) out vec3 fragColor;

layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionViewMatrix;
    vec3 directionToLight;
} ubo;

void main() {
    gl_Position = ubo.projectionViewMatrix * vec4(position, 1.0);
    
    fragColor = vec3(brightness, 1, brightness);
}