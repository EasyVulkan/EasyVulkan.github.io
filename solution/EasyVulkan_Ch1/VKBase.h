#include "EasyVKStart.h"

namespace vulkan {
	constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

	class graphicsBase {
		uint32_t apiVersion = VK_API_VERSION_1_0;
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		std::vector<VkPhysicalDevice> availablePhysicalDevices;

		VkDevice device;
		uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
		VkQueue queue_graphics;
		VkQueue queue_presentation;
		VkQueue queue_compute;

		VkSurfaceKHR surface;
		std::vector <VkSurfaceFormatKHR> availableSurfaceFormats;

		VkSwapchainKHR swapchain;
		std::vector <VkImage> swapchainImages;
		std::vector <VkImageView> swapchainImageViews;
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

		std::vector<const char*> instanceLayers;
		std::vector<const char*> instanceExtensions;
		std::vector<const char*> deviceExtensions;

		VkDebugUtilsMessengerEXT debugUtilsMessenger;

		std::vector<void(*)()> callbacks_createSwapchain;
		std::vector<void(*)()> callbacks_destroySwapchain;
		std::vector<void(*)()> callbacks_createDevice;
		std::vector<void(*)()> callbacks_destroyDevice;
		//Static
		static graphicsBase singleton;
		//--------------------
		graphicsBase() = default;
		graphicsBase(graphicsBase&&) = delete;
		~graphicsBase() {
			if (!instance)
				return;
			if (device) {
				WaitIdle();
				if (swapchain) {
					for (auto& i : callbacks_destroySwapchain)
						i();
					for (auto& i : swapchainImageViews)
						if (i)
							vkDestroyImageView(device, i, nullptr);
					vkDestroySwapchainKHR(device, swapchain, nullptr);
				}
				for (auto& i : callbacks_destroyDevice)
					i();
				vkDestroyDevice(device, nullptr);
			}
			if (surface)
				vkDestroySurfaceKHR(instance, surface, nullptr);
			if (debugUtilsMessenger) {
				PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessenger =
					reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
				if (DestroyDebugUtilsMessenger)
					DestroyDebugUtilsMessenger(instance, debugUtilsMessenger, nullptr);
			}
			vkDestroyInstance(instance, nullptr);
		}
		//Non-const Function
		VkResult CreateSwapchain_Internal() {
			if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
				return result;
			}

