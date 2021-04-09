#pragma once
#include "VKBase.h"
#pragma warning(disable:4554)
#pragma warning(disable:26451)

VULKAN_BEGIN
using namespace glm;

class graphicsBasePlus {
	friend class deviceLocalBuffer;
	friend class texture;
	commandPool commandPool_graphics;
	commandBuffer commandBuffers_transfer[2];
	commandPool commandPool_presentation;
	commandBuffer commandBuffer_acquireOwnership;
	//Static
	static graphicsBasePlus singleton;
	//--------------------
	graphicsBasePlus() {
		auto Initialize = [] {
			singleton.commandPool_graphics.Create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsBase::Base().QueueFamilyIndex_Graphics());
			singleton.commandPool_graphics.AllocateCommandBuffers((VkCommandBuffer*)singleton.commandBuffers_transfer, 2);
			if (graphicsBase::Base().QueueFamilyIndex_Graphics() != graphicsBase::Base().QueueFamilyIndex_Presentation() &&
				graphicsBase::Base().SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
				singleton.commandPool_presentation.Create(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, graphicsBase::Base().QueueFamilyIndex_Presentation()),
				singleton.commandPool_presentation.AllocateCommandBuffers((VkCommandBuffer*)&singleton.commandBuffer_acquireOwnership);
		};
		graphicsBase::Base().Plus(singleton);
		graphicsBase::Base().PushCallback_CreateDevice(Initialize);
		//A cleanUp callback isn't necessary.
		//Static and global objects are destructed in the reverse order of construction,
		//and if those objects are all not trivially constructible, constructions happen in definition order.
	}
	graphicsBasePlus(graphicsBasePlus&&) = delete;
	~graphicsBasePlus() = default;
public:
	//Getter
	const commandPool& CommandPool_Graphics() const { return commandPool_graphics; }
	//Const Function
	VkResult AcquireImageOwnership_Presentation(VkSemaphore semaphore_renderingIsOver, VkSemaphore semaphore_ownershipIsTransfered) const {
		static fence fence(true);
		fence.WaitAndReset();
		commandBuffer_acquireOwnership.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		graphicsBase::Base().CmdAcquireImageOwnership_Presentation(commandBuffer_acquireOwnership);
		commandBuffer_acquireOwnership.End();
		return graphicsBase::Base().SubmitPresentationCommandBuffer(commandBuffer_acquireOwnership, semaphore_renderingIsOver, semaphore_ownershipIsTransfered, fence);
	}
};
inline graphicsBasePlus graphicsBasePlus::singleton;

