#include "GlfwClientGeneral.hpp"
#include "EasyVulkan.hpp"
#include "EasyVKUtility.h"

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec4 albedoSpecular;
};

using namespace vulkan;
descriptorSetLayout descriptorSetLayouts_gBuffer[2];
descriptorSetLayout descriptorSetLayout_composition;
pipelineLayout pipelineLayout_gBuffer;
pipelineLayout pipelineLayout_composition;
pipeline pipeline_gBuffer;
pipeline pipeline_composition;
void CreateLayout() {
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_projView = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1, VK_SHADER_STAGE_VERTEX_BIT };
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_models = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC , 1, VK_SHADER_STAGE_VERTEX_BIT };
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings_composition[4]{
		{ 0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
	};
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding_projView;
	descriptorSetLayouts_gBuffer[0].Create(descriptorSetLayoutCreateInfo);
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding_models;
	descriptorSetLayouts_gBuffer[1].Create(descriptorSetLayoutCreateInfo);
	descriptorSetLayoutCreateInfo.bindingCount = 4;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings_composition;
	descriptorSetLayout_composition.Create(descriptorSetLayoutCreateInfo);
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pipelineLayoutCreateInfo.pSetLayouts = (VkDescriptorSetLayout*)descriptorSetLayouts_gBuffer;
	pipelineLayout_gBuffer.Create(pipelineLayoutCreateInfo);
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = (VkDescriptorSetLayout*)&descriptorSetLayout_composition;
	pipelineLayout_composition.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shader vert_gBuffer("shader/DeferredToScreen_GBuffer.vert.spv");
	static shader frag_gBuffer("shader/DeferredToScreen_GBuffer.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_gBuffer[2]{
		vert_gBuffer.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_gBuffer.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static shader vert_composition("shader/DeferredToScreen_Composition.vert.spv");
	static shader frag_composition("shader/DeferredToScreen_Composition.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_composition[2]{
		vert_composition.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_composition.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static VkSpecializationInfo specializationInfo{};
	static int shininess = 64;
	static VkSpecializationMapEntry specializationMapEntry_maxLightCount = { 1, 0, sizeof shininess };
	specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &specializationMapEntry_maxLightCount;
	specializationInfo.dataSize = sizeof shininess;
	specializationInfo.pData = &shininess;
	shaderStageCreateInfos_composition[1].pSpecializationInfo = &specializationInfo;
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack{};
		pipelineCiPack.createInfo.layout = pipelineLayout_gBuffer;
		pipelineCiPack.createInfo.renderPass = easyVulkan::rpwf_deferredToScreen.renderPass;
		pipelineCiPack.createInfo.subpass = 0;
		pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof vertex, VK_VERTEX_INPUT_RATE_VERTEX);
		pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, position));
		pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, normal));
		pipelineCiPack.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex, albedoSpecular));
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.0f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.depthStencilStateCi.depthTestEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthWriteEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineCiPack.colorBlendAttachmentStates.resize(3);
		pipelineCiPack.colorBlendAttachmentStates[0] = pipelineCiPack.colorBlendAttachmentStates[1] = pipelineCiPack.colorBlendAttachmentStates[2] = {
			false,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
			0b1111
		};
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_gBuffer;
		pipeline_gBuffer.Create(pipelineCiPack.createInfo);
		pipelineCiPack.createInfo.layout = pipelineLayout_composition;
		pipelineCiPack.createInfo.subpass = 1;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_composition;
		pipelineCiPack.vertexInputStateCi.vertexBindingDescriptionCount = 0;
		pipelineCiPack.vertexInputStateCi.vertexAttributeDescriptionCount = 0;
		pipelineCiPack.colorBlendStateCi.attachmentCount = 1;
		pipelineCiPack.colorBlendAttachmentStates[0].blendEnable = true;
		pipeline_composition.Create(pipelineCiPack.createInfo);
	};
	auto Destroy = [] {
		pipeline_gBuffer.~pipeline();
		pipeline_composition.~pipeline();
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(Create);
	graphicsBase::Base().PushCallback_DestroySwapchain(Destroy);
	Create();
}

