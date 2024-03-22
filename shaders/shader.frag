#version 450

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragmentColor;
layout(location = 1) in vec2 fragmentTexture;

layout(location = 0) out vec4 outputColor;

void main() {
	outputColor = texture(textureSampler, fragmentTexture);
	//outputColor = vec4(fragmentTexture, 0.0, 1.0);
}