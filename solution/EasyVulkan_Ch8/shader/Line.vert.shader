#version 460
#pragma shader_stage(vertex)

layout(push_constant) uniform pushConstants {
	vec2 viewportSize;
	vec2 offsets[2];
};

void main() {
	gl_Position = vec4(2 * offsets[gl_VertexIndex] / viewportSize - 1, 0, 1);
}