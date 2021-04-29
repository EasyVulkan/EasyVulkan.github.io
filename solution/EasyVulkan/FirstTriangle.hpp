#include "GlfwClientGeneral.hpp"
#include "EasyVulkan.hpp"

using namespace vulkan;
pipelineLayout pipelineLayout_triangle;
pipeline pipeline_triangle;
void CreateLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shader vert_triangle("shader/FirstTriangle.vert.spv");
	static shader frag_triangle("shader/FirstTriangle.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2]{
		vert_triangle.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag_triangle.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack{};
		pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
		pipelineCiPack.createInfo.renderPass = easyVulkan::rpwf_screen.renderPass;
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.0f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.rasterizationStateCi.cullMode = VK_CULL_MODE_BACK_BIT;
		//pipelineCiPack.rasterizationStateCi.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;//Default
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.colorBlendAttachmentStates.emplace_back(
			true,
			VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
			0b1111
		);
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;
		pipeline_triangle.Create(pipelineCiPack.createInfo);
	};
	auto Destroy = [] {
		pipeline_triangle.~pipeline();
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(Create);
	graphicsBase::Base().PushCallback_DestroySwapchain(Destroy);
	Create();
}

int main() {
	using namespace vulkan;
	if (!InitializeWindow({ 1280, 720 }))
		return -1;
	easyVulkan::CreateRpwf_Screen();
	CreateLayout();
	CreatePipeline();

	//Synchronization Object
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;
	fence fence(true);
	//Command Object
	commandPool commandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsBase::Base().QueueFamilyIndex_Graphics());
	commandBuffer commandBuffer_graphics;
	commandPool.AllocateCommandBuffers((VkCommandBuffer*)&commandBuffer_graphics);

	//Clear Color
	VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

	while (!glfwWindowShouldClose(pWindow)) {
		TitleFps();
		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		auto i = graphicsBase::Base().CurrentImageIndex();
		fence.WaitAndReset();

		commandBuffer_graphics.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		easyVulkan::rpwf_screen.renderPass.CmdBegin(commandBuffer_graphics, easyVulkan::rpwf_screen.framebuffers[i], clearColor);

		vkCmdBindPipeline(commandBuffer_graphics, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
		vkCmdDraw(commandBuffer_graphics, 3, 1, 0, 0);

		easyVulkan::rpwf_screen.renderPass.CmdEnd(commandBuffer_graphics);
		commandBuffer_graphics.End();
		graphicsBase::Base().SubmitGraphicsCommandBuffer(commandBuffer_graphics, semaphore_imageIsAvailable, semaphore_renderingIsOver, fence);

		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);
		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}