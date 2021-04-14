#pragma once
#include "EasyVKStart.h"
#pragma warning(disable:26812)
#pragma warning(disable:6385)
#pragma warning(disable:4267)

#define DestroyHandleBy(x) if (handle) { x; handle = VK_NULL_HANDLE; }
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;

//Constexpr
#ifdef GAMMA_CORRECTION
constexpr VkFormat format_rgba8 = VK_FORMAT_R8G8B8A8_SRGB;
constexpr VkFormat format_r8 = VK_FORMAT_R8_SRGB;
#else
constexpr VkFormat format_rgba8 = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat format_r8 = VK_FORMAT_R8_UNORM;
#endif

//Defalut Value
constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

#define VULKAN_BEGIN namespace vulkan {

VULKAN_BEGIN
//Forward Declaration
class graphicsBasePlus;

class graphicsBase {
	VkInstance instance = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;

	VkDevice device = nullptr;
	uint32_t queueFamilyIndex_graphics = -1;
	uint32_t queueFamilyIndex_presentation = -1;
	VkQueue queue_graphics = nullptr;
	VkQueue queue_presentation = nullptr;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	uint32_t currentImageIndex = 0;

	std::vector<const char*> instanceLayers;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> deviceLayers;
	std::vector<const char*> deviceExtensions;

	VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;

	std::vector<void(*)()> callbacks_createSwapchain;
	std::vector<void(*)()> callbacks_destroySwapchain;
	std::vector<void(*)()> callbacks_createDevice;
	std::vector<void(*)()> callbacks_cleanUp;
	//Static
	static graphicsBase singleton;
	static graphicsBasePlus* pPlus;//Pimpl
	//--------------------
	graphicsBase() = default;
	graphicsBase(graphicsBase&&) = delete;
	~graphicsBase() {
		vkDeviceWaitIdle(device);
#ifndef NDEBUG
		DestroyDebugMessenger();
#endif
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		for (auto& i : swapchainImageViews)
			vkDestroyImageView(device, i, nullptr);
		for (auto& i : callbacks_destroySwapchain)
			i();
		for (auto& i : callbacks_cleanUp)
			i();
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
	}
	//Non-const Function
	/*For initialization*/
	//Only check those extensions provided by the Vulkan implementation or by implicitly enabled layers.
	void CheckInstanceLayersAndExtensions() {
#ifdef DISABLE_VK_LAYERS_AND_EXTENSIONS_CHECK
		return;
#endif
		//Get available instance level layers and extensions
		uint32_t layerCount;
		std::vector<VkLayerProperties> availableInstanceLayers;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		if (layerCount)
			availableInstanceLayers.resize(layerCount),
			vkEnumerateInstanceLayerProperties(&layerCount, availableInstanceLayers.data());
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> availableInstanceExtensions;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		if (layerCount)
			availableInstanceExtensions.resize(extensionCount),
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data());
		//Check if required layers and extensions are available
		size_t layerCount_found = 0;
		for (auto& i : instanceLayers)
			for (auto& j : availableInstanceLayers)
				if (!strcmp(i, j.layerName)) {
					layerCount_found++;
					break;
				}
		if (layerCount_found != instanceLayers.size())
			std::cout << "[ graphicsBase ]\nNot all instance level layers are supported!\n",
			abort();
		size_t extensionCount_found = 0;
		for (auto& i : instanceExtensions)
			for (auto& j : availableInstanceExtensions)
				if (!strcmp(i, j.extensionName)) {
					extensionCount_found++;
					break;
				}
		if (extensionCount_found != instanceExtensions.size())
			std::cout << "[ graphicsBase ]\nNot all instance level extensions are supported!\n",
			abort();
	}
	bool CheckDeviceLayersAndExtensions(VkPhysicalDevice& physicalDevice) {
#ifdef DISABLE_VK_LAYERS_AND_EXTENSIONS_CHECK
		return true;
#endif
		//Get available device level layers and extensions
		uint32_t layerCount;
		std::vector<VkLayerProperties> availableDeviceLayers;
		vkEnumerateDeviceLayerProperties(physicalDevice, &layerCount, nullptr);
		if (layerCount)
			availableDeviceLayers.resize(layerCount),
			vkEnumerateDeviceLayerProperties(physicalDevice, &layerCount, availableDeviceLayers.data());
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> availableDeviceExtensions;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		if (layerCount)
			availableDeviceExtensions.resize(extensionCount),
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableDeviceExtensions.data());
		//Check if required layers and extensions are available
		size_t layerCount_found = 0;
		for (auto& i : deviceLayers)
			for (auto& j : availableDeviceLayers)
				if (!strcmp(i, j.layerName)) {
					layerCount_found++;
					break;
				}
		if (layerCount_found != deviceLayers.size())
			return false;
		size_t extensionCount_found = 0;
		for (auto& i : deviceExtensions)
			for (auto& j : availableDeviceExtensions)
				if (!strcmp(i, j.extensionName)) {
					extensionCount_found++;
					break;
				}
		if (extensionCount_found != deviceExtensions.size())
			return false;
		return true;
	}
	bool GetQueueFamilyIndex(VkPhysicalDevice& physicalDevice) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		if (!queueFamilyCount)
			return false;
		std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			VkBool32
				supportGraphics = queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT,
				supportPresentation;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation);
			if (supportGraphics && supportPresentation) {
				queueFamilyIndex_graphics = i;
				queueFamilyIndex_presentation = i;
				break;
			}
			if (supportGraphics && queueFamilyIndex_graphics == -1)
				queueFamilyIndex_graphics = i;
			if (supportPresentation && queueFamilyIndex_presentation == -1)
				queueFamilyIndex_presentation = i;
		}
		if (queueFamilyIndex_graphics == -1 || queueFamilyIndex_presentation == -1) {
			queueFamilyIndex_graphics = queueFamilyIndex_presentation = -1;
			return false;
		}
		return true;
	}
	void CreateDebugMessenger() {
		static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)->VkBool32 {
			std::cout << pCallbackData->pMessage << "\n\n";
			return false;
		};
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
		debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugUtilsMessengerCreateInfo.messageSeverity = 0x1100;
		debugUtilsMessengerCreateInfo.messageType = 0x7;
		debugUtilsMessengerCreateInfo.pfnUserCallback = DebugUtilsMessengerCallback;
		PFN_vkCreateDebugUtilsMessengerEXT
			CreateDebugUtilsMessenger = PFN_vkCreateDebugUtilsMessengerEXT(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		if (CreateDebugUtilsMessenger)
			if (VkResult result = CreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugUtilsMessenger))
				std::cout << "[ graphicsBase ]\nFailed to create a debug messenger!\nError code: " << std::hex << result << std::endl,
				abort();
	}
	void DestroyDebugMessenger() {
		PFN_vkDestroyDebugUtilsMessengerEXT
			DestroyDebugUtilsMessenger = PFN_vkDestroyDebugUtilsMessengerEXT(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (DestroyDebugUtilsMessenger)
			DestroyDebugUtilsMessenger(instance, debugUtilsMessenger, nullptr);
	}
	void CreateSwapchain_Internal() {
		//Create a swapchain
		if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain))
			std::cout << "[ graphicsBase ]\nFailed to create a swapchain!\nError code: " << std::hex << result << std::endl,
			abort();

		//Get swapchain images
		uint32_t swapchainImageCount;
		if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr))
			std::cout << "[ graphicsBase ]\nFailed to get the count of swapchain images!\nError code: " << std::hex << result << std::endl,
			abort();
		swapchainImages.resize(swapchainImageCount);
		if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()))
			std::cout << "[ graphicsBase ]\nFailed to get swapchain images!\nError code: " << std::hex << result << std::endl,
			abort();

		//Destroy previous swapchain image views
		for (auto& i : swapchainImageViews)
			vkDestroyImageView(device, i, nullptr);
		//Create swapchain image views
		swapchainImageViews.resize(swapchainImageCount);
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
		imageViewCreateInfo.components;//All VK_COMPONENT_SWIZZLE_IDENTITY
		imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		for (size_t i = 0; i < swapchainImageCount; i++) {
			imageViewCreateInfo.image = swapchainImages[i];
			VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);
			if (result)
				std::cout << "[ graphicsBase ]\nFailed to create a swapchain image view!\nError code: " << std::hex << result << std::endl,
				abort();
		}
	}
