//sRGB
float Eotf_sRGB(float v) {
	if (v <= 0.04045f)
		return v / 12.92f;
	return pow((v + 0.055f) / 1.055f, 2.4f);
}
vec3 Eotf_sRGB(vec3 v) {
	return vec3(Eotf_sRGB(v.x), Eotf_sRGB(v.y), Eotf_sRGB(v.z));
}
float InverseEotf_sRGB(float v) {
	if (v <= 0.0031308f)
		return v * 12.92f;
	return  1.055f * pow(v, 1 / 2.4f) - 0.055f;
}
vec3 InverseEotf_sRGB(vec3 v) {
	return vec3(InverseEotf_sRGB(v.x), InverseEotf_sRGB(v.y), InverseEotf_sRGB(v.z));
}

//PQ
float Eotf_PQ(float v) {
	const float m1 = 2610.f / 16384;
	const float m2 = 2523.f / 4096 * 128;
	const float c1 = 3424.f / 4096;
	const float c2 = 2413.f / 4096 * 32;
	const float c3 = 2392.f / 4096 * 32;
	v = pow(v, 1 / m2);
	if (v <= c1)
		return 0;
	return pow((v - c1) / (c2 - c3 * v), 1 / m1);
}
vec3 Eotf_PQ(vec3 v) {
	return vec3(Eotf_PQ(v.x), Eotf_PQ(v.y), Eotf_PQ(v.z));
}
float InverseEotf_PQ(float v) {
	const float m1 = 2610.f / 16384;
	const float m2 = 2523.f / 4096 * 128;
	const float c1 = 3424.f / 4096;
	const float c2 = 2413.f / 4096 * 32;
	const float c3 = 2392.f / 4096 * 32;
	v = pow(v, m1);
	return pow((c1 + c2 * v) / (1 + c3 * v), m2);
}
vec3 InverseEotf_PQ(vec3 v) {
	return vec3(InverseEotf_PQ(v.x), InverseEotf_PQ(v.y), InverseEotf_PQ(v.z));
}