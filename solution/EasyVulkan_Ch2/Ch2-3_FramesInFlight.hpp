#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
using namespace vulkan;

pipelineLayout pipelineLayout_triangle;
pipeline pipeline_triangle;
const auto& RenderPassAndFramebuffers() {
	static const auto& rpwf = easyVulkan::CreateRpwf_Screen();
	return rpwf;
}
void CreateLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayout_triangle.Create(pipelineLayoutCreateInfo);
}
void CreatePipeline() {
	static shaderModule vert("shader/FirstTriangle.vert.spv");
	static shaderModule frag("shader/FirstTriangle.frag.spv");
	static VkPipelineShaderStageCreateInfo shaderStageCreateInfos_triangle[2] = {
		vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	auto Create = [] {
		graphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
		pipelineCiPack.createInfo.renderPass = RenderPassAndFramebuffers().renderPass;
		pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.f);
		pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
		pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineCiPack.colorBlendAttachmentStates.push_back({ .colorWriteMask = 0b1111 });
		pipelineCiPack.UpdateAllArrays();
		pipelineCiPack.createInfo.stageCount = 2;
		pipelineCiPack.createInfo.pStages = shaderStageCreateInfos_triangle;
		pipeline_triangle.Create(pipelineCiPack);
	};
	auto Destroy = [] {
		pipeline_triangle.~pipeline();
	};
	graphicsBase::Base().AddCallback_CreateSwapchain(Create);
	graphicsBase::Base().AddCallback_DestroySwapchain(Destroy);
	Create();
}

class renderingLoopSynchronization {
protected:
	std::vector<fence> fences;
	std::vector<semaphore> semaphores_imageIsAvailable;
	std::vector<semaphore> semaphores_renderingIsOver;
	uint32_t index_swapImage = 0;
public:
	renderingLoopSynchronization() :
		semaphores_imageIsAvailable(graphicsBase::Base().SwapchainImageCount() + 1),
		semaphores_renderingIsOver(graphicsBase::Base().SwapchainImageCount()) {
		fences.reserve(graphicsBase::Base().SwapchainImageCount());
		for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++)
			fences.emplace_back(VK_FENCE_CREATE_SIGNALED_BIT);
	}
	//Getter
	const fence& Fence() const { return fences[*this]; }
	VkSemaphore Semaphore_ImageIsAvailable() const { return semaphores_imageIsAvailable[index_swapImage]; }
	VkSemaphore Semaphore_RenderingIsOver() const { return semaphores_renderingIsOver[*this]; }
	//Const Function
	operator uint32_t() const { return graphicsBase::Base().CurrentImageIndex(); }
	//Non-const Function
	result_t SwapImage() {
		index_swapImage = (index_swapImage + 1) % semaphores_imageIsAvailable.size();
		return graphicsBase::Base().SwapImage(Semaphore_ImageIsAvailable());
	}
};

int main() {
	if (!InitializeWindow({ 1280, 720 }))
		return -1;

	const auto& [renderPass, framebuffers] = RenderPassAndFramebuffers();
	CreateLayout();
	CreatePipeline();

	renderingLoopSynchronization sync;
	std::vector<commandBuffer> commandBuffers(graphicsBase::Base().SwapchainImageCount());
	commandPool commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (auto& i : commandBuffers)
		commandPool.AllocateBuffers(i);

	VkClearValue clearColor = { .color = { 1.f, 0.f, 0.f, 1.f } };

	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();

		sync.SwapImage();
		sync.Fence().WaitAndReset();

		auto& commandBuffer = commandBuffers[sync];
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		renderPass.CmdBegin(commandBuffer, framebuffers[sync], { {}, windowSize }, clearColor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		renderPass.CmdEnd(commandBuffer);
		commandBuffer.End();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffer, sync.Semaphore_ImageIsAvailable(), sync.Semaphore_RenderingIsOver(), sync.Fence());
		graphicsBase::Base().PresentImage(sync.Semaphore_RenderingIsOver());

		glfwPollEvents();
		TitleFps();
	}
	TerminateWindow();
	return 0;
}