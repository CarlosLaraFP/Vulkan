# Vulkan Tutorial from Khronos

Render passes' subpasses are responsible for submitting rendering commands to the graphics pipeline in the form of (multiple) draw calls. 
Each draw call is a command sent to the GPU to render specific geometry. 
The graphics pipeline outputs fragments/pixels after the initially submitted geometry went through these sequential transformations: 
Object/model space, world space, view/camera space, clip space, NDC, framebuffer/window space. 
These fragments/pixels are then stored in the framebuffer's image attachments. 
Once a frame is ready after all render passes, all the images that comprise the frame are swapped 
with the front buffer (last displayed frame), resulting in the screen smoothly updating.

* Allocate multiple buffers from a single memory allocation.
* Store vertex and index buffer into a single VkBuffer and use offsets in (vkCmdBindVertexBuffers, etc).
* Dedicated memory operations queue with command pool.
* 