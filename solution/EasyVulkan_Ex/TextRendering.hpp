#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
#include "ChimeImpl.hpp"
#include "QueueThread.h"
#include "Timer.h"
using namespace vulkan;

auto& swapchainImageExtent = graphicsBase::Base().SwapchainCreateInfo().imageExtent;

struct ToScreen {
	static void CmdBeginRendering(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex) {
		//Transition image layout before rendering
		VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = graphicsBase::Base().SwapchainImage(swapchainImageIndex),
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkRenderingAttachmentInfo colorAttachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = graphicsBase::Base().SwapchainImageView(swapchainImageIndex),
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE
		};
		VkRenderingInfo renderingInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = { {}, windowSize },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachmentInfo
		};
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
	}
	static void CmdEndRendering(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex) {
		vkCmdEndRendering(commandBuffer);

		//Transition image layout after rendering
		VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = graphicsBase::Base().SwapchainImage(swapchainImageIndex),
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}
};

int main() {
	using namespace chime;

	ttfLoader ttfLoader;
	queueThread thread;
	thread.PushWork([&] { timer t; ttfLoader.LoadTtf(R"(resource\Ancient Medium.ttf)", 512, 8); });

	graphicsBase::Base().UseLatestApiVersion();
	PreInitialization_EnableSrgb();
	InitializeWindow({ 1280, 720 });
	graphics::Base().Initialize();

	renderingLoopSynchronization sync;
	std::vector<commandBuffer> commandBuffers(graphicsBase::Base().SwapchainImageCount());
	commandPool commandPool(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (auto& i : commandBuffers)
		commandPool.AllocateBuffers(i);

	thread.Wait();
	font font(ttfLoader);
	textPrinter printer(256, 64, 1, 0, float(swapchainImageExtent.width), 0xff0000ff);

	while (!glfwWindowShouldClose(pWindow)) {
		TitleFps();

		sync.SwapImage();
		sync.Fence().WaitAndReset();
		commandBuffers[sync].Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		ToScreen::CmdBeginRendering(commandBuffers[sync], sync);

		graphics::Base().CurrentContext(commandBuffers[sync], 0, graphicsBase::Base().SwapchainCreateInfo().imageFormat, VK_SAMPLE_COUNT_1_BIT, graphicsBase::Base().SwapchainCreateInfo().imageExtent);

		printer.FontHeight(128);
		float baseline = printer.GetBaseline(0, font);
		printer.Print({ 0, baseline }, u"Font name: ", font);
		font.FontNameEncodingIsU16() ?
			printer.Print({ 0, baseline }, font.FontName().U16StringView(), font) :
			printer.Print({ 0, baseline }, font.FontName(), font);

		printer.ResetPrintArea();
		printer.FontHeight(64);
		baseline /= 2;
		textPrinter::fnPrint deferredPrinting;
		float printAreaHeight = printer.Print_RightAligned(deferredPrinting, u"Text Rendering Sample \u00a9 2026 Citrus Qiao ", font);
		deferredPrinting({ swapchainImageExtent.width + 0.f, swapchainImageExtent.height - printAreaHeight + baseline });

		ToScreen::CmdEndRendering(commandBuffers[sync], sync);
		commandBuffers[sync].End();
		printer.UpdateApiData();

		graphicsBase::Base().SubmitCommandBuffer_Graphics(commandBuffers[sync], sync.Semaphore_ImageIsAvailable(), sync.Semaphore_RenderingIsOver(), sync.Fence());
		graphicsBase::Base().PresentImage(sync.Semaphore_RenderingIsOver());

		glfwPollEvents();
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();
	}
	TerminateWindow();
	return 0;
}