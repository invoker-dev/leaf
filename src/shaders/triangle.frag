#version 450

layout(push_constant) uniform PushData {
  layout(offset = 80) vec4 color;
} pushData;

// layout (location = 0) in vec3 inColor;
layout (location = 0) out vec4 outFragColor;

void main() {

  outFragColor = pushData.color;

  // outFragColor = vec4(inColor,1.0f);
}
