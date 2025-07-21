#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

layout(push_constant) uniform constants
{
    mat4 model;
    vec4 color;
    VertexBuffer vertexBuffer;
} pushConstants;

void main()
{
    //load vertex data from device adress
    Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];

    //output data
    gl_Position = camera.projection * camera.view * pushConstants.model *
            vec4(v.position, 1.f);
    outColor = pushConstants.color.xyz;
    outUV.x = v.uv_x;
    outUV.y = v.uv_y;
}
