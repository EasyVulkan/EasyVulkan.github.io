#version 460
#pragma shader_stage(fragment)

struct light {
	vec3 position;
	vec3 color;
	float strength;
};
layout(constant_id = 0) const uint maxLightCount = 32;
layout(constant_id = 1) const uint shininess = 32;

layout(location = 0) in vec2 i_Position;
layout(location = 0) out vec4 o_Color;
layout(binding = 0) uniform descriptorConstants {
	mat4 proj;
	mat4 view;
	int lightCount;
	light lights[maxLightCount];
};
layout(binding = 1, input_attachment_index = 0) uniform subpassInput u_GBuffers[2];

void main() {
	mat4 inverseView = inverse(view);
	vec3 cameraPosition = vec3(inverseView[3][0], inverseView[3][1], inverseView[3][2]);
	vec3 position;
	position.z = subpassLoad(u_GBuffers[0]).w;
	position.x = (i_Position.x - proj[2][0]) * position.z / proj[0][0];
	position.y = (i_Position.y - proj[2][1]) * position.z / proj[1][1];
	position = vec4(inverseView * vec4(position, 1)).xyz;
	vec3 normal = subpassLoad(u_GBuffers[0]).xyz;
	vec3 albedo = subpassLoad(u_GBuffers[1]).xyz;
	float specular = subpassLoad(u_GBuffers[1]).w;
	o_Color = vec4(0, 0, 0, 1);
	for (uint i = 0; i < lightCount; i++) {
		vec3 toLight = lights[i].position - position;
		//No Ambient
		//Diffuse
		vec3 lightIntensity = lights[i].color * lights[i].strength / pow(length(toLight), 2);//Light strength follows inverse-square law in vacuum.
		toLight = normalize(toLight);
		vec3 diffuse = albedo * lightIntensity * max(dot(normal, toLight), 0);
		//Specular
		vec3 halfway = normalize(/*toCamera*/normalize(cameraPosition - position) + toLight);
		vec3 specular = specular * lightIntensity * pow(
			max(dot(normal, halfway), 0),
			shininess);
		//Final Color
		o_Color.rgb += diffuse + specular;
	}
}