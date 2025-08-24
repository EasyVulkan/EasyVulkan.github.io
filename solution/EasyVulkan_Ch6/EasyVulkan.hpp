#include "VkBase+.h"

using namespace vulkan;
constexpr const VkExtent2D& windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;

namespace easyVulkan {
	struct renderPassWithFramebuffer {
		renderPass renderPass;
		framebuffer framebuffer;
	};
	const auto& CreateRpwf_Screen_ImagelessFramebuffer() {
		static renderPassWithFramebuffer rpwf;

		VkAttachmentDescription attachmentDescription = {
			.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};
		VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkSubpassDescription subpassDescription = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentReference
		};
		VkSubpassDependency subpassDependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			//You may use VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT if synchronization is done by semaphore.
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
		VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 1,
			.pAttachments = &attachmentDescription,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency
		};
		rpwf.renderPass.Create(renderPassCreateInfo);
		auto CreateFramebuffer = [] {
			VkFramebufferAttachmentImageInfo framebufferAttachmentImageInfo = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
				.usage = graphicsBase::Base().SwapchainCreateInfo().imageUsage,
				.width = windowSize.width,
				.height = windowSize.height,
				.layerCount = 1,
				.viewFormatCount = 1,
				.pViewFormats = &graphicsBase::Base().SwapchainCreateInfo().imageFormat
			};
			if (graphicsBase::Base().SwapchainCreateInfo().flags & VK_SWAPCHAIN_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_KHR)
				framebufferAttachmentImageInfo.flags |= VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT;
			if (graphicsBase::Base().SwapchainCreateInfo().flags & VK_SWAPCHAIN_CREATE_PROTECTED_BIT_KHR)
				framebufferAttachmentImageInfo.flags |= VK_IMAGE_CREATE_PROTECTED_BIT;
			if (graphicsBase::Base().SwapchainCreateInfo().flags & VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR)
				framebufferAttachmentImageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
			VkFramebufferAttachmentsCreateInfo framebufferAttachmentsCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO,
				.attachmentImageInfoCount = 1,
				.pAttachmentImageInfos = &framebufferAttachmentImageInfo
			};
			VkFramebufferCreateInfo framebufferCreateInfo = {
				.pNext = &framebufferAttachmentsCreateInfo,
				.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
				.renderPass = rpwf.renderPass,
				.attachmentCount = 1,
				.width = windowSize.width,
				.height = windowSize.height,
				.layers = 1
			};
			rpwf.framebuffer.Create(framebufferCreateInfo);
		};
		auto DestroyFramebuffer = [] {
			rpwf.framebuffer.~framebuffer();
		};
		CreateFramebuffer();

		ExecuteOnce(rpwf);
		graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffer);
		graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffer);
		return rpwf;
	}
}