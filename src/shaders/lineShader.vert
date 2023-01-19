#version 450

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct Constants {
  uint64_t indexBufferAddress;
  uint64_t vertexBufferAddress;
  uint64_t oriBufferAddress;
  uint64_t dirBufferAddress;
//  uint64_t hitBuffer;
  uint64_t triangleIndex;
//  uint64_t nRays;
//  bool recordOri;
//  bool recordDir;
//  bool recordHit;sa
};

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
    
    fragColor = vec3(0.5, 1, 0.5);
}