#include "VKBase.h"
#include "Input.h"
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#pragma comment(lib, "vulkan-1.lib")
#ifndef NDEBUG
#pragma comment(linker, "/subsystem:console")
#define Main main
#else
#pragma comment(linker, "/subsystem:windows")
#define Main _stdcall WinMain
#endif

//Class
class window {
	struct windowClass :WNDCLASSEX {
		windowClass() {
			//ZeroMemory(this, sizeof(WNDCLASSEX)); No necessary. Static object automatically zeros memory at launch.
			cbSize = sizeof(WNDCLASSEX);
			style = CS_OWNDC;
			lpfnWndProc = WindowProcedureSetup;
			hInstance = GetModuleHandle(0);
			hCursor = LoadCursor(NULL, IDC_ARROW);
			lpszClassName = L"Main WindowClass";
			RegisterClassEx(this);
		}
	};
	HWND hWindow = nullptr;
	MSG message = {};
	bool shouldClose = false;
	LRESULT(*fHandleMessage)(HWND, UINT, WPARAM, LPARAM) = nullptr;
	//--------------------
	static LRESULT CALLBACK WindowProcedureSetup(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam) {
		if (message != WM_NCCREATE)//NC means non-client area (e.g. title bar, system menu). This message is sent before WM_CREATE.
			return DefWindowProc(hWindow, message, wParam, lParam);//Def means default.
		window* pWindow = reinterpret_cast<window*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
		//Store a pointer of the user-defined window object, in case you want to acces the object in the WNDPROC function.
		SetWindowLongPtr(hWindow, GWLP_USERDATA, (LONG_PTR)pWindow);
		//WindowProcedureSetup(...) should be called only once, reset WNDPROC to a static function which calls fHandleMessage(...) directly.
		SetWindowLongPtr(hWindow, GWLP_WNDPROC, (LONG_PTR)WindowProcedureThunk);
		return WindowProcedureThunk(hWindow, message, wParam, lParam);
	}
	static LRESULT CALLBACK WindowProcedureThunk(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam) {
		window* pWindow = reinterpret_cast<window*>(GetWindowLongPtr(hWindow, GWLP_USERDATA));
		switch (message) {
			//If the program is running with a console, you should terminate the program by clicking the console's closebox.
		case WM_CLOSE:
			pWindow->shouldClose = true;
			return 0;//When quiting, DestroyWindow(...) is called in ~window()Cno need to return DefWindowProc(...)
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
		return (
			pWindow->fHandleMessage ?
			pWindow->fHandleMessage :
			DefWindowProc)(hWindow, message, wParam, lParam);
	}
public:
	window() = default;
	window(const wchar_t* name, SIZE size, DWORD style, DWORD exStyle = 0, const WNDCLASSEX& windowClass = MainWindowClass()) {
		Create(name, size, style, exStyle, windowClass);
	}
	window(window&&) = delete;
	~window() {
		DestroyWindow(hWindow);
	}
	//Getter
	HWND HWindow() const { return hWindow; }
	const MSG& Message() const { return message; }
	bool ShouldClose() { return shouldClose; }
	//Setter
	void FHandleMessage(LRESULT(*fHandleMessage)(HWND, UINT, WPARAM, LPARAM)) { this->fHandleMessage = fHandleMessage; }
	//Non-const Function
	void Create(const wchar_t* name, SIZE size, DWORD style, DWORD exStyle = 0, const WNDCLASSEX& windowClass = MainWindowClass()) {
		RECT sizeRect = { 0, 0, size.cx, size.cy };
		AdjustWindowRect(&sizeRect, style, false);
		size.cx = sizeRect.right - sizeRect.left;
		size.cy = sizeRect.bottom - sizeRect.top;
		POINT view = {
			(GetSystemMetrics(SM_CXSCREEN) - size.cx) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - size.cy) / 2
		};
		hWindow = CreateWindowEx(exStyle, windowClass.lpszClassName, name, style,
			view.x, view.y, size.cx, size.cy,
			nullptr, nullptr, windowClass.hInstance, this);
		if (hWindow)
			ShowWindow(hWindow, SW_SHOWDEFAULT);
	}
	void PollEvents() {
		while (PeekMessage(&message, hWindow, 0, 0, PM_REMOVE))
			TranslateMessage(&message),
			DispatchMessage(&message);
	}
	void WaitEvent() {
		if (GetMessage(&message, hWindow, 0, 0))
			TranslateMessage(&message),
			DispatchMessage(&message);
	}
	//Static Function
	static const WNDCLASSEX& MainWindowClass() {
		static windowClass mainWindowClass;
		return mainWindowClass;
	}
};

