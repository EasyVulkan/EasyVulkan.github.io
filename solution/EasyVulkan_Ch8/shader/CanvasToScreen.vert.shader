#version 460
#pragma shader_stage(vertex)

vec2 positions[4] = {
	{ 0, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 1 }
};

layout(location = 0) out vec2 o_TexCoord;

layout(push_constant) uniform pushConstants {
	vec2 viewportSize;
	vec2 canvasSize;
};

void main() {
	o_TexCoord = positions[gl_VertexIndex];
	gl_Position = vec4(2 * positions[gl_VertexIndex] * canvasSize / viewportSize - 1, 0, 1);
}