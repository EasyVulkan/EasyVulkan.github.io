#include "VKBase+.h"
#include "Chime_TextPrinter.h"

#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

CHIME_NAMESPACE_BEGIN
using namespace vulkan;
using pipelineAndLayout = std::pair<pipeline, pipelineLayout>;

class graphics {
	float globalScale = 1;
	VkPipelineRenderingCreateInfo renderingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1
	};
	glm::vec2 framebufferSize = {};
	VkSampleCountFlagBits sampleCount = {};
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	std::vector<void(*)()> pipelineRecreationFunctions;
	/* Static */
	static graphics singleton;
	/* Constructor & Destructor */
	graphics() = default;
	graphics(graphics&&) = delete;
	~graphics() = default;
	/* Const Function */
	void CurrentFramebufferSize(VkExtent2D framebufferSize) {
		this->framebufferSize = { framebufferSize.width, framebufferSize.height };
		VkViewport viewport = { 0, 0, float(framebufferSize.width), float(framebufferSize.height), 0, 1 };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}
public:
	/* Getter */
	const float& GlobalScale() const { return globalScale; }
	const VkPipelineRenderingCreateInfo& RenderingCreateInfo() const { return renderingCreateInfo; }
	const glm::vec2& FramebufferSize() const { return framebufferSize; }
	const VkSampleCountFlagBits& SampleCount() const { return sampleCount; }
	const VkCommandBuffer& CommandBuffer() const { return commandBuffer; }
	/* Setter */
	void GlobalScale(float globalScale) { this->globalScale = globalScale; }
	/* Const Function */
	void CurrentScissor(const VkRect2D& scissor) const {
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}
	/* Non-const Function */
	// ======== For initialization
	void Initialize() {
		ExecuteOnce();
		InitializeText();
		auto ResetContext_RecreateDevice = [] {
			singleton.sampleCount = {};
		};
		graphicsBase::Base().AddCallback_DestroyDevice(ResetContext_RecreateDevice);
	}
	void AddPipelineRecreationFunction(void(*function)()) {
		pipelineRecreationFunctions.push_back(function);
	}
	// ======== After initialization
	void CurrentContext(VkCommandBuffer commandBuffer, uint32_t viewMask, VkFormat colorAttachmentFormat, VkSampleCountFlagBits sampleCount, VkExtent2D framebufferSize) {
		this->commandBuffer = commandBuffer;
		CurrentScissor({ {}, framebufferSize });
		CurrentFramebufferSize(framebufferSize);
		static VkFormat _colorAttachmentFormat = VK_FORMAT_UNDEFINED;
		if (renderingCreateInfo.viewMask == viewMask &&
			_colorAttachmentFormat == colorAttachmentFormat &&
			this->sampleCount == sampleCount)
			return;
		renderingCreateInfo.viewMask = viewMask;
		renderingCreateInfo.pColorAttachmentFormats = &(_colorAttachmentFormat = colorAttachmentFormat);
		this->sampleCount = sampleCount;
		for (auto& i : pipelineRecreationFunctions)
			i();
	}
	/* Static Function */
	static constexpr graphics& Base() { return singleton; }
};
DefineStaticDataMember(graphics::singleton);

