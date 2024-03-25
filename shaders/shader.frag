#version 450
#extension GL_KHR_vulkan_glsl : enable

// A combined image sampler descriptor is represented in GLSL by a sampler uniform.
// The sampler automatically takes care of the filtering and transformations in the background. 
layout(set = 0, binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentTexture;

layout(location = 0) out vec4 outputColor;

void main() {
	outputColor = texture(textureSampler, fragmentTexture);
	//outputColor = vec4(fragmentTexture, 0.0, 1.0);
}