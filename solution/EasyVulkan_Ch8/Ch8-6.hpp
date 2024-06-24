#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
using namespace easyVulkan;

descriptorSetLayout descriptorSetLayout_constants;
descriptorSetLayout descriptorSetLayout_texture;
pipelineLayout pipelineLayout_skyboxAndCube;
pipeline pipeline_skybox;
pipeline pipeline_cube;
const auto& RenderPassAndFramebuffers() {
	static const auto& rpwf = CreateRpwf_ScreenWithDS();
	return rpwf;
}
void CreateLayout() {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_constants = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT };
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_texture = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT };
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		.bindingCount = 1,
		.pBindings = &descriptorSetLayoutBinding_constants
	};
	descriptorSetLayout_constants.Create(descriptorSetLayoutCreateInfo);
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding_texture;
	descriptorSetLayout_texture.Create(descriptorSetLayoutCreateInfo);
	VkDescriptorSetLayout descriptorSetLayouts[2] = {
		descriptorSetLayout_constants,
		descriptorSetLayout_texture
	};
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.setLayoutCount = 2,
		.pSetLayouts = descriptorSetLayouts
	};
	pipelineLayout_skyboxAndCube.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shaderModule vert_cube("shader/Cube.vert.spv");
	static shaderModule frag_cube("shader/Cube.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_cube[2] = {
		vert_cube.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_cube.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static shaderModule vert_skybox("shader/Skybox.vert.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_skybox[2] = {
		vert_skybox.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_cube.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		pipelineCiPack.createInfo.layout = pipelineLayout_skyboxAndCube;
		pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().renderPass;
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
		//pipelineCiPack.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//Default
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_skybox;
		pipeline_skybox.Create(pipelineCiPack);
		pipelineCiPack.createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_cube;
		pipelineCiPack.depthStencilStateCi.depthTestEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthWriteEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineCiPack.createInfo.basePipelineHandle = pipeline_skybox;
		pipelineCiPack.createInfo.basePipelineIndex = -1;
		pipeline_cube.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_skybox.~pipeline();
		pipeline_cube.~pipeline();
	};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
}

int main() {
	if (!InitializeWindow(defaultWindowSize))
		return -1;

	const auto& [renderPass, framebuffers] =
		RenderPassAndFramebuffers();
	CreateLayout();
	CreatePipeline();

	//Synchronization object
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;
	fence fence;
	//Command object
	commandBuffer commandBuffer_graphics;
	commandBuffer commandBuffer_transfer;
	auto& gcp = graphicsBase::Plus().CommandPool_Graphics();
	gcp.AllocateBuffers(commandBuffer_graphics);
	gcp.AllocateBuffers(commandBuffer_transfer);

	//Texture
	glm::uvec2 facePositions[] = {
		{ 0, 0 }, { 1, 0 },
		{ 0, 1 }, { 1, 1 },
		{ 0, 2 }, { 1, 2 }
	};
	textureCube skybox("image/skybox_fromInside.png", facePositions, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM);
	textureCube cube("image/cube_fromOutside.png", nullptr, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);

	//Sampler
	VkSamplerCreateInfo samplerCreateInfo = texture::SamplerCreateInfo();
	sampler sampler(samplerCreateInfo);

	//Uniform buffer
	glm::mat4 pvm[3] = {
		FlipVertical(glm::infinitePerspectiveLH_ZO(glm::radians(60.f), float(windowSize.width) / windowSize.height, 0.1f)),
		glm::lookAtLH(glm::vec3(-2, 1, -2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)),
		glm::scale(glm::mat4(1), glm::vec3(0.5f, 0.5f, 0.5f))
	};
	uniformBuffer uniformBuffer_pvm(sizeof pvm);
	uniformBuffer_pvm.TransferData(pvm);

	//Descriptor
	VkDescriptorPoolSize descriptorPoolSizes[2] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
	};
	descriptorPool descriptorPool(3, descriptorPoolSizes);
	descriptorSet descriptorSet_constants;
	descriptorSet descriptorSet_skybox;
	descriptorSet descriptorSet_cube;
	descriptorPool.AllocateSets(descriptorSet_constants, descriptorSetLayout_constants);
	descriptorPool.AllocateSets(descriptorSet_skybox, descriptorSetLayout_texture);
	descriptorPool.AllocateSets(descriptorSet_cube, descriptorSetLayout_texture);
	VkDescriptorBufferInfo bufferInfo = { uniformBuffer_pvm, 0, VK_WHOLE_SIZE };
	VkDescriptorImageInfo imageInfo_skybox = { sampler, skybox.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkDescriptorImageInfo imageInfo_cube = { sampler, cube.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	VkWriteDescriptorSet writeDescriptorSets[3] = {
		{
			.dstSet = descriptorSet_constants,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &bufferInfo },
		{
			.dstSet = descriptorSet_skybox,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo_skybox },
		{
			.dstSet = descriptorSet_cube,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &imageInfo_cube }
	};
	descriptorSet::Update(writeDescriptorSets);

	//Clear value
	VkClearValue clearValues[2] = {
		{},
		{ .depthStencil = { 1.f, 0 } }
	};

	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();

		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();

		commandBuffer_graphics.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer_graphics, framebuffers[i], { {}, windowSize }, clearValues);

		vkCmdBindPipeline(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_skybox);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_skyboxAndCube, 0, 1, descriptorSet_constants.Address(), 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_skyboxAndCube, 1, 1, descriptorSet_skybox.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer_graphics, 14, 1, 0, 0);
		vkCmdBindPipeline(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_cube);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_skyboxAndCube, 1, 1, descriptorSet_cube.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer_graphics, 14, 1, 0, 0);

		renderPass.CmdEnd(commandBuffer_graphics);
		commandBuffer_graphics.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer_graphics, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
		TitleFps();

		fence.WaitAndReset();
	}
	TerminateWindow();
	return 0;
}