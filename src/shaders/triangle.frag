#version 450

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) flat in int textureIndex;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 fragPos;
layout (location = 5) flat in int lit;


layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform sampler2D textures[16];

void main() {

    vec4 texColor = texture(textures[textureIndex], inUV);
    if(lit == 0){
        outFragColor = vec4(texColor.rgb, texColor.a);
    } else {
        vec3 lightColor = vec3(1,1,1);
        vec3 lightPos = vec3(0,0,0);
        vec3 norm = normalize(inNormal);
        vec3 lightDir = normalize(lightPos - fragPos);

        float diff = max(dot(norm, lightDir), 0.0);

        vec3 ambient = 0.01 * lightColor;
        vec3 diffuse = diff * lightColor;

        vec3 lighting = ambient + diffuse;

        outFragColor = vec4(texColor.rgb * lighting, texColor.a);
    }
}

