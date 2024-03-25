# Vulkan Tutorial from Khronos

Render passes' subpasses are responsible for submitting rendering commands to the graphics pipeline in the form of (multiple) draw calls. 
Each draw call is a command sent to the GPU to render specific geometry. 
The graphics pipeline outputs fragments/pixels after the initially submitted geometry went through these sequential transformations: 
Object/model space, world space, view/camera space, clip space, NDC, framebuffer/window space. 
These fragments/pixels are then stored in the framebuffer's image attachments. 
Once a frame is ready after all render passes, all the images that comprise the frame are swapped 
with the front buffer (last displayed frame), resulting in the screen smoothly updating.

TODO next:

* Modularize main.cpp into classes
* Normals and local/global illumination
* Blinn-Phong lighting
* Post-processing effects
* Shadow mapping
* Enable best practice validation [layer]
* Allocate multiple buffers from a single memory allocation
* Store vertex and index buffer into a single VkBuffer and use offsets in (vkCmdBindVertexBuffers, etc.)
* Dedicated memory operations queue with command pool
* Implement push constants [to replace UBO approach]
* Real-time offline on-device inference with rendered images via compute shader (MNIST or a canonical computer vision model)
* Render animating 3D models/meshes
* Execute commands in createTextureImage asynchronously
* Post-Processing Anti-Aliasing: Methods like FXAA (Fast Approximate Anti-Aliasing) and TAA (Temporal Anti-Aliasing) to apply filtering to the rendered image as a post-processing step to smooth out jagged edges and other aliasing artifacts
* Instance rendering
* Dynamic uniforms
* Separate images and sampler descriptors
* Multi-threaded command buffer generation
* Multiple subpasses
* Pipeline cache
* Physics calculations via compute shader on a dedicated compute queue (async compute)
* 