class apiData_texture : public texture {
	descriptorSet descriptorSet;
	uint32_t descriptorPoolIndex = 0;
	/* Static */
	static constexpr uint32_t setCountPerPool = 64;
	static inline descriptorSetLayout descriptorSetLayout;
	static inline std::vector<std::pair<descriptorPool, uint32_t>> descriptorPools;
	/* Non-const Function */
	void CreateDescriptorSet(VkSampler sampler) {
		uint32_t& i = descriptorPoolIndex;
		for (i = 0; i < descriptorPools.size(); i++)
			if (descriptorPools[i].second < setCountPerPool) {
				descriptorPools[i].first.AllocateSets(descriptorSet, descriptorSetLayout);
				descriptorPools[i].second++;
				break;
			}
		if (i == descriptorPools.size()) {
			VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, setCountPerPool };
			descriptorPools.emplace_back(descriptorPool(setCountPerPool, descriptorPoolSize, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT), 1);
			descriptorPools[i].first.AllocateSets(descriptorSet, descriptorSetLayout);
		}
		VkDescriptorImageInfo descriptorImageInfo = { sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		descriptorSet.Write(descriptorImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}
public:
	apiData_texture(texture&& other, VkSampler sampler) :
		texture(std::move(other)) {
		CreateDescriptorSet(sampler);
	}
	apiData_texture(apiData_texture&&) = default;
	~apiData_texture() {
		if (descriptorSet)
			descriptorPools[descriptorPoolIndex].first.FreeSets(descriptorSet),
			descriptorPools[descriptorPoolIndex].second--,
			descriptorPoolIndex = 0;
	}
	/* Getter */
	const class descriptorSet& DescriptorSet() const { return descriptorSet; }
	/* Static Function */
	static void Initialize() {
		ExecuteOnce();
		auto Initialize = [] {
			VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
			};
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
				.bindingCount = 1,
				.pBindings = &descriptorSetLayoutBinding
			};
			descriptorSetLayout.Create(descriptorSetLayoutCreateInfo);
			VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, setCountPerPool };
			descriptorPools.emplace_back(descriptorPool(setCountPerPool, descriptorPoolSize, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT), 0);
		};
		auto CleanUp = [] {
			descriptorSetLayout.Destroy();
			descriptorPools.clear();
		};
		if (graphicsBase::Base().Device())
			Initialize();
		graphicsBase::Base().AddCallback_CreateDevice(Initialize);
		graphicsBase::Base().AddCallback_DestroyDevice(CleanUp);
	}
	static const class descriptorSetLayout& DescriptorSetLayout() { return descriptorSetLayout; }
};

class apiData_font : public apiData_texture {
	static inline VkSamplerCreateInfo samplerCreateInfo_nonSdf;
	static inline VkSamplerCreateInfo samplerCreateInfo_sdf;
	static inline sampler sampler_nonSdf;
	static inline sampler sampler_sdf;
public:
	apiData_font(texture&& other, bool isSdf) : apiData_texture(std::move(other), isSdf ? sampler_sdf : sampler_nonSdf) {}
	/* Static Function */
	static void Initialize() {
		ExecuteOnce();
		auto Initialize = [] {
			if (!samplerCreateInfo_nonSdf.sType)
				samplerCreateInfo_nonSdf = texture::SamplerCreateInfo();
			sampler_nonSdf.Create(samplerCreateInfo_nonSdf);
			if (!samplerCreateInfo_sdf.sType)
				samplerCreateInfo_sdf = texture::SamplerCreateInfo(),
				samplerCreateInfo_sdf.mipLodBias = -1.f;
			sampler_sdf.Create(samplerCreateInfo_sdf);
		};
		auto CleanUp = [] {
			sampler_nonSdf.Destroy();
			sampler_sdf.Destroy();
		};
		if (graphicsBase::Base().Device())
			Initialize();
		graphicsBase::Base().AddCallback_CreateDevice(Initialize);
		graphicsBase::Base().AddCallback_DestroyDevice(CleanUp);
	}
	static void SamplerCreateInfo_NonSdf(const VkSamplerCreateInfo& samplerCreateInfo) {
		samplerCreateInfo_nonSdf = samplerCreateInfo;
	}
	static void SamplerCreateInfo_Sdf(const VkSamplerCreateInfo& samplerCreateInfo) {
		samplerCreateInfo_sdf = samplerCreateInfo;
	}
};