public:
	//Getter
	VkInstance Instance() const { return instance; }
	VkPhysicalDevice PhysicalDevice() const { return physicalDevice; }
	constexpr const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const { return physicalDeviceProperties; }
	constexpr const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const { return physicalDeviceMemoryProperties; }

	VkDevice Device() const { return device; }
	uint32_t QueueFamilyIndex_Graphics() const { return queueFamilyIndex_graphics; }
	uint32_t QueueFamilyIndex_Presentation() const { return queueFamilyIndex_presentation; }
	VkQueue Queue_Graphics() const { return queue_graphics; }
	VkQueue Queue_Presentation() const { return queue_presentation; }

	VkSurfaceKHR Surface() const { return surface; }
	VkSwapchainKHR Swapchain() const { return swapchain; }
	VkImage SwapchainImage(size_t index) const { return swapchainImages[index]; }
	VkImageView SwapchainImageView(size_t index) const { return swapchainImageViews[index]; }
	size_t SwapchainImageCount() const { return swapchainImages.size(); }
	uint32_t CurrentImageIndex() const { return currentImageIndex; }
	constexpr const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const { return swapchainCreateInfo; }
	//Setter
	void Surface(VkSurfaceKHR surface) { if (!this->surface) this->surface = surface; }
	//Const Function
	void WaitIdle() const {
		vkDeviceWaitIdle(device);
	}
	//Non-const Function
	void PushCallback_CreateSwapchain(void(*function)()) {
		callbacks_createSwapchain.push_back(function);
	}
	void PushCallback_DestroySwapchain(void(*function)()) {
		callbacks_destroySwapchain.push_back(function);
	}
	void PushCallback_CreateDevice(void(*function)()) {
		callbacks_createDevice.push_back(function);
	}
	void PushCallback_CleanUp(void(*function)()) {
		callbacks_cleanUp.push_back(function);
	}
	/*For initialization*/
	void PushInstanceLayer(const char* layerName) {
		instanceLayers.push_back(layerName);
	}
	void PushInstanceExtension(const char* extensionName) {
		instanceExtensions.push_back(extensionName);
	}
	void PushDeviceLayer(const char* layerName) {
		deviceLayers.push_back(layerName);
	}
	void PushDeviceExtension(const char* extensionName) {
		deviceExtensions.push_back(extensionName);
	}
	void CreateInstance() {
#ifndef NDEBUG
		//Push validation layer
		instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
		//Push debug extension
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		CheckInstanceLayersAndExtensions();
		VkApplicationInfo applicatianInfo{};
		applicatianInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicatianInfo.apiVersion = VK_API_VERSION_1_0;
		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &applicatianInfo;
		if (instanceCreateInfo.enabledLayerCount = instanceLayers.size())
			instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
		if (instanceCreateInfo.enabledExtensionCount = instanceExtensions.size())
			instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
		if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance))
			std::cout << "[ graphicsBase ]\nFailed to create a vulkan instance!\nError code: " << std::hex << result << std::endl,
			abort();
#ifndef NDEBUG
		CreateDebugMessenger();
