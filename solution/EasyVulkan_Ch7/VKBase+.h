#pragma once
#include "VKBase.h"
#include "VKFormat.h"

namespace vulkan {
	class graphicsBasePlus {
		VkFormatProperties formatProperties[std::size(formatInfos_v1_0)] = {};
		commandPool commandPool_graphics;
		commandPool commandPool_presentation;
		commandPool commandPool_compute;
		commandBuffer commandBuffer_transfer;
		commandBuffer commandBuffer_presentation;
		//Static
		static graphicsBasePlus singleton;
		//--------------------
		graphicsBasePlus() {
			auto Initialize = [] {
				if (graphicsBase::Base().QueueFamilyIndex_Graphics() != VK_QUEUE_FAMILY_IGNORED)
					singleton.commandPool_graphics.Create(graphicsBase::Base().QueueFamilyIndex_Graphics(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
					singleton.commandPool_graphics.AllocateBuffers(singleton.commandBuffer_transfer);
				if (graphicsBase::Base().QueueFamilyIndex_Compute() != VK_QUEUE_FAMILY_IGNORED)
					singleton.commandPool_compute.Create(graphicsBase::Base().QueueFamilyIndex_Compute(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
				if (graphicsBase::Base().QueueFamilyIndex_Presentation() != VK_QUEUE_FAMILY_IGNORED &&
					graphicsBase::Base().QueueFamilyIndex_Presentation() != graphicsBase::Base().QueueFamilyIndex_Graphics() &&
					graphicsBase::Base().SwapchainCreateInfo().imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)
					singleton.commandPool_presentation.Create(graphicsBase::Base().QueueFamilyIndex_Presentation(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT),
					singleton.commandPool_presentation.AllocateBuffers(singleton.commandBuffer_presentation);
				for (size_t i = 0; i < std::size(singleton.formatProperties); i++)
					vkGetPhysicalDeviceFormatProperties(graphicsBase::Base().PhysicalDevice(), VkFormat(i), &singleton.formatProperties[i]);
			};
			auto CleanUp = [] {
				singleton.commandPool_graphics.~commandPool();
				singleton.commandPool_presentation.~commandPool();
				singleton.commandPool_compute.~commandPool();
			};
			graphicsBase::Plus(singleton);
			graphicsBase::Base().AddCallback_CreateDevice(Initialize);
			graphicsBase::Base().AddCallback_DestroyDevice(CleanUp);
		}
		graphicsBasePlus(graphicsBasePlus&&) = delete;
		~graphicsBasePlus() = default;
	public:
		//Getter
		const VkFormatProperties& FormatProperties(VkFormat format) const {
#ifndef NDEBUG
			if (uint32_t(format) >= std::size(formatInfos_v1_0))
				outStream << std::format("[ FormatProperties ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n"),
				abort();
#endif
			return formatProperties[format];
		}
		const commandPool& CommandPool_Graphics() const { return commandPool_graphics; }
		const commandPool& CommandPool_Compute() const { return commandPool_compute; }
		const commandBuffer& CommandBuffer_Transfer() const { return commandBuffer_transfer; }
		//Const Function
		result_t ExecuteCommandBuffer_Graphics(VkCommandBuffer commandBuffer) const {
			fence fence;
			VkSubmitInfo submitInfo = {
				.commandBufferCount = 1,
				.pCommandBuffers = &commandBuffer
			};
			VkResult result = graphicsBase::Base().SubmitCommandBuffer_Graphics(submitInfo, fence);
			if (!result)
				fence.Wait();
			return result;
		}
		result_t ExecuteCommandBuffer_Compute(VkCommandBuffer commandBuffer) const {
			fence fence;
			VkSubmitInfo submitInfo = {
				.commandBufferCount = 1,
				.pCommandBuffers = &commandBuffer
			};
			VkResult result = graphicsBase::Base().SubmitCommandBuffer_Compute(submitInfo, fence);
			if (!result)
				fence.Wait();
			return result;
		}
		result_t AcquireImageOwnership_Presentation(VkSemaphore semaphore_renderingIsOver, VkSemaphore semaphore_ownershipIsTransfered, VkFence fence = VK_NULL_HANDLE) const {
			if (VkResult result = commandBuffer_presentation.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
				return result;
			graphicsBase::Base().CmdTransferImageOwnership(commandBuffer_presentation);
			if (VkResult result = commandBuffer_presentation.End())
				return result;
			return graphicsBase::Base().SubmitCommandBuffer_Presentation(commandBuffer_presentation, semaphore_renderingIsOver, semaphore_ownershipIsTransfered, fence);
		}
	};
	inline graphicsBasePlus graphicsBasePlus::singleton;

	//Format Related
	constexpr formatInfo FormatInfo(VkFormat format) {
#ifndef NDEBUG
		if (uint32_t(format) >= std::size(formatInfos_v1_0))
			outStream << std::format("[ FormatInfo ] ERROR\nThis function only supports definite formats provided by VK_VERSION_1_0.\n"),
			abort();
#endif
		return formatInfos_v1_0[uint32_t(format)];
	}
	constexpr VkFormat Corresponding16BitFloatFormat(VkFormat format_32BitFloat) {
		switch (format_32BitFloat) {
		case VK_FORMAT_R32_SFLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case VK_FORMAT_R32G32_SFLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return VK_FORMAT_R16G16B16_SFLOAT;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		}
		return format_32BitFloat;
	}
	inline const VkFormatProperties& FormatProperties(VkFormat format) {
		return graphicsBase::Plus().FormatProperties(format);
	}

	//Pipeline Related
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
			createInfo.basePipelineIndex = -1;
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

	//Synchronization
	class timelineSemaphore : semaphore {
	public:
		timelineSemaphore(uint64_t initialValue = 0) {
			Create(initialValue);
		}
		//Getter
#ifndef NDEBUG
		using semaphore::operator volatile VkSemaphore;
#else
		using semaphore::operator VkSemaphore;
#endif
		using semaphore::Address;
		//Const Function
		result_t Wait(uint64_t value) const {
			return Wait(*this, value);
		}
		result_t Signal(uint64_t value = 0) const {
			VkSemaphoreSignalInfo signalInfo = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
				.semaphore = *this,
				.value = value
			};
			VkResult result = vkSignalSemaphore(graphicsBase::Base().Device(), &signalInfo);
			if (result)
				outStream << std::format("[ timelineSemaphore ] ERROR\nFailed to signal the semaphore!\nError code: {}\n", int32_t(result));
			return result;
		}
		result_t GetValue(uint64_t& value) const {
			VkResult result = vkGetSemaphoreCounterValue(graphicsBase::Base().Device(), *this, &value);
			if (result)
				outStream << std::format("[ timelineSemaphore ] ERROR\nFailed to get the counter value of the semaphore!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Non-const Function
		result_t Create(uint64_t initialValue = 0) {
			VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
				.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
				.initialValue = initialValue
			};
			VkSemaphoreCreateInfo createInfo = {
				.pNext = &semaphoreTypeCreateInfo
			};
			return semaphore::Create(createInfo);
		}
		//Static Functino
		static result_t Wait(arrayRef<const timelineSemaphore> semaphores, arrayRef<uint64_t> values, bool waitAll = true) {
			if (semaphores.Count() != values.Count())
				if (semaphores.Count() < values.Count()) {
					outStream << std::format("[ timelineSemaphore ] ERROR\nFor each semaphore, must provide a corresponding counter value!\n");
					return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
				}
				else
					outStream << std::format("[ timelineSemaphore ] WARNING\nProvided counter values are more than semaphores!\n");
			VkSemaphoreWaitInfo waitInfo = {
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
				.flags = waitAll ? VkFlags(0) : VK_SEMAPHORE_WAIT_ANY_BIT,
				.semaphoreCount = uint32_t(semaphores.Count()),
				.pSemaphores = semaphores[0].Address(),
				.pValues = values.Pointer()
			};
			VkResult result = vkWaitSemaphores(graphicsBase::Base().Device(), &waitInfo, UINT64_MAX);
			if (result)
				outStream << std::format("[ timelineSemaphore ] ERROR\nFailed to wait for semaphores!\nError code: {}\n", int32_t(result));
			return result;
		}
	};

	//Buffer
	class stagingBuffer {
		static inline class {
			stagingBuffer* pointer = Create();
			stagingBuffer* Create() {
				static stagingBuffer stagingBuffer;
				graphicsBase::Base().AddCallback_DestroyDevice([] { stagingBuffer.~stagingBuffer(); });
				return &stagingBuffer;
			}
		public:
			stagingBuffer& Get() const { return *pointer; }
		} stagingBuffer_mainThread;
	protected:
		bufferMemory bufferMemory;
		VkDeviceSize memoryUsage = 0;
		image aliasedImage;
	public:
		stagingBuffer() = default;
		stagingBuffer(VkDeviceSize size) {
			Expand(size);
		}
		//Getter
		operator VkBuffer() const { return bufferMemory.Buffer(); }
		const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
		VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
		VkImage AliasedImage() const { return aliasedImage; }
		//Const Function
		void RetrieveData(void* pData_src, VkDeviceSize size) const {
			bufferMemory.RetrieveData(pData_src, size);
		}
		//Non-const Function
		void Expand(VkDeviceSize size) {
			if (size <= AllocationSize())
				return;
			Release();
			VkBufferCreateInfo bufferCreateInfo = {
				.size = size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			};
			bufferMemory.Create(bufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		}
		void Release() {
			bufferMemory.~bufferMemory();
		}
		void* MapMemory(VkDeviceSize size) {
			Expand(size);
			void* pData_dst = nullptr;
			bufferMemory.MapMemory(pData_dst, size);
			memoryUsage = size;
			return pData_dst;
		}
		void UnmapMemory() {
			bufferMemory.UnmapMemory(memoryUsage);
			memoryUsage = 0;
		}
		void BufferData(const void* pData_src, VkDeviceSize size) {
			Expand(size);
			bufferMemory.BufferData(pData_src, size);
		}
		[[nodiscard]]
		VkImage AliasedImage2d(VkFormat format, VkExtent2D extent) {
			if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
				return VK_NULL_HANDLE;
			VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format).sizePerPixel) * extent.width * extent.height;
			if (imageDataSize > AllocationSize())
				return VK_NULL_HANDLE;
			VkImageFormatProperties imageFormatProperties = {};
			vkGetPhysicalDeviceImageFormatProperties(graphicsBase::Base().PhysicalDevice(),
				format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, 0, &imageFormatProperties);
			if (extent.width > imageFormatProperties.maxExtent.width ||
				extent.height > imageFormatProperties.maxExtent.height ||
				imageDataSize > imageFormatProperties.maxResourceSize)
				return VK_NULL_HANDLE;
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_LINEAR,
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED
			};
			aliasedImage.~image();
			aliasedImage.Create(imageCreateInfo);
			VkImageSubresource subResource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
			VkSubresourceLayout subresourceLayout = {};
			vkGetImageSubresourceLayout(graphicsBase::Base().Device(), aliasedImage, &subResource, &subresourceLayout);
			if (subresourceLayout.size != imageDataSize)
				return VK_NULL_HANDLE;//No padding bytes
			aliasedImage.BindMemory(bufferMemory.Memory());
			return aliasedImage;
		}
		//Static Function
		static VkBuffer Buffer_MainThread() {
			return stagingBuffer_mainThread.Get();
		}
		static void Expand_MainThread(VkDeviceSize size) {
			stagingBuffer_mainThread.Get().Expand(size);
		}
		static void Release_MainThread() {
			stagingBuffer_mainThread.Get().Release();
		}
		static void* MapMemory_MainThread(VkDeviceSize size) {
			return stagingBuffer_mainThread.Get().MapMemory(size);
		}
		static void UnmapMemory_MainThread() {
			stagingBuffer_mainThread.Get().UnmapMemory();
		}
		static void BufferData_MainThread(const void* pData_src, VkDeviceSize size) {
			stagingBuffer_mainThread.Get().BufferData(pData_src, size);
		}
		static void RetrieveData_MainThread(void* pData_src, VkDeviceSize size) {
			stagingBuffer_mainThread.Get().RetrieveData(pData_src, size);
		}
		[[nodiscard]]
		static VkImage AliasedImage2d_MainThread(VkFormat format, VkExtent2D extent) {
			return stagingBuffer_mainThread.Get().AliasedImage2d(format, extent);
		}
	};

	class deviceLocalBuffer {
	protected:
		bufferMemory bufferMemory;
	public:
		deviceLocalBuffer() = default;
		deviceLocalBuffer(VkDeviceSize size, VkBufferUsageFlags desiredUsages__Without_transfer_dst) {
			Create(size, desiredUsages__Without_transfer_dst);
		}
		//Getter
		operator VkBuffer() const { return bufferMemory.Buffer(); }
		const VkBuffer* Address() const { return bufferMemory.AddressOfBuffer(); }
		VkDeviceSize AllocationSize() const { return bufferMemory.AllocationSize(); }
		//Const Function
		void TransferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
			if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				bufferMemory.BufferData(pData_src, size, offset);
				return;
			}
			stagingBuffer::BufferData_MainThread(pData_src, size);
			auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VkBufferCopy region = { 0, offset, size };
			vkCmdCopyBuffer(commandBuffer, stagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), 1, &region);
			commandBuffer.End();
			graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
		}
		void TransferData(const void* pData_src, uint32_t elementCount, VkDeviceSize elementSize, VkDeviceSize stride_src, VkDeviceSize stride_dst, VkDeviceSize offset = 0) const {
			if (bufferMemory.MemoryProperties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
				void* pData_dst = nullptr;
				bufferMemory.MapMemory(pData_dst, stride_dst * elementCount, offset);
				for (size_t i = 0; i < elementCount; i++)
					memcpy(stride_dst * i + static_cast<uint8_t*>(pData_dst), stride_src * i + static_cast<const uint8_t*>(pData_src), size_t(elementSize));
				bufferMemory.UnmapMemory(elementCount * stride_dst, offset);
				return;
			}
			stagingBuffer::BufferData_MainThread(pData_src, stride_src * elementCount);
			auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			std::unique_ptr<VkBufferCopy[]> regions = std::make_unique<VkBufferCopy[]>(elementCount);
			for (size_t i = 0; i < elementCount; i++)
				regions[i] = { stride_src * i, stride_dst * i + offset, elementSize };
			vkCmdCopyBuffer(commandBuffer, stagingBuffer::Buffer_MainThread(), bufferMemory.Buffer(), elementCount, regions.get());
			commandBuffer.End();
			graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
		}
		void TransferData(const auto& data_src) const {
			TransferData(&data_src, sizeof data_src);
		}
		void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const void* pData_src, VkDeviceSize size__Less_than_65536, VkDeviceSize offset = 0) const {
			vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), offset, size__Less_than_65536, pData_src);
		}
		void CmdUpdateBuffer(VkCommandBuffer commandBuffer, const auto& data_src) const {
			vkCmdUpdateBuffer(commandBuffer, bufferMemory.Buffer(), 0, sizeof data_src, &data_src);
		}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags desiredUsages__Without_transfer_dst) {
			VkBufferCreateInfo bufferCreateInfo = {
				.size = size,
				.usage = desiredUsages__Without_transfer_dst | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			};
			false ||
				bufferMemory.CreateBuffer(bufferCreateInfo) ||
				bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
				bufferMemory.AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) ||
				bufferMemory.BindMemory();
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags desiredUsages__Without_transfer_dst) {
			graphicsBase::Base().WaitIdle();
			bufferMemory.~bufferMemory();
			Create(size, desiredUsages__Without_transfer_dst);
		}
	};
	class vertexBuffer :public deviceLocalBuffer {
	public:
		vertexBuffer() = default;
		vertexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | otherUsages);
		}
	};
	class indexBuffer :public deviceLocalBuffer {
	public:
		indexBuffer() = default;
		indexBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | otherUsages);
		}
	};
	class uniformBuffer :public deviceLocalBuffer {
	public:
		uniformBuffer() = default;
		uniformBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | otherUsages);
		}
		//Static Function
		static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize) {
			const VkDeviceSize& alignment = graphicsBase::Base().PhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
			return alignment + dataSize - 1 & ~(alignment - 1);
		}
	};
	class storageBuffer :public deviceLocalBuffer {
	public:
		storageBuffer() = default;
		storageBuffer(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) :deviceLocalBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages) {}
		//Non-const Function
		void Create(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Create(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
		}
		void Recreate(VkDeviceSize size, VkBufferUsageFlags otherUsages = 0) {
			deviceLocalBuffer::Recreate(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | otherUsages);
		}
		//Static Function
		static VkDeviceSize CalculateAlignedSize(VkDeviceSize dataSize) {
			const VkDeviceSize& alignment = graphicsBase::Base().PhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
			return alignment + dataSize - 1 & ~(alignment - 1);
		}
	};

	//Attachment
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
		const VkImageView* AddressOfImageView() const { return imageView.Address(); }
		const VkImage* AddressOfImage() const { return imageMemory.AddressOfImage(); }
		//Const Function
		VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler) const {
			return { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
	};

	class colorAttachment :public attachment {
	public:
		colorAttachment() = default;
		colorAttachment(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
			Create(format, extent, layerCount, sampleCount, otherUsages);
		}
		//Non-const Function
		void Create(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0) {
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = layerCount,
				.samples = sampleCount,
				.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | otherUsages
			};
			imageMemory.Create(
				imageCreateInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			imageView.Create(
				imageMemory.Image(),
				layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
				format,
				{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount });
		}
		//Static Function
		static bool FormatAvailability(VkFormat format, bool supportBlending = true) {
			return FormatProperties(format).optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT << uint32_t(supportBlending);
		}
	};
	class depthStencilAttachment :public attachment {
	public:
		depthStencilAttachment() = default;
		depthStencilAttachment(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0, bool stencilOnly = false) {
			Create(format, extent, layerCount, sampleCount, otherUsages, stencilOnly);
		}
		//Non-const Function
		void Create(VkFormat format, VkExtent2D extent, uint32_t layerCount = 1, VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageUsageFlags otherUsages = 0, bool stencilOnly = false) {
			VkImageCreateInfo imageCreateInfo = {
				.imageType = VK_IMAGE_TYPE_2D,
				.format = format,
				.extent = { extent.width, extent.height, 1 },
				.mipLevels = 1,
				.arrayLayers = layerCount,
				.samples = sampleCount,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | otherUsages
			};
			imageMemory.Create(
				imageCreateInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | bool(otherUsages & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) * VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
			VkImageAspectFlags aspectMask = (!stencilOnly) * VK_IMAGE_ASPECT_DEPTH_BIT;
			if (format > VK_FORMAT_S8_UINT)
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			else if (format == VK_FORMAT_S8_UINT)
				aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			imageView.Create(
				imageMemory.Image(),
				layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
				format,
				{ aspectMask, 0, 1, 0, layerCount });
		}
		//Static Function
		static bool FormatAvailability(VkFormat format) {
			return FormatProperties(format).optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
	};

	//Texture
	struct imageOperation {
		struct imageMemoryBarrierParameterPack {
			const bool isNeeded = false;
			const VkPipelineStageFlags stage = 0;
			const VkAccessFlags access = 0;
			const VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
			constexpr imageMemoryBarrierParameterPack() = default;
			constexpr imageMemoryBarrierParameterPack(VkPipelineStageFlags stage, VkAccessFlags access, VkImageLayout layout) :
				isNeeded(true), stage(stage), access(access), layout(layout) {}
		};
		static void CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, const VkBufferImageCopy& region,
			imageMemoryBarrierParameterPack imb_from, imageMemoryBarrierParameterPack imb_to) {
			//Pre-copy barrier
			VkImageMemoryBarrier imageMemoryBarrier = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				imb_from.access,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				imb_from.layout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,//No ownership transfer
				VK_QUEUE_FAMILY_IGNORED,
				image,
				{
					region.imageSubresource.aspectMask,
					region.imageSubresource.mipLevel,
					1,
					region.imageSubresource.baseArrayLayer,
					region.imageSubresource.layerCount }
			};
			if (imb_from.isNeeded)
				vkCmdPipelineBarrier(commandBuffer, imb_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			//Copy
			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			//Post-copy barrier
			if (imb_to.isNeeded) {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.dstAccessMask = imb_to.access;
				imageMemoryBarrier.newLayout = imb_to.layout;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}
		static void CmdBlitImage(VkCommandBuffer commandBuffer, VkImage image_src, VkImage image_dst, const VkImageBlit& region,
			imageMemoryBarrierParameterPack imb_dst_from, imageMemoryBarrierParameterPack imb_dst_to, VkFilter filter = VK_FILTER_LINEAR) {
			//Pre-blit barrier
			VkImageMemoryBarrier imageMemoryBarrier = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				imb_dst_from.access,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				imb_dst_from.layout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image_dst,
				{
					region.dstSubresource.aspectMask,
					region.dstSubresource.mipLevel,
					1,
					region.dstSubresource.baseArrayLayer,
					region.dstSubresource.layerCount }
			};
			if (imb_dst_from.isNeeded)
				vkCmdPipelineBarrier(commandBuffer, imb_dst_from.stage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			//Blit
			vkCmdBlitImage(commandBuffer,
				image_src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image_dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &region, filter);
			//Post-blit barrier
			if (imb_dst_to.isNeeded) {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.dstAccessMask = imb_dst_to.access;
				imageMemoryBarrier.newLayout = imb_dst_to.layout;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_dst_to.stage, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}
		static void CmdGenerateMipmap2d(VkCommandBuffer commandBuffer, VkImage image, VkExtent2D imageExtent, uint32_t mipLevelCount, uint32_t layerCount,
			imageMemoryBarrierParameterPack imb_to, VkFilter minFilter = VK_FILTER_LINEAR) {
			auto MipmapExtent = [](VkExtent2D imageExtent, uint32_t mipLevel) {
				VkOffset3D extent = { int32_t(imageExtent.width >> mipLevel), int32_t(imageExtent.height >> mipLevel), 1 };
				extent.x += !extent.x;
				extent.y += !extent.y;
				return extent;
			};
			//Blit
			if (layerCount > 1) {
				std::unique_ptr<VkImageBlit[]> regions = std::make_unique<VkImageBlit[]>(layerCount);
				for (uint32_t i = 1; i < mipLevelCount; i++) {
					VkOffset3D mipmapExtent_src = MipmapExtent(imageExtent, i - 1);
					VkOffset3D mipmapExtent_dst = MipmapExtent(imageExtent, i);
					for (uint32_t j = 0; j < layerCount; j++)
						regions[j] = {
							{ VK_IMAGE_ASPECT_COLOR_BIT, i - 1, j, 1 },	//srcSubresource
							{ {}, mipmapExtent_src },					//srcOffsets
							{ VK_IMAGE_ASPECT_COLOR_BIT, i, j, 1 },		//dstSubresource
							{ {}, mipmapExtent_dst }					//dstOffsets
						};
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
					vkCmdBlitImage(commandBuffer,
						image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						layerCount, regions.get(), minFilter);
					//Post-blit barrier
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
				}
			}
			else
				for (uint32_t i = 1; i < mipLevelCount; i++) {
					VkImageBlit region = {
						{ VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, layerCount },//srcSubresource
						{ {}, MipmapExtent(imageExtent, i - 1) },			//srcOffsets
						{ VK_IMAGE_ASPECT_COLOR_BIT, i, 0, layerCount },	//dstSubresource
						{ {}, MipmapExtent(imageExtent, i) }				//dstOffsets
					};
					CmdBlitImage(commandBuffer, image, image, region,
						{ VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED },
						{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }, minFilter);
				}
			//Post-blit barrier
			if (imb_to.isNeeded) {
				VkImageMemoryBarrier imageMemoryBarrier = {
					VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					nullptr,
					0,
					imb_to.access,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					imb_to.layout,
					VK_QUEUE_FAMILY_IGNORED,
					VK_QUEUE_FAMILY_IGNORED,
					image,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, layerCount }
				};
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, imb_to.stage, 0,
					0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}
	};

	class texture {
	protected:
		imageView imageView;
		imageMemory imageMemory;
		//--------------------
		texture() = default;
		void CreateImageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageCreateFlags flags = 0) {
			VkImageCreateInfo imageCreateInfo = {
				.flags = flags,
				.imageType = imageType,
				.format = format,
				.extent = extent,
				.mipLevels = mipLevelCount,
				.arrayLayers = arrayLayerCount,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			};
			imageMemory.Create(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
		void CreateImageView(VkImageViewType viewType, VkFormat format, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkImageViewCreateFlags flags = 0) {
			imageView.Create(imageMemory.Image(), viewType, format, { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevelCount, 0, arrayLayerCount }, flags);
		}
		//Static Function
		static std::unique_ptr<uint8_t[]> LoadFile_Internal(const auto* address, size_t fileSize, VkExtent2D& extent, formatInfo requiredFormatInfo) {
#ifndef NDEBUG
			if (!(requiredFormatInfo.rawDataType == formatInfo::floatingPoint && requiredFormatInfo.sizePerComponent == 4) &&
				!(requiredFormatInfo.rawDataType == formatInfo::integer && Between_Closed<int32_t>(1, requiredFormatInfo.sizePerComponent, 2)))
				outStream << std::format("[ texture ] ERROR\nRequired format is not available for source image data!\n"),
				abort();
#endif
			int& width = reinterpret_cast<int&>(extent.width);
			int& height = reinterpret_cast<int&>(extent.height);
			int channelCount;
			void* pImageData = nullptr;
			if constexpr (std::same_as<decltype(address), const char*>) {
				if (requiredFormatInfo.rawDataType == formatInfo::integer)
					if (requiredFormatInfo.sizePerComponent == 1)
						pImageData = stbi_load(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
					else
						pImageData = stbi_load_16(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				else
					pImageData = stbi_loadf(address, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				if (!pImageData)
					outStream << std::format("[ texture ] ERROR\nFailed to load the file: {}\n", address);
			}
			if constexpr (std::same_as<decltype(address), const uint8_t*>) {
				if (fileSize > INT32_MAX) {
					outStream << std::format("[ texture ] ERROR\nFailed to load image data from the given address! Data size must be less than 2G!\n");
					return {};
				}
				if (requiredFormatInfo.rawDataType == formatInfo::integer)
					if (requiredFormatInfo.sizePerComponent == 1)
						pImageData = stbi_load_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
					else
						pImageData = stbi_load_16_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				else
					pImageData = stbi_loadf_from_memory(address, fileSize, &width, &height, &channelCount, requiredFormatInfo.componentCount);
				if (!pImageData)
					outStream << std::format("[ texture ] ERROR\nFailed to load image data from the given address!\n");
			}
			return std::unique_ptr<uint8_t[]>(static_cast<uint8_t*>(pImageData));
		}
	public:
		//Getter
		VkImageView ImageView() const { return imageView; }
		VkImage Image() const { return imageMemory.Image(); }
		const VkImageView* AddressOfImageView() const { return imageView.Address(); }
		const VkImage* AddressOfImage() const { return imageMemory.AddressOfImage(); }
		//Const Function
		VkDescriptorImageInfo DescriptorImageInfo(VkSampler sampler) const {
			return { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		}
		//Static Function
		/*CheckArguments(...) should only be called in tests*/
		static bool CheckArguments(VkImageType imageType, VkExtent3D extent, uint32_t arrayLayerCount, VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
			auto AliasedImageAvailability = [](VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t arrayLayerCount, VkImageUsageFlags usage) {
				if (!(FormatProperties(format).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
					return false;
				VkImageFormatProperties imageFormatProperties = {};
				vkGetPhysicalDeviceImageFormatProperties(
					graphicsBase::Base().PhysicalDevice(),
					format,
					imageType,
					VK_IMAGE_TILING_LINEAR,
					usage,
					0,
					&imageFormatProperties);
				VkDeviceSize imageDataSize = VkDeviceSize(FormatInfo(format).sizePerPixel) * extent.width * extent.height * extent.depth;
				return
					extent.width <= imageFormatProperties.maxExtent.width &&
					extent.height <= imageFormatProperties.maxExtent.height &&
					extent.depth <= imageFormatProperties.maxExtent.depth &&
					arrayLayerCount <= imageFormatProperties.maxArrayLayers &&
					imageDataSize <= imageFormatProperties.maxResourceSize;
			};
			if (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
				//Case: Copy data from pre-initialized image to final image
				if (FormatProperties(format_initial).linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
					if (AliasedImageAvailability(imageType, format_initial, extent, arrayLayerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
						if (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT &&
							generateMipmap * (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
							return true;
				//Case: Copy data from staging buffer to final image
				if (format_initial == format_final)
					return
					FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT &&
					generateMipmap * (FormatProperties(format_final).optimalTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT));
				//Case: Copy data from staging buffer to initial image, then blit initial image to final image
				else
					return
					FormatProperties(format_initial).optimalTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT) &&
					FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT &&
					generateMipmap * (FormatProperties(format_final).optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
			}
			return false;
		}
		[[nodiscard]]
		static std::unique_ptr<uint8_t[]> LoadFile(const char* filepath, VkExtent2D& extent, formatInfo requiredFormatInfo) {
			return LoadFile_Internal(filepath, 0, extent, requiredFormatInfo);
		}
		[[nodiscard]]
		static std::unique_ptr<uint8_t[]> LoadFile(const uint8_t* fileBinaries, size_t fileSize, VkExtent2D& extent, formatInfo requiredFormatInfo) {
			return LoadFile_Internal(fileBinaries, fileSize, extent, requiredFormatInfo);
		}
		static uint32_t CalculateMipLevelCount(VkExtent2D extent) {
			return uint32_t(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
		}
		static void CopyBlitAndGenerateMipmap2d(VkBuffer buffer_copyFrom, VkImage image_copyTo, VkImage image_blitTo, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR) {
			static constexpr imageOperation::imageMemoryBarrierParameterPack imbs[2] = {
				{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
			};
			bool generateMipmap = mipLevelCount > 1;
			bool blitMipLevel0 = image_copyTo != image_blitTo;
			auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
			commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			VkBufferImageCopy region = {
				.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
				.imageExtent = { imageExtent.width, imageExtent.height, 1 }
			};
			imageOperation::CmdCopyBufferToImage(commandBuffer, buffer_copyFrom, image_copyTo, region,
				{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap || blitMipLevel0]);
			//Blit to another image if necessary
			if (blitMipLevel0) {
				VkImageBlit region = {
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
					{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
					{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
				};
				imageOperation::CmdBlitImage(commandBuffer, image_copyTo, image_blitTo, region,
					{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);
			}
			//Generate mipmap if necessary, transition layout
			if (generateMipmap)
				imageOperation::CmdGenerateMipmap2d(commandBuffer, image_blitTo, imageExtent, mipLevelCount, layerCount,
					{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
			commandBuffer.End();
			//Submit
			graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
		}
		static void BlitAndGenerateMipmap2d(VkImage image_preinitialized, VkImage image_final, VkExtent2D imageExtent,
			uint32_t mipLevelCount = 1, uint32_t layerCount = 1, VkFilter minFilter = VK_FILTER_LINEAR) {
			static constexpr imageOperation::imageMemoryBarrierParameterPack imbs[2] = {
				{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
				{ VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL }
			};
			bool generateMipmap = mipLevelCount > 1;
			bool blitMipLevel0 = image_preinitialized != image_final;
			if (generateMipmap || blitMipLevel0) {
				auto& commandBuffer = graphicsBase::Plus().CommandBuffer_Transfer();
				commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
				//Blit to another image if necessary
				if (blitMipLevel0) {
					VkImageMemoryBarrier imageMemoryBarrier = {
						VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						nullptr,
						0,
						VK_ACCESS_TRANSFER_READ_BIT,
						VK_IMAGE_LAYOUT_PREINITIALIZED,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						VK_QUEUE_FAMILY_IGNORED,
						VK_QUEUE_FAMILY_IGNORED,
						image_preinitialized,
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layerCount }
					};
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
					VkImageBlit region = {
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
						{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } },
						{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, layerCount },
						{ {}, { int32_t(imageExtent.width), int32_t(imageExtent.height), 1 } }
					};
					imageOperation::CmdBlitImage(commandBuffer, image_preinitialized, image_final, region,
						{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, VK_IMAGE_LAYOUT_UNDEFINED }, imbs[generateMipmap], minFilter);
				}
				//Generate mipmap if necessary, transition layout
				if (generateMipmap)
					imageOperation::CmdGenerateMipmap2d(commandBuffer, image_final, imageExtent, mipLevelCount, layerCount,
						{ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }, minFilter);
				commandBuffer.End();
				//Submit
				graphicsBase::Plus().ExecuteCommandBuffer_Graphics(commandBuffer);
			}
		}
		static VkSamplerCreateInfo SamplerCreateInfo() {
			return {
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_LINEAR,
				.minFilter = VK_FILTER_LINEAR,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				.mipLodBias = 0.f,
				.anisotropyEnable = VK_TRUE,
				.maxAnisotropy = graphicsBase::Base().PhysicalDeviceProperties().limits.maxSamplerAnisotropy,
				.compareEnable = VK_FALSE,
				.compareOp = VK_COMPARE_OP_ALWAYS,
				.minLod = 0.f,
				.maxLod = VK_LOD_CLAMP_NONE,
				.borderColor = {},
				.unnormalizedCoordinates = VK_FALSE
			};
		}
	};

	class texture2d :public texture {
	protected:
		VkExtent2D extent = {};
		//--------------------
		void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
			uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;
			//Create image and allocate memory
			CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, 1);
			//Create view
			CreateImageView(VK_IMAGE_VIEW_TYPE_2D, format_final, mipLevelCount, 1);
			//Copy data and generate mipmap
			if (format_initial == format_final)
				CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory.Image(), imageMemory.Image(), extent, mipLevelCount, 1);
			else
				if (VkImage image_conversion = stagingBuffer::AliasedImage2d_MainThread(format_initial, extent))
					BlitAndGenerateMipmap2d(image_conversion, imageMemory.Image(), extent, mipLevelCount, 1);
				else {
					VkImageCreateInfo imageCreateInfo = {
						.imageType = VK_IMAGE_TYPE_2D,
						.format = format_initial,
						.extent = { extent.width, extent.height, 1 },
						.mipLevels = 1,
						.arrayLayers = 1,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
					};
					vulkan::imageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
					CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory_conversion.Image(), imageMemory.Image(), extent, mipLevelCount, 1);
				}
		}
	public:
		texture2d() = default;
		texture2d(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(filepath, format_initial, format_final, generateMipmap);
		}
		texture2d(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(pImageData, extent, format_initial, format_final, generateMipmap);
		}
		//Getter
		VkExtent2D Extent() const { return extent; }
		uint32_t Width() const { return extent.width; }
		uint32_t Height() const { return extent.height; }
		//Non-const Function
		void Create(const char* filepath, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			VkExtent2D extent;
			formatInfo formatInfo = FormatInfo(format_initial);
			std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, extent, formatInfo);
			if (pImageData)
				Create(pImageData.get(), extent, format_initial, format_final, generateMipmap);
		}
		void Create(const uint8_t* pImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			this->extent = extent;
			//Copy data to staging buffer
			size_t imageDataSize = size_t(FormatInfo(format_initial).sizePerPixel) * extent.width * extent.height;
			stagingBuffer::BufferData_MainThread(pImageData, imageDataSize);
			//Create image and allocate memory, create image view, then copy data from staging buffer to image
			Create_Internal(format_initial, format_final, generateMipmap);
		}
	};
	class texture2dArray :public texture {
	protected:
		VkExtent2D extent = {};
		uint32_t layerCount = 0;
		//--------------------
		void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
			//Create image and allocate memory
			uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;
			CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, layerCount);
			//Create view
			CreateImageView(VK_IMAGE_VIEW_TYPE_2D_ARRAY, format_final, mipLevelCount, layerCount);
			//Copy data and generate mipmap
			if (format_initial == format_final)
				CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory.Image(), imageMemory.Image(), extent, mipLevelCount, layerCount);
			else {
				VkImageCreateInfo imageCreateInfo = {
					.imageType = VK_IMAGE_TYPE_2D,
					.format = format_initial,
					.extent = { extent.width, extent.height, 1 },
					.mipLevels = 1,
					.arrayLayers = layerCount,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
				};
				vulkan::imageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory_conversion.Image(), imageMemory.Image(), extent, mipLevelCount, layerCount);
			}
		}
	public:
		texture2dArray() = default;
		texture2dArray(const char* filepath, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(filepath, extentInTiles, format_initial, format_final, generateMipmap);
		}
		texture2dArray(const uint8_t* pImageData, VkExtent2D fullExtent, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(pImageData, fullExtent, extentInTiles, format_initial, format_final, generateMipmap);
		}
		texture2dArray(arrayRef<const char* const> filepaths, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(filepaths, format_initial, format_final, generateMipmap);
		}
		texture2dArray(arrayRef<const uint8_t* const> psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			Create(psImageData, extent, format_initial, format_final, generateMipmap);
		}
		//Getter
		VkExtent2D Extent() const { return extent; }
		uint32_t Width() const { return extent.width; }
		uint32_t Height() const { return extent.height; }
		uint32_t LayerCount() const { return layerCount; }
		//Non-const Function
		void Create(const char* filepath, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			if (extentInTiles.width * extentInTiles.height > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
				outStream << std::format(
					"[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\nFile: {}\n",
					graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers, filepath);
				return;
			}
			VkExtent2D fullExtent;
			formatInfo formatInfo = FormatInfo(format_initial);
			std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, fullExtent, formatInfo);
			if (pImageData)
				if (fullExtent.width % extentInTiles.width ||
					fullExtent.height % extentInTiles.height)
					outStream << std::format(
						"[ texture2dArray ] ERROR\nImage not available!\nFile: {}\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
						filepath, extentInTiles.width, extentInTiles.height);//fallthrough
				else
					Create(pImageData.get(), fullExtent, extentInTiles, format_initial, format_final, generateMipmap);
		}
		void Create(const uint8_t* pImageData, VkExtent2D fullExtent, VkExtent2D extentInTiles, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			layerCount = extentInTiles.width * extentInTiles.height;
			if (layerCount > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
				outStream << std::format(
					"[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\n",
					graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers);
				return;
			}
			if (fullExtent.width % extentInTiles.width ||
				fullExtent.height % extentInTiles.height) {
				outStream << std::format(
					"[ texture2dArray ] ERROR\nImage not available!\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
					extentInTiles.width, extentInTiles.height);
				return;
			}
			extent.width = fullExtent.width / extentInTiles.width;
			extent.height = fullExtent.height / extentInTiles.height;
			size_t dataSizePerPixel = FormatInfo(format_initial).sizePerPixel;
			size_t imageDataSize = dataSizePerPixel * fullExtent.width * fullExtent.height;
			//Data rearrangement can also be peformed by using tiled regions in vkCmdCopyBufferToImage(...).
			if (extentInTiles.width == 1)
				stagingBuffer::BufferData_MainThread(pImageData, imageDataSize);
			else {
				uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
				size_t dataSizePerRow = dataSizePerPixel * extent.width;
				for (size_t j = 0; j < extentInTiles.height; j++)
					for (size_t i = 0; i < extentInTiles.width; i++)
						for (size_t k = 0; k < extent.height; k++)
							memcpy(
								pData_dst,
								pImageData + (i * extent.width + (k + j * extent.height) * fullExtent.width) * dataSizePerPixel,
								dataSizePerRow),
							pData_dst += dataSizePerRow;
				stagingBuffer::UnmapMemory_MainThread();
			}
			//Create image and allocate memory, create image view, then copy data from staging buffer to image
			Create_Internal(format_initial, format_final, generateMipmap);
		}
		void Create(arrayRef<const char* const> filepaths, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			if (filepaths.Count() > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
				outStream << std::format(
					"[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\n",
					graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers);
				return;
			}
			formatInfo formatInfo = FormatInfo(format_initial);
			auto psImageData = std::make_unique<std::unique_ptr<uint8_t[]>[]>(filepaths.Count());
			for (size_t i = 0; i < filepaths.Count(); i++) {
				VkExtent2D extent_currentLayer;
				psImageData[i] = LoadFile(filepaths[i], extent_currentLayer, formatInfo);
				if (psImageData[i]) {
					if (i == 0)
						extent = extent_currentLayer;
					if (extent.width == extent_currentLayer.width &&
						extent.height == extent_currentLayer.height)
						continue;
					else
						outStream << std::format(
							"[ texture2dArray ] ERROR\nImage not available!\nFile: {}\nAll the images must be in same size!\n",
							filepaths[i]);//fallthrough
				}
				return;
			}
			Create({ reinterpret_cast<const uint8_t* const*>(psImageData.get()), filepaths.Count() }, extent, format_initial, format_final, generateMipmap);
		}
		void Create(arrayRef<const uint8_t* const> psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool generateMipmap = true) {
			layerCount = psImageData.Count();
			if (layerCount > graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers) {
				outStream << std::format(
					"[ texture2dArray ] ERROR\nLayer count is out of limit! Must be less than: {}\n",
					graphicsBase::Base().PhysicalDeviceProperties().limits.maxImageArrayLayers);
				return;
			}
			this->extent = extent;
			size_t dataSizePerImage = size_t(FormatInfo(format_initial).sizePerPixel) * extent.width * extent.height;
			size_t imageDataSize = dataSizePerImage * layerCount;
			uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
			for (size_t i = 0; i < layerCount; i++)
				memcpy(pData_dst, psImageData[i], dataSizePerImage),
				pData_dst += dataSizePerImage;
			stagingBuffer::UnmapMemory_MainThread();
			//Create image and allocate memory, create image view, then copy data from staging buffer to image
			Create_Internal(format_initial, format_final, generateMipmap);
		}
	};
	class textureCube :public texture {
	protected:
		VkExtent2D extent = {};
		//--------------------
		VkExtent2D GetExtentInTiles(const glm::uvec2*& facePositions, bool lookFromOutside, bool loadPreviousResult = false) {
			static constexpr glm::uvec2 facePositions_default[][6] = {
				{ { 2, 1 }, { 0, 1 }, { 1, 0 }, { 1, 2 }, { 1, 1 }, { 3, 1 } },
				{ { 2, 1 }, { 0, 1 }, { 1, 0 }, { 1, 2 }, { 3, 1 }, { 1, 1 } }
			};
			static VkExtent2D extentInTiles;
			if (loadPreviousResult)
				return extentInTiles;
			extentInTiles = { 1, 1 };
			if (!facePositions)
				facePositions = facePositions_default[lookFromOutside],
				extentInTiles = { 4, 3 };
			else
				for (size_t i = 0; i < 6; i++) {
					if (facePositions[i].x >= extentInTiles.width)
						extentInTiles.width = facePositions[i].x + 1;
					if (facePositions[i].y >= extentInTiles.height)
						extentInTiles.height = facePositions[i].y + 1;
				}
			return extentInTiles;
		}
		void Create_Internal(VkFormat format_initial, VkFormat format_final, bool generateMipmap) {
			//Create image and allocate memory
			uint32_t mipLevelCount = generateMipmap ? CalculateMipLevelCount(extent) : 1;
			CreateImageMemory(VK_IMAGE_TYPE_2D, format_final, { extent.width, extent.height, 1 }, mipLevelCount, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
			//Create view
			CreateImageView(VK_IMAGE_VIEW_TYPE_CUBE, format_final, mipLevelCount, 6);
			//Copy data and generate mipmap
			if (format_initial == format_final)
				CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory.Image(), imageMemory.Image(), extent, mipLevelCount, 6);
			else {
				VkImageCreateInfo imageCreateInfo = {
					.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
					.imageType = VK_IMAGE_TYPE_2D,
					.format = format_initial,
					.extent = { extent.width, extent.height, 1 },
					.mipLevels = 1,
					.arrayLayers = 6,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
				};
				vulkan::imageMemory imageMemory_conversion(imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				CopyBlitAndGenerateMipmap2d(stagingBuffer::Buffer_MainThread(), imageMemory_conversion.Image(), imageMemory.Image(), extent, mipLevelCount, 6);
			}
		}
	public:
		textureCube() = default;
		/*
		  Order of facePositions[6], in left handed coordinate, looking from inside:
		  right(+x) left(-x) top(+y) bottom(-y) front(+z) back(-z)
		  Not related to NDC.
		  If lookFromOutside is true, the order is the same.
		  --------------------
		  Default face positions, looking from inside, is:
		  [      ][ top  ][      ][      ]
		  [ left ][front ][right ][ back ]
		  [      ][bottom][      ][      ]
		  If lookFromOutside is true, front and back is swapped.
		  What ever the facePositions are, make sure the image matches the looking which a cube is unwrapped as above.
		*/
		textureCube(const char* filepath, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			Create(filepath, facePositions, format_initial, format_final, lookFromOutside, generateMipmap);
		}
		textureCube(const uint8_t* pImageData, VkExtent2D fullExtent, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			Create(pImageData, fullExtent, facePositions, format_initial, format_final, lookFromOutside, generateMipmap);
		}
		textureCube(const char* const* filepaths, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			Create(filepaths, format_initial, format_final, lookFromOutside, generateMipmap);
		}
		textureCube(const uint8_t* const* psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			Create(psImageData, extent, format_initial, format_final, lookFromOutside, generateMipmap);
		}
		//Getter
		VkExtent2D Extent() const { return extent; }
		uint32_t Width() const { return extent.width; }
		uint32_t Height() const { return extent.height; }
		//Non-const Function
		void Create(const char* filepath, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			VkExtent2D fullExtent;
			formatInfo formatInfo = FormatInfo(format_initial);
			std::unique_ptr<uint8_t[]> pImageData = LoadFile(filepath, fullExtent, formatInfo);
			if (pImageData)
				if (VkExtent2D extentInTiles = GetExtentInTiles(facePositions, lookFromOutside);
					fullExtent.width % extentInTiles.width ||
					fullExtent.height % extentInTiles.height)
					outStream << std::format(
						"[ textureCube ] ERROR\nImage not available!\nFile: {}\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
						filepath, extentInTiles.width, extentInTiles.height);//fallthrough
				else {
					extent.width = fullExtent.width / extentInTiles.width;
					extent.height = fullExtent.height / extentInTiles.height;
					Create(pImageData.get(), { fullExtent.width, UINT32_MAX }, facePositions, format_initial, format_final, lookFromOutside, generateMipmap);
				}
		}
		void Create(const uint8_t* pImageData, VkExtent2D fullExtent, const glm::uvec2 facePositions[6], VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			VkExtent2D extentInTiles;
			if (fullExtent.height == UINT32_MAX)//See previous Create(...), value of fullExtent.height doesn't matter after this if statement
				extentInTiles = GetExtentInTiles(facePositions, lookFromOutside, true);
			else {
				extentInTiles = GetExtentInTiles(facePositions, lookFromOutside);
				if (fullExtent.width % extentInTiles.width ||
					fullExtent.height % extentInTiles.height) {
					outStream << std::format(
						"[ textureCube ] ERROR\nImage not available!\nImage width should be in multiples of {}\nImage height should be in multiples of {}\n",
						extentInTiles.width, extentInTiles.height);
					return;
				}
				extent.width = fullExtent.width / extentInTiles.width;
				extent.height = fullExtent.height / extentInTiles.height;
			}
			size_t dataSizePerPixel = FormatInfo(format_initial).sizePerPixel;
			size_t dataSizePerRow = dataSizePerPixel * extent.width;
			size_t dataSizePerImage = dataSizePerRow * extent.height;
			size_t imageDataSize = dataSizePerImage * 6;
			uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
			if (lookFromOutside)
				for (size_t face = 0; face < 6; face++)
					if (face != 2 && face != 3)
						for (uint32_t i = 0; i < extent.height; i++)
							for (uint32_t j = 0; j < extent.width; j++)
								memcpy(
									pData_dst,
									pImageData + dataSizePerPixel * (extent.width - 1 - j + facePositions[face].x * extent.width + (i + facePositions[face].y * extent.height) * fullExtent.width),
									dataSizePerPixel),
								pData_dst += dataSizePerPixel;
					else
						for (uint32_t j = 0; j < extent.height; j++)
							for (uint32_t k = 0; k < extent.width; k++)
								memcpy(
									pData_dst,
									pImageData + dataSizePerPixel * (k + facePositions[face].x * extent.width + ((extent.height - 1 - j) + facePositions[face].y * extent.height) * fullExtent.width),
									dataSizePerPixel),
								pData_dst += dataSizePerPixel;
			else
				if (extentInTiles.width == 1 && extentInTiles.height == 6 &&
					facePositions[0].y == 0 && facePositions[1].y == 1 &&
					facePositions[2].y == 2 && facePositions[3].y == 3 &&
					facePositions[4].y == 4 && facePositions[5].y == 5)
					memcpy(pData_dst, pImageData, imageDataSize);
				else
					for (size_t face = 0; face < 6; face++)
						for (uint32_t j = 0; j < extent.height; j++)
							memcpy(
								pData_dst,
								pImageData + dataSizePerPixel * (facePositions[face].x * extent.width + (j + facePositions[face].y * extent.height) * fullExtent.width),
								dataSizePerRow),
							pData_dst += dataSizePerRow;
			stagingBuffer::UnmapMemory_MainThread();
			//Create image and allocate memory, create image view, then copy data from staging buffer to image
			Create_Internal(format_initial, format_final, generateMipmap);
		}
		void Create(const char* const* filepaths, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			formatInfo formatInfo = FormatInfo(format_initial);
			std::unique_ptr<uint8_t[]> psImageData[6] = {};
			for (size_t i = 0; i < 6; i++) {
				VkExtent2D extent_currentLayer;
				psImageData[i] = LoadFile(filepaths[i], extent_currentLayer, formatInfo);
				if (psImageData[i]) {
					if (i == 0)
						extent = extent_currentLayer;
					if (extent.width == extent_currentLayer.width ||
						extent.height == extent_currentLayer.height)
						continue;
					else
						outStream << std::format(
							"[ textureCube ] ERROR\nImage not available!\nFile: {}\nAll the images must be in same size!\n",
							filepaths[i]);//fallthrough
				}
				return;
			}
			Create(reinterpret_cast<const uint8_t* const*>(psImageData), extent, format_initial, format_final, lookFromOutside, generateMipmap);
		}
		void Create(const uint8_t* const* psImageData, VkExtent2D extent, VkFormat format_initial, VkFormat format_final, bool lookFromOutside = false, bool generateMipmap = true) {
			this->extent = extent;
			size_t dataSizePerPixel = FormatInfo(format_initial).sizePerPixel;
			size_t dataSizePerImage = dataSizePerPixel * extent.width * extent.height;
			size_t imageDataSize = dataSizePerImage * 6;
			uint8_t* pData_dst = static_cast<uint8_t*>(stagingBuffer::MapMemory_MainThread(imageDataSize));
			if (lookFromOutside)
				for (size_t face = 0; face < 6; face++)
					if (face != 2 && face != 3)
						for (uint32_t j = 0; j < extent.height; j++)
							for (uint32_t i = 0; i < extent.width; i++)
								memcpy(
									pData_dst,
									psImageData[face] + dataSizePerPixel * ((j + 1) * extent.width - 1 - i),
									dataSizePerPixel),
								pData_dst += dataSizePerPixel;
					else
						for (uint32_t j = 0; j < extent.height; j++)
							for (uint32_t i = 0; i < extent.width; i++)
								memcpy(
									pData_dst,
									psImageData[face] + dataSizePerPixel * ((extent.height - 1 - j) * extent.width + i),
									dataSizePerPixel),
								pData_dst += dataSizePerPixel;
			else
				for (size_t i = 0; i < 6; i++)
					memcpy(pData_dst + dataSizePerImage * i, psImageData[i], dataSizePerImage);
			stagingBuffer::UnmapMemory_MainThread();
			//Create image and allocate memory, create image view, then copy data from staging buffer to image
			Create_Internal(format_initial, format_final, generateMipmap);
		}
	};

	//Query
	class occlusionQueries {
	protected:
		queryPool queryPool;
		std::vector<uint32_t> passingSampleCounts;
	public:
		occlusionQueries() = default;
		occlusionQueries(uint32_t capacity) {
			Create(capacity);
		}
		//Getter
		operator VkQueryPool() const { return queryPool; }
		const VkQueryPool* Address() const { return queryPool.Address(); }
		uint32_t Capacity() const { return passingSampleCounts.size(); }
		uint32_t PassingSampleCount(uint32_t index) const { return passingSampleCounts[index]; }
		//Const Function
		void CmdReset(VkCommandBuffer commandBuffer) const {
			queryPool.CmdReset(commandBuffer, 0, Capacity());
		}
		void CmdBegin(VkCommandBuffer commandBuffer, uint32_t queryIndex, bool isPrecise = false) const {
			queryPool.CmdBegin(commandBuffer, queryIndex, isPrecise);
		}
		void CmdEnd(VkCommandBuffer commandBuffer, uint32_t queryIndex) const {
			queryPool.CmdEnd(commandBuffer, queryIndex);
		}
		/*For GPU-driven occlusion culling*/
		void CmdCopyResults(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount, VkBuffer buffer_dst, VkDeviceSize offset_dst, VkDeviceSize stride) const {
			queryPool.CmdCopyResults(commandBuffer, firstQueryIndex, queryCount, buffer_dst, offset_dst, stride, VK_QUERY_RESULT_WAIT_BIT);
		}
		//Non-const Function
		void Create(uint32_t capacity) {
			passingSampleCounts.resize(capacity);
			passingSampleCounts.shrink_to_fit();
			queryPool.Create(VK_QUERY_TYPE_OCCLUSION, Capacity());
		}
		void Recreate(uint32_t capacity) {
			graphicsBase::Base().WaitIdle();
			queryPool.~queryPool();
			Create(capacity);
		}
		result_t GetResults() {
			return GetResults(Capacity());
		}
		result_t GetResults(uint32_t queryCount) {
			return queryPool.GetResults(0, queryCount, queryCount * 4, passingSampleCounts.data(), 4);
		}
	};
	class pipelineStatisticQuery {
	protected:
		enum statisticName {
			//Input Assembly
			vertexCount_ia,
			primitiveCount_ia,
			//Vertex Shader
			invocationCount_vs,
			//Geometry Shader
			invocationCount_gs,
			primitiveCount_gs,
			//Clipping
			invocationCount_clipping,
			primitiveCount_clipping,
			//Fragment Shader
			invocationCount_fs,
			//Tessellation
			patchCount_tcs,
			invocationCount_tes,
			//Compute Shader
			invocationCount_cs,
			statisticCount
		};
		//--------------------
		queryPool queryPool;
		uint32_t statistics[statisticCount] = {};
	public:
		pipelineStatisticQuery() {
			Create();
		}
		//Getter
		operator VkQueryPool() const { return queryPool; }
		const VkQueryPool* Address() const { return queryPool.Address(); }
		uint32_t     VertexCount_Ia() const { return statistics[vertexCount_ia]; }
		uint32_t  PrimitiveCount_Ia() const { return statistics[primitiveCount_ia]; }
		uint32_t InvocationCount_Vs() const { return statistics[invocationCount_vs]; }
		uint32_t InvocationCount_Gs() const { return statistics[invocationCount_gs]; }
		uint32_t  PrimitiveCount_Gs() const { return statistics[primitiveCount_gs]; }
		uint32_t InvocationCount_Clipping() const { return statistics[invocationCount_clipping]; }
		uint32_t  PrimitiveCount_Clipping() const { return statistics[primitiveCount_clipping]; }
		uint32_t InvocationCount_Fs() const { return statistics[invocationCount_fs]; }
		uint32_t      PatchCount_Tcs() const { return statistics[patchCount_tcs]; }
		uint32_t InvocationCount_Tes() const { return statistics[invocationCount_tes]; }
		uint32_t InvocationCount_Cs() const { return statistics[invocationCount_cs]; }
		//Const Function
		void CmdReset(VkCommandBuffer commandBuffer) const {
			queryPool.CmdReset(commandBuffer, 0, 1);
		}
		void CmdBegin(VkCommandBuffer commandBuffer) const {
			queryPool.CmdBegin(commandBuffer, 0);
		}
		void CmdEnd(VkCommandBuffer commandBuffer) const {
			queryPool.CmdEnd(commandBuffer, 0);
		}
		void CmdResetAndBegin(VkCommandBuffer commandBuffer) const {
			CmdReset(commandBuffer);
			CmdBegin(commandBuffer);
		}
		//Non-const Function
		void Create() {
			queryPool.Create(VK_QUERY_TYPE_PIPELINE_STATISTICS, 1, (1 << statisticCount) - 1);
		}
		result_t GetResults() {
			return queryPool.GetResults(0, 1, sizeof statistics, statistics, sizeof statistics);
		}
	};
	class timestampQueries {
	protected:
		queryPool queryPool;
		std::vector<uint32_t> timestamps;
	public:
		timestampQueries() = default;
		timestampQueries(uint32_t capacity) {
			Create(capacity);
		}
		//Getter
		operator VkQueryPool() const { return queryPool; }
		const VkQueryPool* Address() const { return queryPool.Address(); }
		uint32_t Capacity() const { return timestamps.size(); }
		uint32_t Timestamp(uint32_t index) const { return timestamps[index]; }
		uint32_t Duration(uint32_t index) const { return timestamps[index + 1] - timestamps[index]; }
		//Const Function
		void CmdReset(VkCommandBuffer commandBuffer) const {
			queryPool.CmdReset(commandBuffer, 0, Capacity());
		}
		void CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t queryIndex) const {
			queryPool.CmdWriteTimestamp(commandBuffer, pipelineStage, queryIndex);
		}
		//Non-const Function
		void Create(uint32_t capacity) {
			timestamps.resize(capacity);
			timestamps.shrink_to_fit();
			queryPool.Create(VK_QUERY_TYPE_TIMESTAMP, Capacity());
		}
		void Recreate(uint32_t capacity) {
			graphicsBase::Base().WaitIdle();
			queryPool.~queryPool();
			Create(capacity);
		}
		result_t GetResults() {
			return GetResults(Capacity());
		}
		result_t GetResults(uint32_t queryCount) {
			return queryPool.GetResults(0, queryCount, queryCount * 4, timestamps.data(), 4);
		}
	};
}