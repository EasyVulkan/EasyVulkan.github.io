#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
using namespace vulkan;

struct vertex {
	glm::vec2 position;
	glm::vec2 texCoord;
};

descriptorSetLayout descriptorSetLayout_texture;
pipelineLayout pipelineLayout_texture;
pipeline pipeline_straightAlpha;
pipeline pipeline_premultiplyAlpha;
const auto& RenderPassAndFramebuffers() {
	static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
	return rpwf;
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
		pipelineCiPack.colorBlendAttachmentStates.push_back({
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = 0b1111
		});
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_texture;
		pipeline_straightAlpha.Create(pipelineCiPack);
		pipelineCiPack.colorBlendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		pipeline_premultiplyAlpha.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_straightAlpha.~pipeline();
		pipeline_premultiplyAlpha.~pipeline();
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
	VkExtent2D imageExtent;
	auto pImageData = texture::LoadFile("texture/testImage.png", imageExtent, FormatInfo(VK_FORMAT_R8G8B8A8_UNORM));
	texture2d texture_straightAlpha(pImageData.get(), imageExtent, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
	easyVulkan::fCreateTexture2d_multiplyAlpha function(VK_FORMAT_R8G8B8A8_UNORM, true, nullptr);
	texture2d texture_premultiplyAlpha = function(pImageData.get(), imageExtent, VK_FORMAT_R8G8B8A8_UNORM);
	//Create sampler
	VkSamplerCreateInfo samplerCreateInfo = texture::SamplerCreateInfo();
	sampler sampler(samplerCreateInfo);
	//Create descriptor
	VkDescriptorPoolSize descriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
	};
	descriptorPool descriptorPool(2, descriptorPoolSizes);
	descriptorSet descriptorSet_straightAlpha;
	descriptorSet descriptorSet_premultiplyAlpha;
	descriptorPool.AllocateSets(descriptorSet_straightAlpha, descriptorSetLayout_texture);
	descriptorPool.AllocateSets(descriptorSet_premultiplyAlpha, descriptorSetLayout_texture);
	descriptorSet_straightAlpha.Write(texture_straightAlpha.DescriptorImageInfo(sampler), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorSet_premultiplyAlpha.Write(texture_premultiplyAlpha.DescriptorImageInfo(sampler), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	float w = 2 * imageExtent.width * 10.f / windowSize.width;
	float halfH = imageExtent.height * 10.f / windowSize.height;
	vertex vertices[] = {
		{ { -w, -halfH }, { 0, 0 } },
		{ {  0, -halfH }, { 1, 0 } },
		{ { -w,  halfH }, { 0, 1 } },
		{ {  0,  halfH }, { 1, 1 } },
		{ {  0, -halfH }, { 0, 0 } },
		{ {  w, -halfH }, { 1, 0 } },
		{ {  0,  halfH }, { 0, 1 } },
		{ {  w,  halfH }, { 1, 1 } }
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
		//Straight alpha
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_straightAlpha);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_texture, 0, 1, descriptorSet_straightAlpha.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer, 4, 1, 0, 0);
		//Premultiply alpha
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_premultiplyAlpha);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout_texture, 0, 1, descriptorSet_premultiplyAlpha.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer, 4, 1, 4, 0);
		renderPass.CmdEnd(commandBuffer);
		commandBuffer.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}