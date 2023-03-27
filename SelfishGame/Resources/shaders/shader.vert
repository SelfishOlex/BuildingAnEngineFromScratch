
#version 450

// Uniforms ////
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Outputs
layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord; // values will be smoothly interpolated
}
