#version 460
#extension GL_EXT_ray_tracing : enable

#include "common.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;


hitAttributeEXT vec2 baryCoord;

void main() {
    payload.uv = baryCoord;
    payload.hitIdx = gl_PrimitiveID;
    payload.energy = 1;
}
