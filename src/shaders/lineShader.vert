#version 450

#include "common.glsl"

layout(buffer_reference, scalar) buffer VertPos{ vec4 v[];};
layout(buffer_reference, scalar) buffer Dirbuf{ vec4 d[];};


layout(push_constant) uniform _Constants { Constants consts;};

layout (location = 0) out vec3 fragColor;

layout (set = 0, binding = 0) uniform GlobalUbo {
    mat4 projectionViewMatrix;
    vec3 directionToLight;
} ubo;

void main() {
    VertPos vert = VertPos(consts.oriBufferAddress);
    Dirbuf dir = Dirbuf(consts.dirBufferAddress);

    gl_Position = ubo.projectionViewMatrix * vec4(vert.v[gl_VertexIndex/2].xyz, 1.0);
    if( (gl_VertexIndex % 2) == 0) {
        gl_Position =  ubo.projectionViewMatrix * vec4(dir.d[gl_VertexIndex/2].xyz,1.0);
    }
    if(bool(vert.v[gl_VertexIndex/2].q) == true) {
      fragColor = vec3(0.5, 1, 0.5);
    } else {
      fragColor = vec3(1.0, 1, 0.0);
    }

}