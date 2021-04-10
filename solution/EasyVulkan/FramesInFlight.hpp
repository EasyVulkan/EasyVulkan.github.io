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
		pipelineCiPack.createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
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
	uint32_t framesInFlightCount = std::min(uint32_t(graphicsBase::Base().SwapchainImageCount()), 3u);//Should not greater than swapchainImageCount.
	std::vector<semaphore> semaphores_imageIsAvailable(framesInFlightCount);
	std::vector<semaphore> semaphores_renderingIsOver(framesInFlightCount);
	std::vector<fence> fences;
	for (size_t i = 0; i < framesInFlightCount; i++)
		fences.emplace_back(true);
	//Command Object
	commandPool commandPool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsBase::Base().QueueFamilyIndex_Graphics());
	std::vector<commandBuffer> commandBuffers_graphics(framesInFlightCount);
	commandPool.AllocateCommandBuffers((VkCommandBuffer*)commandBuffers_graphics.data(), framesInFlightCount);

	//Clear Color
	VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

	while (!glfwWindowShouldClose(pWindow)) {
		TitleFps();
		static uint32_t i = 0;
		graphicsBase::Base().SwapImage(semaphores_imageIsAvailable[i]);
		fences[i].WaitAndReset();

		commandBuffers_graphics[i].Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		easyVulkan::rpwf_screen.renderPass.CmdBegin(commandBuffers_graphics[i], easyVulkan::rpwf_screen.framebuffers[graphicsBase::Base().CurrentImageIndex()], clearColor);

		vkCmdBindPipeline(commandBuffers_graphics[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
		vkCmdDraw(commandBuffers_graphics[i], 3, 1, 0, 0);

		easyVulkan::rpwf_screen.renderPass.CmdEnd(commandBuffers_graphics[i]);
		commandBuffers_graphics[i].End();
		graphicsBase::Base().SubmitGraphicsCommandBuffer(commandBuffers_graphics[i], semaphores_imageIsAvailable[i], semaphores_renderingIsOver[i], fences[i]);

		graphicsBase::Base().PresentImage(semaphores_renderingIsOver[i]);
		i = (i + 1) % framesInFlightCount;

		glfwPollEvents();
	}
	TerminateWindow();
	return 0;
}