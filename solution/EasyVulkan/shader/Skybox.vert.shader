#version 450
#pragma shader_stage(vertex)

//Looking from inside, all counterclockwise triangles
vec3 positions[14] = {
	{-1, 1, 1},
	{ 1,-1, 1},
	{ 1, 1, 1},
	{ 1, 1,-1},
	{-1, 1, 1},

	{-1, 1,-1},
	{-1,-1,-1},
	{ 1, 1,-1},

	{ 1,-1,-1},
	{ 1,-1, 1},
	{-1,-1,-1},

	{-1,-1, 1},
	{-1, 1, 1},
	{ 1,-1, 1}
};

layout(location = 0) out vec3 o_Uvw;
layout(binding = 0) uniform descriptorConstants{
	mat4 proj;
	mat4 view;
};

void main() {
	//If the proj and view matrix are used for right handed coordinate:
	//gl_Position = proj * mat4(mat3(view)) * vec4(positions[gl_VertexIndex].xy, -positions[gl_VertexIndex].z, 1);
	gl_Position = proj * mat4(mat3(view)) * vec4(positions[gl_VertexIndex], 1);
	o_Uvw = positions[gl_VertexIndex];
}