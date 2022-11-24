#pragma once
#include "VKBase.h"

namespace vulkan {
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
}