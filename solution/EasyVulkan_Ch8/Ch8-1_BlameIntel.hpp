#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
using namespace vulkan;

pipelineLayout pipelineLayout_line;
pipeline pipeline_line;
descriptorSetLayout descriptorSetLayout_texture;
pipelineLayout pipelineLayout_screen;
pipeline pipeline_screen;

const auto& RenderPassAndFramebuffers_Screen() {
	static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
	return rpwf;
}
const auto& RenderPassAndFramebuffer_Offscreen(VkExtent2D canvasSize) {
	static const auto& rpwf = easyVulkan::CreateRpwf_Canvas(canvasSize);
	return rpwf;
}
void CreateLayout() {
	//Offscreen
	VkPushConstantRange pushConstantRange_offscreen = { VK_SHADER_STAGE_VERTEX_BIT, 0, 24 };
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange_offscreen,
	};
	pipelineLayout_line.Create(pipelineLayoutCreateInfo);
	//Screen
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
	VkPushConstantRange pushConstantRanges_screen[] = {
		{ VK_SHADER_STAGE_VERTEX_BIT, 0, 16 },
		{ VK_SHADER_STAGE_FRAGMENT_BIT, 8, 8 }
	};
	pipelineLayoutCreateInfo.pushConstantRangeCount = 2;
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges_screen;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout_texture.Address();
	pipelineLayout_screen.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline(VkExtent2D canvasSize) {
	//Offscreen
	static shaderModule vert_offscreen("shader/Line.vert.spv");
	static shaderModule frag_offscreen("shader/Line.frag.spv");
	VkPipelineShaderStageCreateInfo shaderStageCreateInfos_line[2] = {
		vert_offscreen.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_offscreen.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	graphicsPipelineCreateInfoPack pipelineCiPack;
	pipelineCiPack.createInfo.layout = pipelineLayout_line;
	pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffer_Offscreen(canvasSize).renderPass;
	pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(canvasSize.width), float(canvasSize.height), 0.f, 1.f);
	pipelineCiPack.scissors.emplace_back(VkOffset2D{}, canvasSize);
	pipelineCiPack.rasterizationStateCi.lineWidth = 1;
	pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
	pipelineCiPack.UpdateAllArrays();
	pipelineCiPack.createInfo.stageCount = 2;
	pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_line;
	pipeline_line.Create(pipelineCiPack);
	//Screen
	static shaderModule vert_screen("shader/CanvasToScreen.vert.spv");
	static shaderModule frag_screen("shader/CanvasToScreen.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_screen[2] = {
		vert_screen.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_screen.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.layout = pipelineLayout_screen;
		pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers_Screen().renderPass;
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_screen;
		pipeline_screen.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_screen.~pipeline();
	};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
}

int main() {
	if (!InitializeWindow({ 1280, 720 }))
		return -1;

	VkExtent2D canvasSize = windowSize;
	const auto& [renderPass_screen, framebuffers_screen] = RenderPassAndFramebuffers_Screen();
	const auto& [renderPass_offscreen, framebuffer_offscreen] = RenderPassAndFramebuffer_Offscreen(canvasSize);
	CreateLayout();
	CreatePipeline(canvasSize);

	fence fence;
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;

	commandBuffer commandBuffer;
	commandPool commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool.AllocateBuffers(commandBuffer);

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
	descriptorSet_texture.Write(easyVulkan::ca_canvas.DescriptorImageInfo(sampler), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	VkClearValue clearColor = {};

	double mouseX, mouseY;
	glfwGetCursorPos(pWindow, &mouseX, &mouseY);
	struct {
		glm::vec2 viewportSize;
		glm::vec2 offsets[2];
	} pushConstants_offscreen = {
		{ canvasSize.width, canvasSize.height },
		{ { mouseX, mouseY }, { mouseX, mouseY } }
	};

	bool clearCanvas = true;
	bool index = 0;

	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();

		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();

		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		if (clearCanvas)
			easyVulkan::CmdClearCanvas(commandBuffer, VkClearColorValue{}),
			clearCanvas = false;

		//Offscreen
		renderPass_offscreen.CmdBegin(commandBuffer, framebuffer_offscreen, { {}, canvasSize });
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_line);
		vkCmdPushConstants(commandBuffer, pipelineLayout_line, VK_SHADER_STAGE_VERTEX_BIT, 0, 24, &pushConstants_offscreen);
		vkCmdDraw(commandBuffer, 2, 1, 0, 0);
		renderPass_offscreen.CmdEnd(commandBuffer);

		//Screen
		renderPass_screen.CmdBegin(commandBuffer, framebuffers_screen[i], { {}, windowSize }, VkClearValue{ .color = { 1.f, 1.f, 1.f, 1.f } });
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_screen);
		//Explanation (in Chinese) of following if-else statement: https://easyvulkan.github.io/Ch8-1%20%E7%A6%BB%E5%B1%8F%E6%B8%B2%E6%9F%93.html#id8
		if (graphicsBase::Base().PhysicalDeviceProperties().vendorID == 0x8086 &&//Blame Intel
			graphicsBase::Base().PhysicalDeviceProperties().deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
			struct {
				glm::vec2 viewportSize;
				glm::vec2 canvasSize;
			} pushConstants = {
				{ windowSize.width, windowSize.height },
				{ canvasSize.width, canvasSize.height }
			};
			vkCmdPushConstants(commandBuffer, pipelineLayout_screen, VK_SHADER_STAGE_VERTEX_BIT, 0, 16, &pushConstants);
			vkCmdPushConstants(commandBuffer, pipelineLayout_screen, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 8, 8, &pushConstants.canvasSize);
		}
		else {
			glm::vec2 windowSize = { ::windowSize.width, ::windowSize.height };
			vkCmdPushConstants(commandBuffer, pipelineLayout_screen, VK_SHADER_STAGE_VERTEX_BIT, 0, 8, &windowSize);
			vkCmdPushConstants(commandBuffer, pipelineLayout_screen, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 8, 8, &pushConstants_offscreen.viewportSize);
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout_screen, 0, 1, descriptorSet_texture.Address(), 0, nullptr);
		vkCmdDraw(commandBuffer, 4, 1, 0, 0);
		renderPass_screen.CmdEnd(commandBuffer);

		commandBuffer.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
		glfwGetCursorPos(pWindow, &mouseX, &mouseY);
		pushConstants_offscreen.offsets[index = !index] = { mouseX, mouseY };
		clearCanvas = glfwGetMouseButton(pWindow, GLFW_MOUSE_BUTTON_LEFT);
		TitleFps();

		fence.WaitAndReset();
	}
	TerminateWindow();
	return 0;
}