struct graphicsPipelineCreateInfoPack {
	VkGraphicsPipelineCreateInfo createInfo =
	{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	//Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	std::vector<VkVertexInputBindingDescription> vertexInputBindings;
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
	//Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	//Tessellation
	VkPipelineTessellationStateCreateInfo tessellationStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	//Viewport
	VkPipelineViewportStateCreateInfo viewportStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	std::vector<VkViewport> viewports;
	std::vector<VkRect2D> scissors;
	uint32_t dynamicViewportCount = 1;
	uint32_t dynamicScissorCount = 1;
	//Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizationStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	//Multisample
	VkPipelineMultisampleStateCreateInfo multisampleStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	//Depth & Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	//Color Blend
	VkPipelineColorBlendStateCreateInfo colorBlendStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
	//Dynamic
	VkPipelineDynamicStateCreateInfo dynamicStateCi =
	{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	std::vector<VkDynamicState> dynamicStates;
	//--------------------
	graphicsPipelineCreateInfoPack() {
		SetCreateInfos();
	}
	graphicsPipelineCreateInfoPack(const graphicsPipelineCreateInfoPack& other) noexcept {
		createInfo = other.createInfo;
		SetCreateInfos();

		vertexInputStateCi = other.vertexInputStateCi;
		inputAssemblyStateCi = other.inputAssemblyStateCi;
		tessellationStateCi = other.tessellationStateCi;
		viewportStateCi = other.viewportStateCi;
		rasterizationStateCi = other.rasterizationStateCi;
		multisampleStateCi = other.multisampleStateCi;
		depthStencilStateCi = other.depthStencilStateCi;
		colorBlendStateCi = other.colorBlendStateCi;
		dynamicStateCi = other.dynamicStateCi;

		shaderStages = other.shaderStages;
		vertexInputBindings = other.vertexInputBindings;
		vertexInputAttributes = other.vertexInputAttributes;
		viewports = other.viewports;
		scissors = other.scissors;
		colorBlendAttachmentStates = other.colorBlendAttachmentStates;
		dynamicStates = other.dynamicStates;
		UpdateAllArrayAddresses();
	}
	//Getter
	operator VkGraphicsPipelineCreateInfo& () { return createInfo; }
	//Non-const Function
	void UpdateAllArrays() {
		createInfo.stageCount = shaderStages.size();
		vertexInputStateCi.vertexBindingDescriptionCount = vertexInputBindings.size();
		vertexInputStateCi.vertexAttributeDescriptionCount = vertexInputAttributes.size();
		viewportStateCi.viewportCount = viewports.size() ? uint32_t(viewports.size()) : dynamicViewportCount;
		viewportStateCi.scissorCount = scissors.size() ? uint32_t(scissors.size()) : dynamicScissorCount;
		colorBlendStateCi.attachmentCount = colorBlendAttachmentStates.size();
		dynamicStateCi.dynamicStateCount = dynamicStates.size();
		UpdateAllArrayAddresses();
	}
private:
	void SetCreateInfos() {
		createInfo.pVertexInputState = &vertexInputStateCi;
		createInfo.pInputAssemblyState = &inputAssemblyStateCi;
		createInfo.pTessellationState = &tessellationStateCi;
		createInfo.pViewportState = &viewportStateCi;
		createInfo.pRasterizationState = &rasterizationStateCi;
		createInfo.pMultisampleState = &multisampleStateCi;
		createInfo.pDepthStencilState = &depthStencilStateCi;
		createInfo.pColorBlendState = &colorBlendStateCi;
		createInfo.pDynamicState = &dynamicStateCi;
	}
	void UpdateAllArrayAddresses() {
		createInfo.pStages = shaderStages.data();
		vertexInputStateCi.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputStateCi.pVertexAttributeDescriptions = vertexInputAttributes.data();
		viewportStateCi.pViewports = viewports.data();
		viewportStateCi.pScissors = scissors.data();
		colorBlendStateCi.pAttachments = colorBlendAttachmentStates.data();
		dynamicStateCi.pDynamicStates = dynamicStates.data();
	}
};

#pragma region DeviceLocalBuffer
class deviceLocalBuffer {
protected:
	bufferMemory bufferMemory;
public:
	deviceLocalBuffer(VkDeviceSize size, VkBufferUsageFlags desiredBufferUsage__Without_transfer_dst) {
		bufferMemory.CreateAndAllocate(size, desiredBufferUsage__Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
	//Getter
	operator VkBuffer() const { return	bufferMemory.Buffer(); }
	//Const Function
	void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
		vulkan::bufferMemory bufferMemory_staging(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		bufferMemory_staging.BufferData(pData_src, size, 0);
		fence fence;
		auto& commandBuffer = graphicsBase::Plus().commandBuffers_transfer[0];
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VkBufferCopy region = { 0, offset, size };
		vkCmdCopyBuffer(commandBuffer, bufferMemory_staging.Buffer(), bufferMemory.Buffer(), 1, &region);
		commandBuffer.End();
		graphicsBase::Base().SubmitTransferCommandBuffer(commandBuffer, fence);
		fence.Wait();
	}
	template<typename T>
	void TransferData(const T& data_src) const {
		TransferData(&data_src, sizeof data_src);
	}
	void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const void* pData_src, VkDeviceSize size__Less_than_65536, VkDeviceSize offset = 0) const {
		vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), offset, size__Less_than_65536, pData_src);
	}
	template<typename T>
	void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const T& data_src) const {
		vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), 0, sizeof data_src, &data_src);
	}
	//Non-const Function
	void Recreate(VkDeviceSize size, VkBufferUsageFlags desiredBufferUsage__Without_transfer_dst) {
		vkDeviceWaitIdle(graphicsBase::Base().Device());
		bufferMemory.~bufferMemory();
		bufferMemory.CreateAndAllocate(size, desiredBufferUsage__Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
};
class vertexBuffer :public deviceLocalBuffer {
	using deviceLocalBuffer::Recreate;
public:
	vertexBuffer(VkDeviceSize size) :deviceLocalBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {}
	//Non-const Function
	void Recreate(VkDeviceSize size) {
		Recreate(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	}
};
class indexBuffer :public deviceLocalBuffer {
	using deviceLocalBuffer::Recreate;
public:
	indexBuffer(VkDeviceSize size) :deviceLocalBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {}
	//Non-const Function
	void Recreate(VkDeviceSize size) {
		Recreate(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}
};
class uniformBuffer :public deviceLocalBuffer {
	using deviceLocalBuffer::Recreate;
public:
	uniformBuffer(VkDeviceSize size) :deviceLocalBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {}
	//Non-const Function
	void Recreate(VkDeviceSize size) {
		Recreate(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}
};

#pragma endregion

#pragma region Attachment
class attachment {
protected:
	imageView imageView;
	imageMemory imageMemory;
	//--------------------
	attachment() = default;
public:
	//Getter
	VkImageView ImageView() const { return imageView; }
	VkImage Image() const { return imageMemory.Image(); }
	//Const Function
	VkDescriptorImageInfo DescriptorImageInfo(const VkSampler& sampler) const {
		return { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	}
};

class colorAttachment :public attachment {
public:
	colorAttachment() = default;
	colorAttachment(VkFormat format, VkExtent2D extent, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
		Create(format, extent, sampleCount, otherUsages);
	}
	//Non-const Function
	void Create(VkFormat format, VkExtent2D extent, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
		imageMemory.CreateAndAllocate(
			VK_IMAGE_TYPE_2D,
			format,
			{ extent.width, extent.height, 1 },
			1,
			1,
			sampleCount,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | otherUsages,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
		imageView.Create(
			imageMemory.Image(),
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
	}
};
class depthStencilAttachment :public attachment {
public:
	depthStencilAttachment() = default;
	depthStencilAttachment(VkFormat format, VkExtent2D extent, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
		Create(format, extent, sampleCount, otherUsages);
	}
	//Non-const Function
	void Create(VkFormat format, VkExtent2D extent, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
		imageMemory.CreateAndAllocate(
			VK_IMAGE_TYPE_2D,
			format,
			{ extent.width, extent.height, 1 },
			1,
			1,
			sampleCount,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | otherUsages,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (format > VK_FORMAT_S8_UINT)
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		else if (format == VK_FORMAT_S8_UINT)
			aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		imageView.Create(
			imageMemory.Image(),
			VK_IMAGE_VIEW_TYPE_2D,
			format,
			{ aspectMask, 0, 1, 0, 1 });
	}
	//Static Function
	static bool FormatAvailability(VkFormat format) {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(graphicsBase::Base().PhysicalDevice(), format, &formatProperties);
		return formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
};

#pragma endregion

#pragma region Texture
class texture {
protected:
	imageView imageView;
	imageMemory imageMemory;
	//--------------------
	texture() = default;
public:
	//Getter
	VkImageView ImageView() const { return imageView; }
	VkImage Image() const { return imageMemory.Image(); }
	//Const Function
	VkDescriptorImageInfo DescriptorImageInfo(const VkSampler& sampler) const {
		return { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	}
	//Static Function
	static uint8_t* LoadFile(const char* filepath, ivec2& size, bool formatIsR8 = false, bool flip = false) {
		stbi_set_flip_vertically_on_load(flip);
		int channels;
		uint8_t* pImageData = stbi_load(filepath, (int*)&size.x, (int*)&size.y, &channels, formatIsR8 ? 1 : 4);
		if (!pImageData)
			std::cout << "[ texture ]\nFailed to load file: " << filepath << std::endl;
		return pImageData;
	}
	static void CopyBufferToImage2d(
		VkBuffer buffer, VkImage image,
		VkExtent3D imageExtent, uint32_t mipLevelCount = 1, uint32_t layerCount = 1, bool generateMipmap = true, VkFilter minFilter = VK_FILTER_LINEAR) {
		generateMipmap = generateMipmap && mipLevelCount - 1;
		fence fence_copy;
		{
			auto& commandBuffer = graphicsBase::Plus().commandBuffers_transfer[0];
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			//Pre-copy barrier
			VkImageMemoryBarrier imageMemoryBarrier = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				0,//No srcAccess
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,//No ownership transfer
				VK_QUEUE_FAMILY_IGNORED,
				image,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
			};
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			//Copy
			VkBufferImageCopy region{};
			region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount };
			region.imageExtent = imageExtent;
			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			//Post-copy barrier
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			if (generateMipmap)
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			else
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			commandBuffer.End();
			//Submit
			graphicsBase::Base().SubmitTransferCommandBuffer(commandBuffer, fence_copy);
			if (!generateMipmap) {
				fence_copy.Wait();
				return;
			}
		}
		//Generate mipmap
		fence fence_blit;
		{
			auto& commandBuffer = graphicsBase::Plus().commandBuffers_transfer[1];
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			for (uint32_t i = 1; i < mipLevelCount; i++) {
				//Pre-blit barrier
				VkImageMemoryBarrier imageMemoryBarrier = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					0,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					image,
					{ VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, layerCount }
				};
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				//Blit
				VkImageBlit region{};
				region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, layerCount };
				region.srcOffsets[1] = { (int32_t&)imageExtent.width >> i - 1, (int32_t&)imageExtent.height >> i - 1, 1 };
				region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, i , 0, layerCount };
				region.dstOffsets[1] = { (int32_t&)imageExtent.width >> i, (int32_t&)imageExtent.height >> i, 1 };
				vkCmdBlitImage(
					commandBuffer,
					image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&region,
					minFilter);
				//Post-blit barrier
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
			//Layout transition 
			VkImageMemoryBarrier imageMemoryBarrier = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, layerCount }
			};
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			commandBuffer.End();
			//Submit command buffer
			fence_copy.Wait();
			graphicsBase::Base().SubmitTransferCommandBuffer(commandBuffer, fence_blit);
			fence_blit.Wait();
		}
	}
	static VkSamplerCreateInfo SamplerCreateInfo() {
		static VkSamplerCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			nullptr,
			0,
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_MIPMAP_MODE_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			0.f,
			VK_TRUE,
			graphicsBase::Base().PhysicalDeviceProperties().limits.maxSamplerAnisotropy,
			VK_FALSE,
			VK_COMPARE_OP_ALWAYS,
			0.f,
			VK_LOD_CLAMP_NONE,
			{},
			VK_FALSE
		};
		return createInfo;
	}
};
#define CalculateMipLevelCount(size) generateMipmap * uint32_t(std::floor(std::log2(std::min(size.x, size.y)))) + 1

class texture2d :public texture {
protected:
	uvec2 size = {};
	//--------------------
	texture2d() = default;
public:
	texture2d(const char* filepath, bool generateMipmap = true, bool formatIsR8 = false, bool flip = false) {
		uint8_t* pImageData = LoadFile(filepath, (ivec2&)size, formatIsR8, flip);
		if (!pImageData)
			return;
		//Staging buffer
		VkDeviceSize imageDataSize = (formatIsR8 ? 1ull : 4ull) * size.x * size.y;
		bufferMemory bufferMemory_staging(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		bufferMemory_staging.BufferData(pImageData, imageDataSize);
		free(pImageData);
		//Create image and memory
		uint32_t mipLevelCount = CalculateMipLevelCount(size);
		imageMemory.CreateAndAllocate(
			VK_IMAGE_TYPE_2D,
			formatIsR8 ? format_r8 : format_rgba8,
			{ size.x, size.y, 1 },
			mipLevelCount,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CopyBufferToImage2d(bufferMemory_staging.Buffer(), imageMemory.Image(), { size.x, size.y, 1 }, mipLevelCount);
		//Create view
		imageView.Create(
			imageMemory.Image(),
			VK_IMAGE_VIEW_TYPE_2D,
			formatIsR8 ? format_r8 : format_rgba8,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, 1 });
	}
	//Getter
	ivec2 Size() const { return size; }
	int32_t W() const { return size.x; }
	int32_t H() const { return size.y; }
};
class texture2dArray :public texture2d {
protected:
	uint32_t layerCount = 0;
public:
	texture2dArray(const char* filepath, uvec2 size__In_units, bool generateMipmap = true, bool formatIsR8 = false, bool flip = false) :
		layerCount(size__In_units.x* size__In_units.y) {
		if (layerCount > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers)
			std::cout <<
			"[ texture2dArray ]\nLayer count is out of limit! Must be less than: " <<
			graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers << std::endl <<
			"File: " << filepath << std::endl;
		uint8_t* pImageData = LoadFile(filepath, (ivec2&)size, formatIsR8, flip);
		if (!pImageData)
			return;
		if (size.x % size__In_units.x || size.y % size__In_units.y) {
			std::cout <<
				"[ texture2dArray ]\nImage not available!\nFile: " << filepath << std::endl <<
				"Image width must be in multiples of " << size__In_units.x << std::endl <<
				"Image height must be in multiples of " << size__In_units.y << std::endl;
			free(pImageData);
			return;
		}
		uvec2 fullSize = size;
		size /= size__In_units;
		//Staging buffer
		uint32_t channelCount = formatIsR8 ? 1 : 4;
		VkDeviceSize imageDataSize = uint64_t(channelCount) * fullSize.x * fullSize.y;
		bufferMemory bufferMemory_staging(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		if (size__In_units.x == 1)
			bufferMemory_staging.BufferData(pImageData, imageDataSize);
		else {
			uint8_t* pData_dst = nullptr;
			bufferMemory_staging.MapMemory((void*&)pData_dst, imageDataSize);
			uint32_t perRowDataSize = size.x * channelCount;
			size_t offset = 0;
			for (uint32_t j = 0; j < size__In_units.y; j++)
				for (uint32_t i = 0; i < size__In_units.x; i++)
					for (uint32_t k = 0; k < size.y; k++)
						memcpy(&pData_dst[offset], &pImageData[(i * size.x + (k + j * size.y) * fullSize.x) * channelCount], perRowDataSize),
						offset += perRowDataSize;
			bufferMemory_staging.UnmapMemory(imageDataSize);
			free(pImageData);
		}
		//Create image and memory
		uint32_t mipLevelCount = CalculateMipLevelCount(size);
		imageMemory.CreateAndAllocate(
			VK_IMAGE_TYPE_2D,
			formatIsR8 ? format_r8 : format_rgba8,
			{ size.x, size.y, 1 },
			mipLevelCount,
			layerCount,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CopyBufferToImage2d(bufferMemory_staging.Buffer(), imageMemory.Image(), { size.x, size.y, 1 }, mipLevelCount, layerCount);
		//Create view
		imageView.Create(
			imageMemory.Image(),
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			formatIsR8 ? format_r8 : format_rgba8,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, layerCount });
	}
	texture2dArray(const char** filepaths, uint32_t layerCount, bool generateMipmap = true, bool formatIsR8 = false, bool flip = false) :
		layerCount(layerCount) {
		if (layerCount > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers)
			std::cout <<
			"[ texture2dArray ]\nLayer count is out of limit! Must be less than: " <<
			graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers << std::endl;
		uint8_t** pImageDatas = new uint8_t * [layerCount] {};
		for (size_t i = 0; i < layerCount; i++) {
			uvec2 size = {};
			pImageDatas[i] = LoadFile(filepaths[i], (ivec2&)size, formatIsR8, flip);
			if (pImageDatas[i]) {
				if (i == 0)
					this->size = size;
				continue;
			}
			if (size != this->size)
				std::cout <<
				"[ texture2dArray ]\nImage not available!\nFile: " << filepaths[i] << std::endl <<
				"Image width must be " << size.x << std::endl <<
				"Image height must be " << size.y << std::endl;
			for (size_t i = 0; i < layerCount; i++)
				free(pImageDatas[i]);
			delete[] pImageDatas;
			return;
		}
		//Staging buffer
		size_t perImageDataSize = (formatIsR8 ? 1ull : 4ull) * size.x * size.y;
		VkDeviceSize imageDataSize = perImageDataSize * layerCount;
		bufferMemory bufferMemory_staging(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uint8_t* pData_dst = nullptr;
		bufferMemory_staging.MapMemory((void*&)pData_dst, imageDataSize);
		for (size_t i = 0; i < layerCount; i++)
			memcpy(pData_dst, pImageDatas[i], perImageDataSize),
			pData_dst += perImageDataSize;
		bufferMemory_staging.UnmapMemory(imageDataSize);
		for (size_t i = 0; i < layerCount; i++)
			free(pImageDatas[i]);
		delete[] pImageDatas;
		//Create image and memory
		uint32_t mipLevelCount = CalculateMipLevelCount(size);
		imageMemory.CreateAndAllocate(
			VK_IMAGE_TYPE_2D,
			formatIsR8 ? format_r8 : format_rgba8,
			{ size.x, size.y, 1 },
			mipLevelCount,
			layerCount,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CopyBufferToImage2d(bufferMemory_staging.Buffer(), imageMemory.Image(), { size.x, size.y, 1 }, mipLevelCount, layerCount);
		//Create view
		imageView.Create(
			imageMemory.Image(),
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			formatIsR8 ? format_r8 : format_rgba8,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, layerCount });
	}
	//Getter
	int32_t LayerCount() const { return layerCount; }
};

class textureCube :public texture2d {
	void Constructor_Internal(uint8_t* pImageDatas[6], bool generateMipmap) {
		//Staging buffer
		size_t perImageDataSize = 4ull * size.x * size.y;
		VkDeviceSize imageDataSize = perImageDataSize * 6;
		bufferMemory bufferMemory_staging(imageDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uint8_t* pData_dst = nullptr;
		bufferMemory_staging.MapMemory((void*&)pData_dst, imageDataSize);
		for (size_t i = 0; i < 6; i++)
			memcpy(pData_dst, pImageDatas[i], perImageDataSize),
			pData_dst += perImageDataSize;
		bufferMemory_staging.UnmapMemory(imageDataSize);
		for (size_t i = 0; i < 6; i++)
			free(pImageDatas[i]);
		//Create image and memory
		uint32_t mipLevelCount = CalculateMipLevelCount(size);
		imageMemory.CreateAndAllocate(
			VK_IMAGE_TYPE_2D,
			format_rgba8,
			{ size.x, size.y, 1 },
			mipLevelCount,
			6,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
		CopyBufferToImage2d(bufferMemory_staging.Buffer(), imageMemory.Image(), { size.x, size.y, 1 }, mipLevelCount, 6);
		//Create view
		imageView.Create(
			imageMemory.Image(),
			VK_IMAGE_VIEW_TYPE_CUBE,
			format_rgba8,
			{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, 6 });
	}
public:
	//Default face positions, looking from inside, is:
	//[      ][ top  ][      ][      ]
	//[ left ][front ][right ][ back ]
	//[      ][bottom][      ][      ]
	//If lookFromOutside is true, front and back is swapped.
	//What ever the facePositions are, make sure the image matches the looking which a cube is unwrapped as above.
	textureCube(const char* filepath, const uvec2 facePositions[6] = nullptr, bool lookFromOutside = false, bool generateMipmap = true, bool flip = false) {
		static const uvec2 facePositions_default[][6] = {
			{ {2, 1}, {0, 1}, {1, 0}, {1, 2}, {1, 1}, {3, 1} },
			{ {2, 1}, {0, 1}, {1, 0}, {1, 2}, {3, 1}, {1, 1} },
		};
		uvec2 size__In_units = { 1, 1 };
		if (!facePositions) {
			facePositions = facePositions_default[lookFromOutside];
			size__In_units = { 4, 3 };
		}
		else
			for (size_t i = 0; i < 6; i++) {
				if (facePositions[i].x >= size__In_units.x)
					size__In_units.x = facePositions[i].x + 1;
				if (facePositions[i].y >= size__In_units.y)
					size__In_units.y = facePositions[i].y + 1;
			}
		uint8_t* pImageData = LoadFile(filepath, (ivec2&)size, false, flip);
		if (!pImageData)
			return;
		if (size.x % size__In_units.x || size.y % size__In_units.y) {
			std::cout <<
				"[ textureCube ]\nImage not available!\nFile: " << filepath << std::endl <<
				"Image width must be in multiples of " << size__In_units.x << std::endl <<
				"Image height must be in multiples of " << size__In_units.y << std::endl;
			free(pImageData);
			return;
		}
		uvec2 unitSize = size / size__In_units;
		uint32_t* pImageDatas[6]{};
		for (size_t face = 0; face < 6; face++) {
			pImageDatas[face] = new uint32_t[unitSize.x * unitSize.y];
			uint32_t offset = 0;
			if (lookFromOutside)
				if (face != 2 && face != 3)
					for (uint32_t i = 0; i < unitSize.y; i++)
						for (uint32_t j = 0; j < unitSize.x; j++)
							pImageDatas[face][offset] = ((uint32_t*)pImageData)[unitSize.x - 1 - j + facePositions[face].x * unitSize.x + (i + facePositions[face].y * unitSize.y) * size.x],
							offset++;
				else
					for (uint32_t j = 0; j < unitSize.y; j++)
						for (uint32_t k = 0; k < unitSize.x; k++)
							pImageDatas[face][offset] = ((uint32_t*)pImageData)[k + facePositions[face].x * unitSize.x + ((unitSize.y - 1 - j) + facePositions[face].y * unitSize.y) * size.x],
							offset++;
			else
				for (uint32_t j = 0; j < unitSize.y; j++)
					memcpy(pImageDatas[face] + offset, (uint32_t*)pImageData + facePositions[face].x * unitSize.x + (j + facePositions[face].y * unitSize.y) * size.x, unitSize.x * 4),
					offset += unitSize.x;
		}
		free(pImageData);
		size = unitSize;
		Constructor_Internal((uint8_t**)pImageDatas, generateMipmap);
	}
	//Order, in left handed coordinate, looking from inside:
	//right(+x) left(-x) top(+y) bottom(-y) front(+z) back(-z)
	//Not related to NDC.
	//If lookFromOutside is true, the order is the same.
	textureCube(const char** filepaths, bool lookFromOutside = false, bool generateMipmap = true, bool flip = false) {
		uint8_t* pImageDatas[6]{};
		for (size_t i = 0; i < 6; i++) {
			uvec2 size_currentFace = {};
			pImageDatas[i] = LoadFile(filepaths[i], (ivec2&)size_currentFace, false, flip);
			if (pImageDatas[i]) {
				if (i == 0)
					size = size_currentFace;
				continue;
			}
			if (size_currentFace != size)
				std::cout <<
				"[ textureCube ]\nImage not available!\nFile: " << filepaths[i] << std::endl <<
				"All the image width must be in same size!\n";
			for (size_t i = 0; i < 6; i++)
				free(pImageDatas[i]);
			return;
		}
		if (lookFromOutside) {
			uint32_t* pTextureDatas[6]{};
			for (size_t face = 0; face < 6; face++) {
				pTextureDatas[face] = new uint32_t[size.x * size.y];
				if (face != 2 && face != 3)
					for (uint32_t j = 0; j < size.y; j++)
						for (uint32_t i = 0; i < size.x; i++)
							pTextureDatas[face][j * size.x + i] = ((uint32_t*)pImageDatas[face])[(j + 1) * size.x - 1 - i];
				else
					for (uint32_t j = 0; j < size.y; j++)
						for (uint32_t i = 0; i < size.x; i++)
							pTextureDatas[face][j * size.x + i] = ((uint32_t*)pImageDatas[face])[(size.y - 1 - j) * size.x + i];
				free(pImageDatas[face]);
				pImageDatas[face] = (uint8_t*)pTextureDatas[face];
			}
		}
		Constructor_Internal(pImageDatas, generateMipmap);
	}
};

#pragma endregion
NAMESPACE_END

inline glm::mat4 FlipVertical(const glm::mat4& projection) {
	glm::mat4 _projection = projection;
	for (uint32_t i = 0; i < 4; i++)
		_projection[i][1] *= -1;
	return _projection;
}