#endif
	}
	void GetPhysicalDevice() {
		uint32_t deviceCount;
		if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr))
			std::cout << "[ graphicsBase ]\nFailed to get the count of physical devices!\nError code: " << std::hex << result << std::endl,
			abort();
		if (!deviceCount)
			std::cout << "[ graphicsBase ]\nFailed to find any physical device supports vulkan!\n",
			abort();
		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()))
			std::cout << "[ graphicsBase ]\nFailed to enumerate physical devices!\nError code: " << std::hex << result << std::endl,
			abort();
		for (auto& i : physicalDevices)
			if (GetQueueFamilyIndex(i) && CheckDeviceLayersAndExtensions(i)) {
				physicalDevice = i;
				vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
				std::cout << "Renderer: " << physicalDeviceProperties.deviceName << std::endl;
				vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
				return;
			}
		std::cout << "[ graphicsBase ]\nFailed to find any physical device satisfies all requirements!\n";
		abort();
	}
	void CreateLogicalDevice() {
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo deviceQueueCreateInfos[2]{};
		deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfos[0].queueFamilyIndex = queueFamilyIndex_graphics;
		deviceQueueCreateInfos[0].queueCount = 1;
		deviceQueueCreateInfos[0].pQueuePriorities = &queuePriority;
		deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		deviceQueueCreateInfos[1].queueFamilyIndex = queueFamilyIndex_presentation;
		deviceQueueCreateInfos[1].queueCount = 1;
		deviceQueueCreateInfos[1].pQueuePriorities = &queuePriority;
		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1 + (queueFamilyIndex_graphics != queueFamilyIndex_presentation);
		deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
		deviceCreateInfo.enabledLayerCount = deviceLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = deviceLayers.data();
		deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
		if (VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device))
			std::cout << "[ graphicsBase ]\nFailed to create a vulkan logical device!\nError code: " << std::hex << result << std::endl,
			abort();
		vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
		vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
		for (auto& i : callbacks_createDevice)
			i();
	}
	void CreateSwapchain(bool limit_framerate = true) {
		//Get surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities))
			std::cout << "[ graphicsBase ]\nFailed to get physical device surface capabilities!\nError code: " << std::hex << result << std::endl,
			abort();
		//Define image count
		swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
		//Get swapchain extent
		swapchainCreateInfo.imageExtent =
			surfaceCapabilities.currentExtent.width == -1 ?
			VkExtent2D{
			glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
			glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height) } :
			surfaceCapabilities.currentExtent;
		//Check supported usage
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		else
			std::cout << "[ graphicsBase ]\nWarning: VK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n";

		//Get surface formats
		uint32_t surfaceFormatCount;
		if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr))
			std::cout << "[ graphicsBase ]\nFailed to get the count of surface formats!\nError code: " << std::hex << result << std::endl,
			abort();
		if (!surfaceFormatCount)
			std::cout << "[ graphicsBase ]\nFailed to find any supported surface format!\n",
			abort();
		std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
		if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()))
			std::cout << "[ graphicsBase ]\nFailed to get surface formats!\nError code: " << std::hex << result << std::endl,
			abort();
		//Select a surface format
#ifdef GAMMA_CORRECTION
		constexpr VkFormat requiredSurfaceFormats[]{ VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB };
#else
		constexpr VkFormat requiredSurfaceFormats[]{ VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM };
