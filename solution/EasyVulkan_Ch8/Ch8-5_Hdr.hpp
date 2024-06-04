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
pipeline pipeline_calibration;
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
	VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8 };
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.setLayoutCount = 1,
		.pSetLayouts = descriptorSetLayout_texture.Address(),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange
	};
	pipelineLayout_texture.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shaderModule vert_texture("shader/Texture.vert.spv");
	static shaderModule frag_texture("shader/Texture_Hdr.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_texture[2] = {
		vert_texture.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_texture.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	static shaderModule vert_calibration("shader/RenderToImage2d.vert.spv");
	static shaderModule frag_calibration("shader/HdrCalibration.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_calibration[2] = {
		vert_calibration.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_calibration.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
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
		pipelineCiPack.vertexInputStateCi.vertexBindingDescriptionCount = 0;
		pipelineCiPack.vertexInputStateCi.vertexAttributeDescriptionCount = 0;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_calibration;
		pipeline_calibration.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_texture.~pipeline();
		pipeline_calibration.~pipeline();
	};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
}

#ifdef _WIN32
float GetSdrWhiteLevel() {
	UINT32 pathInfoCount = 1;
	DISPLAYCONFIG_PATH_INFO pathInfo;
	UINT32 modeInfoCount = 1;
	DISPLAYCONFIG_MODE_INFO modeInfo;
	if (LONG result = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathInfoCount, &pathInfo, &modeInfoCount, &modeInfo, nullptr);
		result == ERROR_INSUFFICIENT_BUFFER ||
		result == ERROR_SUCCESS) {
		DISPLAYCONFIG_SDR_WHITE_LEVEL sdrWhiteLevel = {
			{
				.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL,
				.size = sizeof DISPLAYCONFIG_SDR_WHITE_LEVEL,
				.adapterId = pathInfo.targetInfo.adapterId,
				.id = pathInfo.targetInfo.id
			}
		};
		if (DisplayConfigGetDeviceInfo(&sdrWhiteLevel.header) == ERROR_SUCCESS)
			return sdrWhiteLevel.SDRWhiteLevel / 12.5f;
	}
	return 0;
}
float sdrWhiteLevel = GetSdrWhiteLevel();
#else
float sdrWhiteLevel = 203.f;
#endif

int main() {
	PreInitialization_TrySetColorSpaceByOrder(VK_COLOR_SPACE_HDR10_ST2084_EXT);
	if (!InitializeWindow({ 1280, 720 }))
		return -1;

	const auto& [renderPass, framebuffers] = RenderPassAndFramebuffers();
	CreateLayout();
	CreatePipeline();

	fence fence;
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;

	commandBuffer commandBuffer;
	commandPool commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool.AllocateBuffers(commandBuffer);

	//Load image
	texture2d referenceTexture("image/Ch8-5 reference.png", VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, true);
	texture2d texture("image/memorial.hdr", VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT, true);
	//Create sampler
	VkSamplerCreateInfo samplerCreateInfo = texture::SamplerCreateInfo();
	sampler sampler_texture(samplerCreateInfo);
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	sampler sampler_calibration(samplerCreateInfo);
	//Create descriptor
	VkDescriptorPoolSize descriptorPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 }
	};
	descriptorPool descriptorPool(2, descriptorPoolSizes);
	descriptorSet descriptorSet_texture;
	descriptorPool.AllocateSets(descriptorSet_texture, descriptorSetLayout_texture);
	descriptorSet_texture.Write(texture.DescriptorImageInfo(sampler_texture), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorSet descriptorSet_calibration;
	descriptorPool.AllocateSets(descriptorSet_calibration, descriptorSetLayout_texture);
	descriptorSet_calibration.Write(referenceTexture.DescriptorImageInfo(sampler_calibration), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	float halfW = float(texture.Width()) / windowSize.width;
	float halfH = float(texture.Height()) / windowSize.height;
	vertex vertices[] = {
		{ { -halfW, -halfH }, { 0, 0 } },
		{ {  halfW, -halfH }, { 1, 0 } },
		{ { -halfW,  halfH }, { 0, 1 } },
		{ {  halfW,  halfH }, { 1, 1 } }
	};
	vertexBuffer vertexBuffer(sizeof vertices);
	vertexBuffer.TransferData(vertices);

	VkClearValue clearColor = { .color = {} };

	static float brightnessScale = sdrWhiteLevel / 10000;
	glfwSetScrollCallback(pWindow,
		[](GLFWwindow* window, double, double dy) {
			brightnessScale = dy > 0 ?
				std::min(brightnessScale + dy * 10.f / 10000, 1.0) :
				std::max(brightnessScale + dy * 10.f / 10000, 0.0);
		});
	bool showHdrTexture = false;

	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();

		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();

		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer, framebuffers[i], { {}, windowSize }, clearColor);
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffer.Address(), &offset);
		if (showHdrTexture) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_texture);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_texture, 0, 1, descriptorSet_texture.Address(), 0, nullptr);
			vkCmdPushConstants(commandBuffer, pipelineLayout_texture, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8, &brightnessScale);
			vkCmdDraw(commandBuffer, 4, 1, 0, 0);
		}
		else {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_calibration);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_texture, 0, 1, descriptorSet_calibration.Address(), 0, nullptr);
			vkCmdPushConstants(commandBuffer, pipelineLayout_texture, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8, &brightnessScale);
			vkCmdDraw(commandBuffer, 4, 1, 0, 0);
		}
		renderPass.CmdEnd(commandBuffer);
		commandBuffer.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
		if (glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT))
			showHdrTexture = true;
		TitleFps();

		fence.WaitAndReset();
	}
	TerminateWindow();
	return 0;
}