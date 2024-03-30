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
    T* const pArray = nullptr;
    size_t count = 0;
public:
    arrayRef() = default;
    arrayRef(T& data) :pArray(&data), count(1) {}
    template<size_t elementCount>
    arrayRef(T(&data)[elementCount]) : pArray(data), count(elementCount) {}
    arrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
    arrayRef(const arrayRef<std::remove_const_t<T>>& other) :pArray(other.Pointer()), count(other.Count()) {}
    //Getter
    T* Pointer() const { return pArray; }
    size_t Count() const { return count; }
    //Const Function
    T& operator[](size_t index) const { return pArray[index]; }
    T* begin() const { return pArray; }
    T* end() const { return pArray + count; }
    //Non-const Function
    arrayRef& operator=(const arrayRef&) = delete;
};
#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }

//----------Math Related-------------------------------------------------------
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
/*Matrix*/
inline glm::mat4 FlipVertical(const glm::mat4& projection) {
    glm::mat4 _projection = projection;
    for (uint32_t i = 0; i < 4; i++)
        _projection[i][1] *= -1;
    return _projection;
}