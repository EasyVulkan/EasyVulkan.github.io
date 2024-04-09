#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
using namespace vulkan;

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec4 albedoSpecular;
};

descriptorSetLayout descriptorSetLayout_gBuffer;
pipelineLayout pipelineLayout_gBuffer;
pipeline pipeline_gBuffer;
descriptorSetLayout descriptorSetLayout_composition;
pipelineLayout pipelineLayout_composition;
pipeline pipeline_composition;
const auto& RenderPassAndFramebuffers() {
	static const auto& rpwf = easyVulkan::CreateRpwf_DeferredToScreen();
	return rpwf;
}
void CreateLayout() {
	//G-buffer
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding_gBuffer = { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT };
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		.bindingCount = 1,
		.pBindings = &descriptorSetLayoutBinding_gBuffer
	};
	descriptorSetLayout_gBuffer.Create(descriptorSetLayoutCreateInfo);
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.setLayoutCount = 1,
		.pSetLayouts = descriptorSetLayout_gBuffer.Address()
	};
	pipelineLayout_gBuffer.Create(pipelineLayoutCreateInfo);
	//Composition
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings_composition[2] = {
		{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ 1, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, VK_SHADER_STAGE_FRAGMENT_BIT }
	};
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings_composition;
	descriptorSetLayout_composition.Create(descriptorSetLayoutCreateInfo);
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout_composition.Address();
	pipelineLayout_composition.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shaderModule vert_gBuffer("shader/GBuffer.vert.spv");
	static shaderModule frag_gBuffer("shader/GBuffer.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_gBuffer[2] = {
		vert_gBuffer.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_gBuffer.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static shaderModule vert_composition("shader/Composition.vert.spv");
	static shaderModule frag_composition("shader/Composition.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_composition[2] = {
		vert_composition.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_composition.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static constexpr int32_t shininess = 64;
	static VkSpecializationMapEntry mapEntry = { 1, 0, sizeof shininess };
	static VkSpecializationInfo specializationInfo = {
		.mapEntryCount = 1,
		.pMapEntries = &mapEntry,
		.dataSize = sizeof shininess,
		.pData = &shininess
	};
	auto Create = [] {
		//G-buffer
		graphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.layout = pipelineLayout_gBuffer;
		pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().renderPass;
		pipelineCiPack.createInfo.subpass = 0;
		pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX);
		pipelineCiPack.vertexInputBindings.emplace_back(1, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_INSTANCE);
		pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, position));
		pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, normal));
		pipelineCiPack.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vertex, albedoSpecular));
		pipelineCiPack.vertexInputAttributes.emplace_back(3, 1, VK_FORMAT_R32G32B32_SFLOAT, 0);
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineCiPack.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.depthStencilStateCi.depthTestEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthWriteEnable = VK_TRUE;
		pipelineCiPack.depthStencilStateCi.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineCiPack.colorBlendAttachmentStates.resize(2);
		pipelineCiPack.colorBlendAttachmentStates[0].colorWriteMask = 0b1111;
		pipelineCiPack.colorBlendAttachmentStates[1].colorWriteMask = 0b1111;
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_gBuffer;
		pipeline_gBuffer.Create(pipelineCiPack);
		//Composition
		pipelineCiPack.createInfo.layout = pipelineLayout_composition;
		pipelineCiPack.createInfo.subpass = 1;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_composition;
		pipelineCiPack.vertexInputStateCi.vertexBindingDescriptionCount = 0;
		pipelineCiPack.vertexInputStateCi.vertexAttributeDescriptionCount = 0;
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineCiPack.colorBlendStateCi.attachmentCount = 1;
		pipeline_composition.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_gBuffer.~pipeline();
		pipeline_composition.~pipeline();
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

	vertex vertices[] = {
		//x+
		{ {  1,  1, -1 }, {  1,  0,  0 }, glm::vec4(1) },
		{ {  1, -1, -1 }, {  1,  0,  0 }, glm::vec4(1) },
		{ {  1,  1,  1 }, {  1,  0,  0 }, glm::vec4(1) },
		{ {  1, -1,  1 }, {  1,  0,  0 }, glm::vec4(1) },
		//x-
		{ { -1,  1,  1 }, { -1,  0,  0 }, glm::vec4(1) },
		{ { -1, -1,  1 }, { -1,  0,  0 }, glm::vec4(1) },
		{ { -1,  1, -1 }, { -1,  0,  0 }, glm::vec4(1) },
		{ { -1, -1, -1 }, { -1,  0,  0 }, glm::vec4(1) },
		//y+
		{ {  1,  1, -1 }, {  0,  1,  0 }, glm::vec4(1) },
		{ {  1,  1,  1 }, {  0,  1,  0 }, glm::vec4(1) },
		{ { -1,  1, -1 }, {  0,  1,  0 }, glm::vec4(1) },
		{ { -1,  1,  1 }, {  0,  1,  0 }, glm::vec4(1) },
		//y-
		{ {  1, -1, -1 }, {  0, -1,  0 }, glm::vec4(1) },
		{ { -1, -1, -1 }, {  0, -1,  0 }, glm::vec4(1) },
		{ {  1, -1,  1 }, {  0, -1,  0 }, glm::vec4(1) },
		{ { -1, -1,  1 }, {  0, -1,  0 }, glm::vec4(1) },
		//z+
		{ {  1,  1,  1 }, {  0,  0,  1 }, glm::vec4(1) },
		{ {  1, -1,  1 }, {  0,  0,  1 }, glm::vec4(1) },
		{ { -1,  1,  1 }, {  0,  0,  1 }, glm::vec4(1) },
		{ { -1, -1,  1 }, {  0,  0,  1 }, glm::vec4(1) },
		//z-
		{ { -1,  1, -1 }, {  0,  0, -1 }, glm::vec4(1) },
		{ { -1, -1, -1 }, {  0,  0, -1 }, glm::vec4(1) },
		{ {  1,  1, -1 }, {  0,  0, -1 }, glm::vec4(1) },
		{ {  1, -1, -1 }, {  0,  0, -1 }, glm::vec4(1) }
	};
	vertexBuffer vertexBuffer_perVertex(sizeof vertices);
	vertexBuffer_perVertex.TransferData(vertices);
	glm::vec3 offsets[] = {
		{ -4, -4,  6 },
		{ -4,  4, 10 },
		{ -4, -4, 14 },
		{ -4,  4, 18 },
		{ -4, -4, 22 },
		{ -4,  4, 26 },
		{  4, -4,  6 },
		{  4,  4, 10 },
		{  4, -4, 14 },
		{  4,  4, 18 },
		{  4, -4, 22 },
		{  4,  4, 26 }
	};
	vertexBuffer vertexBuffer_perInstance(sizeof offsets);
	vertexBuffer_perInstance.TransferData(offsets);
	uint16_t indices[36] = { 0, 1, 2, 2, 1, 3 };
	for (size_t i = 1; i < 6; i++)
		for (size_t j = 0; j < 6; j++)
			indices[i * 6 + j] = indices[j] + i * 4;
	indexBuffer indexBuffer(sizeof indices);
	indexBuffer.TransferData(indices);

	struct {
		glm::mat4 proj = FlipVertical(glm::infinitePerspectiveLH_ZO(glm::radians(60.f), float(windowSize.width) / windowSize.height, 0.1f));
		glm::mat4 view = glm::lookAtLH(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(-1, 0, 0));
		int32_t lightCount;
		struct {
			alignas(16) glm::vec3 position;
			alignas(16) glm::vec3 color;
			float strength;
		} lights[8];
	} descriptorConstants;
	descriptorConstants.lightCount = 3;
	descriptorConstants.lights[0] = { { 0.f,  4.f,  0.f }, { 1.f, 0.f, 0.f }, 100.f };
	descriptorConstants.lights[1] = { { 0.f,  0.f, 16.f }, { 0.f, 1.f, 0.f }, 100.f };
	descriptorConstants.lights[2] = { { 0.f, -4.f,  0.f }, { 0.f, 0.f, 1.f }, 100.f };
	uniformBuffer uniformBuffer(sizeof descriptorConstants);
	uniformBuffer.TransferData(descriptorConstants);

	VkDescriptorPoolSize descriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2 }
	};
	descriptorPool descriptorPool(2, descriptorPoolSizes);
	descriptorSet descriptorSet_gBuffer;
	static descriptorSet descriptorSet_composition;
	descriptorPool.AllocateSets(descriptorSet_gBuffer, descriptorSetLayout_gBuffer);
	descriptorPool.AllocateSets(descriptorSet_composition, descriptorSetLayout_composition);
	VkDescriptorBufferInfo bufferInfos[] = {
		{ uniformBuffer, 0, sizeof(glm::mat4) * 2 },
		{ uniformBuffer, 0, VK_WHOLE_SIZE }
	};
	descriptorSet_gBuffer.Write(bufferInfos[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 0);
	descriptorSet_composition.Write(bufferInfos[1], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 0);
	auto UpdateDescriptorSet_InputAttachments = [] {
		VkDescriptorImageInfo imageInfos[2] = {
			{ VK_NULL_HANDLE, easyVulkan::ca_deferredToScreen_normalZ.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ VK_NULL_HANDLE, easyVulkan::ca_deferredToScreen_albedoSpecular.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		};
		descriptorSet_composition.Write(imageInfos, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, 0);
	};
	graphicsBase::Base().AddCallback_CreateSwapchain(UpdateDescriptorSet_InputAttachments);
	UpdateDescriptorSet_InputAttachments();

	VkClearValue clearValues[4] = {
		{ .color = {} },
		{ .color = {} },
		{ .color = {} },
		{ .depthStencil = { 1.f, 0 } }
	};

	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();
		TitleFps();

		fence.WaitAndReset();
		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();

		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearValues);
		//G-buffer
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_gBuffer);
		VkBuffer buffers[2] = { vertexBuffer_perVertex, vertexBuffer_perInstance };
		VkDeviceSize offsets[2] = {};
		vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_gBuffer, 0, 1, descriptorSet_gBuffer.Address(), 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, 36, 12, 0, 0, 0);
		renderPass.CmdNext(commandBuffer);
		//Composition
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_composition);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_composition, 0, 1, descriptorSet_composition.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer, 4, 1, 0, 0);
		renderPass.CmdEnd(commandBuffer);
		commandBuffer.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}