#include "GraphicsPipeline.hpp"

#include <fstream>

GraphicsPipeline::GraphicsPipeline(
	const std::string& vertexShaderPath, 
	const std::string& fragmentShaderPath, 
    VkSampleCountFlagBits msaaSamples,
	const VkDescriptorSetLayout& descriptorLayout,
    const VkRenderPass& renderPass,
    const VkDevice& logicalDevice
) : m_LogicalDevice{ logicalDevice }, m_DescriptorSetLayout { descriptorLayout }, m_RenderPass { renderPass }, m_Samples { msaaSamples }
{
    loadShaders(vertexShaderPath, fragmentShaderPath);
    createPipeline();
    freeShaders();
}

GraphicsPipeline::~GraphicsPipeline()
{
    vkDestroyPipeline(m_LogicalDevice, m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_LogicalDevice, m_PipelineLayout, nullptr);
}

// Reads all of the bytes from the specified file and returns them in a byte array managed by a vector.
std::vector<char> GraphicsPipeline::readFile(const std::string& filename)
{
    // ate: Start reading at the end of the file
    // binary: Read the file as a binary file (avoid text transformations)
    std::ifstream file{ filename, std::ios::ate | std::ios::binary };

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file.");
    }

    // The advantage of starting to read at the end of the file is that we can use 
    // the read position to determine the size of the file and allocate a buffer:
    size_t fileSize = (size_t)file.tellg(); // returns the current position of the file pointer (end, which = file size)
    // the distinction between an initializer list and a single size argument matters for std::vector (be careful)
    std::vector<char> buffer(fileSize);

    file.seekg(0); // Resets the file pointer to the beginning of the file, preparing it for reading from the start.
    file.read(buffer.data(), fileSize); // reads fileSize bytes from the file into the buffer
    file.close();

    return buffer;
}

/*
        Vulkan expects shader code to be passed as a pointer to uint32_t in the pCode field of the VkShaderModuleCreateInfo struct,
        aligned to a 4-byte boundary, because shaders are consumed as an array of 32-bit words. This is because GPU hardware and the
        SPIR-V shader bytecode format are designed to work with 32-bit instructions. In practice, this means that when you store shader
        code in a std::vector<char>, even though char could technically be stored at any byte boundary, the memory allocated by the
        vector's default allocator will be aligned in such a way that converting the address to a uint32_t* pointer for Vulkan's use
        will still respect the 4-byte alignment requirement. This alignment ensures that when Vulkan accesses the shader code as an
        array of 32-bit words, those accesses are correctly aligned according to the hardware and API requirements, thus maintaining
        performance and preventing potential errors. Using reinterpret_cast to convert char* to uint32_t* makes the programmer's
        intent explicit and maintains type safety. The use of const in this context is also aligned with the Vulkan API's requirement
        that the shader code should not be modified during shader module creation, although the const qualifier may need to be adjusted
        depending on the API's expectations and the const correctness of the data.
    */
VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    /*
        C-style casts like (uint32_t*)code.data() are versatile but potentially unsafe, as they do not provide
        compile-time type checking. They can perform a combination of static_cast, reinterpret_cast, const_cast,
        and even dynamic_cast operations, depending on the context.

        reinterpret_cast<const uint32_t*>(code.data()) explicitly reinterprets the pointer type without changing
        the bit pattern of the pointer value. reinterpret_cast is more specific and safer than C-style casts because
        it limits the kinds of conversions that can be performed, making the programmer's intent clearer and reducing
        the risk of unintended conversions. Adding const in this cast also indicates that the pointed-to data should
        not be modified, which is important for type safety and maintaining const correctness.
    */
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule{};

    if (vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module.");
    }

    // The buffer with the code can be freed immediately after creating the shader module.
    //delete code.data();

    return shaderModule;
}

