#pragma once
#include "EasyVKStart.h"

//----------Math Related-------------------------------------------------------
#define ReturnLess(x,y) ((x) < (y) ? (x) : (y))
#define ReturnGreater(x,y) ((x) > (y) ? (x) : (y))
constexpr float d2rCoefficient = float(M_PI / 180);
constexpr float r2dCoefficient = float(180 / M_PI);
constexpr float operator""_d2r(long double degree) {
	return float(degree * d2rCoefficient);
}
constexpr float operator""_r2d(long double rad) {
	return float(rad * r2dCoefficient);
}
inline float d2r(float degree) {
	return degree * d2rCoefficient;
}
inline float r2d(float rad) {
	return rad * r2dCoefficient;
}

//----------Type Related-------------------------------------------------------
template<typename T> struct add_lvalue_reference_if_is_not_pointer {
	using type = std::conditional_t<std::is_pointer_v<T>, T, T&>;
};
template<typename T> using add_lvalue_reference_if_is_not_pointer_t = typename add_lvalue_reference_if_is_not_pointer<T>::type;

using std::same_as;
using std::derived_from;
using std::convertible_to;
using std::constructible_from;

template<typename T> struct lrValue;//Forward Declaration
template<typename T>
class lrReferenceWrapper {
	template<typename T, size_t TSize = sizeof(T)>
	struct rMarkedPointerWrapper {
		uintptr_t pointer;
		//--------------------
		rMarkedPointerWrapper(void* pointer, bool rMark) :
			pointer(uintptr_t(pointer) + rMark) {}
		rMarkedPointerWrapper(const rMarkedPointerWrapper& other) :
			pointer(other.pointer & ~1) {}
		rMarkedPointerWrapper(rMarkedPointerWrapper&& other) noexcept :
			pointer(other.pointer) {
			other.pointer = NULL;
		}
		//Getter
		T& Get() const { return *(T*)(pointer & ~1); }
		bool RMark() const { return pointer & 1; }
	};
	template<typename T>
	struct rMarkedPointerWrapper<T, 1> {
		uintptr_t pointer;
		bool rMark = false;
		//--------------------
		rMarkedPointerWrapper(void* pointer, bool rMark) :
			pointer(uintptr_t(pointer)), rMark(rMark) {}
		rMarkedPointerWrapper(const rMarkedPointerWrapper& other) :
			pointer(other.pointer) {}
		rMarkedPointerWrapper(rMarkedPointerWrapper&& other) noexcept :
			pointer(other.pointer), rMark(other.rMark) {
			other.rMark = false;
		}
		//Getter
		T& Get() const { return *(T*)pointer; }
		bool RMark() const { return rMark; }
	};
	//--------------------
	rMarkedPointerWrapper<T> pwPointer;
public:
	lrReferenceWrapper(T& lValue) :
		pwPointer(&lValue, false) {}
	template<derived_from<T> VT>
	lrReferenceWrapper(VT&& rValue) :
		pwPointer(new VT(std::move(rValue)), true) {}
	template<convertible_to<T> VT = T> requires(!derived_from<VT, T>)
		lrReferenceWrapper(VT rValue) :
		pwPointer(new T(std::move(rValue)), true) {}
	//Support list initialization (excludes aggregate initialization), but arguments won't be converted implicitly.
	template<typename... TArgs> requires(constructible_from<T, TArgs...> && sizeof...(TArgs) > 1)
		lrReferenceWrapper(TArgs&&...args) :
		pwPointer(new T(std::forward<TArgs>(args)...), true) {}
	lrReferenceWrapper(const lrReferenceWrapper&) = default;
	lrReferenceWrapper(lrReferenceWrapper&&) = default;
	lrReferenceWrapper(lrValue<T>& other) :
		lrReferenceWrapper((lrReferenceWrapper&&)other) {}
	~lrReferenceWrapper() {
		if (IsRMarked())
			delete& Get();
	}
	//Getter
	T& Get() const { return pwPointer.Get(); }
	operator T& () const { return Get(); }
	template<typename VT>
	explicit operator VT& () const { return (VT&)Get(); }
	template<constructible_from<T> VT>
	operator VT() const { return VT(Get()); }
	//Const Function
	bool IsRMarked() const { return pwPointer.RMark(); }
	bool IsNull() const { return &Get(); }
	//Non-const function
	//Why I don't use T& operator(T value) : If value is converted from a convertible varibale,
	//VS will select default copy assignment rather than it, and show an error that tells default copy assignment is implicitly deleted. What the heck!
	template<convertible_to<T> VT = T>
	T& operator=(VT value) {
		if (IsRMarked()) {
			if constexpr (!std::is_trivially_copyable_v<T>)
				Get().~T();
			Get() = value;
		}
		return Get();
	}
};
template<typename T>
struct lrValue : private lrReferenceWrapper<T> {
	//Use lrValue<T> instead of lrReferenceWrapper<T>&& as parameter in a constructor, no need to use std::move().
#ifdef FxxkIntelliSense //IntelliSense will show errors, but code compiles.
	//The best viable function of copy construction is lrReferenceWrapper(lrValue<T>&).
	//And that function will always call lrReferenceWrapper(lrReferenceWrapper&&).
	using lrReferenceWrapper<T>::lrReferenceWrapper;
#else
	lrValue(T& lValue) :
		lrReferenceWrapper<T>(lValue) {}
	template<derived_from<T> VT>
	lrValue(VT&& rValue) :
		lrReferenceWrapper<T>(std::move(rValue)) {}
	template<convertible_to<T> VT = T> requires(!derived_from<VT, T>)
		lrValue(VT rValue) :
		lrReferenceWrapper<T>(std::move(rValue)) {}
	template<typename... TArgs> requires(constructible_from<T, TArgs...> && sizeof...(TArgs) > 1)
		lrValue(TArgs&&...args) :
		lrReferenceWrapper<T>(args...) {}
	lrValue(lrValue& other) :
		lrReferenceWrapper<T>((lrReferenceWrapper<T>&&)other) {}
#endif
	lrValue(lrValue&&) = default;
	//Copy/move assignment is not allowed and deleted.
};

