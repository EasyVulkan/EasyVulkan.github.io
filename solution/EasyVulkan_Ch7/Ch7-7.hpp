#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
using namespace vulkan;

struct vertex {
	glm::vec2 position;
	glm::vec2 texCoord;
};

descriptorSetLayout descriptorSetLayout_texture;
pipelineLayout pipelineLayout_texture;
pipeline pipeline_texture;
const auto& RenderPassAndFramebuffers() {
	static const auto& rpwf_screen = easyVulkan::CreateRpwf_Screen();
	return rpwf_screen;
}
void CreateLayout() {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_texture = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo_texture = {
		.bindingCount = 1,
		.pBindings = &descriptorSetLayoutBinding_texture
	};
	descriptorSetLayout_texture.Create(descriptorSetLayoutCreateInfo_texture);
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.setLayoutCount = 1,
		.pSetLayouts = descriptorSetLayout_texture.Address()
	};
	pipelineLayout_texture.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shaderModule vert("shader/Texture.vert.spv");
	static shaderModule frag("shader/Texture.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_texture[2] = {
		vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.layout = pipelineLayout_texture;
		pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().renderPass;
		pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, position));
		pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, texCoord));
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_texture;
		pipeline_texture.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_texture.~pipeline();
	};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
}

int main() {
	if (!InitializeWindow({ 1280, 720 }))
		return -1;

	const auto& [renderPass, framebuffers] = RenderPassAndFramebuffers();
	CreateLayout();
	CreatePipeline();

	fence fence(VK_FENCE_CREATE_SIGNALED_BIT);
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;

	commandBuffer commandBuffer;
	commandPool commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool.AllocateBuffers(commandBuffer);

	//Load image
	texture2d texture("image/testImage.png", VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, 0);
	//Create sampler
	VkSamplerCreateInfo samplerCreateInfo = texture::SamplerCreateInfo();
	sampler sampler(samplerCreateInfo);
	//Create descriptor
	VkDescriptorPoolSize descriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};
	descriptorPool descriptorPool(1, descriptorPoolSizes);
	descriptorSet descriptorSet_texture;
	descriptorPool.AllocateSets(descriptorSet_texture, descriptorSetLayout_texture);
	VkDescriptorImageInfo imageInfo = {
		.sampler = sampler,
		.imageView = texture.ImageView(),
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	descriptorSet_texture.Write(imageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	vertex vertices[] = {
		{ { -.5f, -.5f }, { 0, 0 } },
		{ {  .5f, -.5f }, { 1, 0 } },
		{ { -.5f,  .5f }, { 0, 1 } },
		{ {  .5f,  .5f }, { 1, 1 } }
	};
	vertexBuffer vertexBuffer(sizeof vertices);
	vertexBuffer.TransferData(vertices);

	VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();
		TitleFps();

		fence.WaitAndReset();
		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();

		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_texture);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout_texture, 0, 1, descriptorSet_texture.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer, 4, 1, 0, 0);
		renderPass.CmdEnd(commandBuffer);
		commandBuffer.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}