//Mouse Operation
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

	easyVulkan::CreateRpwf_DeferredToScreen();
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

	//Camera
	vec3 camPos(-2, -2, -2);
	vec3 camUp(0, 1, 0);
	vec3 camFront(1, 1, 1);
	easyVulkan::camera myDogEye(camPos, camUp, camFront);

	//Vertex and index buffer
	vertex vertices[] = {
		{ { 1.f, 1.f,-1.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f, 1.f, 1.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f, 1.f,-1.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f, 1.f, 1.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },

		{ {-1.f, 1.f,-1.f }, { 0.f, 0.f,-1.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f,-1.f,-1.f }, { 0.f, 0.f,-1.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f, 1.f,-1.f }, { 0.f, 0.f,-1.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f,-1.f,-1.f }, { 0.f, 0.f,-1.f }, { 1.f, 1.f, 1.f, 1.f } },

		{ { 1.f, 1.f,-1.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f,-1.f,-1.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f, 1.f, 1.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f,-1.f, 1.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },

		{ { 1.f, 1.f, 1.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f,-1.f, 1.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f, 1.f, 1.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f,-1.f, 1.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },

		{ {-1.f, 1.f, 1.f }, {-1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f,-1.f, 1.f }, {-1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f, 1.f,-1.f }, {-1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f,-1.f,-1.f }, {-1.f, 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },

		{ { 1.f,-1.f,-1.f }, { 0.f,-1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f,-1.f,-1.f }, { 0.f,-1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ { 1.f,-1.f, 1.f }, { 0.f,-1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
		{ {-1.f,-1.f, 1.f }, { 0.f,-1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
	};
	uint16_t indices[36] = { 0, 1, 2, 2, 1, 3 };
	for (size_t i = 1; i < 6; i++)
		for (size_t j = 0; j < 6; j++)
			indices[i * 6 + j] = indices[j] + i * 4;
	vertexBuffer vertexBufer_cube(sizeof vertices);
	vertexBufer_cube.TransferData(vertices);
	indexBuffer indexBuffer_cube(sizeof indices);
	indexBuffer_cube.TransferData(indices);

	//Uniform buffer
	mat4 projView[2]{};
	projView[0] = { FlipVertical(glm::infinitePerspective(60._d2r, float(windowSize.width) / windowSize.height, 0.1f)) };
	uniformBuffer uniformBuffer_projView(sizeof projView);

	mat4 models[3]{};
	models[0] = glm::scale(mat4(1), vec3(0.5f, 0.5f, 0.5f));
	models[1] = glm::translate(mat4(1), vec3(1.5f, 0.0f, 0.0f)) * glm::rotate(mat4(1), 45._d2r, vec3(0.f, 1.0f, 0.0f)) * models[0];
	models[2] = glm::translate(mat4(1), vec3(-1.5f, 0.0f, 0.0f)) * glm::rotate(mat4(1), 30._d2r, vec3(1.f, 1.0f, 1.0f)) * models[0];
	VkDeviceSize uniformBufferAlignment = graphicsBase::Base().PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	uniformBufferAlignment *= uint32_t(std::ceil(float(sizeof mat4) / uniformBufferAlignment));
	//Or you can just:
	//uniformBufferAlignment = (uniformBufferAlignment + sizeof mat4 - 1) & ~(uniformBufferAlignment - 1)
	uniformBuffer uniformBuffer_models(uniformBufferAlignment * std::size(models));
	{
		vulkan::bufferMemory bufferMemory_staging(sizeof models, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		bufferMemory_staging.BufferData(models, sizeof models, 0);
		vulkan::fence fence;
		commandBuffer_transfer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VkBufferCopy regions[std::size(models)];
		for (size_t i = 0; i < std::size(models); i++)
			regions[i] = { sizeof mat4 * i, uniformBufferAlignment * i, sizeof mat4 };
		vkCmdCopyBuffer(commandBuffer_transfer, bufferMemory_staging.Buffer(), uniformBuffer_models, std::size(models), regions);
		commandBuffer_transfer.End();
		graphicsBase::Base().SubmitTransferCommandBuffer(commandBuffer_transfer, fence);
		fence.Wait();
	}

	struct {
		alignas(16) vec3 cameraPosition;
		int lightCount;
		struct {
			alignas(16) vec3 position;
			alignas(16) vec3 color;
			float strength;
		} lights[8];
	} lightInfo;
	lightInfo.lightCount = 6;
	lightInfo.lights[0] = { {-3.f, 2.f, 2.f }, { 1.f, 0.f, 0.f }, 10.f };
	lightInfo.lights[1] = { { 0.f, 2.f, 2.f }, { 0.f, 1.f, 0.f }, 10.f };
	lightInfo.lights[2] = { { 3.f, 2.f, 2.f }, { 0.f, 0.f, 1.f }, 10.f };
	lightInfo.lights[3] = { {-3.f,-2.f,-2.f }, { 0.f, 1.f, 1.f }, 10.f };
	lightInfo.lights[4] = { { 0.f,-2.f,-2.f }, { 1.f, 0.f, 1.f }, 10.f };
	lightInfo.lights[5] = { { 3.f,-2.f,-2.f }, { 1.f, 1.f, 0.f }, 10.f };
	uniformBuffer uniformBuffer_lights(sizeof lightInfo);
	uniformBuffer_lights.TransferData(lightInfo);

	//Descriptor set
	VkDescriptorPoolSize descriptorPoolSizes[3]{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3 },
	};
	vulkan::descriptorPool descriptorPool(0/*No flags, no need to free*/, 3, descriptorPoolSizes);
	vulkan::descriptorSet descriptorSet_projView;
	vulkan::descriptorSet descriptorSet_models;
	static vulkan::descriptorSet descriptorSet_composition;//Static, in order to use a lambda as callback.
	descriptorPool.AllocateDescriptorSet((VkDescriptorSet&)descriptorSet_projView, descriptorSetLayouts_gBuffer[0]);
	descriptorPool.AllocateDescriptorSet((VkDescriptorSet&)descriptorSet_models, descriptorSetLayouts_gBuffer[1]);
	descriptorPool.AllocateDescriptorSet((VkDescriptorSet&)descriptorSet_composition, descriptorSetLayout_composition);
	VkWriteDescriptorSet writeDescriptorSets[3]{};
	VkDescriptorBufferInfo bufferInfos[3]{
		{ uniformBuffer_projView, 0, VK_WHOLE_SIZE },
		{ uniformBuffer_models, 0, VK_WHOLE_SIZE },
		{ uniformBuffer_lights, 0, VK_WHOLE_SIZE }
	};
	writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[0].dstSet = descriptorSet_projView;
	writeDescriptorSets[0].descriptorCount = 1;
	writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[0].pBufferInfo = bufferInfos;
	writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[1].dstSet = descriptorSet_models;
	writeDescriptorSets[1].descriptorCount = 1;
	writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescriptorSets[1].pBufferInfo = bufferInfos + 1;
	writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSets[2].dstSet = descriptorSet_composition;
	writeDescriptorSets[2].dstBinding = 3;
	writeDescriptorSets[2].descriptorCount = 1;
	writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSets[2].pBufferInfo = bufferInfos + 2;
	vkUpdateDescriptorSets(graphicsBase::Base().Device(), 3, writeDescriptorSets, 0, nullptr);
	auto UpdateDescriptorSet_InputAttachments = [] {
		VkWriteDescriptorSet writeDescriptorSets[3]{};
		VkDescriptorImageInfo imageInfos[3] = {
			{ VK_NULL_HANDLE, easyVulkan::ca_deferredToScreen_position.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ VK_NULL_HANDLE, easyVulkan::ca_deferredToScreen_normal.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ VK_NULL_HANDLE, easyVulkan::ca_deferredToScreen_albedoSpecular.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		};
		writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[0].dstSet = descriptorSet_composition;
		writeDescriptorSets[0].dstBinding = 0;
		writeDescriptorSets[0].descriptorCount = 1;
		writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		writeDescriptorSets[0].pImageInfo = imageInfos;
		writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[1].dstSet = descriptorSet_composition;
		writeDescriptorSets[1].dstBinding = 1;
		writeDescriptorSets[1].descriptorCount = 1;
		writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		writeDescriptorSets[1].pImageInfo = imageInfos + 1;
		writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSets[2].dstSet = descriptorSet_composition;
		writeDescriptorSets[2].dstBinding = 2;
		writeDescriptorSets[2].descriptorCount = 1;
		writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		writeDescriptorSets[2].pImageInfo = imageInfos + 2;
		vkUpdateDescriptorSets(graphicsBase::Base().Device(), 3, writeDescriptorSets, 0, nullptr);
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(UpdateDescriptorSet_InputAttachments);
	UpdateDescriptorSet_InputAttachments();

	//Clear value
	VkClearValue clearValues[5]{
		{.color = {} },
		{.color = {} },
		{.color = {} },
		{.color = {} },
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
		projView[1] = myDogEye.RecalculateView();
		lightInfo.cameraPosition = myDogEye.Position();

		//Update
		fence.WaitAndReset();
		commandBuffer_transfer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		uniformBuffer_projView.CmdUpdateBuffer(commandBuffer_transfer, projView);
		uniformBuffer_lights.CmdUpdateBuffer(commandBuffer_transfer, &lightInfo.cameraPosition, sizeof lightInfo.cameraPosition, 0);
		commandBuffer_transfer.End();
		graphicsBase::Base().SubmitTransferCommandBuffer(commandBuffer_transfer, fence);

		//Render
		fence.WaitAndReset();
		commandBuffer_graphics.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		easyVulkan::rpwf_deferredToScreen.renderPass.CmdBegin(commandBuffer_graphics, easyVulkan::rpwf_deferredToScreen.framebuffers[i], clearValues);

		vkCmdBindPipeline(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_gBuffer);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer_graphics, 0, 1, (VkBuffer*)&vertexBufer_cube, &offset);
		vkCmdBindIndexBuffer(commandBuffer_graphics, indexBuffer_cube, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_gBuffer, 0, 1, (VkDescriptorSet*)&descriptorSet_projView, 0, nullptr);
		uint32_t dynamicOffset = 0;
		for (size_t i = 0; i < std::size(models); i++)
			dynamicOffset = i * uniformBufferAlignment,
			vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_gBuffer, 1, 1, (VkDescriptorSet*)&descriptorSet_models, 1, &dynamicOffset),
			vkCmdDrawIndexed(commandBuffer_graphics, 36, 1, 0, 0, 0);

		vkCmdNextSubpass(commandBuffer_graphics, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_composition);
		vkCmdBindDescriptorSets(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_composition, 0, 1, (VkDescriptorSet*)&descriptorSet_composition, 0, nullptr);
		vkCmdDrawIndexed(commandBuffer_graphics, 6, 1, 0, 0, 0);

		easyVulkan::rpwf_deferredToScreen.renderPass.CmdEnd(commandBuffer_graphics);
		commandBuffer_graphics.End();

		graphicsBase::Base().SubmitGraphicsCommandBuffer(commandBuffer_graphics, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);

		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);
		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}