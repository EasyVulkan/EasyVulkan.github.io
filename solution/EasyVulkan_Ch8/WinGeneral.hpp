#define DISABLE_VK_LAYERS_AND_EXTENSIONS_CHECK
#include "VKBase.h"
#include <windows.h>
#include <Vulkan/vulkan_win32.h>
#if defined(DEBUG) || defined(_DEBUG)
#pragma comment(linker, "/subsystem:console")
#define Main main
#else
#pragma comment(linker, "/subsystem:windows")
#define Main _stdcall WinMain
#endif

//Enum
enum class hdrColorSpace {
	DISABLE = 0,
	HDR10_ST2084 = VK_COLOR_SPACE_HDR10_ST2084_EXT,
	DOLBYVISION = VK_COLOR_SPACE_DOLBYVISION_EXT,
	HDR10_HLG = VK_COLOR_SPACE_HDR10_HLG_EXT
};
//Class
class window {
	struct windowClass :WNDCLASSEX {
		windowClass() {
			//ZeroMemory(this, sizeof(WNDCLASSEX)); No necessary. Static object automatically zeros memory at launch.
			hInstance = GetModuleHandle(0);
			cbSize = sizeof(WNDCLASSEX);
			style = CS_OWNDC;
			lpfnWndProc = window::HandleMessageSetup;
			hInstance = GetModuleHandle(0);;
			hCursor = LoadCursor(NULL, IDC_ARROW);
			lpszClassName = L"Main WindowClass";
			RegisterClassEx(this);
		}
	};
	HWND hWindow = nullptr;
	const wchar_t* name;
	MSG message = {};
	//--------------------
	static LRESULT CALLBACK HandleMessageSetup(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam) {
		if (message == WM_NCCREATE) {//NC means non-client area (e.g. title bar, system menu). This message is sent before WM_CREATE.
			window* const pWindow = (window*)(((CREATESTRUCTW*)lParam)->lpCreateParams);
			//Store a pointer to the user-defined window object, in case you want to acces the object in the WNDPROC function.
			SetWindowLongPtr(hWindow, GWLP_USERDATA, (LONG_PTR)pWindow);
			//HandleMessageSetup(...) should be called only once, reset WNDPROC to be a non-member function.
			SetWindowLongPtr(hWindow, GWLP_WNDPROC, (LONG_PTR)HandleMessageThunk);
			return pWindow->HandleMessage(hWindow, message, wParam, lParam);
		}
		return DefWindowProc(hWindow, message, wParam, lParam);//Def means default.
	}
	static LRESULT CALLBACK HandleMessageThunk(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam) {
		return ((window*)GetWindowLongPtr(hWindow, GWLP_USERDATA))->HandleMessage(hWindow, message, wParam, lParam);
	}
	LRESULT HandleMessage(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
public:
	window(const wchar_t* name) :
		name(name) {}
	window(const wchar_t* name, SIZE size, DWORD style = WS_CAPTION | WS_SYSMENU) :
		name(name) {
		Create(size, style);
	}
	window(window&&) = delete;
	~window() {
		DestroyWindow(hWindow);
	}
	//Getter
	HWND HWindow() const { return hWindow; }
	const wchar_t* Name() const { return name; }
	const MSG& Message() const { return message; }
	//Const Function
	bool ShouldClose() { return message.message == WM_QUIT; }
	//Non-const Function
	void Create(SIZE size, DWORD style) {
		RECT sizeRect = { 0, 0, size.cx, size.cy };
		AdjustWindowRect(&sizeRect, style, false);
		size.cx = sizeRect.right - sizeRect.left;
		size.cy = sizeRect.bottom - sizeRect.top;
		POINT view = {
			(GetSystemMetrics(SM_CXSCREEN) - size.cx) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - size.cy) / 2
		};
		hWindow = CreateWindow(WindowClass().lpszClassName, name, style,
			view.x, view.y, size.cx, size.cy,
			nullptr, nullptr, WindowClass().hInstance, this);
		ShowWindow(hWindow, SW_SHOWDEFAULT);
	}
	void PollEvents() {
		while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) && !ShouldClose())
			TranslateMessage(&message),
			DispatchMessage(&message);
	}
	//Static Function
	static const windowClass& WindowClass() {
		static windowClass mainWindowClass;
		return mainWindowClass;
	}
};
//Macro
#define RETURN_MSG return (int)mainWindow.Message().wParam;

//Defalut Variable
window mainWindow(L"EasyVK");
DWORD style_windowed;
/*Input*/
int32_t currentButton;
constexpr easyVulkan::mouseStatus& easyVK_mouse = easyVulkan::mouseStatus::mouse;
inline bool KeyPressed(uint16_t keyCode) { return GetAsyncKeyState(keyCode); }
easyVulkan::keyManager keyManager(KeyPressed);
/*Changeable*/
template<> ULONGLONG easyVulkan::doubleClickMaxInterval<ULONGLONG> = 500;

