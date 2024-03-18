#version 450
//#extension GL_KHR_vulkan_glsl : enable

// Vertex Attributes: properties that are specified per-vertex in the vertex buffer
//
// The layout(location = x) annotations assign indices to the inputs that we can later use to reference them. 
// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots. 
// That means that the index after it must be at least 2 higher:
//		layout(location = 0) in dvec3 inPosition;
//		layout(location = 2) in vec3 inColor;
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragmentColor;

void main() {
	gl_Position = vec4(inPosition, 0.0, 1.0);
	fragmentColor = inColor;
}