class apiData_textPrinter : public vertexBuffer {
public:
	using characterVertex = textPrinter::characterVertex;
	enum pipelineName {
		Text,
		Text_Sdf,
		_textPipelineCount
	};
private:
	std::unique_ptr<apiData_textPrinter> pNext;
	uint32_t swapchainIndex_lastAllocation = 0; // For deferred destruction
	/* Static */
	static inline pipelineAndLayout pals[_textPipelineCount];
	/* Const Function */
	using vertexBuffer::TransferData;
	using vertexBuffer::CmdUpdateBuffer;
	vertexBuffer* UpdateBuffer_Internal(const void* pData, VkDeviceSize dataSize) {
		if (dataSize > AllocationSize()) {
			if (AllocationSize())
				TransferData(pData, AllocationSize());
			return pNext->UpdateBuffer_Internal(pData, dataSize);
		}
		TransferData(pData, dataSize);
		return this;
	}
	/* Non-const Function */
	using vertexBuffer::Create;
	using vertexBuffer::Recreate;
public:
	apiData_textPrinter(uint32_t capacity) {
		if (capacity)
			Create(sizeof(characterVertex) * capacity, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	}
	apiData_textPrinter(apiData_textPrinter&&) = default;
	/* Const Function */
	void CmdBindPipelineAndUpdateConstants(VkCommandBuffer commandBuffer, const textPrinter& textPrinter, glm::vec2 view, const font& font) const {
		struct { glm::vec2 framebufferSize; glm::vec2 view; glm::vec2 textureSize; float pixelDistanceScale; } constants = {
			graphics::Base().FramebufferSize() / graphics::Base().GlobalScale(), view,
			font.ImageLayerSize()
		};
		if (font.PixelDistanceScale()) {
			constants.pixelDistanceScale = std::min(font.PixelDistanceScale() * font.FontHeight() / (textPrinter.FontHeight() * graphics::Base().GlobalScale()), 255.f);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline(Text_Sdf));
			vkCmdPushConstants(commandBuffer, PipelineLayout(Text_Sdf), VK_SHADER_STAGE_VERTEX_BIT, 0, 24, &constants);
			vkCmdPushConstants(commandBuffer, PipelineLayout(Text_Sdf), VK_SHADER_STAGE_FRAGMENT_BIT, 24, 4, &constants.pixelDistanceScale);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout(Text_Sdf), 0, 1, font->DescriptorSet().Address(), 0, nullptr);
		}
		else {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline(Text));
			vkCmdPushConstants(commandBuffer, PipelineLayout(Text), VK_SHADER_STAGE_VERTEX_BIT, 0, 24, &constants);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout(Text), 0, 1, font->DescriptorSet().Address(), 0, nullptr);
		}
	}
	/* Non-const Function */
	void CmdBindVertexBufferAndDraw(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance) {
		VkDeviceSize dataSize = sizeof(characterVertex) * (instanceCount + firstInstance);
		if (dataSize > AllocationSize()) {
			if (!pNext)
				pNext = std::make_unique<apiData_textPrinter>(std::max(uint32_t(AllocationSize() * 2 / sizeof(characterVertex)), instanceCount + firstInstance));
			return pNext->CmdBindVertexBufferAndDraw(commandBuffer, instanceCount, firstInstance);
		}
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, Address(), &offset);
		vkCmdDraw(commandBuffer, 4, instanceCount, 0, firstInstance);
	}
	void UpdateBuffer(const textPrinter& textPrinter) {
		struct _textPrinter : textPrinter {
			using textPrinter::vertices;
		};
		vertexBuffer* pBack = UpdateBuffer_Internal(static_cast<const _textPrinter&>(textPrinter).vertices.data(), sizeof(characterVertex) * textPrinter.VertexCount());
		if (pNext)
			if (pBack != this) {
				uint8_t temp[sizeof(vertexBuffer)];
				memcpy(temp, this, sizeof(vertexBuffer));
				memcpy(this, pBack, sizeof(vertexBuffer));
				memcpy(pBack, temp, sizeof(vertexBuffer));
				swapchainIndex_lastAllocation = graphicsBase::Base().CurrentImageIndex();
			}
			else
				if (swapchainIndex_lastAllocation == graphicsBase::Base().CurrentImageIndex())
					swapchainIndex_lastAllocation *= -1;
				else if (swapchainIndex_lastAllocation == graphicsBase::Base().CurrentImageIndex() * -1)
					pNext = nullptr;
	}
	/* Static Function */
	static void Initialize() {
		ExecuteOnce();
		enum shaderName {
			V_Text,
			F_Text,
			F_Text_Sdf,
			_shaderCount
		};
		static shaderModule shaders[_shaderCount];
		static VkPipelineShaderStageCreateInfo shaderStageCreateInfos[_textPipelineCount][2];
		auto Initialize = [] {
			VkDescriptorSetLayout descriptorSetLayout = apiData_texture::DescriptorSetLayout();
			VkPushConstantRange pushConstantRanges[] = {
				{ VK_SHADER_STAGE_VERTEX_BIT },
				{ VK_SHADER_STAGE_FRAGMENT_BIT }
			};
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
				.setLayoutCount = 1,
				.pSetLayouts = &descriptorSetLayout,
				.pPushConstantRanges = pushConstantRanges
			};
			// Create pipeline layouts
			pushConstantRanges[0].size = 24;
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pals[Text].second.Create(pipelineLayoutCreateInfo);
			pushConstantRanges[1].offset = 24;
			pushConstantRanges[1].size = 4;
			pipelineLayoutCreateInfo.pushConstantRangeCount = 2;
			pals[Text_Sdf].second.Create(pipelineLayoutCreateInfo);
			// Load shaders
			shaders[V_Text].Create("shader/TextRendering/Text.vert.spv");
			shaders[F_Text].Create("shader/TextRendering/Text.frag.spv");
			shaderStageCreateInfos[Text][0] = shaders[V_Text].StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
			shaderStageCreateInfos[Text][1] = shaders[F_Text].StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
			shaders[F_Text_Sdf].Create("shader/TextRendering/Text_Sdf.frag.spv");
			shaderStageCreateInfos[Text_Sdf][0] = shaders[V_Text].StageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT);
			shaderStageCreateInfos[Text_Sdf][1] = shaders[F_Text_Sdf].StageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT);
		};
		auto CreatePipelines = [] {
			for (auto& i : pals)
				i.first.Destroy();
			graphicsPipelineCreateInfoPack pipelineCiPack;
			pipelineCiPack.createInfo.pNext = &graphics::Base().RenderingCreateInfo();
			pipelineCiPack.vertexInputBindings.emplace_back(0, sizeof(characterVertex), VK_VERTEX_INPUT_RATE_INSTANCE);
			pipelineCiPack.vertexInputAttributes.emplace_back(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(characterVertex, position));
			pipelineCiPack.vertexInputAttributes.emplace_back(1, 0, VK_FORMAT_R32_SFLOAT, offsetof(characterVertex, scale));
			pipelineCiPack.vertexInputAttributes.emplace_back(2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(characterVertex, color));
			pipelineCiPack.vertexInputAttributes.emplace_back(3, 0, VK_FORMAT_R16_UINT, offsetof(characterVertex, layerIndex));
			pipelineCiPack.vertexInputAttributes.emplace_back(4, 0, VK_FORMAT_R16_UINT, offsetof(characterVertex, offsetU));
			pipelineCiPack.vertexInputAttributes.emplace_back(5, 0, VK_FORMAT_R16_UINT, offsetof(characterVertex, sizeU));
			pipelineCiPack.inputAssemblyStateCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			pipelineCiPack.multisampleStateCi.rasterizationSamples = graphics::Base().SampleCount();
			pipelineCiPack.colorBlendAttachmentStates.emplace_back(
				true,
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD,
				0b1111);
			pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			pipelineCiPack.dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
			pipelineCiPack.UpdateAllArrays();
			pipelineCiPack.createInfo.stageCount = 2;
			pipelineCiPack.createInfo.pStages = shaderStageCreateInfos[Text];
			pipelineCiPack.createInfo.layout = pals[Text].second;
			pals[Text].first.Create(pipelineCiPack);
			pipelineCiPack.createInfo.pStages = shaderStageCreateInfos[Text_Sdf];
			pipelineCiPack.createInfo.layout = pals[Text_Sdf].second;
			pals[Text_Sdf].first.Create(pipelineCiPack);
		};
		auto CleanUp = [] {
			for (auto& i : pals)
				i.first.Destroy(),
				i.second.Destroy();
			for (auto& i : shaders)
				i.Destroy();
		};
		if (graphicsBase::Base().Device())
			Initialize();
		graphicsBase::Base().AddCallback_CreateDevice(Initialize);
		graphics::Base().AddPipelineRecreationFunction(CreatePipelines);
		graphicsBase::Base().AddCallback_DestroyDevice(CleanUp);
	}
	static VkPipeline Pipeline(pipelineName name) {
		return pals[name].first;
	}
	static VkPipelineLayout PipelineLayout(pipelineName name) {
		return pals[name].second;
	}
};
CHIME_NAMESPACE_END