void GraphicsPipeline::loadShaders(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
{
    auto vertexShaderCode = readFile(vertexShaderPath);
    auto fragmentShaderCode = readFile(fragmentShaderPath);

    //std::cout << vertexShaderCode.size() << std::endl;
    //std::cout << fragmentShaderCode.size() << std::endl;

    auto vertexModule = createShaderModule(vertexShaderCode);
    auto fragmentModule = createShaderModule(fragmentShaderCode);

    m_Shaders[0] = vertexModule;
    m_Shaders[1] = fragmentModule;

    VkPipelineShaderStageCreateInfo vertexStageInfo{};

    vertexStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // there is an enum value for each of the programmable stages
    vertexStageInfo.module = vertexModule;
    /*
        Specifies the function to invoke (entrypoint). Tt’s possible to combine multiple fragment shaders into
        a single shader module and use different entry points to differentiate between their behaviors.
    */
    vertexStageInfo.pName = "main";
    /*
        There is one more (optional) member, pSpecializationInfo, that allows you to specify values for shader constants.
        You can use a single shader module where its behavior can be configured at pipeline creation by specifying different
        values for the constants used in it. This is more efficient than configuring the shader using variables at render time,
        because the compiler can do optimizations like eliminating if statements that depend on these values. If you don’t have
        any constants like that, then you can set the member to nullptr, which our struct initialization does automatically.
    */

    VkPipelineShaderStageCreateInfo fragmentStageInfo{};

    fragmentStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageInfo.module = fragmentModule;
    fragmentStageInfo.pName = "main";

    m_ShaderStages[0] = vertexStageInfo;
    m_ShaderStages[1] = fragmentStageInfo;
}

void GraphicsPipeline::createPipeline()
{
    // Fixed functions start

    // TODO: Parametrize
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    /*
        Describes the format of the vertex data that will be passed to the vertex shader in roughly two ways:

        Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
        Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // could be an array with multiple bindings
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    /*
        The topology member describes what kind of geometry will be drawn from the vertices and can have values like:

        VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices

        VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse

        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line

        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse

        `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP `: the second and third vertex of every triangle are used as first two vertices of the next triangle

        Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer
        you can specify the indices to use yourself. This allows you to perform optimizations like reusing vertices. If
        you set the primitiveRestartEnable member to VK_TRUE, then it’s possible to break up lines and triangles in the
        _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
    */
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // We only draw triangles in this project
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /*
        A viewport describes the region of the framebuffer that the output will be rendered to. This will almost always be
        (0, 0) to (width, height). The swap chain images will be used as framebuffers later, so we should stick to their size.
        The minDepth and maxDepth values specify the range of depth values for the framebuffer and must be within [0.0f, 1.0f].
    */
    /*
        Viewport(s) and scissor rectangle(s) can either be specified as a static part of the pipeline or as a dynamic
        state set in the command buffer. While the former is more in line with the other states it’s often convenient
        to make viewport and scissor state dynamic as it gives you a lot more flexibility. This is very common and all
        implementations can handle this dynamic state without a performance penalty. When opting for dynamic viewport(s)
        and scissor rectangle(s) you need to enable the respective dynamic states for the pipeline.
        The actual viewport(s) and scissor rectangle(s) would then later be set up at drawing time.
    */
    std::vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};

    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    /*
        Without dynamic state, the viewport and scissor rectangle need to be set in the pipeline using the
        VkPipelineViewportStateCreateInfo struct. This makes the viewport and scissor rectangle for this pipeline
        immutable. Any changes required to these values would require a new pipeline to be created with the new values.
    */
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    //viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    //viewportState.pScissors = &scissor;

    /*
        The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments
        to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and it can
        be configured to output fragments that fill entire polygons or just the edges (wireframe rendering).
    */
    VkPipelineRasterizationStateCreateInfo rasterizer{};

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    /*
        If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as
        opposed to discarding them. This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature
        because not all graphics cards support this. For shadow maps, rendering objects that are partially or fully outside the
        camera's view can be necessary to correctly calculate shadows for objects that are within view. Depth clamping allows these
        out-of-view objects to still contribute to the shadow map, improving the quality of shadows at the edges of the view frustum.

        Normally, fragments that would end up outside the near or far planes after the perspective division (which converts clip space
        coordinates to normalized device coordinates) are discarded. When depthClampEnable is set to VK_TRUE, these fragments are not
        discarded. Instead, their depth values are clamped to the near or far plane values. The key point is that while the clipping
        operations to the side planes of the frustum still occur, the depth clamping modifies how out-of-bound depth values are treated,
        preventing their outright discarding based solely on depth.
    */
    rasterizer.depthClampEnable = VK_FALSE;
    // If VK_TRUE, then geometry never passes through the rasterizer stage. This basically disables any output to the framebuffer.
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    /*
        The polygonMode determines how fragments are generated for geometry. The following modes are available:

        VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments

        VK_POLYGON_MODE_LINE: polygon edges are drawn as lines

        VK_POLYGON_MODE_POINT: polygon vertices are drawn as points

        Using any mode other than fill requires enabling a GPU feature because not all graphics cards support the other modes.
    */
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    /*
        The lineWidth member describes the thickness of lines in terms of number of fragments. The maximum line width that is
        supported depends on the hardware and any line thicker than 1.0f requires you to enable the wideLines GPU feature.
    */
    rasterizer.lineWidth = 1.0f;
    /*
        The cullMode variable determines the type of face culling to use. You can disable culling,
        cull the front faces, cull the back faces, or both. The frontFace variable specifies the
        vertex order for faces to be considered front-facing and can be clockwise or counterclockwise.
        Because of the Y-flip we did in the projection matrix, the vertices are now being drawn in counter-clockwise order
        instead of clockwise order. This causes backface culling to kick in and prevents any geometry from being drawn,
        unless we make the front face match this (VK_FRONT_FACE_COUNTER_CLOCKWISE).
    */
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /*
        The rasterizer can alter the depth values by adding a constant value or biasing them based on a fragment’s slope.
        This is sometimes used for shadow mapping, but we won’t be using it.
    */
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    /*
        Configures multisampling, which is one of the ways to perform anti-aliasing.
        It works by combining the fragment shader results of multiple polygons that rasterize to the same pixel.
        This mainly occurs along edges, which is also where the most noticeable aliasing artifacts occur.
        Because it doesn’t need to run the fragment shader multiple times if only one polygon maps to a pixel,
        it is significantly less expensive than simply rendering to a higher resolution and then downscaling.
        Enabling it requires enabling a GPU feature because not all graphics cards support it.
    */
    VkPipelineMultisampleStateCreateInfo multisampling{}; // disabled for now

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = m_Samples; // enable MSAA in the pipeline
    multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
    multisampling.minSampleShading = 0.2f; // min fraction for sample shading; closer to one is smoother
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // A depth stencil state must always be specified if the render pass contains a depth stencil attachment.
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    /*
        The depthTestEnable field specifies if the depth of new fragments should be compared to the depth buffer to see if they should
        be discarded. The depthWriteEnable field specifies if the new depth of fragments that pass the depth test should actually be
        written to the depth buffer.
    */
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    // Specifies the comparison that is performed to keep or discard fragments. 
    // The convention is: lower depth = closer, so the depth of new fragments should be less.
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    /*
        The depthBoundsTestEnable, minDepthBounds, and maxDepthBounds fields are used for the optional depth bound test.
        This allows us to only keep fragments that fall within the specified depth range. We won’t be using this functionality.
    */
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    /*
        The last three fields configure stencil buffer operations, which we also won’t be using in this tutorial. If you want to use
        these operations, then you will have to make sure that the format of the depth/stencil image contains a stencil component.
    */
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    /*
        After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer.
        This transformation is known as color blending and there are two ways to do it:

        - Mix the old and new value to produce a final color (per-framebuffer struct below)
        - Combine the old and new value using a bitwise operation

        There are two types of structs to configure color blending. The first struct, VkPipelineColorBlendAttachmentState
        contains the configuration per attached framebuffer and the second struct, VkPipelineColorBlendStateCreateInfo
        contains the global color blending settings. In our case we only have one framebuffer.
    */
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    /*
        If blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified.
        Otherwise, the two mixing operations are performed to compute a new color.
        The resulting color is AND’d with the colorWriteMask to determine which channels are actually passed through.

        The most common way to use color blending is to implement alpha blending, where we want
        the new color to be blended with the old color based on its opacity.
    */
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE; // VK_FALSE and below comments means no blending (ignore destination pixels / overwrite)
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // VK_BLEND_FACTOR_ONE
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // VK_BLEND_FACTOR_ZERO
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    /*
        References the array of structures for all of the framebuffers and allows us to
        set blend constants that we can use as blend factors in the above calculations.
    */
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    /*
        If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE.
        The bitwise operation can then be specified in the logicOp field. Note that this will automatically disable the first
        method, as if you had set blendEnable to VK_FALSE for every attached framebuffer! The colorWriteMask will also be used
        in this mode to determine which channels in the framebuffer will actually be affected. It is also possible to disable
        both modes, as we’ve done here, in which case the fragment colors will be written to the framebuffer unmodified.
    */
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    // Fixed functions end

    /*
            Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time.

            We can use uniform values in shaders, which are globals similar to dynamic state variables that can be changed at
            drawing time to alter the behavior of our shaders without having to recreate them. They are commonly used to pass
            the transformation matrix to the vertex shader, or to create texture samplers in the fragment shader.

            These uniform values need to be specified during pipeline creation by creating a VkPipelineLayout object.
            The structure also specifies push constants, which are another way of passing dynamic values to shaders.
            We need to specify the descriptor set layout to tell Vulkan which descriptors the shaders will be using.
            It’s possible to specify multiple descriptor set layouts here, even though a single one already includes
            all of the bindings, because of descriptor pools and descriptor sets.
        */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    // required even if the pipeline does not use any
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout.");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};

    // Referencing the shader tages
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2; // 1 vertex and 1 fragment
    pipelineInfo.pStages = m_ShaderStages.data();
    // Referencing all of the structures describing the fixed-function stage.
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    // the pipeline layout is a Vulkan handle rather than a struct pointer
    pipelineInfo.layout = m_PipelineLayout;
    /*
        Referencing the render pass and the index of the subpass where this graphics pipeline will be used.
        It is also possible to use other render passes with this pipeline instead of this specific instance,
        but they have to be compatible with renderPass. The requirements for compatibility are described in the documentation.
    */
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;
    /*
        Vulkan allows us to create a new graphics pipeline by deriving from an existing pipeline. The idea of pipeline derivatives
        is that it is less expensive to set up pipelines when they have much functionality in common with an existing pipeline and
        switching between pipelines from the same parent can also be done quicker. You can either specify the handle of an existing
        pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex.
        Right now there is only a single pipeline, so we simply specify a null handle and an invalid index. These values are only
        used if the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is also specified in the flags field of VkGraphicsPipelineCreateInfo.
    */
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    /*
        The vkCreateGraphicsPipelines function is designed to take multiple VkGraphicsPipelineCreateInfo objects and create multiple
        VkPipeline objects in a single call. The second parameter, for which we’ve passed the VK_NULL_HANDLE argument, references an
        optional VkPipelineCache object. A pipeline cache can be used to store and reuse data relevant to pipeline creation across
        multiple calls to vkCreateGraphicsPipelines and even across program executions if the cache is stored to a file. This makes
        it possible to significantly speed up pipeline creation at a later time. We’ll get into this in the pipeline cache chapter.
    */
    if (vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline.");
    }
}

/*
    In Vulkan we have to be explicit about most pipeline states as it will be baked into an immutable pipeline state object.
    The compilation and linking of the SPIR-V bytecode to machine code for execution by the GPU doesn’t happen until the graphics
    pipeline is created. That means that we’re allowed to destroy the shader modules again as soon as pipeline creation is finished.
*/
void GraphicsPipeline::freeShaders()
{
    vkDestroyShaderModule(m_LogicalDevice, m_Shaders[0], nullptr);
    vkDestroyShaderModule(m_LogicalDevice, m_Shaders[1], nullptr);
}

void GraphicsPipeline::bind(VkCommandBuffer& commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
}
