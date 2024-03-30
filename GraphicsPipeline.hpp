#pragma once

#include "Vertex.hpp"
#include <string>
#include <vector>

// In Vulkan, pipelines are immutable. Each distinct configuration requires its own instance.
class GraphicsPipeline
{
public:
	GraphicsPipeline(
		const std::string& vertexShaderPath, 
		const std::string& fragmentShaderPath,
		VkSampleCountFlagBits msaaSamples,
		const std::vector<VkDescriptorSetLayout>& descriptorLayouts,
		const VkRenderPass& renderPass,
		const VkDevice& logicalDevice
	);
	~GraphicsPipeline();
	void bind(VkCommandBuffer& commandBuffer);
	VkPipelineLayout getPipelineLayout() const { return m_PipelineLayout; }

private:
	const VkDevice& m_LogicalDevice;
	std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	const VkRenderPass& m_RenderPass;
	VkSampleCountFlagBits m_Samples;
	VkPipelineLayout m_PipelineLayout = nullptr;
	VkPipeline m_GraphicsPipeline = nullptr;
	std::array<VkShaderModule, 2> m_Shaders{};
	std::array<VkPipelineShaderStageCreateInfo, 2> m_ShaderStages{};
	std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void loadShaders(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
	void createPipeline();
	void freeShaders();
};
