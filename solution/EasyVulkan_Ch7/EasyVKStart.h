#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <span>
#include <memory>
#include <functional>
#include <concepts>
#include <format>
#include <chrono>
#include <numeric>
#include <numbers>

//GLM
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

//stb_image.h
#include <stb_image.h>

//Vulkan
#include <vulkan/vulkan.h>
#pragma comment(lib, "vulkan-1.lib")

template<typename T>
class arrayRef {
    T* pArray = nullptr;
    size_t count = 0;
public:
    arrayRef() = default;
    arrayRef(T& data) :pArray(&data), count(1) {}
    template<size_t elementCount>
    arrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
    arrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
    //Getter
    T* Pointer() const { return pArray; }
    size_t Count() const { return count; }
    //Const Function
    T& operator[](size_t index) const { return pArray[index]; }
    T* begin() const { return pArray; }
    T* end() const { return pArray + count; }
};
#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

//----------Math Related-------------------------------------------------------
/*Compare*/
template<typename T0, typename T1>
constexpr auto Less(T0 num0, T1 num1) {
    return num0 < num1 ? num0 : num1;
}
template<typename T0, typename T1>
constexpr auto Greater(T0 num0, T1 num1) {
    return num0 >= num1 ? num0 : num1;
}
template<std::signed_integral T>
constexpr int GetSign(T num) {
    return (num > 0) - (num < 0);
}
template<std::signed_integral T>
constexpr bool SameSign(T num0, T num1) {
    return num0 == num1 || !(num0 >= 0 && num1 <= 0 || num0 <= 0 && num1 >= 0);
}
template<std::signed_integral T>//0 is treated as positive
constexpr bool SameSign_Weak(T num0, T num1) {
    return (num0 ^ num1) >= 0;
}
template<std::signed_integral T>
constexpr bool Between_Open(T min, T num, T max) {
    return ((min - num) & (num - max)) < 0;
}
template<std::signed_integral T>
constexpr bool Between_Closed(T min, T num, T max) {
    return ((num - min) | (max - num)) >= 0;
}
/*Convert*/
constexpr float d2rCoefficient = float(std::numbers::pi / 180);
constexpr float r2dCoefficient = float(180 / std::numbers::pi);
constexpr float operator""_d2r(long double degree) {
    return float(degree * d2rCoefficient);
}
constexpr float operator""_r2d(long double rad) {
    return float(rad * r2dCoefficient);
}
constexpr float d2r(float degree) {
    return degree * d2rCoefficient;
}
constexpr float r2d(float rad) {
    return rad * r2dCoefficient;
}