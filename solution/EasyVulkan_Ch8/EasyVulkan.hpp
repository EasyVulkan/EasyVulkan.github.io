#include "VkBase+.h"

using namespace vulkan;
const VkExtent2D& windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;

namespace easyVulkan {
	struct renderPassWithFramebuffer {
		renderPass renderPass;
		framebuffer framebuffer;
	};
	struct renderPassWithFramebuffers {
		renderPass renderPass;
		std::vector<framebuffer> framebuffers;
	};

	const auto& CreateRpwf_Screen() {
		static renderPassWithFramebuffers rpwf;
		if (rpwf.renderPass)
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
			rpwf.renderPass.Create(renderPassCreateInfo);

			auto CreateFramebuffers = [] {
				rpwf.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
				VkFramebufferCreateInfo framebufferCreateInfo = {
					.renderPass = rpwf.renderPass,
					.attachmentCount = 1,
					.width = windowSize.width,
					.height = windowSize.height,
					.layers = 1
				};
				for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
					VkImageView attachment = graphicsBase::Base().SwapchainImageView(i);
					framebufferCreateInfo.pAttachments = &attachment;
					rpwf.framebuffers[i].Create(framebufferCreateInfo);
				}
			};
			auto DestroyFramebuffers = [] {
				rpwf.framebuffers.clear();
			};
			graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
			graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);
			CreateFramebuffers();
		}
		return rpwf;
	}
	void BootScreen(const char* imagePath, VkFormat imageFormat) {
		VkExtent2D imageExtent;
		std::unique_ptr<uint8_t[]> pImageData = texture2d::LoadFile(imagePath, imageExtent, FormatInfo(imageFormat));
		if (!pImageData)
			return;
		stagingBuffer::BufferData_MainThread(pImageData.get(), FormatInfo(imageFormat).sizePerPixel * imageExtent.width * imageExtent.height);

		semaphore semaphore_imageIsAvailable;
		fence fence;
		commandBuffer commandBuffer;
		graphicsBase::Plus().CommandPool_Graphics().AllocateBuffers(commandBuffer);

		graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VkExtent2D swapchainImageSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
		bool blit =
			imageExtent.width != swapchainImageSize.width ||
			imageExtent.height != swapchainImageSize.height ||
			imageFormat != graphicsBase::Base().SwapchainCreateInfo().imageFormat;
		imageMemory imageMemory;
		if (blit) {
			VkImage image = stagingBuffer::AliasedImage2d_MainThread(imageFormat, imageExtent);
			if (image) {
				VkImageMemoryBarrier imageMemoryBarrier = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					0,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					image,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
				};
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
			else {
				VkImageCreateInfo imageCreateInfo = {
					.imageType = VK_IMAGE_TYPE_2D,
					.format = imageFormat,
					.extent = { imageExtent.width, imageExtent.height, 1 },
					.mipLevels = 1,
					.arrayLayers = 1,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
				};
				imageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				VkBufferImageCopy region_copy = {
					.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
					.imageExtent = imageCreateInfo.extent
				};
				imageOperation::CmdCopyBufferToImage(commandBuffer,
					stagingBuffer::Buffer_MainThread(),
					imageMemory.Image(),
					region_copy,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
					{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL });

				image = imageMemory.Image();
			}
			VkImageBlit region_blit = {
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
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
				.imageExtent = { imageExtent.width, imageExtent.height, 1 }
			};
			imageOperation::CmdCopyBufferToImage(commandBuffer,
				stagingBuffer::Buffer_MainThread(),
				graphicsBase::Base().SwapchainImage(graphicsBase::Base().CurrentImageIndex()),
				region_copy,
				{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
				{ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR });
		}
		commandBuffer.End();

		VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo = {
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = semaphore_imageIsAvailable.Address(),
			.pWaitDstStageMask = &waitDstStage,
			.commandBufferCount = 1,
			.pCommandBuffers = commandBuffer.Address()
		};
		graphicsBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
		fence.WaitAndReset();
		graphicsBase::Base().PresentImage();

		graphicsBase::Plus().CommandPool_Graphics().FreeBuffers(commandBuffer);
	}

	colorAttachment ca_canvas;
	const auto& CreateRpwf_Canvas(VkExtent2D canvasSize) {
		static renderPassWithFramebuffer rpwf;
		ExecuteOnce(rpwf);

		//When this render pass begins, the image keeps its contents.
		VkAttachmentDescription attachmentDescription = {
			.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkSubpassDescription subpassDescription = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentReference
		};
		VkSubpassDependency subpassDependencies[2] = {
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				//You may use VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT if synchronization is done by fence.
				.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT },
			{
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT }
		};
		VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 1,
			.pAttachments = &attachmentDescription,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 2,
			.pDependencies = subpassDependencies,
		};
		rpwf.renderPass.Create(renderPassCreateInfo);

		ca_canvas.Create(graphicsBase::Base().SwapchainCreateInfo().imageFormat, canvasSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		VkFramebufferCreateInfo framebufferCreateInfo = {
			.renderPass = rpwf.renderPass,
			.attachmentCount = 1,
			.pAttachments = ca_canvas.AddressOfImageView(),
			.width = canvasSize.width,
			.height = canvasSize.height,
			.layers = 1
		};
		rpwf.framebuffer.Create(framebufferCreateInfo);
		return rpwf;
	}
	void CmdClearCanvas(VkCommandBuffer commandBuffer, VkClearColorValue clearColor) {
		//Call this function before rpwf.renderPass begins.
		VkImageSubresourceRange imageSubresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = ca_canvas.Image(),
			.subresourceRange = imageSubresourceRange
		};
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		vkCmdClearColorImage(commandBuffer, ca_canvas.Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageSubresourceRange);
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = 0;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
			0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	std::vector<depthStencilAttachment> dsas_screenWithDS;
	const auto& CreateRpwf_ScreenWithDS(VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT) {
		static renderPassWithFramebuffers rpwf;
		ExecuteOnce(rpwf);
		static VkFormat _depthStencilFormat = depthStencilFormat;

		VkAttachmentDescription attachmentDescriptions[2] = {
			{//Color attachment
				.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
			{//Depth stencil attachment
				.format = _depthStencilFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = _depthStencilFormat != VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = _depthStencilFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};
		VkAttachmentReference attachmentReferences[2] = {
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			//Unless the separateDepthStencilLayouts feature is enabled, even if depthStencilFormat doesn't support stencil, layout must be VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
			{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};
		VkSubpassDescription subpassDescription = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = attachmentReferences,
			.pDepthStencilAttachment = attachmentReferences + 1
		};
		//At EARLY_FRAGMENT_TESTS stage, the ds image'll be cleared (if performs) then readed, ds tests are performed for each fragment.
		//At LATE_FRAGMENT_TESTS stage, ds tests are performed for each sample.
		VkSubpassDependency subpassDependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,//Because of VK_ATTACHMENT_LOAD_OP_CLEAR
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
		VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 2,
			.pAttachments = attachmentDescriptions,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency
		};
		rpwf.renderPass.Create(renderPassCreateInfo);

		auto CreateFramebuffers = [] {
			dsas_screenWithDS.resize(graphicsBase::Base().SwapchainImageCount());
			rpwf.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
			for (auto& i : dsas_screenWithDS)
				i.Create(_depthStencilFormat, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
			VkFramebufferCreateInfo framebufferCreateInfo = {
				.renderPass = rpwf.renderPass,
				.attachmentCount = 2,
				.width = windowSize.width,
				.height = windowSize.height,
				.layers = 1
			};
			for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
				VkImageView attachments[2] = {
					graphicsBase::Base().SwapchainImageView(i),
					dsas_screenWithDS[i].ImageView()
				};
				framebufferCreateInfo.pAttachments = attachments;
				rpwf.framebuffers[i].Create(framebufferCreateInfo);
			}
		};
		auto DestroyFramebuffers = [] {
			dsas_screenWithDS.clear();
			rpwf.framebuffers.clear();
		};
		graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
		graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);
		CreateFramebuffers();
		return rpwf;
	}

	colorAttachment ca_deferredToScreen_normalZ;
	colorAttachment ca_deferredToScreen_albedoSpecular;
	depthStencilAttachment dsa_deferredToScreen;
	const auto& CreateRpwf_DeferredToScreen(VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT) {
		static renderPassWithFramebuffers rpwf;
		ExecuteOnce(rpwf);
		static VkFormat _depthStencilFormat = depthStencilFormat;
		VkAttachmentDescription attachmentDescriptions[4] = {
			{//Swapchain attachment
				.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
			{//Deferred normal & z attachment
				.format = VK_FORMAT_R16G16B16A16_SFLOAT,//Or VK_FORMAT_R32G32B32A32_SFLOAT
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{//Deffered albedo & specular attachment
				.format = VK_FORMAT_R8G8B8A8_UNORM,//The only difference from above
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{//Depth stencil attachment
				.format = _depthStencilFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = _depthStencilFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};
		VkAttachmentReference attachmentReferences_subpass0[3] = {
			{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
		};
		VkAttachmentReference attachmentReferences_subpass1[3] = {
			{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
		};
		VkSubpassDescription subpassDescriptions[2] = {
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 2,
				.pColorAttachments = attachmentReferences_subpass0,
				.pDepthStencilAttachment = attachmentReferences_subpass0 + 2 },
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.inputAttachmentCount = 2,
				.pInputAttachments = attachmentReferences_subpass1,
				.colorAttachmentCount = 1,
				.pColorAttachments = attachmentReferences_subpass1 + 2 }
		};
		VkSubpassDependency subpassDependencies[2] = {
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT },
			{
				.srcSubpass = 0,
				.dstSubpass = 1,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT }
		};
		VkRenderPassCreateInfo renderPassCreateInfo = {
			.attachmentCount = 4,
			.pAttachments = attachmentDescriptions,
			.subpassCount = 2,
			.pSubpasses = subpassDescriptions,
			.dependencyCount = 2,
			.pDependencies = subpassDependencies
		};
		rpwf.renderPass.Create(renderPassCreateInfo);
		auto CreateFramebuffers = [] {
			rpwf.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
			ca_deferredToScreen_normalZ.Create(VK_FORMAT_R16G16B16A16_SFLOAT, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
			ca_deferredToScreen_albedoSpecular.Create(VK_FORMAT_R8G8B8A8_UNORM, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
			dsa_deferredToScreen.Create(_depthStencilFormat, windowSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
			VkImageView attachments[4] = {
				VK_NULL_HANDLE,
				ca_deferredToScreen_normalZ.ImageView(),
				ca_deferredToScreen_albedoSpecular.ImageView(),
				dsa_deferredToScreen.ImageView()
			};
			VkFramebufferCreateInfo framebufferCreateInfo = {
				.renderPass = rpwf.renderPass,
				.attachmentCount = 4,
				.pAttachments = attachments,
				.width = windowSize.width,
				.height = windowSize.height,
				.layers = 1
			};
			for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++)
				attachments[0] = graphicsBase::Base().SwapchainImageView(i),
				rpwf.framebuffers[i].Create(framebufferCreateInfo);
		};
		auto DestroyFramebuffers = [] {
			ca_deferredToScreen_normalZ.~colorAttachment();
			ca_deferredToScreen_albedoSpecular.~colorAttachment();
			dsa_deferredToScreen.~depthStencilAttachment();
			rpwf.framebuffers.clear();
		};
		graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
		graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);
		CreateFramebuffers();
		return rpwf;
	}

	std::vector<colorAttachment> cas_msaaToScreen;
	const auto& CreateRpwf_MsaaToScreen(VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_4_BIT) {
		static renderPassWithFramebuffers rpwf;
		ExecuteOnce(rpwf);
		static VkSampleCountFlagBits _sampleCount = sampleCount;
		VkAttachmentDescription attachmentDescriptions[2] = {
			{//Swapchain attachment, used as resolve attachment
				.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR },
			{//Msaa color attachment
				.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat,//Must be the same format as above
				.samples = sampleCount,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
		};
		VkAttachmentReference attachmentReferences[2] = {
			{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
		};
		VkSubpassDescription subpassDescription = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = attachmentReferences + 1,
			.pResolveAttachments = attachmentReferences
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
			.attachmentCount = 2,
			.pAttachments = attachmentDescriptions,
			.subpassCount = 1,
			.pSubpasses = &subpassDescription,
			.dependencyCount = 1,
			.pDependencies = &subpassDependency
		};
		rpwf.renderPass.Create(renderPassCreateInfo);
		auto CreateFramebuffers = [] {
			cas_msaaToScreen.resize(graphicsBase::Base().SwapchainImageCount());
			rpwf.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
			for (auto& i : cas_msaaToScreen)
				i.Create(graphicsBase::Base().SwapchainCreateInfo().imageFormat, windowSize, 1, _sampleCount, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
			VkFramebufferCreateInfo framebufferCreateInfo = {
				.renderPass = rpwf.renderPass,
				.attachmentCount = 2,
				.width = windowSize.width,
				.height = windowSize.height,
				.layers = 1
			};
			for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
				VkImageView attachments[2] = {
					graphicsBase::Base().SwapchainImageView(i),
					cas_msaaToScreen[i].ImageView(),
				};
				framebufferCreateInfo.pAttachments = attachments;
				rpwf.framebuffers[i].Create(framebufferCreateInfo);
			}
		};
		auto DestroyFramebuffers = [] {
			cas_msaaToScreen.clear();
			rpwf.framebuffers.clear();
		};
		graphicsBase::Base().AddCallback_CreateSwapchain(CreateFramebuffers);
		graphicsBase::Base().AddCallback_DestroySwapchain(DestroyFramebuffers);
		CreateFramebuffers();
		return rpwf;
	}

	using callback_copyData_t = void(*)(const void* pData, VkDeviceSize dataSize);
	class fCreateTexture2d_multiplyAlpha {
	protected:
		VkFormat format_final = VK_FORMAT_UNDEFINED;
		bool generateMipmap = false;
		callback_copyData_t callback_copyData = nullptr;
		renderPass renderPass;
		pipelineLayout pipelineLayout;
		pipeline pipeline;
		//--------------------
		void CmdTransferDataToImage(VkCommandBuffer commandBuffer, const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, imageMemory& imageMemory_conversion, VkImage image) const {
			static constexpr imageOperation::imageMemoryBarrierParameterPack imbs[2] = {
				{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
				{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
			};
			VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format_initial).sizePerPixel) * extent.width * extent.height;
			stagingBuffer::BufferData_MainThread(pImageData, imageDataSize);
			VkImage image_copyTo = VK_NULL_HANDLE;
			VkImage image_conversion = VK_NULL_HANDLE;
			VkImage image_blitTo = VK_NULL_HANDLE;
			if (format_initial == format_final)
				image_copyTo = image;
			else {
				if (!(image_conversion = stagingBuffer::AliasedImage2d_MainThread(format_initial, extent))) {
					VkImageCreateInfo imageCreateInfo = {
						.imageType = VK_IMAGE_TYPE_2D,
						.format = format_initial,
						.extent = { extent.width, extent.height, 1 },
						.mipLevels = 1,
						.arrayLayers = 1,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
					};
					imageMemory_conversion.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					image_copyTo = image_conversion = imageMemory_conversion.Image();
				}
				image_blitTo = image;
			}
			if (image_copyTo) {
				VkBufferImageCopy region = {
					.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
					.imageExtent = { extent.width, extent.height, 1 }
				};
				imageOperation::CmdCopyBufferToImage(commandBuffer, stagingBuffer::Buffer_MainThread(), image_copyTo, region,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[bool(image_blitTo)]);
			}
			if (image_blitTo) {
				if (!image_copyTo) {
					VkImageMemoryBarrier imageMemoryBarrier = {
						VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						nullptr,
						0,
						VK_ACCESS_TRANSFER_READ_BIT,
						VK_IMAGE_LAYOUT_PREINITIALIZED,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						image_conversion,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
					};
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}
				VkImageBlit region = {
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
					{ {}, { int32_t(extent.width), int32_t(extent.height), 1 } },
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
					{ {}, { int32_t(extent.width), int32_t(extent.height), 1 } }
				};
				imageOperation::CmdBlitImage(commandBuffer, image_conversion, image_blitTo, region,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
					{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			}
		}
		//Static
		static constexpr const char* filepath_vert = "shader/RenderToImage2d_NoUV.vert.spv";
		static constexpr const char* filepath_frag = "shader/RenderNothing.frag.spv";
		//Static Function
		static const VkPipelineShaderStageCreateInfo Ssci_Vert() {
			static shaderModule shader;
			if (!shader) {
				shader.Create(filepath_vert);
				ExecuteOnce(shader.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT));
				graphicsBase::Base().AddCallback_DestroyDevice([] { shader.~shaderModule(); });
			}
			return shader.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
		}
		static const VkPipelineShaderStageCreateInfo Ssci_Frag() {
			static shaderModule shader;
			if (!shader) {
				shader.Create(filepath_frag);
				ExecuteOnce(shader.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT));
				graphicsBase::Base().AddCallback_DestroyDevice([] { shader.~shaderModule(); });
			}
			return shader.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
		}
	public:
		fCreateTexture2d_multiplyAlpha() = default;
		fCreateTexture2d_multiplyAlpha(VkFormat format_final, bool generateMipmap, callback_copyData_t callback_copyMipLevel0) {
			Instantiate(format_final, generateMipmap, callback_copyMipLevel0);
		}
		fCreateTexture2d_multiplyAlpha(fCreateTexture2d_multiplyAlpha&&) = default;
		//Const Function
		texture2d operator()(const char* filepath, VkFormat format_initial) const {
			VkExtent2D extent;
			formatInfo formatInfo = FormatInfo(format_initial);
			std::unique_ptr<uint8_t[]> pImageData = texture::LoadFile(filepath, extent, formatInfo);
			if (pImageData)
				return (*this)(pImageData.get(), extent, format_initial);
			return texture2d{};
		}
		texture2d operator()(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial) const {
			VkDeviceSize imageDataSize_final = VkDeviceSize(FormatInfo(format_final).sizePerPixel) * extent.width * extent.height;
			if (callback_copyData)
				stagingBuffer::Expand_MainThread(imageDataSize_final);
			struct texture2d_local :texture2d {
				using texture::imageView;
				using texture::imageMemory;
				using texture2d::extent;
			}*pTexture;
			texture2d texture;
			pTexture = static_cast<texture2d_local*>(&texture);
			pTexture->extent = extent;
			imageMemory imageMemory_conversion;
			//Create imageMemory
			uint32_t mipLevelCount = generateMipmap ? texture::CalculateMipLevelCount(extent) : 1;
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format_final,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = mipLevelCount,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			};
			pTexture->imageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VkImage image = pTexture->imageMemory.Image();
			//Create framebuffer
			pTexture->imageView.Create(image, VK_IMAGE_VIEW_TYPE_2D, format_final, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
			VkFramebufferCreateInfo framebufferCreateInfo = {
				.renderPass = renderPass,
				.attachmentCount = 1,
				.pAttachments = pTexture->imageView.Address(),
				.width = extent.width,
				.height = extent.height,
				.layers = 1
			};
			framebuffer framebuffer(framebufferCreateInfo);
			//Record command buffer
			{
				VkCommandBuffer commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
				//Transfer data to image
				CmdTransferDataToImage(commandBuffer, pImageData, extent, format_initial, imageMemory_conversion, image);
				//Render
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
				VkViewport viewport = { 0, 0, float(extent.width), float(extent.height), 0.f, 1.f };
				VkRect2D renderArea = { {}, extent };
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
				renderPass.CmdBegin(commandBuffer, framebuffer, renderArea);
				vkCmdDraw(commandBuffer, 4, 1, 0, 0);
				renderPass.CmdEnd(commandBuffer);
				//Copy data to staging buffer if necessary
				if (callback_copyData) {
					VkBufferImageCopy region = {
						.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
						.imageExtent = { extent.width, extent.height, 1 }
					};
					vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer::Buffer_MainThread(), 1, &region);
				}
				//Generate mipmap if necessary, transition layout for shader reading
				if (mipLevelCount > 1 || callback_copyData)
					imageOperation::CmdGenerateMipmap2d(commandBuffer, image, extent, mipLevelCount, 1,
						{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
				//Submit
				graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
			}
			//Execute callback
			if (callback_copyData)
				callback_copyData(stagingBuffer::MapMemory_MainThread(imageDataSize_final), imageDataSize_final),
				stagingBuffer::UnmapMemory_MainThread();
			//Create imageView if necessary
			if (mipLevelCount > 1)
				pTexture->imageView.~imageView(),
				pTexture->imageView.Create(image, VK_IMAGE_VIEW_TYPE_2D, format_final, { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, 1 });
			return texture;
		}
		//Non-const Function
		void Instantiate(VkFormat format_final, bool generateMipmap, callback_copyData_t callback_copyMipLevel0) {
			this->format_final = format_final;
			this->generateMipmap = generateMipmap;
			callback_copyData = callback_copyMipLevel0;
			//Create renderpass
			VkAttachmentDescription attachmentDescription = {
				.format = format_final,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			};
			VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			VkSubpassDescription subpassDescription = {
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &attachmentReference
			};
			VkSubpassDependency subpassDependency = {
				.srcSubpass = 0,
				.dstSubpass = VK_SUBPASS_EXTERNAL,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
				.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT
			};
			VkRenderPassCreateInfo renderPassCreateInfo = {
				.attachmentCount = 1,
				.pAttachments = &attachmentDescription,
				.subpassCount = 1,
				.pSubpasses = &subpassDescription,
				.dependencyCount = 1,
				.pDependencies = &subpassDependency
			};
			if (!(generateMipmap || callback_copyData))
				attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				renderPassCreateInfo.dependencyCount = 0;
			renderPass.Create(renderPassCreateInfo);
			//Create pipeline
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayout.Create(pipelineLayoutCreateInfo);
			VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2] = {
				Ssci_Vert(),
				Ssci_Frag()
			};
			graphicsPipelineCreateInfoPack pipelineCiPack;
			for (auto& i : shaderStageCreateInfos)
				pipelineCiPack.shaderStages.push_back(i);
			pipelineCiPack.createInfo.layout = pipelineLayout;
			pipelineCiPack.createInfo.renderPass = renderPass;
			pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineCiPack.scissors.emplace_back(VkOffset2D{}, VkExtent2D{
				graphicsBase::Base().PhysicalDeviceProperties().limits.maxFramebufferWidth,
				graphicsBase::Base().PhysicalDeviceProperties().limits.maxFramebufferHeight });
			pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			pipelineCiPack.colorBlendAttachmentStates.emplace_back(
				true,
				VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_OP_ADD,
				0b1111);
			pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			pipelineCiPack.UpdateAllArrays();
			pipeline.Create(pipelineCiPack);
		}
	};
}