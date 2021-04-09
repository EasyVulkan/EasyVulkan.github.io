#include "VKBase.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#pragma comment(lib, "glfw3.lib")

//Defalut Variable
GLFWwindow* pWindow;
/*Changeable*/
const char* windowTitle = "EasyVK";
//Function
bool InitializeWindow(VkExtent2D windowSize, bool resizable = true, bool limit_framerate = true) {
	using namespace vulkan;
	if (!glfwInit())//glfwInit() returns 0 if FAIL
		return false;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//Push extensions
#ifdef _WIN32
	graphicsBase::Base().PushInstanceExtension("VK_KHR_surface");
	graphicsBase::Base().PushInstanceExtension("VK_KHR_win32_surface");
#else
	uint32_t extensionCount = 0;
	const char** extensionNames;
	extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);
	if (!extensionNames) {
		std::cout << "[ InitializeWindow ]\nVulkan is not available on this machine!\n";
		glfwTerminate();
		return false;
	}
	for (size_t i = 0; i < extensionCount; i++)
		graphicsBase::Base().PushInstanceExtension(extensionNames[i]);
#endif
	graphicsBase::Base().PushDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	//Create a vulkan instance
	graphicsBase::Base().CreateInstance();
	//Create a glfw window
	glfwWindowHint(GLFW_RESIZABLE, resizable);
	pWindow = glfwCreateWindow(windowSize.width, windowSize.height, windowTitle, nullptr, nullptr);//Returns 0 if FAIL
	if (!pWindow) {
		std::cout << "[ InitializeWindow ]\nFailed to create a glfw window!\n";
		glfwTerminate();
		return false;
	}
	//Create a vulkan surface
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	if (VkResult result = glfwCreateWindowSurface(graphicsBase::Base().Instance(), pWindow, nullptr, &surface)) {
		std::cout << "[ InitializeWindow ]\nFailed to create a window surface!\nError code: " << std::hex << result << std::endl;
		glfwTerminate();
		return false;
	}
	graphicsBase::Base().Surface(surface);
	//Get a physical device
	graphicsBase::Base().GetPhysicalDevice();
	//Create a logical device and a swapchain
	graphicsBase::Base().CreateLogicalDevice();
	graphicsBase::Base().CreateSwapchain(limit_framerate);
	return true;
}
void TerminateWindow() {
	vulkan::graphicsBase::Base().WaitIdle();
	glfwTerminate();
}
void TitleFps() {
	static double time0 = glfwGetTime();
	static double time1;
	static double dt;
	static int dframe = -1;
	static std::stringstream info;
	time1 = glfwGetTime();
	dframe++;
	if ((dt = time1 - time0) >= 1) {
		info.precision(1);
		info << windowTitle << "    " << std::fixed << dframe / dt << " FPS";
		glfwSetWindowTitle(pWindow, info.str().c_str());
		info.str("");
		time0 = time1;
		dframe = 0;
	}
}