			uint32_t swapchainImageCount;
			if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
				return result;
			}
			swapchainImages.resize(swapchainImageCount);
			if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data())) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
				return result;
			}

			swapchainImageViews.resize(swapchainImageCount);
			VkImageViewCreateInfo imageViewCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = swapchainCreateInfo.imageFormat,
				//.components = {},
				.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
			};
			for (size_t i = 0; i < swapchainImageCount; i++) {
				imageViewCreateInfo.image = swapchainImages[i];
				if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i])) {
					std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
					return result;
				}
			}
			return VK_SUCCESS;
		}
		VkResult GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
			if (!queueFamilyCount)
				return VK_RESULT_MAX_ENUM;
			std::vector<VkQueueFamilyProperties> queueFamilyPropertieses(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertieses.data());
			auto& [ig, ip, ic] = queueFamilyIndices;
			ig = ip = ic = VK_QUEUE_FAMILY_IGNORED;
			for (uint32_t i = 0; i < queueFamilyCount; i++) {
				VkBool32
					supportGraphics = enableGraphicsQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_GRAPHICS_BIT,
					supportPresentation = false,
					supportCompute = enableComputeQueue && queueFamilyPropertieses[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
				if (surface)
					if (VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportPresentation)) {
						std::cout << std::format("[ graphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
						return result;
					}
				if (supportGraphics && supportCompute) {
					if (supportPresentation) {
						ig = ip = ic = i;
						break;
					}
					if (ig != ic ||
						ig == VK_QUEUE_FAMILY_IGNORED)
						ig = ic = i;
					if (!surface)
						break;
				}
				if (supportGraphics &&
					ig == VK_QUEUE_FAMILY_IGNORED)
					ig = i;
				if (supportPresentation &&
					ip == VK_QUEUE_FAMILY_IGNORED)
					ip = i;
				if (supportCompute &&
					ic == VK_QUEUE_FAMILY_IGNORED)
					ic = i;
			}
			if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
				ip == VK_QUEUE_FAMILY_IGNORED && surface ||
				ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue)
				return VK_RESULT_MAX_ENUM;
			queueFamilyIndex_graphics = ig;
			queueFamilyIndex_presentation = ip;
			queueFamilyIndex_compute = ic;
			return VK_SUCCESS;
		}
		VkResult CreateDebugMessenger() {
			static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageTypes,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData)->VkBool32 {
					std::cout << std::format("{}\n\n", pCallbackData->pMessage);
					return VK_FALSE;
			};
			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
				.pfnUserCallback = DebugUtilsMessengerCallback
			};
			PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessenger =
				reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			if (CreateDebugUtilsMessenger) {
				VkResult result = CreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugUtilsMessenger);
				if (result)
					std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: {}\n", int32_t(result));
				return result;
			}
			std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n");
			return VK_RESULT_MAX_ENUM;
		}
	public:
		//Getter
		uint32_t ApiVersion() const {
			return apiVersion;
		}
		VkInstance Instance() const {
			return instance;
		}
		VkPhysicalDevice PhysicalDevice() const {
			return physicalDevice;
		}
		const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const {
			return physicalDeviceProperties;
		}
		const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const {
			return physicalDeviceMemoryProperties;
		}
		VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const {
			return availablePhysicalDevices[index];
		}
		uint32_t AvailablePhysicalDeviceCount() const {
			return uint32_t(availablePhysicalDevices.size());
		}

		VkDevice Device() const {
			return device;
		}
		uint32_t QueueFamilyIndex_Graphics() const {
			return queueFamilyIndex_graphics;
		}
		uint32_t QueueFamilyIndex_Presentation() const {
			return queueFamilyIndex_presentation;
		}
		uint32_t QueueFamilyIndex_Compute() const {
			return queueFamilyIndex_compute;
		}
		VkQueue Queue_Graphics() const {
			return queue_graphics;
		}
		VkQueue Queue_Presentation() const {
			return queue_presentation;
		}
		VkQueue Queue_Compute() const {
			return queue_compute;
		}

		VkSurfaceKHR Surface() const {
			return surface;
		}
		const VkFormat& AvailableSurfaceFormat(uint32_t index) const {
			return availableSurfaceFormats[index].format;
		}
		const VkColorSpaceKHR& AvailableSurfaceColorSpace(uint32_t index) const {
			return availableSurfaceFormats[index].colorSpace;
		}
		uint32_t AvailableSurfaceFormatCount() const {
			return uint32_t(availableSurfaceFormats.size());
		}

		VkSwapchainKHR Swapchain() const {
			return swapchain;
		}
		VkImage SwapchainImage(uint32_t index) const {
			return swapchainImages[index];
		}
		VkImageView SwapchainImageView(uint32_t index) const {
			return swapchainImageViews[index];
		}
		uint32_t SwapchainImageCount() const {
			return uint32_t(swapchainImages.size());
		}
		const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const {
			return swapchainCreateInfo;
		}

		const std::vector<const char*>& InstanceLayers() const {
			return instanceLayers;
		}
		const std::vector<const char*>& InstanceExtensions() const {
			return instanceExtensions;
		}
		const std::vector<const char*>& DeviceExtensions() const {
			return deviceExtensions;
		}

		//Const Function
		VkResult WaitIdle() const {
			VkResult result = vkDeviceWaitIdle(device);
			if (result)
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
			return result;
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
		void PushCallback_DestroyDevice(void(*function)()) {
			callbacks_destroyDevice.push_back(function);
		}
		//                    Create Instance
		void PushInstanceLayer(const char* layerName) {
			instanceLayers.push_back(layerName);
		}
		void PushInstanceExtension(const char* extensionName) {
			instanceExtensions.push_back(extensionName);
		}
		VkResult UseLatestApiVersion() {
			if (vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"))
				return vkEnumerateInstanceVersion(&apiVersion);
			return VK_SUCCESS;
		}
		VkResult CreateInstance(const void* pNext = nullptr, VkInstanceCreateFlags flags = 0) {
#ifndef NDEBUG
			static constexpr const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			static constexpr const char* debugUtilsExtensionName = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
			if (instanceLayers.size() && instanceLayers.back() != validationLayerName)
				instanceLayers.push_back(validationLayerName);
			if (instanceExtensions.size() && instanceExtensions.back() != validationLayerName)
				instanceExtensions.push_back(debugUtilsExtensionName);
#endif
			VkApplicationInfo applicatianInfo = {
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.apiVersion = apiVersion
			};
			VkInstanceCreateInfo instanceCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pNext = pNext,
				.flags = flags,
				.pApplicationInfo = &applicatianInfo,
				.enabledLayerCount = uint32_t(instanceLayers.size()),
				.ppEnabledLayerNames = instanceLayers.data(),
				.enabledExtensionCount = uint32_t(instanceExtensions.size()),
				.ppEnabledExtensionNames = instanceExtensions.data()
			};
			if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan instance!\nError code: {}\n", int32_t(result));
				return result;
			}
			std::cout << std::format(
				"Vulkan API Version: {}.{}.{}\n",
				VK_VERSION_MAJOR(apiVersion),
				VK_VERSION_MINOR(apiVersion),
				VK_VERSION_PATCH(apiVersion));
#ifndef NDEBUG
			CreateDebugMessenger();
#endif
			return VK_SUCCESS;
		}
		VkResult CheckInstanceLayers(std::span<const char*> layersToCheck) const {
			uint32_t layerCount;
			std::vector<VkLayerProperties> availableLayers;
			if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance layers!\n");
				return result;
			}
			if (layerCount) {
				availableLayers.resize(layerCount);
				if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) {
					std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: {}\n", int32_t(result));
					return result;
				}
				for (auto& i : layersToCheck) {
					bool found = false;
					for (auto& j : availableLayers)
						if (!strcmp(i, j.layerName)) {
							found = true;
							break;
						}
					if (!found)
						i = nullptr;
				}
			}
			else
				for (auto& i : layersToCheck)
					i = nullptr;
			return VK_SUCCESS;
		}
		VkResult CheckInstanceExtensions(std::span<const char*> extensionsToCheck, const char* layerName) const {
			uint32_t extensionCount;
			std::vector<VkExtensionProperties> availableExtensions;
			if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr)) {
				layerName ?
					std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:{}\n", layerName) :
					std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\n");
				return result;
			}
			if (extensionCount) {
				availableExtensions.resize(extensionCount);
				if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data())) {
					std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: {}\n", int32_t(result));
					return result;
				}
				for (auto& i : extensionsToCheck) {
					bool found = false;
					for (auto& j : availableExtensions)
						if (!strcmp(i, j.extensionName)) {
							found = true;
							break;
						}
					if (!found)
						i = nullptr;
				}
			}
			else
				for (auto& i : extensionsToCheck)
					i = nullptr;
			return VK_SUCCESS;
		}
		void InstanceLayers(const std::vector<const char*>& layerNames) {
			instanceLayers = layerNames;
		}
		void InstanceExtensions(const std::vector<const char*>& extensionNames) {
			instanceExtensions = extensionNames;
		}
		//                    Set Window Surface
		void Surface(VkSurfaceKHR surface) {
			if (!this->surface)
				this->surface = surface;
		}
		//                    Create Logical Device
		void PushDeviceExtension(const char* extensionName) {
			deviceExtensions.push_back(extensionName);
		}
		VkResult GetPhysicalDevices() {
			uint32_t deviceCount;
			if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of physical devices!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (!deviceCount)
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any physical device supports vulkan!\n"),
				abort();
			availablePhysicalDevices.resize(deviceCount);
			VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());
			if (result)
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true) {
			static constexpr uint32_t notFound = INT32_MAX;//== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
			struct queueFamilyIndexCombination {
				uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
				uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
				uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
			};
			static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
			auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];
			if (ig == notFound && enableGraphicsQueue ||
				ip == notFound && surface ||
				ic == notFound && enableComputeQueue)
				return VK_RESULT_MAX_ENUM;
			if (ig == VK_QUEUE_FAMILY_IGNORED && enableGraphicsQueue ||
				ip == VK_QUEUE_FAMILY_IGNORED && surface ||
				ic == VK_QUEUE_FAMILY_IGNORED && enableComputeQueue) {
				uint32_t indices[3];
				VkResult result = GetQueueFamilyIndices(availablePhysicalDevices[deviceIndex], enableGraphicsQueue, enableComputeQueue, indices);
				if (result == VK_SUCCESS ||
					result == VK_RESULT_MAX_ENUM) {
					if (enableGraphicsQueue)
						ig = indices[0] & INT32_MAX;
					if (surface)
						ip = indices[1] & INT32_MAX;
					if (enableComputeQueue)
						ic = indices[2] & INT32_MAX;
				}
				if (result)
					return result;
			}
			else {
				queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
				queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
				queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
			}
			physicalDevice = availablePhysicalDevices[deviceIndex];
			return VK_SUCCESS;
		}
		VkResult CreateLogicalDevice(const void* pNext = nullptr, VkDeviceCreateFlags flags = 0) {
			float queuePriority = 1.f;
			VkDeviceQueueCreateInfo queueCreateInfos[3] = {
				{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueCount = 1,
					.pQueuePriorities = &queuePriority },
				{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueCount = 1,
					.pQueuePriorities = &queuePriority },
				{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueCount = 1,
					.pQueuePriorities = &queuePriority } };
			uint32_t queueCreateInfoCount = 0;
			if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
				queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_graphics;
			if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED &&
				queueFamilyIndex_presentation != queueFamilyIndex_graphics)
				queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_presentation;
			if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED &&
				queueFamilyIndex_compute != queueFamilyIndex_graphics &&
				queueFamilyIndex_compute != queueFamilyIndex_presentation)
				queueCreateInfos[queueCreateInfoCount++].queueFamilyIndex = queueFamilyIndex_compute;
			VkPhysicalDeviceFeatures physicalDeviceFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);
			VkDeviceCreateInfo deviceCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = pNext,
				.flags = flags,
				.queueCreateInfoCount = queueCreateInfoCount,
				.pQueueCreateInfos = queueCreateInfos,
				.enabledExtensionCount = uint32_t(deviceExtensions.size()),
				.ppEnabledExtensionNames = deviceExtensions.data(),
				.pEnabledFeatures = &physicalDeviceFeatures
			};
			if (VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to create a vulkan logical device!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
				vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
			if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED)
				vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
			if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED)
				vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
			std::cout << std::format("Renderer: {}\n", physicalDeviceProperties.deviceName);
			return VK_SUCCESS;
		}
		VkResult CheckDeviceExtensions(std::span<const char*> extensionsToCheck, const char* layerName = nullptr) const {
		}
		void DeviceExtensions(const std::vector<const char*>& extensionNames) {
			deviceExtensions = extensionNames;
		}
		//                    Create Swapchain
		VkResult GetSurfaceFormats() {
			uint32_t surfaceFormatCount;
			if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface formats!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (!surfaceFormatCount)
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any supported surface format!\n"),
				abort();
			availableSurfaceFormats.resize(surfaceFormatCount);
			VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
			if (result)
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
			return result;
		}
		VkResult SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
			bool formatIsAvailable = false;
			if (!surfaceFormat.format) {
				for (auto& i : availableSurfaceFormats)
					if (i.colorSpace == surfaceFormat.colorSpace) {
						swapchainCreateInfo.imageFormat = surfaceFormat.format;
						swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
						formatIsAvailable = true;
						break;
					}
			}
			else
				for (auto& i : availableSurfaceFormats)
					if (i.format == surfaceFormat.format &&
						i.colorSpace == surfaceFormat.colorSpace) {
						swapchainCreateInfo.imageFormat = surfaceFormat.format;
						swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
						formatIsAvailable = true;
						break;
					}
			if (!formatIsAvailable)
				return VK_ERROR_FORMAT_NOT_SUPPORTED;
			if (swapchain)
				return RecreateSwapchain();
			return VK_SUCCESS;
		}
		VkResult CreateSwapchain(bool limitFrameRate = true, const void* pNext = nullptr, VkSwapchainCreateFlagsKHR flags = 0) {
			//Get surface capabilities
			VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
			if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
				return result;
			}
			//Set image count
			swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
			//Set image extent
			swapchainCreateInfo.imageExtent =
				surfaceCapabilities.currentExtent.width == -1 ?
				VkExtent2D{
				glm::clamp(defaultWindowSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
				glm::clamp(defaultWindowSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height) } :
				surfaceCapabilities.currentExtent;
			//Set transformation
			swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
			//Set alpha compositing mode
			if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
				swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
			else
				for (size_t i = 0; i < 4; i++)
					if (surfaceCapabilities.supportedCompositeAlpha & 1 << i) {
						swapchainCreateInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR(surfaceCapabilities.supportedCompositeAlpha & 1 << i);
						break;
					}
			//Set image usage
			swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
				swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
				swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			else
				std::cout << std::format("[ graphicsBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT isn't supported!\n");

			//Get surface formats
			if (availableSurfaceFormats.empty())
				if (VkResult result = GetSurfaceFormats())
					return result;
			//If surface format is not determined, select a a four-component UNORM format
			if (!swapchainCreateInfo.imageFormat)
				if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
					SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })) {
					swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
					swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
					std::cout << std::format("[ graphicsBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
				}

			//Get surface present modes
			uint32_t surfacePresentModeCount;
			if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (!surfacePresentModeCount)
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to find any surface present mode!\n"),
				abort();
			std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
			if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data())) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
				return result;
			}
			//Set present mode to mailbox if available and necessary
			swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			if (!limitFrameRate)
				for (size_t i = 0; i < surfacePresentModeCount; i++)
					if (surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
						swapchainCreateInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
						break;
					}

			swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapchainCreateInfo.pNext = pNext;
			swapchainCreateInfo.flags = flags;
			swapchainCreateInfo.surface = surface;
			swapchainCreateInfo.imageArrayLayers = 1;
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchainCreateInfo.clipped = VK_TRUE;

			if (VkResult result = CreateSwapchain_Internal())
				return result;
			for (auto& i : callbacks_createSwapchain)
				i();
			return VK_SUCCESS;
		}

		//                    After initialization
		void Terminate() {
			this->~graphicsBase();
			instance = VK_NULL_HANDLE;
			physicalDevice = VK_NULL_HANDLE;
			device = VK_NULL_HANDLE;
			surface = VK_NULL_HANDLE;
			swapchain = VK_NULL_HANDLE;
			swapchainImages.resize(0);
			swapchainImageViews.resize(0);
			swapchainCreateInfo = {};
			debugUtilsMessenger = VK_NULL_HANDLE;
		}
		VkResult RecreateDevice(const void* pNext = nullptr, VkDeviceCreateFlags flags = 0) {
			if (VkResult result = WaitIdle())
				return result;
			if (swapchain) {
				for (auto& i : callbacks_destroySwapchain)
					i();
				for (auto& i : swapchainImageViews)
					if (i)
						vkDestroyImageView(device, i, nullptr);
				swapchainImageViews.resize(0);
				vkDestroySwapchainKHR(device, swapchain, nullptr);
				swapchain = VK_NULL_HANDLE;
				swapchainCreateInfo = {};
			}
			for (auto& i : callbacks_destroyDevice)
				i();
			if (device)
				vkDestroyDevice(device, nullptr),
				device = VK_NULL_HANDLE;
			return CreateLogicalDevice(pNext, flags);
		}
		VkResult RecreateSwapchain() {
			VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
			if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
				return result;
			}
			if (surfaceCapabilities.currentExtent.width == 0 ||
				surfaceCapabilities.currentExtent.height == 0)
				return VK_SUCCESS;
			swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
			swapchainCreateInfo.oldSwapchain = swapchain;
			VkResult result = vkQueueWaitIdle(queue_graphics);
			if (!result &&
				queue_graphics != queue_presentation)
				result = vkQueueWaitIdle(queue_presentation);
			if (result) {
				std::cout << std::format("[ graphicsBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
				return result;
			}

			for (auto& i : callbacks_destroySwapchain)
				i();
			for (auto& i : swapchainImageViews)
				if (i)
					vkDestroyImageView(device, i, nullptr);
			swapchainImageViews.resize(0);
			if (result = CreateSwapchain_Internal())
				return result;
			for (auto& i : callbacks_createSwapchain)
				i();
			return VK_SUCCESS;
		}

		//Static Function
		static graphicsBase& Base() {
			return singleton;
		}
	};
	inline graphicsBase graphicsBase::singleton;
}