EASY_VULKAN_BEGIN
using namespace glm;

#pragma region Camera
#ifdef GLM_FORCE_LEFT_HANDED
constexpr bool rhc = false;
#else
constexpr bool rhc = true;
#endif

class camera {
protected:
	lrReferenceWrapper<vec3> rwPosition;
	lrReferenceWrapper<vec3> rwTargeted;
	lrReferenceWrapper<vec3> rwUp0;			//Automatically normalized
	lrReferenceWrapper<vec3> rwFront;		//Automatically normalized
	vec3 up = {};							//Automatically normalized
	vec3 rotationPerpendicularToUp0 = {};	//Automatically normalized
	mat4 view;
	bool isLookingAt = false;
	bool up0AndFrontIsParellel = cross(rwUp0.Get(), rwFront.Get()) == vec3{};
	//--------------------
	void RecalculateView_Internal() {
		vec3 front = isLookingAt ? normalize(rwTargeted.Get() - rwPosition.Get()) : rwFront.Get();
		if (up0AndFrontIsParellel = cross(rwUp0.Get(), rwFront.Get()) == vec3{})
			dot(rwUp0.Get(), rwFront.Get()) < 0 ?
			rotationPerpendicularToUp0 = up :
			rotationPerpendicularToUp0 = -up;
		else
			up = normalize(rwUp0.Get() - dot(rwUp0.Get(), front) * front),
			rotationPerpendicularToUp0 = normalize(rwFront.Get() - dot(rwFront.Get(), rwUp0.Get()) * rwUp0.Get());
		view = lookAt(
			rwPosition.Get(),
			isLookingAt ? rwTargeted.Get() : rwPosition.Get() + rwFront.Get(),
			up
		);
	}
public:
	camera(lrValue<vec3> position, lrValue<vec3> up, lrValue<vec3> front, lrValue<vec3> targeted = { 0,0,0 }) :
		rwPosition(position), rwUp0(up), rwFront(front), rwTargeted(targeted) {
		rwUp0.Get() = normalize(rwUp0.Get());
		rwFront.Get() = normalize(rwFront.Get());
		RecalculateView_Internal();
	}
	//Getter
	vec3 Position() const { return rwPosition; }
	vec3 Up0() const { return rwUp0; }
	vec3 Up() const { return up; }
	vec3 Front() const { return rwFront; }
	vec3 LookAt() const { return rwTargeted; }
	bool IsLookingAt() const { return isLookingAt; }
	const mat4& View() const { return view; }
	//Setter
	void Position(vec3 position) { rwPosition = position; RecalculateView_Internal(); }
	void Up0(vec3 up) { rwUp0 = normalize(up); RecalculateView_Internal(); }
	void Front(vec3 front) { rwFront = normalize(front); IsLookingAt(false); }
	void LookAt(vec3 targeted) { rwTargeted = targeted; IsLookingAt(true); }
	void IsLookingAt(bool isLookingAt) { this->isLookingAt = isLookingAt; RecalculateView_Internal(); }
	//Non-const Function
	const mat4& RecalculateView() {
		vec3 front = isLookingAt ? normalize(rwTargeted.Get() - rwPosition.Get()) : rwFront.Get();
		if (!up0AndFrontIsParellel)
			up = normalize(rwUp0.Get() - dot(rwUp0.Get(), front) * front);
		return view = lookAt(
			rwPosition.Get(),
			isLookingAt ? rwTargeted.Get() : rwPosition.Get() + rwFront.Get(),
			up
		);
	}
	/*Control*/
	void Zoom(float dz, float sensitivity) {//If isLookingAt is false, this is dolly zoom.
		isLookingAt ?
			rwPosition.Get() += (rwTargeted.Get() - rwPosition.Get()) * dz * sensitivity :
			rwPosition.Get() += rwFront.Get() * dz * sensitivity;
		RecalculateView();
	}
	void Rotate(float dx, float dy, float sensitivity) {//If isLookingAt is true, no effect.
		if (isLookingAt)
			return;
		vec3& front = rwFront.Get();
		vec3 right = cross(front, up);//Or left, if rhc. Considering the rotation angle is also opposite, calculating is same.
		if constexpr (rhc)
			dx *= -1;
		mat4 rotateX = rotate(mat4(1), dy * sensitivity, right);
		mat4 rotateY = rotate(mat4(1), dx * sensitivity, rwUp0.Get());
		if (up0AndFrontIsParellel) {
			if (dot(front, rwUp0.Get()) * dy < 0)
				up0AndFrontIsParellel = false,
				front = vec3(rotateY * rotateX * vec4(front, 1));//up is calculated in RecalculateView();
			else
				up = vec3(rotateY * vec4(up, 1));
			rotationPerpendicularToUp0 = vec3(rotateY * vec4(rotationPerpendicularToUp0, 1));
		}
		else {
			vec4 rotatedFront = rotateX * vec4(front, 1);
			if (dot(vec3(rotatedFront), rotationPerpendicularToUp0) < 0) {
				up0AndFrontIsParellel = true;
				if (dot(vec3(rotatedFront), rwUp0.Get()) < 0)
					front = -rwUp0.Get(),
					up = rotationPerpendicularToUp0;
				else
					front = rwUp0.Get(),
					up = -rotationPerpendicularToUp0;
			}
			else
				front = vec3(rotateY * rotatedFront),
				rotationPerpendicularToUp0 = vec3(rotateY * vec4(rotationPerpendicularToUp0, 1));
		}
		RecalculateView();
	}
	void OrbitOrPan(float dx, float dy, float sensitivity) {//isLookingAt? Orbit(): Pan();
		if constexpr (rhc)
			dx *= -1;
		if (isLookingAt) {
			vec3 back = rwPosition.Get() - rwTargeted.Get();
			vec3 left = cross(back, up);//Or right, if rhc.
			mat4 orbitX = rotate(mat4(1), -dy * sensitivity, left);
			mat4 orbitY = rotate(mat4(1), dx * sensitivity, rwUp0.Get());
			if (up0AndFrontIsParellel) {
				if (dot(back, rwUp0.Get()) * dy > 0)
					up0AndFrontIsParellel = false,
					rwPosition.Get() = rwTargeted.Get() + vec3(orbitY * orbitX * vec4(back, 1));
				else
					up = vec3(orbitY * vec4(up, 1));
				rotationPerpendicularToUp0 = vec3(orbitY * vec4(rotationPerpendicularToUp0, 1));
			}
			else {
				vec4 orbitedBack = orbitX * vec4(back, 1);
				if (dot(vec3(orbitedBack), rotationPerpendicularToUp0) > 0) {
					up0AndFrontIsParellel = true;
					if (dot(vec3(orbitedBack), rwUp0.Get()) < 0)
						rwPosition.Get() = rwTargeted.Get() - rwUp0.Get() * length(back),
						up = -rotationPerpendicularToUp0;
					else
						rwPosition.Get() = rwTargeted.Get() + rwUp0.Get() * length(back),
						up = rotationPerpendicularToUp0;
				}
				else
					rwPosition.Get() = rwTargeted.Get() + vec3(orbitY * orbitedBack),
					rotationPerpendicularToUp0 = vec3(orbitY * vec4(rotationPerpendicularToUp0, 1));
			}
		}
		else {
			vec3 right = normalize(cross(rwFront.Get(), up));
			rwPosition.Get() += right * dx * sensitivity - up * dy * sensitivity;
		}
		RecalculateView();
	}
};
#pragma endregion

