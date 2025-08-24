#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <span>
#include <ranges>
#include <memory>
#include <functional>
#include <concepts>
#include <format>
#include <print>
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
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#pragma comment(lib, "vulkan-1.lib")
#endif
#include <vulkan/vulkan.h>
#define VULKAN_BEGIN namespace vulkan {
#define NAMESPACE_END	}

//----------Helper Class-------------------------------------------------------
using emptyList = std::initializer_list<std::monostate>;
template<typename T>
class optionalRef {
	T* const pointer = nullptr;
public:
	optionalRef() = default;
	optionalRef(T& value) :pointer(&value) {}
	optionalRef(const optionalRef<std::remove_const_t<T>>& other) :pointer(&other) {}
	//Const Function
	T& Get() const {
#ifndef NDEBUG
		if (!pointer)
			throw 0;
#endif
		return *pointer;
	}
	operator T& () const { return Get(); }
	T* operator&() const { return pointer; }
	//Non-const Function
	optionalRef& operator=(const optionalRef&) = delete;
};
class optionalRef_any {
	const void* const pointer = nullptr;
public:
	optionalRef_any() = default;
	optionalRef_any(const auto& value) :pointer(&value) {}
	//Const Function
	const void* operator&() const { return pointer; }
	//Non-const Function
	optionalRef_any& operator=(const optionalRef_any&) = delete;
};
template<typename T>
class arrayRef {
	T* const pArray = nullptr;
	size_t count = 0;
public:
	arrayRef() = default;
	arrayRef(T& data) :pArray(&data), count(1) {}
	template<typename R>
	arrayRef(R&& range) requires requires(R r) {
		requires std::ranges::contiguous_range<R>;
		requires std::ranges::sized_range<R>;
		requires std::ranges::borrowed_range<R>;
		requires std::convertible_to<decltype(std::ranges::data(r)), T*>;
	} :pArray(std::ranges::data(range)), count(std::ranges::size(range)) {}
	arrayRef(T* pData, size_t elementCount) :pArray(pData), count(elementCount) {}
	arrayRef(const arrayRef<std::remove_const_t<T>>& other) :pArray(&other), count(other.Count()) {}
	//Const Function
	/*Deprecated*/ T* Pointer() const { return pArray; }
	operator T* () const { return pArray; }
	size_t Count() const { return count; }
	T* operator&() const { return pArray; }
	T* begin() const { return pArray; }
	T* end() const { return pArray + count; }
	//Non-const Function
	arrayRef& operator=(const arrayRef&) = delete;
};

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

//----------Other--------------------------------------------------------------
#define ExecuteOnce(...) { static bool executed = false; if (executed) return __VA_ARGS__; executed = true; }
#define DefineStaticDataMember(a) inline decltype(a) a

template<typename... Ts>
void OutputMessage(const std::format_string<Ts...> format, Ts&&... arguments) {
	std::print(format, std::forward<Ts>(arguments)...);
}