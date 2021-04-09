#include "VKBase+.h"

EASY_VULKAN_BEGIN
using namespace vulkan;
struct renderPassWithFramebuffers {
	renderPass renderPass;
	std::vector<framebuffer> framebuffers;
};

renderPassWithFramebuffers rpwf_screen;
void CreateRpwf_Screen() {
	if (rpwf_screen.renderPass) {
		std::cout << "[ easyVulkan ]\nDon't call CreateRpwf_Screen() twice!\n";
		return;
	}
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = graphicsBase::Base().SwapchainCreateInfo().imageFormat;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference attachmentReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attachmentReference;
	//Why this subpass dependency is necessary:
	//Spec 7.4.2. Semaphore Waiting, Note:
	//Some implementations may be able to execute transfer operations and/or vertex processing work before the semaphore is signaled.
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;//Not earlier than VkSubmitInfo.pWaitDstStageMask
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;//Not necessary, waiting for semaphore is enough
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;
	rpwf_screen.renderPass.Create(renderPassCreateInfo);
	auto CreateFramebuffers = [] {
		rpwf_screen.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.renderPass = rpwf_screen.renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.width = windowSize.width;
		framebufferCreateInfo.height = windowSize.height;
		framebufferCreateInfo.layers = 1;
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
#define bootScreenImagePath "texture/logo.png"
void BootScreen() {
	CreateRpwf_Screen();
	//Synchronization objects
	semaphore semaphore_imageIsAvailable;
	fence fence;
	//Acquire swapchain image
	graphicsBase::Base().SwapImage(semaphore_imageIsAvailable);
	//Image
	texture2d bootScreenImage(bootScreenImagePath, false);
	//Sampler
	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.magFilter = samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.addressModeU = samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler sampler(samplerCreateInfo);
	//Descriptor layout
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
	descriptorSetLayoutBinding.binding = 0;
	descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBinding.descriptorCount = 1;
	descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;
	descriptorSetLayout descriptorSetLayout(descriptorSetLayoutCreateInfo);
	//Descriptor
	VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
	vulkan::descriptorPool descriptorPool(0, 1, descriptorPoolSize);
	vulkan::descriptorSet descriptorSet;
	descriptorPool.AllocateDescriptorSet((VkDescriptorSet&)descriptorSet, descriptorSetLayout);
	VkWriteDescriptorSet writeDescriptorSet{};
	VkDescriptorImageInfo imageInfo = { sampler, bootScreenImage.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(graphicsBase::Base().Device(), 1, &writeDescriptorSet, 0, nullptr);
	//Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = (VkDescriptorSetLayout*)&descriptorSetLayout;
	pipelineLayout pipelineLayout(pipelineLayoutCreateInfo);
	//Pipeline
	shader vert("shader/FullscreenImage.vert.spv");
	shader frag("shader/FullscreenImage.frag.spv");
	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2]{
		vert.StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT),
		frag.StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	graphicsPipelineCreateInfoPack pipelineCiPack;
	for (size_t i = 0; i < std::size(shaderStageCreateInfos); i++)
		pipelineCiPack.shaderStages.push_back(shaderStageCreateInfos[i]);
	pipelineCiPack.createInfo.layout = pipelineLayout;
	pipelineCiPack.createInfo.renderPass = rpwf_screen.renderPass;
	pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	pipelineCiPack.viewports.emplace_back(0.f, 0.f, float(windowSize.width), float(windowSize.height), 0.f, 1.0f);
	pipelineCiPack.scissors.emplace_back(VkOffset2D{}, windowSize);
	pipelineCiPack.multisampleStateCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineCiPack.colorBlendAttachmentStates.emplace_back(
		true,
		VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
		0b1111
	);
	pipelineCiPack.UpdateAllArrays();
	pipeline pipeline(pipelineCiPack.createInfo);
	//Command buffer
	commandBuffer commandBuffer;
	graphicsBase::Plus().CommandPool_Graphics().AllocateCommandBuffers((VkCommandBuffer*)&commandBuffer);
	//Record commands
	VkClearValue clearColor{};
	commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	rpwf_screen.renderPass.CmdBegin(commandBuffer, easyVulkan::rpwf_screen.framebuffers[0], clearColor);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, (VkDescriptorSet*)&descriptorSet, 0, nullptr);
	vkCmdDraw(commandBuffer, 4, 1, 0, 0);
	rpwf_screen.renderPass.CmdEnd(commandBuffer);
	commandBuffer.End();
	//Submit
	graphicsBase::Base().SubmitGraphicsCommandBuffer(commandBuffer, semaphore_imageIsAvailable, VK_NULL_HANDLE, fence);
	//Present
	fence.WaitAndReset();
	graphicsBase::Base().PresentImage();
}

VkFormat dsFormat = VK_FORMAT_D24_UNORM_S8_UINT;
std::vector<depthStencilAttachment> dsas_screenWithDs;
renderPassWithFramebuffers rpwf_screenWithDs;
void CreateRpwf_ScreenWithDs() {
	VkAttachmentDescription attachmentDescriptions[2]{};
	attachmentDescriptions[0].format = graphicsBase::Base().SwapchainCreateInfo().imageFormat;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescriptions[1].format = dsFormat;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].stencilLoadOp = dsFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//initialLayout can be VK_IMAGE_LAYOUT_UNDEFINED or same as finalLayout.
	//Debug layer may thorw warninigs at beginning, not a big deal.
	//I don't want any warning, let it be VK_IMAGE_LAYOUT_UNDEFINED, which is 0, the default value.
	//Unless the separateDepthStencilLayouts feature is enabled, even if dsFormat doesn't support stencil, finalLayout must be VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference attachmentReferences[2]{
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
	};
	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = attachmentReferences;
	subpassDescription.pDepthStencilAttachment = attachmentReferences + 1;
	//Even if the layout transition of the ds image won't happen if initialLayout is VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	//ensure no external reading/writing to the ds image when EARLY_FRAGMENT_TESTS begin.
	//At EARLY_FRAGMENT_TESTS stage, the ds image'll be cleared (if performs) then readed, ds tests are performed for each fragment.
	//At LATE_FRAGMENT_TESTS stage, ds tests are performed for each sample.
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;//Because of VK_ATTACHMENT_LOAD_OP_CLEAR
	subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = attachmentDescriptions;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;
	rpwf_screenWithDs.renderPass.Create(renderPassCreateInfo);
	auto CreateFramebuffers = [] {
		dsas_screenWithDs.resize(graphicsBase::Base().SwapchainImageCount());
		rpwf_screenWithDs.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
		for (auto& i : dsas_screenWithDs)
			i.Create(dsFormat, windowSize, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.renderPass = rpwf_screenWithDs.renderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.width = windowSize.width;
		framebufferCreateInfo.height = windowSize.height;
		framebufferCreateInfo.layers = 1;
		for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
			VkImageView attachments[2]{ graphicsBase::Base().SwapchainImageView(i), dsas_screenWithDs[i].ImageView() };
			framebufferCreateInfo.pAttachments = attachments;
			rpwf_screenWithDs.framebuffers[i].Create(framebufferCreateInfo);
		}
	};
	auto DestroyFramebuffers = [] {
		dsas_screenWithDs.clear();
		rpwf_screenWithDs.framebuffers.clear();
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(CreateFramebuffers);
	graphicsBase::Base().PushCallback_DestroySwapchain(DestroyFramebuffers);
	CreateFramebuffers();
}

VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_4_BIT;
std::vector<colorAttachment> cas_msaaWithDsToScreen;
std::vector<depthStencilAttachment> dsas_msaaWithDsToScreen;
renderPassWithFramebuffers rpwf_msaaWithDsToScreen;
void CreateRpwf_MsaaWithDsToScreen() {
	VkAttachmentDescription attachmentDescriptions[3]{};
	//Swapchain attachment, used as resolve attachment
	attachmentDescriptions[0].format = graphicsBase::Base().SwapchainCreateInfo().imageFormat;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//Msaa color attachment
	attachmentDescriptions[1].format = graphicsBase::Base().SwapchainCreateInfo().imageFormat;//Must be same with the resolve attachment
	attachmentDescriptions[1].samples = sampleCount;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	//Msaa depth stencil attachment
	attachmentDescriptions[2].format = dsFormat;
	attachmentDescriptions[2].samples = sampleCount;
	attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[2].stencilLoadOp = dsFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference attachmentReferences[3]{
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
	};
	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = attachmentReferences + 1;
	subpassDescription.pResolveAttachments = attachmentReferences;
	subpassDescription.pDepthStencilAttachment = attachmentReferences + 2;
	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.attachmentCount = 3;
	renderPassCreateInfo.pAttachments = attachmentDescriptions;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;
	rpwf_msaaWithDsToScreen.renderPass.Create(renderPassCreateInfo);
	auto CreateFramebuffers = [] {
		cas_msaaWithDsToScreen.resize(graphicsBase::Base().SwapchainImageCount());
		dsas_msaaWithDsToScreen.resize(graphicsBase::Base().SwapchainImageCount());
		rpwf_msaaWithDsToScreen.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
		for (auto& i : cas_msaaWithDsToScreen)
			i.Create(graphicsBase::Base().SwapchainCreateInfo().imageFormat, windowSize, sampleCount, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		for (auto& i : dsas_msaaWithDsToScreen)
			i.Create(dsFormat, windowSize, sampleCount, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.renderPass = rpwf_msaaWithDsToScreen.renderPass;
		framebufferCreateInfo.attachmentCount = 3;
		framebufferCreateInfo.width = windowSize.width;
		framebufferCreateInfo.height = windowSize.height;
		framebufferCreateInfo.layers = 1;
		for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++) {
			VkImageView attachments[3]{ graphicsBase::Base().SwapchainImageView(i), cas_msaaWithDsToScreen[i].ImageView(), dsas_msaaWithDsToScreen[i].ImageView() };
			framebufferCreateInfo.pAttachments = attachments;
			rpwf_msaaWithDsToScreen.framebuffers[i].Create(framebufferCreateInfo);
		}
	};
	auto DestroyFramebuffers = [] {
		cas_msaaWithDsToScreen.clear();
		dsas_msaaWithDsToScreen.clear();
		rpwf_msaaWithDsToScreen.framebuffers.clear();
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(CreateFramebuffers);
	graphicsBase::Base().PushCallback_DestroySwapchain(DestroyFramebuffers);
	CreateFramebuffers();
}

colorAttachment ca_canvas;
renderPassWithFramebuffers rpwf_canvas;
void CreateRpwf_Canvas() {
	//When this render pass begins, the image keeps its contents.
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = format_rgba8;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkAttachmentReference attachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpassDescription{};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attachmentReference;
	VkSubpassDependency subpassDependencies[2]{};
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 2;
	renderPassCreateInfo.pDependencies = subpassDependencies;
	rpwf_canvas.renderPass.Create(renderPassCreateInfo);
	ca_canvas.Create(format_rgba8, { 1280, 720 }, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
	VkFramebufferCreateInfo framebufferCreateInfo{};
	framebufferCreateInfo.renderPass = rpwf_canvas.renderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.width = windowSize.width;
	framebufferCreateInfo.height = windowSize.height;
	framebufferCreateInfo.layers = 1;
	framebufferCreateInfo.pAttachments = (VkImageView*)&ca_canvas;
	rpwf_canvas.framebuffers.resize(1);
	rpwf_canvas.framebuffers[0].Create(framebufferCreateInfo);
}
void CmdClearCanvas(VkCommandBuffer commandBuffer, VkClearColorValue clearColor) {
	//Call this function before rpwf_canvas.renderPass begins.
	VkImageSubresourceRange imageSubresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkImageMemoryBarrier imageMemoryBarrier = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		VK_ACCESS_SHADER_READ_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED,//No ownership transfer
		VK_QUEUE_FAMILY_IGNORED,
		ca_canvas.Image(),
		imageSubresourceRange
	};
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	vkCmdClearColorImage(commandBuffer, ca_canvas.Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageSubresourceRange);
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = 0;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
		0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

//To simplify the example, not considering frames in flight.
colorAttachment ca_deferredToScreen_position;
colorAttachment ca_deferredToScreen_normal;
colorAttachment ca_deferredToScreen_albedoSpecular;
depthStencilAttachment dsa_deferredToScreen;
renderPassWithFramebuffers rpwf_deferredToScreen;
void CreateRpwf_DeferredToScreen() {
	VkAttachmentDescription attachmentDescriptions[5]{};
	//Swapchain attachment
	attachmentDescriptions[0].format = graphicsBase::Base().SwapchainCreateInfo().imageFormat;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	//Deferred position attachment
	attachmentDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;//Or VK_FORMAT_R16G16B16A16_SFLOAT
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	//Deferred normal attachment, same as above
	memcpy(attachmentDescriptions + 2, attachmentDescriptions + 1, sizeof VkAttachmentDescription);
	//Deffered albedo specular attachment
	memcpy(attachmentDescriptions + 3, attachmentDescriptions + 1, sizeof VkAttachmentDescription);
	attachmentDescriptions[3].format = VK_FORMAT_R8G8B8A8_UNORM;
	//Depth stencil attachment
	attachmentDescriptions[4].format = dsFormat;
	attachmentDescriptions[4].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[4].stencilLoadOp = dsFormat >= VK_FORMAT_S8_UINT ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference attachmentReferences_subpass0[4]{
		{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL }
	};
	VkAttachmentReference attachmentReferences_subpass1[4]{
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
	};
	VkSubpassDescription subpassDescriptions[2]{};
	subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].colorAttachmentCount = 3;
	subpassDescriptions[0].pColorAttachments = attachmentReferences_subpass0;
	subpassDescriptions[0].pDepthStencilAttachment = attachmentReferences_subpass0 + 3;
	subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[1].inputAttachmentCount = 3;
	subpassDescriptions[1].pInputAttachments = attachmentReferences_subpass1 + 1;
	subpassDescriptions[1].colorAttachmentCount = 1;
	subpassDescriptions[1].pColorAttachments = attachmentReferences_subpass1;
	VkSubpassDependency subpassDependencies[2]{};
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].dstSubpass = 1;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].dstSubpass = 1;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.attachmentCount = 5;
	renderPassCreateInfo.pAttachments = attachmentDescriptions;
	renderPassCreateInfo.subpassCount = 2;
	renderPassCreateInfo.pSubpasses = subpassDescriptions;
	renderPassCreateInfo.dependencyCount = 2;
	renderPassCreateInfo.pDependencies = subpassDependencies;
	rpwf_deferredToScreen.renderPass.Create(renderPassCreateInfo);
	auto CreateFramebuffers = [] {
		rpwf_deferredToScreen.framebuffers.resize(graphicsBase::Base().SwapchainImageCount());
		ca_deferredToScreen_position.Create(VK_FORMAT_R32G32B32A32_SFLOAT, windowSize, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		ca_deferredToScreen_normal.Create(VK_FORMAT_R32G32B32A32_SFLOAT, windowSize, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		ca_deferredToScreen_albedoSpecular.Create(VK_FORMAT_R8G8B8A8_UNORM, windowSize, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		dsa_deferredToScreen.Create(dsFormat, windowSize, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.renderPass = rpwf_deferredToScreen.renderPass;
		framebufferCreateInfo.attachmentCount = 5;
		VkImageView attachments[5]{
			VK_NULL_HANDLE,
			ca_deferredToScreen_position.ImageView(),
			ca_deferredToScreen_normal.ImageView(),
			ca_deferredToScreen_albedoSpecular.ImageView(),
			dsa_deferredToScreen.ImageView()
		};
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = windowSize.width;
		framebufferCreateInfo.height = windowSize.height;
		framebufferCreateInfo.layers = 1;
		for (size_t i = 0; i < graphicsBase::Base().SwapchainImageCount(); i++)
			attachments[0] = graphicsBase::Base().SwapchainImageView(i),
			rpwf_deferredToScreen.framebuffers[i].Create(framebufferCreateInfo);
	};
	auto DestroyFramebuffers = [] {
		ca_deferredToScreen_position.~colorAttachment();
		ca_deferredToScreen_normal.~colorAttachment();
		ca_deferredToScreen_albedoSpecular.~colorAttachment();
		dsa_deferredToScreen.~depthStencilAttachment();
		rpwf_deferredToScreen.framebuffers.clear();
	};
	graphicsBase::Base().PushCallback_CreateSwapchain(CreateFramebuffers);
	graphicsBase::Base().PushCallback_DestroySwapchain(DestroyFramebuffers);
	CreateFramebuffers();
}

NAMESPACE_END