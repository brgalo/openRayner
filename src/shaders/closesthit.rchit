#version 460
#extension GL_EXT_ray_tracing : enable


struct RayPayload {
    bool hit;
    float energy;
};

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.hit = true;
}
