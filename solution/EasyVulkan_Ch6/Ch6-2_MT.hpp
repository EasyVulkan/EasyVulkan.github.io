#include "GlfwGeneral.hpp"
#include "EasyVulkan.hpp"
#include "QueueThread.h"
using namespace vulkan;

pipelineLayout pipelineLayout_triangle;
pipeline pipeline_triangle;
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
		VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &graphicsBase::Base().SwapchainCreateInfo().imageFormat
		};
		graphicsPipelineCreateInfoPack pipelineCiPack;
		pipelineCiPack.createInfo.pNext = &pipelineRenderingCreateInfo;
		pipelineCiPack.createInfo.layout = pipelineLayout_triangle;
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

int main() {
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRendering = ::vkCmdBeginRendering;
	PFN_vkCmdEndRenderingKHR vkCmdEndRendering = ::vkCmdEndRendering;
	graphicsBase::Base().UseLatestApiVersion();
	if (graphicsBase::Base().ApiVersion() < VK_API_VERSION_1_2)
		return -1;
	if (graphicsBase::Base().ApiVersion() < VK_API_VERSION_1_3) {
		graphicsBase::Base().AddDeviceExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		VkPhysicalDeviceDynamicRenderingFeatures physicalDeviceDynamicRenderingFeatures = {
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		};
		graphicsBase::Base().AddNextStructure_PhysicalDeviceFeatures(physicalDeviceDynamicRenderingFeatures);
		if (!InitializeWindow({ 1280, 720 }) ||
			!physicalDeviceDynamicRenderingFeatures.dynamicRendering)
			return -1;
		vkCmdBeginRendering = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(graphicsBase::Base().Device(), "vkCmdBeginRenderingKHR"));
		vkCmdEndRendering = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(graphicsBase::Base().Device(), "vkCmdEndRenderingKHR"));
	}
	else
		if (!InitializeWindow({ 1280, 720 }) ||
			!graphicsBase::Base().PhysicalDeviceVulkan13Features().dynamicRendering)
			return -1;

	queueThread queueThread;

	CreateLayout();
	CreatePipeline();

	fence fence;
	semaphore semaphore_imageIsAvailable;
	semaphore semaphore_renderingIsOver;

	commandBuffer commandBuffers[2];
	commandPool commandPool0(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool commandPool1(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	commandPool0.AllocateBuffers(commandBuffers[0]);
	commandPool1.AllocateBuffers(commandBuffers[1]);

	VkImageMemoryBarrier imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};
	VkRenderingAttachmentInfo colorAttachmentInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = { .color = { 1.f, 0.f, 0.f, 1.f } }
	};
	VkRenderingInfo renderingInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentInfo
	};
	auto BeforeSuspending = [&, imageMemoryBarrier, renderingInfo]() mutable {
		commandBuffers[0].Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = graphicsBase::Base().SwapchainImage(graphicsBase::Base().CurrentImageIndex());
		vkCmdPipelineBarrier(commandBuffers[0],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		renderingInfo.flags = VK_RENDERING_SUSPENDING_BIT;
		renderingInfo.renderArea = { {}, windowSize };
		vkCmdBeginRendering(commandBuffers[0], &renderingInfo);
		vkCmdEndRendering(commandBuffers[0]);

		commandBuffers[0].End();
	};
	auto AfterResuming = [&, imageMemoryBarrier, renderingInfo]() mutable {
		commandBuffers[1].Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		renderingInfo.flags = VK_RENDERING_RESUMING_BIT;
		renderingInfo.renderArea = { {}, windowSize };
		vkCmdBeginRendering(commandBuffers[1], &renderingInfo);
		vkCmdBindPipeline(commandBuffers[1], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_triangle);
		vkCmdDraw(commandBuffers[1], 3, 1, 0, 0);
		vkCmdEndRendering(commandBuffers[1]);

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier.image = graphicsBase::Base().SwapchainImage(graphicsBase::Base().CurrentImageIndex());
		vkCmdPipelineBarrier(
			commandBuffers[1],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		commandBuffers[1].End();
	};

	while (!glfwWindowShouldClose(pWindow)) {
		while (glfwGetWindowAttrib(pWindow, GLFW_ICONIFIED))
			glfwWaitEvents();

		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		colorAttachmentInfo.imageView = graphicsBase::Base().SwapchainImageView(graphicsBase::Base().CurrentImageIndex());

		queueThread.PushWork(AfterResuming);
		BeforeSuspending();
		queueThread.Wait();

		static constexpr VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo = {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = semaphore_imageIsAvailable.Address(),
			.pWaitDstStageMask = &waitDstStage,
			.commandBufferCount = 2,
			.pCommandBuffers = commandBuffers[0].Address(),
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = semaphore_renderingIsOver.Address(),
		};
		graphicsBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
		graphicsBase::Base().PresentImage(semaphore_renderingIsOver);

		glfwPollEvents();
		TitleFps();

		fence.WaitAndReset();
	}
	TerminateWindow();
	return 0;
}