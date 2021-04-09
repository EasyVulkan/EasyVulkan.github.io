#include "GlfwClientGeneral.hpp"
#include "EasyVulkan.hpp"
#include "EasyVKUtility.h"

using namespace vulkan;
descriptorSetLayout descriptorSetLayout_constants;
descriptorSetLayout descriptorSetLayout_texture;
pipelineLayout pipelineLayout_skyboxAndCube;
pipeline pipeline_skybox;
pipeline pipeline_cube;
void CreateLayout() {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_constants{};
	descriptorSetLayoutBinding_constants.binding = 0;
	descriptorSetLayoutBinding_constants.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBinding_constants.descriptorCount = 1;
	descriptorSetLayoutBinding_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_texture{};
	descriptorSetLayoutBinding_texture.binding = 0;
	descriptorSetLayoutBinding_texture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBinding_texture.descriptorCount = 1;
	descriptorSetLayoutBinding_texture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding_constants;
	descriptorSetLayout_constants.Create(descriptorSetLayoutCreateInfo);
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding_texture;
	descriptorSetLayout_texture.Create(descriptorSetLayoutCreateInfo);
	VkDescriptorSetLayout descriptorSetLayouts[2] = {
		descriptorSetLayout_constants,
		descriptorSetLayout_texture
	};
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
	pipelineLayout_skyboxAndCube.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shader vert_cube("shader/cube.vert.spv");
	static shader frag_cube("shader/cube.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_cube[2]{
		vert_cube.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_cube.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static shader vert_skybox("shader/skybox.vert.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_skybox[2]{
		vert_skybox.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_cube.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack{};
		pipelineCiPack.createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		pipelineCiPack.createInfo.layout = pipelineLayout_skyboxAndCube;
		pipelineCiPack.createInfo.renderPass = easyVulkan::rpwf_msaaWithDsToScreen.renderPass;
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.0f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
		//pipelineCiPack.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//Default
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
		pipelineCiPack.colorBlendAttachmentStates.emplace_back(
			true,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
			0b1111
		);
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_skybox;
		pipeline_skybox.Create(pipelineCiPack.createInfo);
		pipelineCiPack.createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_cube;
		pipelineCiPack.depthStencilStateCi.depthTestEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthWriteEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineCiPack.createInfo.basePipelineHandle = pipeline_skybox;
		pipelineCiPack.createInfo.basePipelineIndex = -1;
		pipeline_cube.Create(pipelineCiPack.createInfo);
	};
	auto Destroy = [] {
		pipeline_skybox.~pipeline();
		pipeline_cube.~pipeline();
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(Create);
	graphicsBase::Base().PushCallback_DestroySwapchain(Destroy);
	Create();
}

//Mouse Operation
template<> double easyVulkan::doubleClickMaxInterval<double> = 0.5;
int32_t currentButton = easyVulkan::mouseButton::none;
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	currentButton = action ? button + 1 : 0;
}
void ScrollCallback(GLFWwindow* window, double sx, double sy) {
	easyVk_mouse.sy = sy;
}
inline void UpdateCursorPosition() {
	glfwGetCursorPos(pWindow, &easyVk_mouse.x, &easyVk_mouse.y);
}

int main() {
	if (!InitializeWindow({ 1280, 720 }))
		return -1;
	easyVulkan::BootScreen();
	glfwSetMouseButtonCallback(pWindow, MouseButtonCallback);
	glfwSetScrollCallback(pWindow, ScrollCallback);

	easyVulkan::CreateRpwf_MsaaWithDsToScreen();
	CreateLayout();
	CreatePipeline();

	//Synchronization object
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;
	fence fence(true);
	//Command object
	commandBuffer commandBuffer_graphics;
	commandBuffer commandBuffer_transfer;
	auto& gcp = graphicsBase::Plus().CommandPool_Graphics();
	gcp.AllocateCommandBuffers((VkCommandBuffer*)&commandBuffer_graphics);
	gcp.AllocateCommandBuffers((VkCommandBuffer*)&commandBuffer_transfer);

	//Texture
	uvec2 facePositions[] = {
		{ 0, 0 }, { 1, 0 },
		{ 0, 1 }, { 1, 1 },
		{ 0, 2 }, { 1, 2 }
	};
	textureCube skybox("texture/cubemap/skybox_fromInside.png", facePositions);
	textureCube cube("texture/cubemap/cube_fromOutside.png", nullptr, true);

	//Sampler
	VkSamplerCreateInfo samplerCreateInfo = texture::SamplerCreateInfo();
	sampler sampler(samplerCreateInfo);

	//Camera
	vec3 camPos(0, 0, -1);
	vec3 camUp(0, 1, 0);
	vec3 camFront(0, 0, 1);
	easyVulkan::camera myDogEye(camPos, camUp, camFront);

	//Uniform buffer
	/*If you want to use multiple uniform buffers in one*/
	//VkDeviceSize uniformAlignment = graphicsBase::Base().PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	//VkDeviceSize dataSize[n] = { ???, ..., ??? };
	//VkDeviceSize uniformBufferSize = uniformAlignment * (std::ceil(float(dataSize[0]) / uniformAlignment) + ... + std::ceil(float(dataSize[n]) / uniformAlignment));
	/*------------------*/
	mat4 pvm[3]{};
	pvm[0] = { FlipVertical(glm::infinitePerspective(60._d2r, float(windowSize.width) / windowSize.height, 0.1f)) };
	pvm[2] = glm::scale(mat4(1), vec3(0.5f, 0.5f, 0.5f));
	uniformBuffer uniformBuffer_pvm(sizeof pvm);

	//Descriptor
	VkDescriptorPoolSize descriptorPoolSizes[2]{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
	};
	vulkan::descriptorPool descriptorPool(0/*No flags, no need to free*/, 3, descriptorPoolSizes);
	vulkan::descriptorSet descriptorSet_constants;
	vulkan::descriptorSet descriptorSet_skybox;
	vulkan::descriptorSet descriptorSet_cube;
	descriptorPool.AllocateDescriptorSet((VkDescriptorSet&)descriptorSet_constants, descriptorSetLayout_constants);
	descriptorPool.AllocateDescriptorSet((VkDescriptorSet&)descriptorSet_skybox, descriptorSetLayout_texture);
	descriptorPool.AllocateDescriptorSet((VkDescriptorSet&)descriptorSet_cube, descriptorSetLayout_texture);
	VkWriteDescriptorSet writeDescriptorSets[3]{};
	VkDescriptorBufferInfo bufferInfo = { uniformBuffer_pvm, 0, VK_WHOLE_SIZE };
	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].dstSet = descriptorSet_constants;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[0].pBufferInfo = &bufferInfo;
	VkDescriptorImageInfo imageInfo_skybox = { sampler, skybox.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[1].dstSet = descriptorSet_skybox;
	writeDescriptorSets[1].descriptorCount = 1;
	writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[1].pImageInfo = &imageInfo_skybox;
	VkDescriptorImageInfo imageInfo_cube = { sampler, cube.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[2].dstSet = descriptorSet_cube;
	writeDescriptorSets[2].descriptorCount = 1;
	writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSets[2].pImageInfo = &imageInfo_cube;
	vkUpdateDescriptorSets(graphicsBase::Base().Device(), 3, writeDescriptorSets, 0, nullptr);

	//Clear value
	VkClearColorValue clearColor = { 238.f / 255, 243.f / 255, 250.f / 255, 0.f };
	VkClearValue clearValues[3]{
		clearColor,
		clearColor,
		{.depthStencil = {1.f, 0}}
	};

	while (!glfwWindowShouldClose(pWindow)) {
		TitleFps();
		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();

		//Control
		static double mouseX0, mouseY0;
		mouseX0 = easyVk_mouse.x;
		mouseY0 = easyVk_mouse.y;
		UpdateCursorPosition();
		easyVk_mouse.Button(currentButton);
		easyVk_mouse.Count(glfwGetTime());

		if (easyVk_mouse.action == easyVulkan::hold)
			if (easyVk_mouse.button == easyVulkan::rButton)
				myDogEye.Rotate(easyVk_mouse.x - mouseX0, mouseY0 - easyVk_mouse.y, 0.005f);
			else if (easyVk_mouse.button == easyVulkan::mButton)
				myDogEye.OrbitOrPan(easyVk_mouse.x - mouseX0, mouseY0 - easyVk_mouse.y, 0.005f);
		if (easyVk_mouse.sy != 0)
			myDogEye.Zoom(easyVk_mouse.sy, 0.1f),
			easyVk_mouse.sy = 0;
		pvm[1] = myDogEye.RecalculateView();

		//Update
		fence.WaitAndReset();
		commandBuffer_transfer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		uniformBuffer_pvm.CmdUpdateBuffer(commandBuffer_transfer, pvm);
		commandBuffer_transfer.End();
		graphicsBase::Base().SubmitTransferCommandBuffer(commandBuffer_transfer, fence);

		//Render
		fence.WaitAndReset();
		commandBuffer_graphics.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		easyVulkan::rpwf_msaaWithDsToScreen.renderPass.CmdBegin(commandBuffer_graphics, easyVulkan::rpwf_msaaWithDsToScreen.framebuffers[i], clearValues);

		vkCmdBindPipeline(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_skybox);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_skyboxAndCube, 0, 1, (VkDescriptorSet*)&descriptorSet_constants, 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_skyboxAndCube, 1, 1, (VkDescriptorSet*)&descriptorSet_skybox, 0, nullptr);
		vkCmdDraw(commandBuffer_graphics, 14, 1, 0, 0);
		vkCmdBindPipeline(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_cube);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_skyboxAndCube, 1, 1, (VkDescriptorSet*)&descriptorSet_cube, 0, nullptr);
		vkCmdDraw(commandBuffer_graphics, 14, 1, 0, 0);

		easyVulkan::rpwf_msaaWithDsToScreen.renderPass.CmdEnd(commandBuffer_graphics);
		commandBuffer_graphics.End();

		graphicsBase::Base().SubmitGraphicsCommandBuffer(commandBuffer_graphics, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);

		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);
		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}