//Defalut Variable
window mainWindow;
DWORD style_windowed;
/*Input Related*/
namespace {
	input::mouseButton lastButton;
	bool buttonStates[input::_mouseButtonCount];
	int16_t scroll = 0;
}
input::mouseManager mouse([](input::mouseButton button) { return buttonStates[button - 1]; });
input::keyManager keys([](uint16_t keyCode) { return bool(GetAsyncKeyState(keyCode)); });
/*Changeable*/
const wchar_t* windowTitle = L"Chime";

//Function
LRESULT HandleMessage(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam) {
	auto ButtonIsClicked = [](HWND hWindow, input::mouseButton button) {
		buttonStates[button - 1] = true;
		lastButton = button;
		SetCapture(hWindow);
	};
	auto ButtonIsReleased = [](input::mouseButton button) {
		buttonStates[button - 1] = false;
		lastButton = input::none;
		if (!std::accumulate(buttonStates, buttonStates + input::_mouseButtonCount, 0))
			ReleaseCapture();
	};
	switch (message) {
	case WM_ACTIVATE:
		if (wParam != WA_INACTIVE) break;
		std::fill(buttonStates, buttonStates + input::_mouseButtonCount, 0);
		lastButton = input::none;
		ReleaseCapture();
		break;
	case WM_LBUTTONDOWN: ButtonIsClicked(hWindow, input::lButton); break;
	case WM_RBUTTONDOWN: ButtonIsClicked(hWindow, input::rButton); break;
	case WM_MBUTTONDOWN: ButtonIsClicked(hWindow, input::mButton); break;
	case WM_LBUTTONUP: ButtonIsReleased(input::lButton); break;
	case WM_RBUTTONUP: ButtonIsReleased(input::rButton); break;
	case WM_MBUTTONUP: ButtonIsReleased(input::mButton); break;
	case WM_MOUSEWHEEL: scroll = short(HIWORD(wParam)) / WHEEL_DELTA; break;
	case WM_KEYUP:
		if (wParam != VK_ESCAPE) break;
		PostMessage(hWindow, WM_CLOSE, 0, 0);
		return 0;
	}
	return DefWindowProc(hWindow, message, wParam, lParam);
}
auto PreInitialization_EnableSrgb() {
	static bool enableSrgb;//Static object will be zero-initialized at launch
	enableSrgb = true;
	return [] { return enableSrgb; };
}
auto PreInitialization_TryEnableHdrByOrder(VkColorSpaceKHR colorSpace0, VkColorSpaceKHR colorSpace1 = {}, VkColorSpaceKHR colorSpace2 = {}) {
	static VkColorSpaceKHR hdrColorSpaces[3];
	if (colorSpace0 == VK_COLOR_SPACE_HDR10_ST2084_EXT || colorSpace0 == VK_COLOR_SPACE_DOLBYVISION_EXT || colorSpace0 == VK_COLOR_SPACE_HDR10_HLG_EXT)
		hdrColorSpaces[0] = colorSpace0;
	if (colorSpace1 == VK_COLOR_SPACE_HDR10_ST2084_EXT || colorSpace1 == VK_COLOR_SPACE_DOLBYVISION_EXT || colorSpace1 == VK_COLOR_SPACE_HDR10_HLG_EXT)
		hdrColorSpaces[1] = colorSpace1;
	if (colorSpace2 == VK_COLOR_SPACE_HDR10_ST2084_EXT || colorSpace2 == VK_COLOR_SPACE_DOLBYVISION_EXT || colorSpace2 == VK_COLOR_SPACE_HDR10_HLG_EXT)
		hdrColorSpaces[2] = colorSpace2;
	return []()->const auto& { return hdrColorSpaces; };
}
bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {
	using namespace vulkan;
	//Create window
	style_windowed = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	if (isResizable)
		style_windowed |= WS_SIZEBOX | WS_MAXIMIZEBOX;
	fullScreen ?
		mainWindow.Create(windowTitle, { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) }, WS_POPUP) :
		mainWindow.Create(windowTitle, { long(size.width), long(size.height) }, style_windowed);
	if (!mainWindow.HWindow()) {
		outStream << std::format("[ InitializeWindow ] ERROR\nFailed to create a win32 window!\n");
		return false;
	}
	mainWindow.FHandleMessage(HandleMessage);
	//Add extensions
	graphicsBase::Base().AddInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME);
	graphicsBase::Base().AddInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	auto& hdrColorSpaces = decltype(PreInitialization_TryEnableHdrByOrder({})){}();
	if (hdrColorSpaces[0])
		graphicsBase::Base().AddInstanceExtension(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
	graphicsBase::Base().AddDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	//Create vulkan instance
	if (graphicsBase::Base().CreateInstance())
		return false;
	//Create surface
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = window::MainWindowClass().hInstance,
		.hwnd = mainWindow.HWindow()
	};
	if (VkResult result = vkCreateWin32SurfaceKHR(graphicsBase::Base().Instance(), &surfaceCreateInfo, nullptr, &surface)) {
		outStream << std::format("[ InitializeWindow ] ERROR\nFailed to create a window surface!\nError code: {}\n", int32_t(result));
		return false;
	}
	graphicsBase::Base().Surface(surface);
	if (//Get physical device
		graphicsBase::Base().GetPhysicalDevices() ||
		graphicsBase::Base().DeterminePhysicalDevice() ||
		//Create logical device
		graphicsBase::Base().CreateDevice())
		return false;
	//Set surface format if necessary
	if (graphicsBase::Base().GetSurfaceFormats())
		return false;
	VkResult result_enableHdr = VK_SUCCESS;
	true &&//Auto formatting alignment
		hdrColorSpaces[0] && (result_enableHdr = graphicsBase::Base().SetSurfaceFormat({ VK_FORMAT_UNDEFINED, hdrColorSpaces[0] })) &&
		hdrColorSpaces[1] && (result_enableHdr = graphicsBase::Base().SetSurfaceFormat({ VK_FORMAT_UNDEFINED, hdrColorSpaces[1] })) &&
		hdrColorSpaces[2] && (result_enableHdr = graphicsBase::Base().SetSurfaceFormat({ VK_FORMAT_UNDEFINED, hdrColorSpaces[2] }));
	if (result_enableHdr)
		outStream << std::format("[ InitializeWindow ] WARNING\nFailed to enable HDR!\n");
	if (!graphicsBase::Base().SwapchainCreateInfo().imageFormat &&
		decltype(PreInitialization_EnableSrgb()){}())
		if (graphicsBase::Base().SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
			graphicsBase::Base().SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }))
			outStream << std::format("[ InitializeWindow ] WARNING\nFailed to enable sRGB!\n");
	//Create swapchain
	if (graphicsBase::Base().CreateSwapchain(limitFrameRate))
		return false;
	return true;
}
void MakeWindowFullScreen() {
	SetWindowLongPtr(mainWindow.HWindow(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
	SetWindowPos(mainWindow.HWindow(), nullptr, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
}
void MakeWindowWindowed(VkOffset2D position, VkExtent2D size) {
	SetWindowLongPtr(mainWindow.HWindow(), GWL_STYLE, style_windowed | WS_VISIBLE);
	RECT sizeRect = { position.x, position.y, long(position.x + size.width), long(position.y + size.height) };
	AdjustWindowRect(&sizeRect, style_windowed, false);
	size.width = sizeRect.right - sizeRect.left;
	size.height = sizeRect.bottom - sizeRect.top;
	SetWindowPos(mainWindow.HWindow(), nullptr, sizeRect.left, sizeRect.top, size.width, size.height, 0);
}
void MakeWindowWindowed(const WINDOWPLACEMENT& windowPlacement) {
	SetWindowLongPtr(mainWindow.HWindow(), GWL_STYLE, style_windowed | WS_VISIBLE | WS_MAXIMIZE * (windowPlacement.showCmd == SW_MAXIMIZE));
	SetWindowPlacement(mainWindow.HWindow(), &windowPlacement);
}
void SwitchWindowMode(WINDOWPLACEMENT& windowPlacement) {
	//Be aware that if the window is initialized in full screen mode,
	//you must initialize windowPlacement. Example:
	//WINDOWPLACEMENT windowPlacement = { sizeof(WINDOWPLACEMENT), 0, 1, {-1,-1}, {-1,-1}, sizeRect };
	if (GetWindowLongPtr(mainWindow.HWindow(), GWL_STYLE) & WS_POPUP)
		MakeWindowWindowed(windowPlacement);
	else
		GetWindowPlacement(mainWindow.HWindow(), &windowPlacement),
		MakeWindowFullScreen();
}
void TitleFps() {
	using namespace std::chrono;
	static steady_clock::time_point time0 = steady_clock::now();
	static steady_clock::time_point time1;
	static double dt;
	static int dframe = -1;
	static std::wstringstream info;
	time1 = steady_clock::now();
	dframe++;
	if ((dt = duration<double>(time1 - time0).count()) >= 1) {
		info.precision(1);
		info << windowTitle << L"   " << std::fixed << dframe / dt << " FPS";
		SetWindowText(mainWindow.HWindow(), info.str().c_str());
		info.str(L"");
		time0 = time1;
		dframe = 0;
	}
}
/*Call this function manually in every rendering loop*/
void UpdateInputs() {
	//Cursor position
	//HandleMessage(...) receives WM_MOUSEMOVE conditionally if the mouse is out of window.
	//Following code retrieves cursor position no matter if the window captures the mouse or not.
	POINT cursorPosition;
	GetCursorPos(&cursorPosition);
	ScreenToClient(mainWindow.HWindow(), &cursorPosition);
	mouse.Position(int16_t(cursorPosition.x), int16_t(cursorPosition.y));
	//Button states
	mouse.UpdateAllButtonStates();
	//Scroll delta
	mouse.ScrollY(scroll);
	scroll = 0;
	//Key states
	keys.UpdateAllKeyStates();
}