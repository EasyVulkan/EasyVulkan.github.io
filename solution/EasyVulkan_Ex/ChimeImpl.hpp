#include "ChimeImpl.h"

CHIME_NAMESPACE_BEGIN
void InitializeText() {
	apiData_texture::Initialize();
	apiData_font::Initialize();
	apiData_textPrinter::Initialize();
}

// class font
void font::CreateApiData(const uint8_t* pImageData, bool generateMipmap) {
	texture2dArray texture(pImageData, { imageLayerWidth, uint32_t(imageLayerCount * imageLayerHeight) }, { 1, imageLayerCount }, VK_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM, generateMipmap);
	pApiData.reset(new apiData_font(std::move(texture), pixelDistanceScale));
}

// class textPrinter
void textPrinter::Draw(vec2 view, const font& font, uint32_t vertexCount, uint32_t firstVertexIndex) const {
	pApiData->CmdBindPipelineAndUpdateConstants(graphics::Base().CommandBuffer(), *this, view, font);
	pApiData->CmdBindVertexBufferAndDraw(graphics::Base().CommandBuffer(), vertexCount, firstVertexIndex);
}
void textPrinter::UpdateApiData_Internal() const {
	pApiData->UpdateBuffer(*this);
}
void textPrinter::CreateApiData(uint32_t capacity) {
	pApiData.reset(new apiData_textPrinter(capacity));
}
CHIME_NAMESPACE_END