#pragma region Input
//Implement this If mouse.clickCount is neressary
template<typename T> T doubleClickMaxInterval;

enum mouseButton {
	none,
	lButton,
	rButton,
	mButton
};
enum inputState :char {
	release = -1,
	hover, keyUp = 0,//'up' is too common
	click, firstPress = 1,
	hold
};
struct mouseStatus {
	union {
		struct { short button, action; };
		int buttonAction = 0;
	};
	unsigned clickCount = 0;
	//Get following variables on your own
	double x = 0, y = 0;//Measured in pixel, relative to the top-left corner of the window
	double sx = 0, sy = 0;//Two-dimensional scroll
	//Static
	static mouseStatus mouse;
#define easyVk_mouse easyVulkan::mouseStatus::mouse
	//--------------------
	//Write A function to get currentButton, call Button() first then call Count() if necessary
	//If no button is pressed, currentButton is 0
	void Button(int currentButton) {
		action = currentButton ?
			action > hover && button == currentButton ? hold : click :
			action > hover ? release : hover;
		if (action != release)
			button = currentButton;
	}
	template<typename T> void Count(T currentTime) {
		static int lastButton;
		static T lastTime = currentTime;
		static T interval;
		if (clickCount) {
			interval = currentTime - lastTime;
			if (interval > doubleClickMaxInterval<T>)
				lastButton = 0,
				clickCount = 0;
		}
		if (action == click)
			if (button == lastButton)
				clickCount++,
				lastTime = currentTime;
			else
				clickCount = 1,
				lastButton = button,
				lastTime = currentTime;
	}
private:
	mouseStatus() = default;
	mouseStatus(mouseStatus&&) = delete;//Delete default copy/move ctor/assignment at once.
};
inline mouseStatus mouseStatus::mouse;

class keyManager {
protected:
	char keyStates[255]{};
	std::vector<unsigned char> availableKeys;
	bool(*function)(unsigned char) = nullptr;
public:
	keyManager(bool(*function)(unsigned char)) :function(function) {}
	//Getter
	char KeyState(unsigned char keyCode) const { return keyStates[keyCode]; }
	//Const Function
	template<same_as<unsigned char>... T>
	bool KeyPressed(T... keyCodes) {
		return (... && keyStates[keyCodes]);
	}
	//Non-const Function
	void Register(unsigned char keyCode) { availableKeys.push_back(keyCode); }
	void UpdateAllKeyStates() {
		bool currentKeyState;
		for (auto i : availableKeys) {
			currentKeyState = function(i);
			keyStates[i] = currentKeyState ?
				keyStates[i] > hover ? hold : firstPress :
				keyStates[i] > hover ? release : hover;
		}
	}
};

#pragma endregion

NAMESPACE_END