//Function
auto PreInitialization_EnableComputeQueue() {
	static bool enbaleComputeQueue;//Static object will be zero-initialized at launch.
	enbaleComputeQueue = true;
	struct {
		static bool Get() { return enbaleComputeQueue; }
	} wrapper;
	return wrapper;
}
auto PreInitialization_TryEnableHdrByOrder(std::array<hdrColorSpace, 3> hdrColorSpaces = {}) {
	static std::array<hdrColorSpace, 3> _hdrColorSpaces;
	_hdrColorSpaces = hdrColorSpaces;
	return [] { return _hdrColorSpaces; };
}
bool InitializeWindow(VkExtent2D size, bool fullScreen = false, bool isResizable = true, bool limitFrameRate = true) {
	using namespace vulkan;
	//Push extensions
	graphicsBase::Base().PushInstanceExtension("VK_KHR_surface");
	graphicsBase::Base().PushInstanceExtension("VK_KHR_win32_surface");
	auto hdrColorSpaces = decltype(PreInitialization_TryEnableHdrByOrder()){}();
	if (hdrColorSpaces[0] != hdrColorSpace::DISABLE)
		graphicsBase::Base().PushInstanceExtension("VK_EXT_swapchain_colorspace");
	graphicsBase::Base().PushDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	//Create a vulkan instance
	graphicsBase::Base().CreateInstance();
	//Create a window
	style_windowed = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	if (isResizable)
		style_windowed |= WS_SIZEBOX | WS_MAXIMIZEBOX;
	fullScreen ?
		mainWindow.Create({ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) }, WS_POPUP) :
		mainWindow.Create({ long(size.width), long(size.height) }, style_windowed);
	//Create a vulkan surface
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = window::WindowClass().hInstance;
	surfaceCreateInfo.hwnd = mainWindow.HWindow();
	if (VkResult result = vkCreateWin32SurfaceKHR(graphicsBase::Base().Instance(), &surfaceCreateInfo, nullptr, &surface)) {
		std::cout << "[ InitializeWindow ]\nFailed to create a window surface!\nError code: " << std::hex << result << std::endl;
		return false;
	}
	graphicsBase::Base().Surface(surface);
	//Get a physical device
	graphicsBase::Base().GetPhysicalDevice(decltype(PreInitialization_EnableComputeQueue())::Get());
	//Set HDR surface format
	for (auto& i : hdrColorSpaces)
		if (i == hdrColorSpace::DISABLE)
			break;
		else
			for (auto& j : graphicsBase::Base().SurfaceFormats())
				if (j.colorSpace == VkColorSpaceKHR(i)) {
					graphicsBase::Base().SetSurfaceFormat(j);
					break;
				}
	//Create a logical device and a swapchain
	graphicsBase::Base().CreateLogicalDevice();
	graphicsBase::Base().CreateSwapchain(limitFrameRate);
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
		info << mainWindow.Name() << L"   " << std::fixed << dframe / dt << " FPS";
		SetWindowText(mainWindow.HWindow(), info.str().c_str());
		info.str(L"");
		time0 = time1;
		dframe = 0;
	}
}
void UpdateCursorPosition() {
	//HandleMessage(...) updates the mouse position conditionally when the mouse is out of window.
	//Call this function manually in every rendering loop.
	POINT cursorPosition;
	GetCursorPos(&cursorPosition);
	ScreenToClient(mainWindow.HWindow(), &cursorPosition);
	easyVK_mouse.x = cursorPosition.x;
	easyVK_mouse.y = cursorPosition.y;
}
/*Changeable*/
LRESULT window::HandleMessage(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_SIZE:
		//TODO If necessary
		break;
	case WM_CHAR:
		//TODO If necessary
		break;
#pragma region Cursor
	case WM_LBUTTONDOWN:
		currentButton = easyVulkan::lButton;
		SetCapture(mainWindow.HWindow());
		break;
	case WM_MBUTTONDOWN:
		currentButton = easyVulkan::mButton;
		SetCapture(mainWindow.HWindow());
		break;
	case WM_RBUTTONDOWN:
		currentButton = easyVulkan::rButton;
		SetCapture(mainWindow.HWindow());
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		currentButton = easyVulkan::none;
		ReleaseCapture();
		break;
	case WM_MOUSEWHEEL:
		//This is a callback function, chime_mouse.sy will be updated automatically only when a mouse wheel is scrolled,
		//which means you should set chime_mouse.sy to 0 in rendering loop.
		easyVulkan::mouseStatus::mouse.sy = short(HIWORD(wParam)) / 120.0;
		break;
#pragma endregion
	case WM_KEYDOWN:
		switch (wParam) {
		default:
			return DefWindowProc(hWindow, message, wParam, lParam);
		case VK_ESCAPE:
			break;
		}
		[[fallthrough]];
	case WM_CLOSE:
	case WM_QUIT://If the window wouldn't quit correctly, loop until it quits.
		PostQuitMessage(0);//Argument'll be stored to MSG::wParam. When quiting, DestroyWindow(...) is called in ~window()Cno need to return DefWindowProc(...).
		return 0;//If the program is running with a console, you should terminate the program by clicking the console's closebox or press Esc key.
	}
	return DefWindowProc(hWindow, message, wParam, lParam);
}