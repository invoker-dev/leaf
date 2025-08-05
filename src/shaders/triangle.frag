#version 450

// layout(push_constant) uniform PushData {
//     layout(offset = 80) vec4 color;
// } pushData;

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) flat in int textureIndex;

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform sampler2D textures[16];

void main() {

    outFragColor = texture(textures[textureIndex], inUV);
}