#endif
		VkSurfaceFormatKHR selectedSurfaceFormat{};
		if (surfaceFormats.size() == 1 &&
			surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			selectedSurfaceFormat = { requiredSurfaceFormats[0] , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		else {
			for (size_t i = 0; i < surfaceFormats.size(); i++) {
				for (size_t j = 0; j < std::size(requiredSurfaceFormats); j++)
					if (surfaceFormats[i].format == requiredSurfaceFormats[j] &&
						surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
						selectedSurfaceFormat = { requiredSurfaceFormats[j], VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
						break;
					}
				if (selectedSurfaceFormat.format != VK_FORMAT_UNDEFINED)
					break;
			}
			if (selectedSurfaceFormat.format == VK_FORMAT_UNDEFINED) {
				selectedSurfaceFormat = surfaceFormats[0];
				std::cout << "[ graphicsBase ]\nWarning: Failed to select the required surface format!\n";
			}
		}
		swapchainCreateInfo.imageFormat = selectedSurfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;

		//Get surface present modes
		uint32_t surfacePresentModeCount;
		if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr))
			std::cout << "[ graphicsBase ]\nFailed to get the count of surface present modes!\nError code: " << std::hex << result << std::endl,
			abort();
		if (!surfacePresentModeCount)
			std::cout << "[ graphicsBase ]\nFailed to find any surface present mode!\n",
			abort();
		std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
		if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data()))
			std::cout << "[ graphicsBase ]\nFailed to get surface present modes!\nError code: " << std::hex << result << std::endl,
			abort();
		//Set present mode to mailbox if available and necessary
		swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (!limit_framerate)
			for (size_t i = 0; i < surfacePresentModeCount; i++)
				if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
					swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}

		//Create a swapchain
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.clipped = VK_TRUE;
		CreateSwapchain_Internal();

		//Create related objects
		for (auto& i : callbacks_createSwapchain)
			i();
	}
	/*After initialization*/
	void RecreateSwapchain() {
		VkSurfaceCapabilitiesKHR surfaceCapabilities{};
		if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities))
			std::cout << "[ graphicsBase ]\nFailed to get physical device surface capabilities!\nError code: " << std::hex << result << std::endl,
			abort();
		swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
		swapchainCreateInfo.oldSwapchain = swapchain;
		vkDeviceWaitIdle(device);
		CreateSwapchain_Internal();
		for (auto& i : callbacks_destroySwapchain)
			i();
		for (auto& i : callbacks_createSwapchain)
			i();
	}
	VkResult SwapImage(VkSemaphore semaphore_imageIsAvailable) {
		switch (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex)) {
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			RecreateSwapchain();
			[[fallthrough]];
		case VK_SUCCESS:
			return VK_SUCCESS;
		default:
			std::cout << "[ graphicsBase ]\nFailed to acquire the next image!\nError code: " << std::hex << result << std::endl;
			return result;
		}
	}
	VkResult SubmitGraphicsCommandBuffer(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) {
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkResult result = vkQueueSubmit(queue_graphics, 1, &submitInfo, fence);
		if (result)
			std::cout << "[ graphicsBase ]\nFailed to submit the graphics command buffer!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult SubmitGraphicsCommandBuffer(VkCommandBuffer commandBuffer,
		VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE) {
		static VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{};
		if (semaphore_imageIsAvailable)
			submitInfo.waitSemaphoreCount = 1,
			submitInfo.pWaitSemaphores = &semaphore_imageIsAvailable,
			submitInfo.pWaitDstStageMask = &waitDstStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		if (semaphore_renderingIsOver)
			submitInfo.signalSemaphoreCount = 1,
			submitInfo.pSignalSemaphores = &semaphore_renderingIsOver;
		return SubmitGraphicsCommandBuffer(submitInfo, fence);
	}
	VkResult SubmitTransferCommandBuffer(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) {
		VkSubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		return SubmitGraphicsCommandBuffer(submitInfo, fence);
	}
	VkResult PresentImage(VkPresentInfoKHR& presentInfo) {
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		switch (VkResult result = vkQueuePresentKHR(queue_presentation, &presentInfo)) {
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			RecreateSwapchain();
			[[fallthrough]];
		case VK_SUCCESS:
			return VK_SUCCESS;
		default:
			std::cout << "[ graphicsBase ]\nFailed to queue the image for presentation!\nError code: " << std::hex << result << std::endl;
			return result;
		}
	}
	VkResult PresentImage(VkSemaphore semaphore_renderingIsOver__Or_ownershipIsTransfered = VK_NULL_HANDLE) {
		VkPresentInfoKHR presentInfo{};
		if (semaphore_renderingIsOver__Or_ownershipIsTransfered)
			presentInfo.waitSemaphoreCount = 1,
			presentInfo.pWaitSemaphores = &semaphore_renderingIsOver__Or_ownershipIsTransfered;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &currentImageIndex;
		return PresentImage(presentInfo);
	}
	/*if (queueFamilyIndex_graphics != queueFamilyIndex_presentation && swapchainCreateInfo.imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)*/
	void CmdReleaseImageOwnership_Graphics(VkCommandBuffer commandBuffer) {
		//The barrier must be executed by a graphics queue, after executing VkCmdEndRenderPass().
		VkImageMemoryBarrier imageMemoryBarrier_g2p_release{};
		imageMemoryBarrier_g2p_release.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier_g2p_release.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier_g2p_release.dstAccessMask;//Access mask is not necessary, if the corresponding stage is TOP/BOTTOM_OF_PIPE.
		//When a renderpass ends, the image layout is transitioned into its final layout, which is specified inside VkAttachmentDescription.
		imageMemoryBarrier_g2p_release.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier_g2p_release.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier_g2p_release.srcQueueFamilyIndex = queueFamilyIndex_graphics;
		imageMemoryBarrier_g2p_release.dstQueueFamilyIndex = queueFamilyIndex_presentation;
		imageMemoryBarrier_g2p_release.image = swapchainImages[currentImageIndex];
		imageMemoryBarrier_g2p_release.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
			0, nullptr, 0, nullptr, 1, &imageMemoryBarrier_g2p_release);
	}
	void CmdAcquireImageOwnership_Presentation(VkCommandBuffer commandBuffer) {
		//The barrier must be executed by a presentation queue, before presenting an image.
		//This function can be exactly the same as above.
		//Access masks are not necessary, waiting for a semaphore is enough.
		VkImageMemoryBarrier imageMemoryBarrier_g2p_acquire{};
		imageMemoryBarrier_g2p_acquire.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier_g2p_acquire.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier_g2p_acquire.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageMemoryBarrier_g2p_acquire.srcQueueFamilyIndex = queueFamilyIndex_graphics;
		imageMemoryBarrier_g2p_acquire.dstQueueFamilyIndex = queueFamilyIndex_presentation;
		imageMemoryBarrier_g2p_acquire.image = swapchainImages[currentImageIndex];
		imageMemoryBarrier_g2p_acquire.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
			0, nullptr, 0, nullptr, 1, &imageMemoryBarrier_g2p_acquire);
	}
	VkResult SubmitPresentationCommandBuffer(VkCommandBuffer commandBuffer,
		VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkSemaphore semaphore_ownershipIsTransfered = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE) {
		static VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		if (semaphore_renderingIsOver)
			submitInfo.waitSemaphoreCount = 1,
			submitInfo.pWaitSemaphores = &semaphore_renderingIsOver,
			submitInfo.pWaitDstStageMask = &waitDstStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		if (semaphore_ownershipIsTransfered)
			submitInfo.signalSemaphoreCount = 1,
			submitInfo.pSignalSemaphores = &semaphore_ownershipIsTransfered;
		VkResult result = vkQueueSubmit(queue_presentation, 1, &submitInfo, fence);
		if (result)
			std::cout << "[ graphicsBase ]\nFailed to submit the presentation command buffer!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	//Static Function
	static constexpr graphicsBase& Base() { return singleton; }
	static graphicsBasePlus& Plus() { return *pPlus; }
	static void Plus(graphicsBasePlus& plus) { if (!pPlus) pPlus = &plus; }
};
inline graphicsBase graphicsBase::singleton;
inline graphicsBasePlus* graphicsBase::pPlus;
constexpr const VkExtent2D& windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;

