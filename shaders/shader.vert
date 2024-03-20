#version 450
//#extension GL_KHR_vulkan_glsl : enable

// the model, view, and projection matrices are updated every frame (reference this binding in the descriptor set layout)
layout(binding = 0) uniform UniformBufferObject
{
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;
/*
	It's possible to bind multiple descriptor sets simultaneously. We'd need to specify a descriptor set layout for each descriptor set 
	when creating the pipeline layout. We can use this feature to put descriptors that vary per-object and descriptors that are shared into 
	separate descriptor sets. In that case you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.

	layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
*/
/*
	Vertex Attributes: properties that are specified per-vertex in the vertex buffer

    The layout(location = x) annotations assign indices to the inputs that we can later use to reference them. 
    It is important to know that some types, like dvec3 64 bit vectors, use multiple slots. 
    That means that the index after it must be at least 2 higher:
	layout(location = 0) in dvec3 inPosition;
	layout(location = 2) in vec3 inColor;
*/
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragmentColor;

void main() {
	/*
		Compute the final position in clip coordinates.
		The last component of the clip coordinates may not be 1, which will result in a division when converted 
		to the final normalized device coordinates on the screen. This is used in perspective projection as the 
		perspective division and is essential for making closer objects look larger than objects that are further away.
	*/
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
	
	fragmentColor = inColor;
}