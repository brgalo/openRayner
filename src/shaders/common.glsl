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
  uint64_t tri2MeshIdxBuffer;
//  bool recordDir;
//  bool recordHit;
};

// x = nTriInMesh
// y = nTriWithCurrMesh 
struct TriangleToMeshIdx {
  vec4 data;
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

struct ComputeConsts {
    uint64_t inBuffer;
    uint64_t outBuffer;
    uint64_t nTriangleToMeshIdxBuffer;
    uint64_t nMeshes;
    uint64_t nTriangles;
    uint64_t launchSize;
};