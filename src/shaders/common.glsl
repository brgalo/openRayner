#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct Constants {
  uint64_t indexBufferAddress;
  uint64_t vertexBufferAddress;
  uint64_t oriBufferAddress;
  uint64_t dirBufferAddress;
  uint64_t hitBufferAddress;
  uint64_t triangleIndex;
  uint64_t nTriangles;
//  bool recordOri;
//  bool recordDir;
//  bool recordHit;
};

struct RayPayload {
    vec2 uv;
    int hitIdx;
    float energy;
};

struct Vertex {
    vec3 position;
    vec3 color;
    vec3 normal;
    vec2 uv;
};