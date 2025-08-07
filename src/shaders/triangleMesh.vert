#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outUV;
layout(location = 2) flat out int textureIndex;

layout(location = 3) out vec3 fragNormal;
layout(location = 4) out vec3 fragPos;
layout(location = 5) flat out int lit;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

layout(binding = 0) uniform GPUSceneData
{
    mat4 view;
    mat4 projection;
    vec3 position;
} camera;

layout(push_constant) uniform constants
{
    mat4 model;
    vec4 color;
    float blendFactor;
    VertexBuffer vertexBuffer;
    int textureIndex;
    int lit;

} pushConstants;

void main()
{
    Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];

    vec4 worldPos = pushConstants.model * vec4(v.position, 1.0);
    gl_Position = camera.projection * camera.view * worldPos;

    outColor = mix(v.color, pushConstants.color, pushConstants.blendFactor);
    outUV = vec2(v.uv_x, v.uv_y);
    textureIndex = pushConstants.textureIndex;

    fragNormal = mat3(transpose(inverse(pushConstants.model))) * v.normal;
    fragPos = vec3(worldPos);

    lit = pushConstants.lit;
}
