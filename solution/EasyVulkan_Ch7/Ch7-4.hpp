#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
using namespace vulkan;

struct vertex {
	glm::vec2 position;
	glm::vec4 color;
};

descriptorSetLayout descriptorSetLayout_triangle;
pipelineLayout pipelineLayout_triangle;
pipeline pipeline_triangle;
const auto& RenderPassAndFramebuffers() {
	static const auto& rpwf_screen = easyVulkan::CreateRpwf_Screen();
	return rpwf_screen;
}
void CreateLayout() {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_trianglePosition = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_triangle = {
		.bindingCount = 1,
		.pBindings = &descriptorSetLayoutBinding_trianglePosition
	};
	descriptorSetLayout_triangle.Create(descriptorSetLayoutCreateInfo_triangle);
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.setLayoutCount = 1,
		.pSetLayouts = descriptorSetLayout_triangle.Address()
	};
	pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shaderModule vert_triangle("shader/FirstTriangle_UniformBuffer.vert.spv");
	static shaderModule frag_triangle("shader/FirstTriangle_VertexBuffer.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
		vert_triangle.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_triangle.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
		pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().renderPass;
		pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, position));
		pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex, color));
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;
		pipeline_triangle.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_triangle.~pipeline();
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(Create);
	graphicsBase::Base().PushCallback_DestroySwapchain(Destroy);
	Create();
}

int main() {
	if (!InitializeWindow({ 1280, 720 }))
		return -1;

	const auto& [renderPass, framebuffers] = RenderPassAndFramebuffers();
	CreateLayout();
	CreatePipeline();

	fence fence(true);
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;

	commandBuffer commandBuffer;
	commandPool commandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsBase::Base().QueueFamilyIndex_Graphics());
	commandPool.AllocateBuffers(commandBuffer);

	vertex vertices[] = {
		{ { .0f, -.5f }, { 1, 0, 0, 1 } },
		{ { -.5f, .5f }, { 0, 1, 0, 1 } },
		{ { .5f, .5f }, { 0, 0, 1, 1 } }
	};
	vertexBuffer vertexBuffer(sizeof vertices);
	vertexBuffer.TransferData(vertices);
	glm::vec2 uniform_positions[] = {
		{ .0f, .0f }, {},
		{ -.5f, .0f }, {},
		{ .5f, .0f }, {}
	};
	uniformBuffer uniformBuffer(sizeof uniform_positions);
	uniformBuffer.TransferData(uniform_positions);

	VkDescriptorPoolSize descriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};
	descriptorPool descriptorPool(0, 1, descriptorPoolSizes);
	descriptorSet descriptorSet_trianglePosition;
	descriptorPool.AllocateSets(descriptorSet_trianglePosition, descriptorSetLayout_triangle);
	VkDescriptorBufferInfo bufferInfo = {
		.buffer = uniformBuffer,
		.offset = 0,
		.range = VK_WHOLE_SIZE
	};
	descriptorSet_trianglePosition.Write(bufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

	VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

	while (!glfwWindowShouldClose(pWindow)) {
		TitleFps();
		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();
		fence.WaitAndReset();

		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer, framebuffers[i], clearColor);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout_triangle, 0, 1, descriptorSet_trianglePosition.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer, 3, 3, 0, 0);
		renderPass.CmdEnd(commandBuffer);
		commandBuffer.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}