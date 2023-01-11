#include "VkBase+.h"

using namespace vulkan;
const VkExtent2D& windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;

namespace easyVulkan {
	struct renderPassWithFramebuffers {
		renderPass renderPass;
		std::vector<framebuffer> framebuffers;
	};
	const auto& CreateRpwf_Screen() {
		static renderPassWithFramebuffers rpwf_screen;
		if (rpwf_screen.renderPass)
			outStream << std::format("[ easyVulkan ] WARNING\nDon't call CreateRpwf_Screen() twice!\n");
		else {
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
			rpwf_screen.renderPass.Create(renderPassCreateInfo);

			auto CreateFramebuffers = [] {
				rpwf_screen.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
				VkFramebufferCreateInfo framebufferCreateInfo = {
					.renderPass = rpwf_screen.renderPass,
					.attachmentCount = 1,
					.width = windowSize.width,
					.height = windowSize.height,
					.layers = 1
				};
				for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
					VkImageView attachment = graphicsBase::Base().SwapchainImageView(i);
					framebufferCreateInfo.pAttachments = &attachment;
					rpwf_screen.framebuffers[i].Create(framebufferCreateInfo);
				}
			};
			auto DestroyFramebuffers = [] {
				rpwf_screen.framebuffers.clear();
			};
			graphicsBase::Base().PushCallback_CreateSwapchain(CreateFramebuffers);
			graphicsBase::Base().PushCallback_DestroySwapchain(DestroyFramebuffers);
			CreateFramebuffers();
		}
		return rpwf_screen;
	}
	void BootScreen(const char* imagePath, VkFormat imageFormat) {
		//Load image data
		VkExtent2D extent;
		formatInfo formatInfo = FormatInfo(imageFormat);
		if (std::unique_ptr<uint8_t[]> pImageData = texture2d::LoadFile(imagePath, extent, formatInfo))
			stagingBuffer::BufferData_MainThread(pImageData.get(), VkDeviceSize(formatInfo.sizePerPixel) * extent.width * extent.height);
		else
			return;
		//Synchronization objects
		semaphore semaphore_imageIsAvailable;
		fence fence;
		//Acquire swapchain image
		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		//Command buffer
		commandBuffer commandBuffer;
		graphicsBase::Plus().CommandPool_Graphics().AllocateBuffers(commandBuffer);
		//Determine if source image data can be copied to swapchain image directly
		VkExtent2D swapchainImageSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
		bool blit =
			extent.width != swapchainImageSize.width ||
			extent.height != swapchainImageSize.height ||
			imageFormat != graphicsBase::Base().SwapchainCreateInfo().imageFormat;
		//Record commands
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		imageMemory imageMemory;
		if (blit) {
			VkImage image = stagingBuffer::AliasedImage2d_MainThread(imageFormat, extent);
			if (image) {
				VkImageMemoryBarrier imageMemoryBarrier = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					VK_ACCESS_HOST_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					image,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
				};
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
			//Create image if necessary
			else {
				VkImageCreateInfo imageCreateInfo = {
					.imageType = VK_IMAGE_TYPE_2D,
					.format = imageFormat,
					.extent = { extent.width, extent.height, 1 },
					.mipLevels = 1,
					.arrayLayers = 1,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
				};
				imageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				VkBufferImageCopy region_copy = {
					.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
					.imageExtent = { extent.width, extent.height, 1 }
				};
				imageOperation::CmdCopyBufferToImage(commandBuffer,
					stagingBuffer::Buffer_MainThread(),
					imageMemory.Image(),
					region_copy,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
					{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL });
				image = imageMemory.Image();
			}
			//Blit to swapchain image
			VkImageBlit region_blit = {
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				{ {}, { int32_t(extent.width), int32_t(extent.height), 1 } },
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				{ {}, { int32_t(swapchainImageSize.width), int32_t(swapchainImageSize.height), 1 } }
			};
			imageOperation::CmdBlitImage(commandBuffer,
				image,
				graphicsBase::Base().SwapchainImage(graphicsBase::Base().CurrentImageIndex()),
				region_blit,
				{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
				{ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }, VK_FILTER_LINEAR);
		}
		else {
			VkBufferImageCopy region_copy = {
				.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				.imageExtent = { extent.width, extent.height, 1 }
			};
			imageOperation::CmdCopyBufferToImage(commandBuffer,
				stagingBuffer::Buffer_MainThread(),
				graphicsBase::Base().SwapchainImage(graphicsBase::Base().CurrentImageIndex()),
				region_copy,
				{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
				{ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR });
		}
		commandBuffer.End();
		//Submit
		VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo = {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = semaphore_imageIsAvailable.Address(),
			.pWaitDstStageMask = &waitDstStage,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffer.Address()
		};
		graphicsBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
		//Present
		fence.WaitAndReset();
		graphicsBase::Base().PresentImage();
		//Free Command Buffer
		graphicsBase::Plus().CommandPool_Graphics().FreeBuffers(commandBuffer);
	}
}