#pragma region Synchronization
//Done
class semaphore {
	VkSemaphore handle = VK_NULL_HANDLE;
public:
	semaphore() {
		if (graphicsBase::Base().Device()) {
			VkSemaphoreCreateInfo createInfo{};
			Create(createInfo);
		}
	}
	semaphore(VkSemaphoreCreateInfo& createInfo) {
		Create(createInfo);
	}
	semaphore(semaphore&& other) noexcept { MoveHandle; }
	~semaphore() {
		DestroyHandleBy(vkDestroySemaphore(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkSemaphore() const { return handle; }
	//Non-const Function
	VkResult Create(VkSemaphoreCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkResult result = vkCreateSemaphore(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ semaphore ]\nFailed to create a semaphore!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};
//Done
class fence {
	VkFence handle = VK_NULL_HANDLE;
public:
	fence(bool signaled = false) {
		if (graphicsBase::Base().Device()) {
			VkFenceCreateInfo createInfo{};
			createInfo.flags = signaled;
			Create(createInfo);
		}
	}
	fence(VkFenceCreateInfo& createInfo) {
		Create(createInfo);
	}
	fence(fence&& other) noexcept { MoveHandle; }
	~fence() {
		DestroyHandleBy(vkDestroyFence(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkFence() const { return handle; }
	//Const Function
	VkResult Wait() const {
		VkResult result = vkWaitForFences(graphicsBase::Base().Device(), 1, &handle, false, UINT64_MAX);
		if (result)
			std::cout << "[ fence ]\nFailed to wait for the fence!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult Reset() const {
		VkResult result = vkResetFences(graphicsBase::Base().Device(), 1, &handle);
		if (result)
			std::cout << "[ fence ]\nFailed to reset the fence!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult WaitAndReset() const {
		VkResult result = Wait();
		result || (result = Reset());
		return result;
	}
	VkResult Status() const {
		VkResult result = vkGetFenceStatus(graphicsBase::Base().Device(), handle);
		if (result < 0)
			std::cout << "[ fence ]\nFailed to get the status of the fence!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	//Non-const Function
	VkResult Create(VkFenceCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkResult result = vkCreateFence(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ fence ]\nFailed to create a fence!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};
#pragma endregion

#pragma region Memory
//Done
class deviceMemory {
	VkDeviceMemory handle = VK_NULL_HANDLE;
	bool isHostCoherent = false;
public:
	deviceMemory() = default;
	deviceMemory(VkMemoryAllocateInfo& allocateInfo) {
		Allocate(allocateInfo);
	}
	deviceMemory(deviceMemory&& other) noexcept { MoveHandle; }
	~deviceMemory() {
		DestroyHandleBy(vkFreeMemory(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkDeviceMemory() const { return handle; }
	//Const Function
	/*If the memory is host visible*/
	VkResult MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const {
		VkResult result = vkMapMemory(graphicsBase::Base().Device(), handle, offset, size, 0, &pData);
		if (result)
			std::cout << "[ deviceMemory ]\nFailed to map the memory!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const {
		VkResult result = VK_SUCCESS;
		if (!isHostCoherent) {
			VkMappedMemoryRange mappedMemoryRange{};
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedMemoryRange.memory = handle;
			mappedMemoryRange.offset = offset;
			mappedMemoryRange.size = size;
			if (result = vkFlushMappedMemoryRanges(graphicsBase::Base().Device(), 1, &mappedMemoryRange))
				std::cout << "[ deviceMemory ]\nWarning: Failed to flush the memory!\nError code: " << std::hex << result << std::endl;
		}
		vkUnmapMemory(graphicsBase::Base().Device(), handle);
		return result;
	}
	VkResult BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
		void* pData_dst;
		VkResult result = MapMemory(pData_dst, size, offset);
		if (result)
			return result;
		memcpy(pData_dst, pData_src, size_t(size));
		return UnmapMemory(size, offset);
	}
	template<typename T>
	VkResult BufferData(const T& data_src) const {
		return BufferData(&data_src, sizeof data_src);
	}
	VkResult RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) {
		void* pData_src;
		VkResult result = MapMemory(pData_src, size, offset);
		if (result)
			return result;
		memcpy(pData_dst, pData_src, size_t(size));
		return UnmapMemory(size, offset);
	}
	//Non-const Function
	VkResult Allocate(VkMemoryAllocateInfo& allocateInfo) {
		isHostCoherent =
			graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags &
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkResult result = vkAllocateMemory(graphicsBase::Base().Device(), &allocateInfo, nullptr, &handle);
		if (result)
			std::cout << "[ deviceMemory ]\nFailed to allocate memory!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	template<typename T> requires (!std::derived_from<T, deviceMemory>)
		int32_t Allocate(T& bufferOrImage, VkMemoryPropertyFlags desiredMemoryProperties) {
		VkMemoryAllocateInfo memoryAllocateInfo = bufferOrImage.MemoryAllocateInfo(desiredMemoryProperties);
		if (memoryAllocateInfo.memoryTypeIndex == -1)
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value.
		VkResult result = Allocate(memoryAllocateInfo);
		return result ?
			result :
			bufferOrImage.Attach(handle, 0);
	}
};

//Done
class buffer {
	VkBuffer handle = VK_NULL_HANDLE;
public:
	buffer() = default;
	buffer(VkBufferCreateInfo& createInfo) {
		Create(createInfo);
	}
	buffer(buffer&& other) noexcept { MoveHandle; }
	~buffer() {
		DestroyHandleBy(vkDestroyBuffer(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkBuffer() const { return handle; }
	//Const Function
	VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
		VkMemoryAllocateInfo memoryAllocateInfo{};
		VkMemoryRequirements memoryRequirements;
		//Get allocation size 
		vkGetBufferMemoryRequirements(graphicsBase::Base().Device(), handle, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		//Get memory type index 
		auto& physicalDeviceMemoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties();
		memoryAllocateInfo.memoryTypeIndex = -1;
		for (size_t j = 0; j < 2; j++) {
			for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
				if (memoryRequirements.memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
					memoryAllocateInfo.memoryTypeIndex = i;
					break;
				}
			//Highly possible that GPU and its driver may not support lazy allocation.
			if (memoryAllocateInfo.memoryTypeIndex == -1 &&
				desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				desiredMemoryProperties &= ~VkFlags(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
		}
		if (memoryAllocateInfo.memoryTypeIndex == -1)
			std::cout << "[ buffer ]\nFailed to find any memory type satisfies all desired memory properties!\n";
		return memoryAllocateInfo;
	}
	VkResult Attach(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
		VkResult result = vkBindBufferMemory(graphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
		if (result)
			std::cout << "[ buffer ]\nFailed to attach the memory!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	//Non-const Function
	VkResult Create(VkBufferCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		VkResult result = vkCreateBuffer(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ buffer ]\nFailed to create a buffer!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};

//Done
class bufferMemory :public buffer, public deviceMemory {
	using buffer::MemoryAllocateInfo;
	using buffer::Attach;
	using buffer::Create;
	using deviceMemory::Allocate;
public:
	bufferMemory() = default;
	bufferMemory(VkDeviceSize size, VkBufferUsageFlags desiredBufferUsages, VkMemoryPropertyFlags desiredMemoryProperties) {
		CreateAndAllocate(size, desiredBufferUsages, desiredMemoryProperties);
	}
	//Getter
	/*Implicit casting is invalid on 32-bit, because VkBuffer and VkDeviceMemory are both uint64_t*/
	VkBuffer Buffer() const { return buffer::operator VkBuffer(); }
	VkDeviceMemory Memory() const { return deviceMemory::operator VkDeviceMemory(); }
	//Non-const Function
	int32_t CreateAndAllocate(VkDeviceSize size, VkBufferUsageFlags desiredBufferUsages, VkMemoryPropertyFlags desiredMemoryProperties) {
		VkBufferCreateInfo createInfo{};
		createInfo.size = size;
		createInfo.usage = desiredBufferUsages;
		int32_t result = Create(createInfo);
		return result ?
			result :
			Allocate((buffer&)*this, desiredMemoryProperties);
	}
};

//Done
class bufferView {
	VkBufferView handle = VK_NULL_HANDLE;
public:
	bufferView() = default;
	bufferView(VkBufferViewCreateInfo& createInfo) {
		Create(createInfo);
	}
	bufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0) {
		Create(buffer, format, offset, range);
	}
	bufferView(bufferView&& other) noexcept { MoveHandle; }
	~bufferView() {
		DestroyHandleBy(vkDestroyBufferView(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkBufferView() const { return handle; }
	//Non-const Function
	VkResult Create(VkBufferViewCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		VkResult result = vkCreateBufferView(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ bufferView ]\nFailed to create a buffer view!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult Create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0) {
		VkBufferViewCreateInfo createInfo{};
		createInfo.buffer = buffer;
		createInfo.format = format;
		createInfo.offset = offset;
		createInfo.range = range;
		return Create(createInfo);
	}
};

//Done
class image {
	VkImage handle = VK_NULL_HANDLE;
public:
	image() = default;
	image(VkImageCreateInfo& createInfo) {
		Create(createInfo);
	}
	image(image&& other) noexcept { MoveHandle; }
	~image() {
		DestroyHandleBy(vkDestroyImage(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkImage() const { return handle; }
	//Const Function
	VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties)const {
		VkMemoryAllocateInfo memoryAllocateInfo{};
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(graphicsBase::Base().Device(), handle, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		auto& physicalDeviceMemoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties();
		memoryAllocateInfo.memoryTypeIndex = -1;
		for (size_t j = 0; j < 2; j++) {
			for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
				if (memoryRequirements.memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
					memoryAllocateInfo.memoryTypeIndex = i;
					break;
				}
			if (memoryAllocateInfo.memoryTypeIndex == -1 &&
				desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
				desiredMemoryProperties &= ~VkFlags(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
		}
		if (memoryAllocateInfo.memoryTypeIndex == -1)
			std::cout << "[ image ]\nFailed to find any memory type satisfies all desired memory properties!\n";
		return memoryAllocateInfo;
	}
	VkResult Attach(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
		VkResult result = vkBindImageMemory(graphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
		if (result)
			std::cout << "[ image ]\nFailed to attach the memory!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	//Non-const Function
	VkResult Create(VkImageCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		VkResult result = vkCreateImage(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ image ]\nFailed to create an image!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};

//Done
class imageMemory :public image, public deviceMemory {
	using image::MemoryAllocateInfo;
	using image::Attach;
	using image::Create;
	using deviceMemory::Allocate;
public:
	imageMemory() = default;
	imageMemory(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkSampleCountFlagBits sampleCount,
		VkImageUsageFlags desiredImageUsages, VkMemoryPropertyFlags desiredMemoryProperties, VkImageCreateFlags flags = 0) {
		CreateAndAllocate(imageType, format, extent, mipLevelCount, arrayLayerCount, sampleCount, desiredImageUsages, desiredMemoryProperties, flags);
	}
	//Getter
	VkImage Image() const { return image::operator VkImage(); }
	VkDeviceMemory Memory() const { return deviceMemory::operator VkDeviceMemory(); }
	//Non-const Function
	int32_t CreateAndAllocate(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t arrayLayerCount, VkSampleCountFlagBits sampleCount,
		VkImageUsageFlags desiredImageUsages, VkMemoryPropertyFlags desiredMemoryProperties, VkImageCreateFlags flags = 0) {
		VkImageCreateInfo createInfo{};
		createInfo.flags = flags;
		createInfo.imageType = imageType;
		createInfo.format = format;
		createInfo.extent = extent;
		createInfo.mipLevels = mipLevelCount;
		createInfo.arrayLayers = arrayLayerCount;
		createInfo.samples = sampleCount;
		createInfo.usage = desiredImageUsages;
		int32_t result = Create(createInfo);
		return result ?
			result :
			Allocate((image&)*this, desiredMemoryProperties);
	}
};

//Done
class imageView {
	VkImageView handle = VK_NULL_HANDLE;
public:
	imageView() = default;
	imageView(VkImageViewCreateInfo& createInfo) {
		Create(createInfo);
	}
	imageView(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange) {
		Create(image, viewType, format, subresourceRange);
	}
	imageView(imageView&& other) noexcept { MoveHandle; }
	~imageView() {
		DestroyHandleBy(vkDestroyImageView(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkImageView() const { return handle; }
	//Non-const Function
	VkResult Create(VkImageViewCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		VkResult result = vkCreateImageView(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ imageView ]\nFailed to create an image view!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange) {
		VkImageViewCreateInfo createInfo{};
		createInfo.image = image;
		createInfo.viewType = viewType;
		createInfo.format = format;
		createInfo.subresourceRange = subresourceRange;
		return Create(createInfo);
	}
};
#pragma endregion

#pragma region Pipeline
//Done
class shader {
	VkShaderModule handle = VK_NULL_HANDLE;
public:
	shader() = default;
	shader(const char* filepath) {
		Create(filepath);
	}
	shader(shader&& other) noexcept { MoveHandle; }
	~shader() {
		DestroyHandleBy(vkDestroyShaderModule(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkShaderModule() const { return handle; }
	//Const Function
	VkPipelineShaderStageCreateInfo StageCreateInfo(VkShaderStageFlagBits stage, const char* entry = "main") const {
		return VkPipelineShaderStageCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,//sType;
			nullptr,											//pNext;
			0,													//flags;
			stage,												//stage;
			handle,												//module;
			entry,												//pName;
			nullptr												//pSpecializationInfo;
		};
	}
	//Non-const Function
	VkResult Create(const char* filepath) {
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if (!file)
			std::cout << "[ shader ]\nFailed to load the shader file: " << filepath << std::endl;
		size_t fileSize = size_t(file.tellg());
		std::vector<char> binaries(fileSize);
		file.seekg(0);
		file.read(binaries.data(), fileSize);
		file.close();
		VkShaderModuleCreateInfo shaderModuleCreateInfo{};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = binaries.size();
		shaderModuleCreateInfo.pCode = (uint32_t*)binaries.data();
		VkResult result = vkCreateShaderModule(graphicsBase::Base().Device(), &shaderModuleCreateInfo, nullptr, &handle);
		if (result)
			std::cout << "[ shader ]\nFailed to create a shader module!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};

//Done
class sampler {
	VkSampler handle = VK_NULL_HANDLE;
public:
	sampler() = default;
	sampler(sampler&& other) noexcept { MoveHandle; }
	sampler(VkSamplerCreateInfo& createInfo) {
		Create(createInfo);
	}
	~sampler() {
		DestroyHandleBy(vkDestroySampler(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkSampler() const { return handle; }
	//Non-const Function
	VkResult Create(VkSamplerCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		VkResult result = vkCreateSampler(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ sampler ]\nFailed to create a sampler!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};

//Done
class descriptorSetLayout {
	VkDescriptorSetLayout handle = VK_NULL_HANDLE;
public:
	descriptorSetLayout() = default;
	descriptorSetLayout(VkDescriptorSetLayoutCreateInfo& createInfo) {
		Create(createInfo);
	}
	descriptorSetLayout(descriptorSetLayout&& other) noexcept { MoveHandle; }
	~descriptorSetLayout() {
		DestroyHandleBy(vkDestroyDescriptorSetLayout(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkDescriptorSetLayout() const { return handle; }
	//Non-const Function
	VkResult Create(VkDescriptorSetLayoutCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		VkResult result = vkCreateDescriptorSetLayout(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ descriptorSetLayout ]\nFailed to create a descriptor set layout!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};
//Done
class pipelineLayout {
	VkPipelineLayout handle = VK_NULL_HANDLE;
public:
	pipelineLayout() = default;
	pipelineLayout(VkPipelineLayoutCreateInfo& createInfo) {
		Create(createInfo);
	}
	pipelineLayout(pipelineLayout&& other) noexcept { MoveHandle; }
	~pipelineLayout() {
		DestroyHandleBy(vkDestroyPipelineLayout(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkPipelineLayout() const { return handle; }
	//Non-const Function
	VkResult Create(VkPipelineLayoutCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkResult result = vkCreatePipelineLayout(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ pipelineLayout ]\nFailed to create a pipeline layout!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};

//Done
class pipeline {
	VkPipeline handle = VK_NULL_HANDLE;
public:
	pipeline() = default;
	pipeline(VkGraphicsPipelineCreateInfo& createInfo) {
		Create(createInfo);
	}
	pipeline(pipeline&& other) noexcept { MoveHandle; }
	~pipeline() {
		DestroyHandleBy(vkDestroyPipeline(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkPipeline() const { return handle; }
	//Non-const Function
	VkResult Create(VkGraphicsPipelineCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		VkResult result = vkCreateGraphicsPipelines(graphicsBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ pipeline ]\nFailed to create a graphics pipeline!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};
#pragma endregion

#pragma region RenderPass & Framebuffer
//Done
class renderPass {
	VkRenderPass handle = VK_NULL_HANDLE;
public:
	renderPass() = default;
	renderPass(VkRenderPassCreateInfo& createInfo) {
		Create(createInfo);
	}
	renderPass(renderPass&& other) noexcept { MoveHandle; }
	~renderPass() {
		DestroyHandleBy(vkDestroyRenderPass(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkRenderPass() const { return handle; }
	//Const Function
	void CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = handle;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
	}
	void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = handle;
		beginInfo.framebuffer = framebuffer;
		beginInfo.renderArea = { {}, windowSize };
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
	}
	void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkClearValue clearValue, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = handle;
		beginInfo.framebuffer = framebuffer;
		beginInfo.renderArea = { {}, windowSize };
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearValue;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
	}
	template<size_t elementCount>
	void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, const VkClearValue(&clearValues)[elementCount], VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
		VkRenderPassBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = handle;
		beginInfo.framebuffer = framebuffer;
		beginInfo.renderArea = { {}, windowSize };
		beginInfo.clearValueCount = elementCount;
		beginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
	}
	void CmdEnd(VkCommandBuffer commandBuffer) const {
		vkCmdEndRenderPass(commandBuffer);
	}
	//Non-const Function
	VkResult Create(VkRenderPassCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		VkResult result = vkCreateRenderPass(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ renderPass ]\nFailed to create a render pass!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};

//Done
class framebuffer {
	VkFramebuffer handle = VK_NULL_HANDLE;
public:
	framebuffer() = default;
	framebuffer(VkFramebufferCreateInfo& createInfo) {
		Create(createInfo);
	}
	framebuffer(framebuffer&& other) noexcept { MoveHandle; }
	~framebuffer() {
		DestroyHandleBy(vkDestroyFramebuffer(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkFramebuffer() const { return handle; }
	//Non-const Function
	VkResult Create(VkFramebufferCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		VkResult result = vkCreateFramebuffer(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ framebuffer ]\nFailed to create a framebuffer!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};
#pragma endregion

#pragma region Pool
//Done
class commandPool {
	VkCommandPool handle = VK_NULL_HANDLE;
public:
	commandPool() = default;
	commandPool(VkCommandPoolCreateInfo& createInfo) {
		Create(createInfo);
	}
	commandPool(VkCommandPoolCreateFlags createFlags, uint32_t queueFamilyIndex) {
		Create(createFlags, queueFamilyIndex);
	}
	commandPool(commandPool&& other) noexcept { MoveHandle; }
	~commandPool() {
		DestroyHandleBy(vkDestroyCommandPool(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkCommandPool() const { return handle; }
	//Const Function
	VkResult AllocateCommandBuffers(VkCommandBuffer* pCommandBuffers, size_t commandBufferCount = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = handle;
		commandBufferAllocateInfo.level = level;
		commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
		VkResult result = vkAllocateCommandBuffers(graphicsBase::Base().Device(), &commandBufferAllocateInfo, pCommandBuffers);
		if (result)
			std::cout << "[ commandPool ]\nFailed to allocate command buffers!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	void FreeCommandBuffers(VkCommandBuffer* pCommandBuffers, size_t commandBufferCount = 1) const {
		vkFreeCommandBuffers(graphicsBase::Base().Device(), handle, commandBufferCount, pCommandBuffers);
		memset(pCommandBuffers, 0, commandBufferCount * sizeof VkCommandBuffer);
	}
	//Non-const Function
	VkResult Create(VkCommandPoolCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		VkResult result = vkCreateCommandPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ commandPool ]\nFailed to create a command pool!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult Create(VkCommandPoolCreateFlags createFlags, uint32_t queueFamilyIndex) {
		VkCommandPoolCreateInfo createInfo{};
		createInfo.flags = createFlags;
		createInfo.queueFamilyIndex = queueFamilyIndex;
		return Create(createInfo);
	}
};
class commandBuffer {
	VkCommandBuffer handle = VK_NULL_HANDLE;
public:
	//Getter
	operator VkCommandBuffer() const { return handle; }
	//Const Function
	VkResult Begin(VkCommandBufferUsageFlags usageFlags = 0, const VkCommandBufferInheritanceInfo& inheritanceInfo = *(VkCommandBufferInheritanceInfo*)nullptr) const {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = usageFlags;
		beginInfo.pInheritanceInfo = &inheritanceInfo;
		VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
		if (result)
			std::cout << "[ commandBuffer ]\nFailed to begin a command buffer!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult End() const {
		VkResult result = vkEndCommandBuffer(handle);
		if (result)
			std::cout << "[ commandBuffer ]\nFailed to end a command buffer!\nError code: " << std::hex << result << std::endl;
		return result;
	}
};

//Done
class descriptorPool {
	VkDescriptorPool handle = VK_NULL_HANDLE;
public:
	descriptorPool() = default;
	descriptorPool(VkDescriptorPoolCreateInfo& createInfo) {
		Create(createInfo);
	}
	descriptorPool(VkDescriptorPoolCreateFlags createFlags, uint32_t maxSetCount, VkDescriptorPoolSize descriptorPoolSize) {
		Create(createFlags, maxSetCount, descriptorPoolSize);
	}
	template<size_t elementCount>
	descriptorPool(VkDescriptorPoolCreateFlags createFlags, uint32_t maxSetCount, const VkDescriptorPoolSize(&descriptorPoolSizes)[elementCount]) {
		Create(createFlags, maxSetCount, descriptorPoolSizes);
	}
	descriptorPool(descriptorPool&& other) noexcept { MoveHandle; }
	~descriptorPool() {
		DestroyHandleBy(vkDestroyDescriptorPool(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkDescriptorPool() const { return handle; }
	//Const Function
	VkResult AllocateDescriptorSets(VkDescriptorSet* pDescriptorSets, const VkDescriptorSetLayout* pDescriptorSetLayouts, size_t descriptorSetCount = 1) const {
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = handle;
		descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
		descriptorSetAllocateInfo.pSetLayouts = pDescriptorSetLayouts;
		VkResult result = vkAllocateDescriptorSets(graphicsBase::Base().Device(), &descriptorSetAllocateInfo, pDescriptorSets);
		if (result)
			std::cout << "[ descriptorPool ]\nFailed to allocate descriptor sets!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult AllocateDescriptorSet(VkDescriptorSet& descriptorSet, VkDescriptorSetLayout descriptorSetLayout) const {
		return AllocateDescriptorSets(&descriptorSet, &descriptorSetLayout);
	}
	void FreeDescriptorSets(VkDescriptorSet* pDescriptorSets, size_t descriptorSetCount = 1) const {
		vkFreeDescriptorSets(graphicsBase::Base().Device(), handle, descriptorSetCount, pDescriptorSets);
		memset(pDescriptorSets, 0, descriptorSetCount * sizeof VkDescriptorSet);
	}
	//Non-const Function
	VkResult Create(VkDescriptorPoolCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		VkResult result = vkCreateDescriptorPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ descriptorPool ]\nFailed to create a descriptor pool!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult Create(VkDescriptorPoolCreateFlags createFlags, uint32_t maxSetCount, VkDescriptorPoolSize descriptorPoolSize) {
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.flags = createFlags;
		createInfo.maxSets = maxSetCount;
		createInfo.poolSizeCount = 1;
		createInfo.pPoolSizes = &descriptorPoolSize;
		return Create(createInfo);
	}
	template<size_t elementCount>
	VkResult Create(VkDescriptorPoolCreateFlags createFlags, uint32_t maxSetCount, const VkDescriptorPoolSize(&descriptorPoolSizes)[elementCount]) {
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.flags = createFlags;
		createInfo.maxSets = maxSetCount;
		createInfo.poolSizeCount = elementCount;
		createInfo.pPoolSizes = descriptorPoolSizes;
		return Create(createInfo);
	}
};
class descriptorSet {
	VkDescriptorSet handle = VK_NULL_HANDLE;
public:
	//Getter
	operator VkDescriptorSet() const { return handle; }
	//Const Function
	void Write(VkDescriptorType descriptorType, const void* pDescriptorInfo, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0, uint32_t count = 1) const {
		VkWriteDescriptorSet writeDescriptorSet = {
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		handle,
		dstBinding,
		dstArrayElement,
		count,
		descriptorType,
		(VkDescriptorImageInfo*)pDescriptorInfo,
		(VkDescriptorBufferInfo*)pDescriptorInfo,
		(VkBufferView*)pDescriptorInfo
		};
		vkUpdateDescriptorSets(graphicsBase::Base().Device(), 1, &writeDescriptorSet, 0, nullptr);
	}
};

class queryPool {
	VkQueryPool handle = VK_NULL_HANDLE;
public:
	queryPool() = default;
	queryPool(VkQueryPoolCreateInfo& createInfo) {
		Create(createInfo);
	}
	queryPool(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics) {
		Create(queryType, queryCount, pipelineStatistics);
	}
	queryPool(queryPool&& other) noexcept { MoveHandle; }
	~queryPool() {
		DestroyHandleBy(vkDestroyQueryPool(graphicsBase::Base().Device(), handle, nullptr));
	}
	//Getter
	operator VkQueryPool() const { return handle; }
	//Non-const Function
	VkResult Create(VkQueryPoolCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		VkResult result = vkCreateQueryPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			std::cout << "[ queryPool ]\nFailed to create a query pool!\nError code: " << std::hex << result << std::endl;
		return result;
	}
	VkResult Create(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics) {
		VkQueryPoolCreateInfo createInfo{};
		createInfo.queryType = queryType;
		createInfo.queryCount = queryCount;
		createInfo.pipelineStatistics = pipelineStatistics;
		return Create(createInfo);
	}
};
#pragma endregion
NAMESPACE_END