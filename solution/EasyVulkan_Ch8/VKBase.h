#pragma once
#include "EasyVKStart.h"
#pragma warning(disable:4267) //Implicit conversion from 'size_t' to 'type'
#pragma warning(disable:4005) //Macro redefinition
#pragma warning(disable:26812)//Prefer enum class over enum
//Encapsulation for the most basic Vulkan objects.
#define VK_RESULT_THROW

#pragma region Macro
#define DestroyHandleBy(Func) if (handle) { Func(graphicsBase::Base().Device(), handle, nullptr); handle = VK_NULL_HANDLE; }
#define MoveHandle handle = other.handle; other.handle = VK_NULL_HANDLE;
#define DefineHandleTypeOperator operator decltype(handle)() const { return handle; }
#define DefineAddressFunction const decltype(handle)* Address() const { return &handle; }

#ifndef NDEBUG
#define ENABLE_DEBUG_MESSENGER true
//Prevent binding an implicitly generated rvalue to a const reference.
#define DefineHandleTypeOperator operator volatile decltype(handle)() const { return handle; }
#else
#define ENABLE_DEBUG_MESSENGER false
#endif
#pragma endregion

VULKAN_BEGIN
//Defalut Value
constexpr VkExtent2D defaultWindowSize = { 1280, 720 };

//Helper Type
#ifdef VK_RESULT_THROW
class result_t {
	VkResult result;
public:
	static void(*callback_throw)(VkResult);
	result_t(VkResult result) :result(result) {}
	result_t(result_t&& other) noexcept :result(other.result) { other.result = VK_SUCCESS; }
	~result_t() noexcept(false) {
		if (uint32_t(result) < VK_RESULT_MAX_ENUM)
			return;
		if (callback_throw)
			callback_throw(result);
		throw result;
	}
	operator VkResult() {
		VkResult result = this->result;
		this->result = VK_SUCCESS;
		return result;
	}
};
inline void(*result_t::callback_throw)(VkResult);
#elifdef VK_RESULT_NODISCARD
struct [[nodiscard]] result_t {
	VkResult result;
	result_t(VkResult result) :result(result) {}
	operator VkResult() const { return result; }
};
#pragma warning(disable:4834)
#pragma warning(disable:6031)
#else
using result_t = VkResult;
#endif

//Forward Declaration
class graphicsBasePlus;

class graphicsBase {
	struct procedureAddress {
		PFN_vkVoidFunction value;
		procedureAddress(PFN_vkVoidFunction value) :value(value) {}
		template<typename T>
		operator T() const requires (std::is_pointer_v<T>) { return reinterpret_cast<T>(value); }
		operator bool() const { return value; }
	};

	uint32_t apiVersion = VK_API_VERSION_1_0;
	VkInstance instance = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkPhysicalDeviceFeatures2 physicalDeviceFeatures;
	VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features;//Provided by VK_API_VERSION_1_2
	VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features;
	VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features;
	VkPhysicalDeviceProperties2 physicalDeviceProperties;
	VkPhysicalDeviceVulkan11Properties physicalDeviceVulkan11Properties;//Provided by VK_API_VERSION_1_2
	VkPhysicalDeviceVulkan12Properties physicalDeviceVulkan12Properties;
	VkPhysicalDeviceVulkan13Properties physicalDeviceVulkan13Properties;
	VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties;
	std::vector<VkPhysicalDevice> availablePhysicalDevices;

	VkDevice device = nullptr;
	uint32_t queueFamilyIndex_graphics = VK_QUEUE_FAMILY_IGNORED;
	uint32_t queueFamilyIndex_presentation = VK_QUEUE_FAMILY_IGNORED;
	uint32_t queueFamilyIndex_compute = VK_QUEUE_FAMILY_IGNORED;
	VkQueue queue_graphics = nullptr;
	VkQueue queue_presentation = nullptr;
	VkQueue queue_compute = nullptr;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;

	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	uint32_t currentImageIndex = 0;
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};

	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	std::vector<const char*> instanceLayers;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> deviceExtensions;

	void* pNext_instanceCreateInfo;
	void* pNext_deviceCreateInfo;
	void* pNext_swapchainCreateInfo;
	void* pNext_physicalDeviceFeatures;
	void* pNext_physicalDeviceProperties;
	void* pNext_physicalDeviceMemoryProperties;

	std::vector<void(*)()> callbacks_createSwapchain;
	std::vector<void(*)()> callbacks_destroySwapchain;
	std::vector<void(*)()> callbacks_createDevice;
	std::vector<void(*)()> callbacks_destroyDevice;

	graphicsBasePlus* pPlus;//Pimpl
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
				ExecuteCallbacks(callbacks_destroySwapchain);
				for (auto& i : swapchainImageViews)
					if (i)
						vkDestroyImageView(device, i, nullptr);
				vkDestroySwapchainKHR(device, swapchain, nullptr);
			}
			ExecuteCallbacks(callbacks_destroyDevice);
			vkDestroyDevice(device, nullptr);
		}
		if (surface)
			vkDestroySurfaceKHR(instance, surface, nullptr);
		if (debugMessenger) {
			PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessenger = InstanceProcedureAddress("vkDestroyDebugUtilsMessengerEXT");
			if (vkDestroyDebugUtilsMessenger)
				vkDestroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
	}
	//Non-const Function
	result_t CreateDebugMessenger() {
		static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)->VkBool32 {
			OutputMessage("{}\n\n", pCallbackData->pMessage);
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
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = InstanceProcedureAddress("vkCreateDebugUtilsMessengerEXT");
		if (vkCreateDebugUtilsMessenger) {
			VkResult result = vkCreateDebugUtilsMessenger(instance, &debugUtilsMessengerCreateInfo, nullptr, &debugMessenger);
			if (result)
				OutputMessage("[ graphicsBase ] ERROR\nFailed to create a debug messenger!\nError code: {}\n", int32_t(result));
			return result;
		}
		OutputMessage("[ graphicsBase ] ERROR\nFailed to get the function pointer of vkCreateDebugUtilsMessengerEXT!\n");
		return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
	}
	result_t GetQueueFamilyIndices(VkPhysicalDevice physicalDevice, bool enableGraphicsQueue, bool enableComputeQueue, uint32_t(&queueFamilyIndices)[3]) {
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		if (!queueFamilyCount)
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
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
					OutputMessage("[ graphicsBase ] ERROR\nFailed to determine if the queue family supports presentation!\nError code: {}\n", int32_t(result));
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
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
		queueFamilyIndex_graphics = ig;
		queueFamilyIndex_presentation = ip;
		queueFamilyIndex_compute = ic;
		return VK_SUCCESS;
	}
	void GetPhysicalDeviceFeatures() {
		if (apiVersion >= VK_API_VERSION_1_1) {
			physicalDeviceFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
			physicalDeviceVulkan11Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
			physicalDeviceVulkan12Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
			physicalDeviceVulkan13Features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
			if (apiVersion >= VK_API_VERSION_1_2) {
				physicalDeviceFeatures.pNext = &physicalDeviceVulkan11Features;
				physicalDeviceVulkan11Features.pNext = &physicalDeviceVulkan12Features;
				if (apiVersion >= VK_API_VERSION_1_3)
					physicalDeviceVulkan12Features.pNext = &physicalDeviceVulkan13Features;
			}
			SetPNext(physicalDeviceFeatures.pNext, pNext_physicalDeviceFeatures);
			vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDeviceFeatures);
		}
		else
			vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures.features);
	}
	void GetPhysicalDeviceProperties() {
		if (apiVersion >= VK_API_VERSION_1_1) {
			physicalDeviceProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
			physicalDeviceVulkan11Properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
			physicalDeviceVulkan12Properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
			physicalDeviceVulkan13Properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES };
			if (apiVersion >= VK_API_VERSION_1_2) {
				physicalDeviceProperties.pNext = &physicalDeviceVulkan11Properties;
				physicalDeviceVulkan11Properties.pNext = &physicalDeviceVulkan12Properties;
				if (apiVersion >= VK_API_VERSION_1_3)
					physicalDeviceVulkan12Properties.pNext = &physicalDeviceVulkan13Properties;
			}
			SetPNext(physicalDeviceProperties.pNext, pNext_physicalDeviceProperties);
			vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
			physicalDeviceMemoryProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2, pNext_physicalDeviceMemoryProperties };
			vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &physicalDeviceMemoryProperties);
		}
		else
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties.properties),
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties.memoryProperties);
	}
	result_t CreateSwapchain_Internal() {
		//Create new swapchain
		if (VkResult result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to create a swapchain!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Destruction of the retired old swapchain is written inside SwapImage(...).
		//The old swapchain must be destroyed after next invocation of vkQueueSubmit(...).
		//Otherwise, error may occur if the application is running with Intel's integrated GPU.

		//Get swapchain images
		uint32_t swapchainImageCount;
		if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of swapchain images!\nError code: {}\n", int32_t(result));
			return result;
		}
		swapchainImages.resize(swapchainImageCount);
		if (VkResult result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data())) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get swapchain images!\nError code: {}\n", int32_t(result));
			return result;
		}

		//Create new swapchain image views
		swapchainImageViews.resize(swapchainImageCount);
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapchainCreateInfo.imageFormat,
			//.components = {},//All VK_COMPONENT_SWIZZLE_IDENTITY
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};
		for (size_t i = 0; i < swapchainImageCount; i++) {
			imageViewCreateInfo.image = swapchainImages[i];
			if (VkResult result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i])) {
				OutputMessage("[ graphicsBase ] ERROR\nFailed to create a swapchain image view!\nError code: {}\n", int32_t(result));
				return result;
			}
		}
		return VK_SUCCESS;
	}
	//Static Function
	static void** SetPNext(void*& pBegin, void* pNext, bool allowDuplicate = false) {
		struct vkStructureHead {
			VkStructureType sType;
			void* pNext;
		};
		if (!pNext)
			return nullptr;
		auto SetPNext_Internal = [](this auto&& self, void*& pBegin, void* pNext, bool allowDuplicate)->void** {
			if (pBegin == pNext)
				return nullptr;
			if (pBegin)
				if (!allowDuplicate &&
					reinterpret_cast<vkStructureHead*>(pBegin)->sType == reinterpret_cast<vkStructureHead*>(pNext)->sType)
					return nullptr;
				else
					return self(reinterpret_cast<vkStructureHead*>(pBegin)->pNext, pNext, allowDuplicate);
			else
				return &(pBegin = pNext);
		};
		return SetPNext_Internal(pBegin, pNext, allowDuplicate);
	}
	static void AddLayerOrExtension(std::vector<const char*>& container, const char* name) {
		for (auto& i : container)
			if (!strcmp(name, i))
				return;
		container.push_back(name);
	}
	static void ExecuteCallbacks(std::vector<void(*)()>& callbacks) {
		for (size_t size = callbacks.size(), i = 0; i < size; i++)
			callbacks[i]();
		//for (auto& i : callbacks) i();                               //Not safe
		//for (size_t i = 0; i < callbacks.size(); i++) callbacks[i]();//Not safe
	}
public:
	//Getter
	uint32_t ApiVersion() const { return apiVersion; }
	VkInstance Instance() const { return instance; }
	VkPhysicalDevice PhysicalDevice() const { return physicalDevice; }
	constexpr const VkPhysicalDeviceFeatures& PhysicalDeviceFeatures() const { return physicalDeviceFeatures.features; }
	constexpr const VkPhysicalDeviceVulkan11Features& PhysicalDeviceVulkan11Features() const { return physicalDeviceVulkan11Features; }
	constexpr const VkPhysicalDeviceVulkan12Features& PhysicalDeviceVulkan12Features() const { return physicalDeviceVulkan12Features; }
	constexpr const VkPhysicalDeviceVulkan13Features& PhysicalDeviceVulkan13Features() const { return physicalDeviceVulkan13Features; }
	constexpr const VkPhysicalDeviceProperties& PhysicalDeviceProperties() const { return physicalDeviceProperties.properties; }
	constexpr const VkPhysicalDeviceVulkan11Properties& PhysicalDeviceVulkan11Properties() const { return physicalDeviceVulkan11Properties; }
	constexpr const VkPhysicalDeviceVulkan12Properties& PhysicalDeviceVulkan12Properties() const { return physicalDeviceVulkan12Properties; }
	constexpr const VkPhysicalDeviceVulkan13Properties& PhysicalDeviceVulkan13Properties() const { return physicalDeviceVulkan13Properties; }
	constexpr const VkPhysicalDeviceMemoryProperties& PhysicalDeviceMemoryProperties() const { return physicalDeviceMemoryProperties.memoryProperties; }
	VkPhysicalDevice AvailablePhysicalDevice(uint32_t index) const { return availablePhysicalDevices[index]; }
	uint32_t AvailablePhysicalDeviceCount() const { return uint32_t(availablePhysicalDevices.size()); }

	VkDevice Device() const { return device; }
	uint32_t QueueFamilyIndex_Graphics() const { return queueFamilyIndex_graphics; }
	uint32_t QueueFamilyIndex_Presentation() const { return queueFamilyIndex_presentation; }
	uint32_t QueueFamilyIndex_Compute() const { return queueFamilyIndex_compute; }
	VkQueue Queue_Graphics() const { return queue_graphics; }
	VkQueue Queue_Presentation() const { return queue_presentation; }
	VkQueue Queue_Compute() const { return queue_compute; }

	VkSurfaceKHR Surface() const { return surface; }
	VkFormat AvailableSurfaceFormat(uint32_t index) const { return availableSurfaceFormats[index].format; }
	VkColorSpaceKHR AvailableSurfaceColorSpace(uint32_t index) const { return availableSurfaceFormats[index].colorSpace; }
	uint32_t AvailableSurfaceFormatCount() const { return uint32_t(availableSurfaceFormats.size()); }

	VkSwapchainKHR Swapchain() const { return swapchain; }
	VkImage SwapchainImage(uint32_t index) const { return swapchainImages[index]; }
	VkImageView SwapchainImageView(uint32_t index) const { return swapchainImageViews[index]; }
	uint32_t SwapchainImageCount() const { return uint32_t(swapchainImages.size()); }
	uint32_t CurrentImageIndex() const { return currentImageIndex; }
	constexpr const VkSwapchainCreateInfoKHR& SwapchainCreateInfo() const { return swapchainCreateInfo; }

	//Const Function
	procedureAddress InstanceProcedureAddress(const char* functionName) const {
		return vkGetInstanceProcAddr(instance, functionName);
	}
	procedureAddress DeviceProcedureAddress(const char* functionName) const {
		return vkGetDeviceProcAddr(device, functionName);
	}
	//                    If CreateInstance() fails
	result_t CheckInstanceLayers(arrayRef<const char*> layersToCheck) const {
		uint32_t layerCount;
		std::vector<VkLayerProperties> availableLayers;
		if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of instance layers!\n");
			return result;
		}
		if (layerCount) {
			availableLayers.resize(layerCount);
			if (VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data())) {
				OutputMessage("[ graphicsBase ] ERROR\nFailed to enumerate instance layer properties!\nError code: {}\n", int32_t(result));
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
					i = nullptr;//If a required layer isn't available, set it to nullptr
			}
		}
		else
			for (auto& i : layersToCheck)
				i = nullptr;
		return VK_SUCCESS;
	}
	/*If layerName is nullptr, extensions should be provided by the Vulkan implementation or by those implicitly enabled layers*/
	result_t CheckInstanceExtensions(arrayRef<const char*> extensionsToCheck, const char* layerName = nullptr) const {
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> availableExtensions;
		if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, nullptr)) {
			layerName ?
				OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\nLayer name:{}\n", layerName) :
				OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of instance extensions!\n");
			return result;
		}
		if (extensionCount) {
			availableExtensions.resize(extensionCount);
			if (VkResult result = vkEnumerateInstanceExtensionProperties(layerName, &extensionCount, availableExtensions.data())) {
				OutputMessage("[ graphicsBase ] ERROR\nFailed to enumerate instance extension properties!\nError code: {}\n", int32_t(result));
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
					i = nullptr;//If a required extension isn't available, set it to nullptr
			}
		}
		else
			for (auto& i : extensionsToCheck)
				i = nullptr;
		return VK_SUCCESS;
	}
	//                    If CreateDevice() fails
	result_t CheckDeviceExtensions(arrayRef<const char*> extensionsToCheck, const char* layerName = nullptr) const {
		uint32_t extensionCount;
		std::vector<VkExtensionProperties> availableExtensions;
		if (VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &extensionCount, nullptr)) {
			layerName ?
				OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of device extensions!\nLayer name:{}\n", layerName) :
				OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of device extensions!\n");
			return result;
		}
		if (extensionCount) {
			availableExtensions.resize(extensionCount);
			if (VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &extensionCount, availableExtensions.data())) {
				OutputMessage("[ graphicsBase ] ERROR\nFailed to enumerate device extension properties!\nError code: {}\n", int32_t(result));
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
					i = nullptr;//If a required extension isn't available, set it to nullptr
			}
		}
		else
			for (auto& i : extensionsToCheck)
				i = nullptr;
		return VK_SUCCESS;
	}
	//                    After initialization
	result_t WaitIdle() const {
		VkResult result = vkDeviceWaitIdle(device);
		if (result)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to wait for the device to be idle!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t SubmitCommandBuffer_Graphics(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const {
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkResult result = vkQueueSubmit(queue_graphics, 1, &submitInfo, fence);
		if (result)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer,
		VkSemaphore semaphore_imageIsAvailable = VK_NULL_HANDLE, VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE,
		VkPipelineStageFlags dstStage_imageIsAvailable = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) const {
		VkSubmitInfo submitInfo = {
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		if (semaphore_imageIsAvailable)
			submitInfo.waitSemaphoreCount = 1,
			submitInfo.pWaitSemaphores = &semaphore_imageIsAvailable,
			submitInfo.pWaitDstStageMask = &dstStage_imageIsAvailable;
		if (semaphore_renderingIsOver)
			submitInfo.signalSemaphoreCount = 1,
			submitInfo.pSignalSemaphores = &semaphore_renderingIsOver;
		return SubmitCommandBuffer_Graphics(submitInfo, fence);
	}
	result_t SubmitCommandBuffer_Graphics(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const {
		VkSubmitInfo submitInfo = {
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		return SubmitCommandBuffer_Graphics(submitInfo, fence);
	}
	result_t SubmitCommandBuffer_Compute(VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE) const {
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkResult result = vkQueueSubmit(queue_compute, 1, &submitInfo, fence);
		if (result)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to submit the command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t SubmitCommandBuffer_Compute(VkCommandBuffer commandBuffer, VkFence fence = VK_NULL_HANDLE) const {
		VkSubmitInfo submitInfo = {
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		return SubmitCommandBuffer_Compute(submitInfo, fence);
	}
	/*if (queueFamilyIndex_graphics != queueFamilyIndex_presentation && swapchainCreateInfo.imageSharingMode == VK_SHARING_MODE_EXCLUSIVE)*/
	void CmdTransferImageOwnership(VkCommandBuffer commandBuffer) const {
		VkImageMemoryBarrier imageMemoryBarrier_g2p = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = 0,//Access mask is not necessary if the corresponding stage is TOP/BOTTOM_OF_PIPE
			//When a renderpass ends, the image layout is transitioned into its final layout, which is specified inside VkAttachmentDescription.
			.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = queueFamilyIndex_graphics,
			.dstQueueFamilyIndex = queueFamilyIndex_presentation,
			.image = swapchainImages[currentImageIndex],
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
			0, nullptr, 0, nullptr, 1, &imageMemoryBarrier_g2p);
	}
	result_t SubmitCommandBuffer_Presentation(VkCommandBuffer commandBuffer,
		VkSemaphore semaphore_renderingIsOver = VK_NULL_HANDLE, VkSemaphore semaphore_ownershipIsTransfered = VK_NULL_HANDLE, VkFence fence = VK_NULL_HANDLE) const {
		static constexpr VkPipelineStageFlags waitDstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		if (semaphore_renderingIsOver)
			submitInfo.waitSemaphoreCount = 1,
			submitInfo.pWaitSemaphores = &semaphore_renderingIsOver,
			submitInfo.pWaitDstStageMask = &waitDstStage;
		if (semaphore_ownershipIsTransfered)
			submitInfo.signalSemaphoreCount = 1,
			submitInfo.pSignalSemaphores = &semaphore_ownershipIsTransfered;
		VkResult result = vkQueueSubmit(queue_presentation, 1, &submitInfo, fence);
		if (result)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to submit the presentation command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}
	//Non-const Function
	void AddCallback_CreateSwapchain(void(*function)()) {
		callbacks_createSwapchain.push_back(function);
	}
	void AddCallback_DestroySwapchain(void(*function)()) {
		callbacks_destroySwapchain.push_back(function);
	}
	void AddCallback_CreateDevice(void(*function)()) {
		callbacks_createDevice.push_back(function);
	}
	void AddCallback_DestroyDevice(void(*function)()) {
		callbacks_destroyDevice.push_back(function);
	}
	//                    For instance creation
	void AddInstanceLayer(const char* layer) {
		AddLayerOrExtension(instanceLayers, layer);
	}
	void AddInstanceExtension(const char* extension) {
		AddLayerOrExtension(instanceExtensions, extension);
	}
	void InstanceLayers(const std::vector<const char*>& layers) {
		instanceLayers = layers;
	}
	void InstanceExtensions(const std::vector<const char*>& extensions) {
		instanceExtensions = extensions;
	}
	void AddNextStructure_InstanceCreateInfo(auto& next, bool allowDuplicate = false) {
		SetPNext(pNext_instanceCreateInfo, &next, allowDuplicate);
	}
	result_t UseLatestApiVersion() {
		if (InstanceProcedureAddress("vkEnumerateInstanceVersion"))
			return vkEnumerateInstanceVersion(&apiVersion);
		return VK_SUCCESS;
	}
	result_t CreateInstance(VkInstanceCreateFlags flags = 0) {
		if constexpr (ENABLE_DEBUG_MESSENGER)
			//Add validation layer
			AddInstanceLayer("VK_LAYER_KHRONOS_validation"),
			//Add debug extension
			AddInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		VkApplicationInfo applicatianInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.apiVersion = apiVersion
		};
		VkInstanceCreateInfo instanceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = pNext_instanceCreateInfo,
			.flags = flags,
			.pApplicationInfo = &applicatianInfo,
			.enabledLayerCount = uint32_t(instanceLayers.size()),
			.ppEnabledLayerNames = instanceLayers.data(),
			.enabledExtensionCount = uint32_t(instanceExtensions.size()),
			.ppEnabledExtensionNames = instanceExtensions.data()
		};
		if (VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to create a Vulkan instance!\nError code: {}\n", int32_t(result));
			return result;
		}
		OutputMessage(
			"Vulkan API Version: {}.{}.{}\n",
			VK_VERSION_MAJOR(apiVersion),
			VK_VERSION_MINOR(apiVersion),
			VK_VERSION_PATCH(apiVersion));
		if constexpr (ENABLE_DEBUG_MESSENGER)
			CreateDebugMessenger();
		return VK_SUCCESS;
	}
	//                    For logical device creation
	void AddDeviceExtension(const char* extension) {
		AddLayerOrExtension(deviceExtensions, extension);
	}
	void DeviceExtensions(const std::vector<const char*>& extensions) {
		instanceExtensions = extensions;
	}
	void AddNextStructure_DeviceCreateInfo(auto& next, bool allowDuplicate = false) {
		SetPNext(pNext_deviceCreateInfo, &next, allowDuplicate);
	}
	void AddNextStructure_PhysicalDeviceFeatures(auto& next, bool allowDuplicate = false) {
		SetPNext(pNext_physicalDeviceFeatures, &next, allowDuplicate);
	}
	void AddNextStructure_PhysicalDeviceProperties(auto& next, bool allowDuplicate = false) {
		SetPNext(pNext_physicalDeviceProperties, &next, allowDuplicate);
	}
	void AddNextStructure_PhysicalDeviceMemoryProperties(auto& next, bool allowDuplicate = false) {
		SetPNext(pNext_physicalDeviceMemoryProperties, &next, allowDuplicate);
	}
	void Surface(VkSurfaceKHR surface) { if (!this->surface) this->surface = surface; }
	result_t GetPhysicalDevices() {
		uint32_t deviceCount;
		if (VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of physical devices!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!deviceCount)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to find any physical device supports Vulkan!\n"),
			abort();
		availablePhysicalDevices.resize(deviceCount);
		VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, availablePhysicalDevices.data());
		if (result)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to enumerate physical devices!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t DeterminePhysicalDevice(uint32_t deviceIndex = 0, bool enableGraphicsQueue = true, bool enableComputeQueue = true) {
		static constexpr uint32_t notFound = INT32_MAX;//== VK_QUEUE_FAMILY_IGNORED & INT32_MAX
		struct queueFamilyIndexCombination {
			uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
			uint32_t presentation = VK_QUEUE_FAMILY_IGNORED;
			uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
		};
		static std::vector<queueFamilyIndexCombination> queueFamilyIndexCombinations(availablePhysicalDevices.size());
		auto& [ig, ip, ic] = queueFamilyIndexCombinations[deviceIndex];
		//If any required queue family is not found.
		if (ig == notFound && enableGraphicsQueue ||
			ip == notFound && surface ||
			ic == notFound && enableComputeQueue)
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
		//Otherwise, if any required queue family is not acquired.
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
		//Otherwise, all required queue families are found.
		else {
			queueFamilyIndex_graphics = enableGraphicsQueue ? ig : VK_QUEUE_FAMILY_IGNORED;
			queueFamilyIndex_presentation = surface ? ip : VK_QUEUE_FAMILY_IGNORED;
			queueFamilyIndex_compute = enableComputeQueue ? ic : VK_QUEUE_FAMILY_IGNORED;
		}
		physicalDevice = availablePhysicalDevices[deviceIndex];
		return VK_SUCCESS;
	}
	result_t CreateDevice(VkDeviceCreateFlags flags = 0) {
		//Determine how many queues
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
		//Get physical device features
		GetPhysicalDeviceFeatures();
		//Create logical device
		VkDeviceCreateInfo deviceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.flags = flags,
			.queueCreateInfoCount = queueCreateInfoCount,
			.pQueueCreateInfos = queueCreateInfos,
			.enabledExtensionCount = uint32_t(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
		};
		void** ppNext = nullptr;
		if (apiVersion >= VK_API_VERSION_1_1)
			ppNext = SetPNext(pNext_deviceCreateInfo, &physicalDeviceFeatures);
		else
			deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures.features;
		deviceCreateInfo.pNext = pNext_deviceCreateInfo;
		VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
		if (ppNext)
			*ppNext = nullptr;//Unset &physicalDeviceFeatures
		if (result) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to create a logical device!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Get queues
		if (queueFamilyIndex_graphics != VK_QUEUE_FAMILY_IGNORED)
			vkGetDeviceQueue(device, queueFamilyIndex_graphics, 0, &queue_graphics);
		if (queueFamilyIndex_presentation != VK_QUEUE_FAMILY_IGNORED)
			vkGetDeviceQueue(device, queueFamilyIndex_presentation, 0, &queue_presentation);
		if (queueFamilyIndex_compute != VK_QUEUE_FAMILY_IGNORED)
			vkGetDeviceQueue(device, queueFamilyIndex_compute, 0, &queue_compute);
		//At this point, there's no reason to change the physical device anymore, get physical device properties.
		GetPhysicalDeviceProperties();
		OutputMessage("Renderer: {}\n", physicalDeviceProperties.properties.deviceName);
		ExecuteCallbacks(callbacks_createDevice);
		return VK_SUCCESS;
	}
	//                    For swapchain creation
	void AddNextStructure_SwapchainCreateInfo(auto& next, bool allowDuplicate = false) {
		SetPNext(pNext_swapchainCreateInfo, &next, allowDuplicate);
	}
	/*No need to call GetSurfaceFormats() if you don't want to manually set the surface format before creating swapchain*/
	result_t GetSurfaceFormats() {
		uint32_t surfaceFormatCount;
		if (VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of surface formats!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!surfaceFormatCount)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to find any supported surface format!\n"),
			abort();
		availableSurfaceFormats.resize(surfaceFormatCount);
		VkResult result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, availableSurfaceFormats.data());
		if (result)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get surface formats!\nError code: {}\n", int32_t(result));
		return result;
	}
	/*Will call RecreateSwapchain() if the swapchain already exists*/
	result_t SetSurfaceFormat(VkSurfaceFormatKHR surfaceFormat) {
		bool formatIsAvailable = false;
		if (!surfaceFormat.format) {
			for (auto& i : availableSurfaceFormats)
				if (i.colorSpace == surfaceFormat.colorSpace) {
					swapchainCreateInfo.imageFormat = i.format;
					swapchainCreateInfo.imageColorSpace = i.colorSpace;
					formatIsAvailable = true;
					break;
				}
		}
		else
			for (auto& i : availableSurfaceFormats)
				if (i.format == surfaceFormat.format &&
					i.colorSpace == surfaceFormat.colorSpace) {
					swapchainCreateInfo.imageFormat = i.format;
					swapchainCreateInfo.imageColorSpace = i.colorSpace;
					formatIsAvailable = true;
					break;
				}
		if (!formatIsAvailable)
			return VK_ERROR_FORMAT_NOT_SUPPORTED;
		if (swapchain)
			return RecreateSwapchain();
		return VK_SUCCESS;
	}
	result_t CreateSwapchain(bool limitFrameRate = true, VkSwapchainCreateFlagsKHR flags = 0) {
		//Get surface capabilities
		VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
		if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
			return result;
		}
		//Set image count
		swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + (surfaceCapabilities.maxImageCount > surfaceCapabilities.minImageCount);
		//Set image extent
		swapchainCreateInfo.imageExtent =
			surfaceCapabilities.currentExtent.width == UINT32_MAX ?
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
			for (size_t i = 0; i < 3; i++)
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
			OutputMessage("[ graphicsBase ] WARNING\nVK_IMAGE_USAGE_TRANSFER_DST_BIT is not supported!\n");

		//Get surface formats
		if (!availableSurfaceFormats.size())
			if (VkResult result = GetSurfaceFormats())
				return result;
		//If surface format is not determined, select a a four-component UNORM format
		if (!swapchainCreateInfo.imageFormat)
			if (SetSurfaceFormat({ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) &&
				SetSurfaceFormat({ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })) {
				swapchainCreateInfo.imageFormat = availableSurfaceFormats[0].format;
				swapchainCreateInfo.imageColorSpace = availableSurfaceFormats[0].colorSpace;
				OutputMessage("[ graphicsBase ] WARNING\nFailed to select a four-component UNORM surface format!\n");
			}

		//Get surface present modes
		uint32_t surfacePresentModeCount;
		if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, nullptr)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get the count of surface present modes!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!surfacePresentModeCount)
			OutputMessage("[ graphicsBase ] ERROR\nFailed to find any surface present mode!\n"),
			abort();
		std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
		if (VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &surfacePresentModeCount, surfacePresentModes.data())) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get surface present modes!\nError code: {}\n", int32_t(result));
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

		//Create swapchain
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.pNext = pNext_swapchainCreateInfo;
		swapchainCreateInfo.flags = flags;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.clipped = VK_TRUE;
		if (VkResult result = CreateSwapchain_Internal())
			return result;

		//Create related objects
		ExecuteCallbacks(callbacks_createSwapchain);
		return VK_SUCCESS;
	}
	//                    After initialization
	/*Call Terminate() if you need to terminate Vulkan before program exits*/
	void Terminate() {
		this->~graphicsBase();
		instance = nullptr;
		physicalDevice = nullptr;
		device = nullptr;
		surface = VK_NULL_HANDLE;
		swapchain = VK_NULL_HANDLE;
		swapchainImages.resize(0);
		swapchainImageViews.resize(0);
		swapchainCreateInfo = {};
		debugMessenger = VK_NULL_HANDLE;
	}
	/*Call RecreateDevice() and CreateSwapchain(...) after DeterminePhysicalDevice(...) if you want to switch physical device at runtime*/
	result_t RecreateDevice(VkDeviceCreateFlags flags = 0) {
		if (device) {
			if (VkResult result = WaitIdle();
				result != VK_SUCCESS &&
				result != VK_ERROR_DEVICE_LOST)
				return result;
			if (swapchain) {
				ExecuteCallbacks(callbacks_destroySwapchain);
				for (auto& i : swapchainImageViews)
					if (i)
						vkDestroyImageView(device, i, nullptr);
				swapchainImageViews.resize(0);
				vkDestroySwapchainKHR(device, swapchain, nullptr);
				swapchain = VK_NULL_HANDLE;
				swapchainCreateInfo = {};
			}
			ExecuteCallbacks(callbacks_destroyDevice);
			vkDestroyDevice(device, nullptr);
			device = nullptr;
		}
		return CreateDevice(flags);
	}
	result_t RecreateSwapchain() {
		VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
		if (VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities)) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to get physical device surface capabilities!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (surfaceCapabilities.currentExtent.width == 0 ||
			surfaceCapabilities.currentExtent.height == 0)
			return VK_SUBOPTIMAL_KHR;
		swapchainCreateInfo.pNext = pNext_swapchainCreateInfo;
		swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
		swapchainCreateInfo.oldSwapchain = swapchain;

		//Wait graphics queue and presentation queue to be idle
		VkResult result = vkQueueWaitIdle(queue_graphics);
		if (result == VK_SUCCESS &&
			queue_graphics != queue_presentation)
			result = vkQueueWaitIdle(queue_presentation);
		if (result) {
			OutputMessage("[ graphicsBase ] ERROR\nFailed to wait for the queue to be idle!\nError code: {}\n", int32_t(result));
			return result;
		}

		//Destroy old swapchain related objects
		ExecuteCallbacks(callbacks_destroySwapchain);
		for (auto& i : swapchainImageViews)
			if (i)
				vkDestroyImageView(device, i, nullptr);
		swapchainImageViews.resize(0);
		//Create swapchain
		if (VkResult result = CreateSwapchain_Internal())
			return result;
		ExecuteCallbacks(callbacks_createSwapchain);
		return VK_SUCCESS;
	}
	result_t SwapImage(VkSemaphore semaphore_imageIsAvailable) {
		//Destroy retired old swapchain and its associated VkImage handles
		if (swapchainCreateInfo.oldSwapchain &&
			swapchainCreateInfo.oldSwapchain != swapchain) {//Don't destroy old swapchain if RecreateSwapchain() fails
			vkDestroySwapchainKHR(device, swapchainCreateInfo.oldSwapchain, nullptr);
			swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		}
		while (VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphore_imageIsAvailable, VK_NULL_HANDLE, &currentImageIndex))
			switch (result) {
			case VK_SUBOPTIMAL_KHR:
			case VK_ERROR_OUT_OF_DATE_KHR:
				if (VkResult result = RecreateSwapchain())
					return result;
				break;
			default:
				OutputMessage("[ graphicsBase ] ERROR\nFailed to acquire the next image!\nError code: {}\n", int32_t(result));
				return result;
			}
		return VK_SUCCESS;
	}
	result_t PresentImage(VkPresentInfoKHR& presentInfo) {
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		switch (VkResult result = vkQueuePresentKHR(queue_presentation, &presentInfo)) {
		case VK_SUCCESS:
			return VK_SUCCESS;
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			return RecreateSwapchain();
		default:
			OutputMessage("[ graphicsBase ] ERROR\nFailed to queue the image for presentation!\nError code: {}\n", int32_t(result));
			return result;
		}
	}
	result_t PresentImage(VkSemaphore semaphore_renderingIsOver/*Or ownershipIsTransfered*/ = VK_NULL_HANDLE) {
		VkPresentInfoKHR presentInfo = {
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &currentImageIndex
		};
		if (semaphore_renderingIsOver)
			presentInfo.waitSemaphoreCount = 1,
			presentInfo.pWaitSemaphores = &semaphore_renderingIsOver;
		return PresentImage(presentInfo);
	}
	//Static Function
	static constexpr graphicsBase& Base() { return singleton; }
	static graphicsBasePlus& Plus() { return *singleton.pPlus; }
	static void Plus(graphicsBasePlus& plus) { if (!singleton.pPlus) singleton.pPlus = &plus; }
};
DefineStaticDataMember(graphicsBase::singleton);

#pragma region Synchronization
//Done+
class semaphore {
	VkSemaphore handle = VK_NULL_HANDLE;
public:
	//semaphore() = default;
	semaphore(VkSemaphoreCreateInfo& createInfo) {
		Create(createInfo);
	}
	semaphore(/*reserved for future use*/) {
		Create();
	}
	semaphore(semaphore&& other) noexcept { MoveHandle; }
	~semaphore() { DestroyHandleBy(vkDestroySemaphore); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkSemaphoreCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkResult result = vkCreateSemaphore(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ semaphore ] ERROR\nFailed to create a semaphore!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(/*reserved for future use*/) {
		VkSemaphoreCreateInfo createInfo = {};
		return Create(createInfo);
	}
};
//Done++
class fence {
	VkFence handle = VK_NULL_HANDLE;
public:
	//fence() = default;
	fence(VkFenceCreateInfo& createInfo) {
		Create(createInfo);
	}
	fence(VkFenceCreateFlags flags = 0) {
		Create(flags);
	}
	fence(fence&& other) noexcept { MoveHandle; }
	~fence() { DestroyHandleBy(vkDestroyFence); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	result_t Wait() const {
		VkResult result = vkWaitForFences(graphicsBase::Base().Device(), 1, &handle, false, UINT64_MAX);
		if (result)
			OutputMessage("[ fence ] ERROR\nFailed to wait for the fence!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Reset() const {
		VkResult result = vkResetFences(graphicsBase::Base().Device(), 1, &handle);
		if (result)
			OutputMessage("[ fence ] ERROR\nFailed to reset the fence!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t WaitAndReset() const {
		VkResult result = Wait();
		result || (result = Reset());
		return result;
	}
	result_t Status() const {
		VkResult result = vkGetFenceStatus(graphicsBase::Base().Device(), handle);
		if (result < 0)
			OutputMessage("[ fence ] ERROR\nFailed to get the status of the fence!\nError code: {}\n", int32_t(result));
		return result;
	}
	//Non-const Function
	result_t Create(VkFenceCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkResult result = vkCreateFence(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ fence ] ERROR\nFailed to create a fence!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkFenceCreateFlags flags = 0) {
		VkFenceCreateInfo createInfo = {
			.flags = flags
		};
		return Create(createInfo);
	}
};
//Done++
class event {
	VkEvent handle = VK_NULL_HANDLE;
public:
	//event() = default;
	event(VkEventCreateInfo& createInfo) {
		Create(createInfo);
	}
	event(VkEventCreateFlags flags = 0) {
		Create(flags);
	}
	event(event& other) noexcept { MoveHandle; }
	~event() { DestroyHandleBy(vkDestroyEvent); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	void CmdSet(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from) const {
		vkCmdSetEvent(commandBuffer, handle, stage_from);
	}
	void CmdReset(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from) const {
		vkCmdResetEvent(commandBuffer, handle, stage_from);
	}
	void CmdWait(VkCommandBuffer commandBuffer, VkPipelineStageFlags stage_from, VkPipelineStageFlags stage_to,
		arrayRef<VkMemoryBarrier> memoryBarriers,
		arrayRef<VkBufferMemoryBarrier> bufferMemoryBarriers,
		arrayRef<VkImageMemoryBarrier> imageMemoryBarriers) const {
		for (auto& i : memoryBarriers)
			i.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		for (auto& i : bufferMemoryBarriers)
			i.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		for (auto& i : imageMemoryBarriers)
			i.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkCmdWaitEvents(commandBuffer, 1, &handle, stage_from, stage_to,
			memoryBarriers.Count(), memoryBarriers,
			bufferMemoryBarriers.Count(), bufferMemoryBarriers,
			imageMemoryBarriers.Count(), imageMemoryBarriers);
	}
	result_t Set() const {
		VkResult result = vkSetEvent(graphicsBase::Base().Device(), handle);
		if (result)
			OutputMessage("[ event ] ERROR\nFailed to singal the event!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Reset() const {
		VkResult result = vkResetEvent(graphicsBase::Base().Device(), handle);
		if (result)
			OutputMessage("[ event ] ERROR\nFailed to unsingal the event!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Status() const {
		VkResult result = vkGetEventStatus(graphicsBase::Base().Device(), handle);
		if (result < 0)
			OutputMessage("[ event ] ERROR\nFailed to get the status of the event!\nError code: {}\n", int32_t(result));
		return result;
	}
	//Non-const Function
	result_t Create(VkEventCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		VkResult result = vkCreateEvent(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ event ] ERROR\nFailed to create a event!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkEventCreateFlags flags = 0) {
		VkEventCreateInfo createInfo = {
			.flags = flags
		};
		return Create(createInfo);
	}
};
#pragma endregion

#pragma region Buffer & Image
//Done
class deviceMemory {
	VkDeviceMemory handle = VK_NULL_HANDLE;
	VkDeviceSize allocationSize = 0;
	VkMemoryPropertyFlags memoryProperties = 0;
	//--------------------
	VkDeviceSize AdjustNonCoherentMemoryRange(VkDeviceSize& size, VkDeviceSize& offset) const {
		//Adjust mapped memory range if memory is not host coherent
		const VkDeviceSize& nonCoherentAtomSize = graphicsBase::Base().PhysicalDeviceProperties().limits.nonCoherentAtomSize;
		VkDeviceSize _offset = offset;
		offset = offset / nonCoherentAtomSize * nonCoherentAtomSize;
		size = std::min((size + _offset + nonCoherentAtomSize - 1) / nonCoherentAtomSize * nonCoherentAtomSize, allocationSize) - offset;
		return _offset - offset;
	}
protected:
	class {
		friend class bufferMemory;
		friend class imageMemory;
		bool value = false;
		operator bool() const { return value; }
		auto& operator=(bool value) { this->value = value; return *this; }
	} areBound;//Used by bufferMemory or imageMemory, declared here to save 8 bytes
public:
	deviceMemory() = default;
	deviceMemory(VkMemoryAllocateInfo& allocateInfo) {
		Allocate(allocateInfo);
	}
	deviceMemory(deviceMemory&& other) noexcept {
		MoveHandle;
		allocationSize = other.allocationSize;
		memoryProperties = other.memoryProperties;
		other.allocationSize = 0;
		other.memoryProperties = 0;
	}
	~deviceMemory() { DestroyHandleBy(vkFreeMemory); allocationSize = 0; memoryProperties = 0; }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	VkDeviceSize AllocationSize() const { return allocationSize; }
	VkMemoryPropertyFlags MemoryProperties() const { return memoryProperties; }
	//Const Function
	/*If allocated memory is host visible*/
	result_t MapMemory(void*& pData, VkDeviceSize size, VkDeviceSize offset = 0) const {
		VkDeviceSize inverseDeltaOffset;
		if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			inverseDeltaOffset = AdjustNonCoherentMemoryRange(size, offset);
		if (VkResult result = vkMapMemory(graphicsBase::Base().Device(), handle, offset, size, 0, &pData)) {
			OutputMessage("[ deviceMemory ] ERROR\nFailed to map the memory!\nError code: {}\n", int32_t(result));
			return result;
		}
		if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			pData = static_cast<uint8_t*>(pData) + inverseDeltaOffset;
			VkMappedMemoryRange mappedMemoryRange = {
				.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				.memory = handle,
				.offset = offset,
				.size = size
			};
			if (VkResult result = vkInvalidateMappedMemoryRanges(graphicsBase::Base().Device(), 1, &mappedMemoryRange)) {
				OutputMessage("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
				return result;
			}
		}
		return VK_SUCCESS;
	}
	result_t UnmapMemory(VkDeviceSize size, VkDeviceSize offset = 0) const {
		if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			AdjustNonCoherentMemoryRange(size, offset);
			VkMappedMemoryRange mappedMemoryRange = {
				.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				.memory = handle,
				.offset = offset,
				.size = size
			};
			if (VkResult result = vkFlushMappedMemoryRanges(graphicsBase::Base().Device(), 1, &mappedMemoryRange)) {
				OutputMessage("[ deviceMemory ] ERROR\nFailed to flush the memory!\nError code: {}\n", int32_t(result));
				return result;
			}
		}
		vkUnmapMemory(graphicsBase::Base().Device(), handle);
		return VK_SUCCESS;
	}
	result_t BufferData(const void* pData_src, VkDeviceSize size, VkDeviceSize offset = 0) const {
		void* pData_dst;
		if (VkResult result = MapMemory(pData_dst, size, offset))
			return result;
		memcpy(pData_dst, pData_src, size_t(size));
		return UnmapMemory(size, offset);
	}
	result_t BufferData(const auto& data_src) const {
		return BufferData(&data_src, sizeof data_src);
	}
	result_t RetrieveData(void* pData_dst, VkDeviceSize size, VkDeviceSize offset = 0) const {
		void* pData_src;
		if (VkResult result = MapMemory(pData_src, size, offset))
			return result;
		memcpy(pData_dst, pData_src, size_t(size));
		return UnmapMemory(size, offset);
	}
	//Non-const Function
	result_t Allocate(VkMemoryAllocateInfo& allocateInfo) {
		if (allocateInfo.memoryTypeIndex >= graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount) {
			OutputMessage("[ deviceMemory ] ERROR\nInvalid memory type index!\n");
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
		}
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		if (VkResult result = vkAllocateMemory(graphicsBase::Base().Device(), &allocateInfo, nullptr, &handle)) {
			OutputMessage("[ deviceMemory ] ERROR\nFailed to allocate memory!\nError code: {}\n", int32_t(result));
			return result;
		}
		allocationSize = allocateInfo.allocationSize;
		memoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypes[allocateInfo.memoryTypeIndex].propertyFlags;
		return VK_SUCCESS;
	}
	//Static Function
	/*Provided by VK_API_VERSION_1_1*/
	static VkMemoryAllocateFlagsInfo AllocateFlagsInfo(VkMemoryAllocateFlags flags, uint32_t deviceMask = 0) {
		return {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
			.flags = flags,
			.deviceMask = deviceMask
		};
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
	buffer(VkDeviceSize size, VkBufferUsageFlags usages, arrayRef<const uint32_t> queueFamilyIndices = {}, VkBufferCreateFlags flags = 0) {
		Create(size, usages, queueFamilyIndices, flags);
	}
	buffer(buffer&& other) noexcept { MoveHandle; }
	~buffer() { DestroyHandleBy(vkDestroyBuffer); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
		VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
		};
		VkMemoryRequirements memoryRequirements;
		//Get allocation size
		vkGetBufferMemoryRequirements(graphicsBase::Base().Device(), handle, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		//Get memory type index
		memoryAllocateInfo.memoryTypeIndex = UINT32_MAX;
		auto& physicalDeviceMemoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties();
		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
			if (memoryRequirements.memoryTypeBits & 1 << i &&
				(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties) {
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		//if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
		//	OutputMessage("[ buffer ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
		return memoryAllocateInfo;
	}
	result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
		VkResult result = vkBindBufferMemory(graphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
		if (result)
			OutputMessage("[ buffer ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
		return result;
	}
	//Non-const Function
	result_t Create(VkBufferCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		VkResult result = vkCreateBuffer(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ buffer ] ERROR\nFailed to create a buffer!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkDeviceSize size, VkBufferUsageFlags usages, arrayRef<const uint32_t> queueFamilyIndices = {}, VkBufferCreateFlags flags = 0) {
		VkBufferCreateInfo createInfo = {
			.flags = flags,
			.size = size,
			.usage = usages,
			.sharingMode = queueFamilyIndices.Count() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = uint32_t(queueFamilyIndices.Count()),
			.pQueueFamilyIndices = &queueFamilyIndices
		};
		return Create(createInfo);
	}
};

//Done
class bufferMemory :buffer, deviceMemory {
public:
	bufferMemory() = default;
	bufferMemory(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties, optionalRef_any next_allocateInfo = {}) {
		Create(createInfo, desiredMemoryProperties, next_allocateInfo);
	}
	bufferMemory(bufferMemory&& other) noexcept :
		buffer(std::move(other)), deviceMemory(std::move(other)) {
		areBound = other.areBound;
		other.areBound = false;
	}
	~bufferMemory() { areBound = false; }
	//Getter
	/*Implicit casting is invalid on 32-bit, because VkBuffer and VkDeviceMemory are both uint64_t*/
	VkBuffer Buffer() const { return static_cast<const buffer&>(*this); }
	const VkBuffer* AddressOfBuffer() const { return buffer::Address(); }
	VkDeviceMemory Memory() const { return static_cast<const deviceMemory&>(*this); }
	const VkDeviceMemory* AddressOfMemory() const { return deviceMemory::Address(); }
	bool AreBound() const { return areBound; }//If AreBound() returns true, both buffer and memory are created and bound together.
	using deviceMemory::AllocationSize;
	using deviceMemory::MemoryProperties;
	//Const Function
	using deviceMemory::MapMemory;
	using deviceMemory::UnmapMemory;
	using deviceMemory::BufferData;
	using deviceMemory::RetrieveData;
	//Non-const Function
	result_t CreateBuffer(auto... arguments) {
		return buffer::Create(arguments...);
	}
	result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties, optionalRef_any next = {}) {
		VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
		allocateInfo.pNext = &next;
		if (allocateInfo.memoryTypeIndex >= graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
		return Allocate(allocateInfo);
	}
	result_t BindMemory() {
		if (VkResult result = buffer::BindMemory(Memory()))
			return result;
		areBound = true;
		return VK_SUCCESS;
	}
	/*Creat buffer, allocate memory, then bind memory*/
	result_t Create(VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties, optionalRef_any next_allocateInfo = {}) {
		VkResult result;
		false ||//Auto formatting alignment
			(result = CreateBuffer(createInfo)) ||
			(result = AllocateMemory(desiredMemoryProperties, next_allocateInfo)) ||
			(result = BindMemory());
		return result;
	}
};

//Done+
class bufferView {
	VkBufferView handle = VK_NULL_HANDLE;
public:
	bufferView() = default;
	bufferView(VkBufferViewCreateInfo& createInfo) {
		Create(createInfo);
	}
	bufferView(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*reserved for future use*/) {
		Create(buffer, format, offset, range);
	}
	bufferView(bufferView&& other) noexcept { MoveHandle; }
	~bufferView() { DestroyHandleBy(vkDestroyBufferView); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkBufferViewCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
		VkResult result = vkCreateBufferView(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ bufferView ] ERROR\nFailed to create a buffer view!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkBuffer buffer, VkFormat format, VkDeviceSize offset = 0, VkDeviceSize range = 0 /*reserved for future use*/) {
		VkBufferViewCreateInfo createInfo = {
			.buffer = buffer,
			.format = format,
			.offset = offset,
			.range = range
		};
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
	image(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t layerCount, VkSampleCountFlagBits sampleCount,
		VkImageTiling tiling, VkImageUsageFlags usages, arrayRef<const uint32_t> queueFamilyIndices = {}, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkImageCreateFlags flags = 0) {
		Create(imageType, format, extent, mipLevelCount, layerCount, sampleCount, tiling, usages, queueFamilyIndices, initialLayout, flags);
	}
	image(image&& other) noexcept { MoveHandle; }
	~image() { DestroyHandleBy(vkDestroyImage); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	VkMemoryAllocateInfo MemoryAllocateInfo(VkMemoryPropertyFlags desiredMemoryProperties) const {
		VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
		};
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(graphicsBase::Base().Device(), handle, &memoryRequirements);
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		auto GetMemoryTypeIndex = [](uint32_t memoryTypeBits, VkMemoryPropertyFlags desiredMemoryProperties) {
			auto& physicalDeviceMemoryProperties = graphicsBase::Base().PhysicalDeviceMemoryProperties();
			for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
				if (memoryTypeBits & 1 << i &&
					(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & desiredMemoryProperties) == desiredMemoryProperties)
					return i;
			return UINT32_MAX;
		};
		memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties);
		//Highly possible that the GPU and its driver may not support lazy allocation.
		if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX &&
			desiredMemoryProperties & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			memoryAllocateInfo.memoryTypeIndex = GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, desiredMemoryProperties & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
		//if (memoryAllocateInfo.memoryTypeIndex == UINT32_MAX)
		//	OutputMessage("[ image ] ERROR\nFailed to find any memory type satisfies all desired memory properties!\n");
		return memoryAllocateInfo;
	}
	result_t BindMemory(VkDeviceMemory deviceMemory, VkDeviceSize memoryOffset = 0) const {
		VkResult result = vkBindImageMemory(graphicsBase::Base().Device(), handle, deviceMemory, memoryOffset);
		if (result)
			OutputMessage("[ image ] ERROR\nFailed to attach the memory!\nError code: {}\n", int32_t(result));
		return result;
	}
	//Non-const Function
	result_t Create(VkImageCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		VkResult result = vkCreateImage(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ image ] ERROR\nFailed to create an image!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkImageType imageType, VkFormat format, VkExtent3D extent, uint32_t mipLevelCount, uint32_t layerCount, VkSampleCountFlagBits sampleCount,
		VkImageTiling tiling, VkImageUsageFlags usages, arrayRef<const uint32_t> queueFamilyIndices = {}, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkImageCreateFlags flags = 0) {
		VkImageCreateInfo createInfo = {
			.flags = flags,
			.imageType = imageType,
			.format = format,
			.extent = extent,
			.mipLevels = mipLevelCount,
			.arrayLayers = layerCount,
			.samples = sampleCount,
			.tiling = tiling,
			.usage = usages,
			.sharingMode = queueFamilyIndices.Count() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = uint32_t(queueFamilyIndices.Count()),
			.pQueueFamilyIndices = &queueFamilyIndices,
			.initialLayout = initialLayout
		};
		return Create(createInfo);
	}
};

//Done
class imageMemory :image, deviceMemory {
public:
	imageMemory() = default;
	imageMemory(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties, optionalRef_any next_allocateInfo = {}) {
		Create(createInfo, desiredMemoryProperties, next_allocateInfo);
	}
	imageMemory(imageMemory&& other) noexcept :
		image(std::move(other)), deviceMemory(std::move(other)) {
		areBound = other.areBound;
		other.areBound = false;
	}
	~imageMemory() { areBound = false; }
	//Getter
	VkImage Image() const { return static_cast<const image&>(*this); }
	const VkImage* AddressOfImage() const { return image::Address(); }
	VkDeviceMemory Memory() const { return static_cast<const deviceMemory&>(*this); }
	const VkDeviceMemory* AddressOfMemory() const { return deviceMemory::Address(); }
	bool AreBound() const { return areBound; }
	using deviceMemory::AllocationSize;
	using deviceMemory::MemoryProperties;
	//Non-const Function
	result_t CreateImage(auto... arguments) {
		return image::Create(arguments...);
	}
	result_t AllocateMemory(VkMemoryPropertyFlags desiredMemoryProperties, optionalRef_any next = {}) {
		VkMemoryAllocateInfo allocateInfo = MemoryAllocateInfo(desiredMemoryProperties);
		allocateInfo.pNext = &next;
		if (allocateInfo.memoryTypeIndex >= graphicsBase::Base().PhysicalDeviceMemoryProperties().memoryTypeCount)
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
		return Allocate(allocateInfo);
	}
	result_t BindMemory() {
		if (VkResult result = image::BindMemory(Memory()))
			return result;
		areBound = true;
		return VK_SUCCESS;
	}
	/*Creat image, allocate memory, then bind memory*/
	result_t Create(VkImageCreateInfo& createInfo, VkMemoryPropertyFlags desiredMemoryProperties, optionalRef_any next_allocateInfo = {}) {
		VkResult result;
		false ||//Auto formatting alignment
			(result = CreateImage(createInfo)) ||
			(result = AllocateMemory(desiredMemoryProperties, next_allocateInfo)) ||
			(result = BindMemory());
		return result;
	}
};

//Done++
class imageView {
	VkImageView handle = VK_NULL_HANDLE;
public:
	imageView() = default;
	imageView(VkImageViewCreateInfo& createInfo) {
		Create(createInfo);
	}
	imageView(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
		Create(image, viewType, format, subresourceRange);
	}
	imageView(imageView&& other) noexcept { MoveHandle; }
	~imageView() { DestroyHandleBy(vkDestroyImageView); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkImageViewCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		VkResult result = vkCreateImageView(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ imageView ] ERROR\nFailed to create an image view!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkImage image, VkImageViewType viewType, VkFormat format, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) {
		VkImageViewCreateInfo createInfo = {
			.flags = flags,
			.image = image,
			.viewType = viewType,
			.format = format,
			.subresourceRange = subresourceRange
		};
		return Create(createInfo);
	}
};

//Done
class sampler {
	VkSampler handle = VK_NULL_HANDLE;
public:
	sampler() = default;
	sampler(VkSamplerCreateInfo& createInfo) {
		Create(createInfo);
	}
	sampler(sampler&& other) noexcept { MoveHandle; }
	~sampler() { DestroyHandleBy(vkDestroySampler); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkSamplerCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		VkResult result = vkCreateSampler(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ sampler ] ERROR\nFailed to create a sampler!\nError code: {}\n", int32_t(result));
		return result;
	}
};
#pragma endregion

#pragma region Pipeline
//Done+
class shaderModule {
	VkShaderModule handle = VK_NULL_HANDLE;
public:
	shaderModule() = default;
	shaderModule(VkShaderModuleCreateInfo& createInfo) {
		Create(createInfo);
	}
	shaderModule(const char* filepath /*reserved for future use*/) {
		Create(filepath);
	}
	shaderModule(size_t codeSize, const uint32_t* pCode /*reserved for future use*/) {
		Create(codeSize, pCode);
	}
	shaderModule(shaderModule&& other) noexcept { MoveHandle; }
	~shaderModule() { DestroyHandleBy(vkDestroyShaderModule); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	VkPipelineShaderStageCreateInfo StageCreateInfo(VkShaderStageFlagBits stage, const char* entry = "main") const {
		return {
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,//sType
			nullptr,											//pNext
			0,													//flags
			stage,												//stage
			handle,												//module
			entry,												//pName
			nullptr												//pSpecializationInfo
		};
	}
	//Non-const Function
	result_t Create(VkShaderModuleCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		VkResult result = vkCreateShaderModule(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ shader ] ERROR\nFailed to create a shader module!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(const char* filepath /*reserved for future use*/) {
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if (!file) {
			OutputMessage("[ shader ] ERROR\nFailed to open the file: {}\n", filepath);
			return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
		}
		size_t fileSize = size_t(file.tellg());
		std::vector<uint32_t> binaries(fileSize / 4);
		file.seekg(0);
		file.read(reinterpret_cast<char*>(binaries.data()), fileSize);
		file.close();
		return Create(fileSize, binaries.data());
	}
	result_t Create(size_t codeSize, const uint32_t* pCode /*reserved for future use*/) {
		VkShaderModuleCreateInfo createInfo = {
			.codeSize = codeSize,
			.pCode = pCode
		};
		return Create(createInfo);
	}
};

//Done++
class descriptorSetLayout {
	VkDescriptorSetLayout handle = VK_NULL_HANDLE;
public:
	descriptorSetLayout() = default;
	descriptorSetLayout(VkDescriptorSetLayoutCreateInfo& createInfo) {
		Create(createInfo);
	}
	descriptorSetLayout(arrayRef<const VkDescriptorSetLayoutBinding> bindings, VkDescriptorSetLayoutCreateFlags flags = 0) {
		Create(bindings, flags);
	}
	descriptorSetLayout(descriptorSetLayout&& other) noexcept { MoveHandle; }
	~descriptorSetLayout() { DestroyHandleBy(vkDestroyDescriptorSetLayout); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkDescriptorSetLayoutCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		VkResult result = vkCreateDescriptorSetLayout(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ descriptorSetLayout ] ERROR\nFailed to create a descriptor set layout!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(arrayRef<const VkDescriptorSetLayoutBinding> bindings, VkDescriptorSetLayoutCreateFlags flags = 0) {
		VkDescriptorSetLayoutCreateInfo createInfo = {
			.flags = flags,
			.bindingCount = uint32_t(bindings.Count()),
			.pBindings = bindings
		};
		return Create(createInfo);
	}
};
//Done++
class pipelineLayout {
	VkPipelineLayout handle = VK_NULL_HANDLE;
public:
	pipelineLayout() = default;
	pipelineLayout(VkPipelineLayoutCreateInfo& createInfo) {
		Create(createInfo);
	}
	pipelineLayout(arrayRef<const VkDescriptorSetLayout> descriptorSetLayouts, arrayRef<const VkPushConstantRange> pushConstantRanges, VkPipelineLayoutCreateFlags flags = 0) {
		Create(descriptorSetLayouts, pushConstantRanges, flags);
	}
	pipelineLayout(arrayRef<const descriptorSetLayout> descriptorSetLayouts, arrayRef<const VkPushConstantRange> pushConstantRanges, VkPipelineLayoutCreateFlags flags = 0) {
		Create(descriptorSetLayouts, pushConstantRanges, flags);
	}
	pipelineLayout(emptyList, arrayRef<const VkPushConstantRange> pushConstantRanges, VkPipelineLayoutCreateFlags flags = 0) {
		Create({}, pushConstantRanges, flags);
	}
	pipelineLayout(pipelineLayout&& other) noexcept { MoveHandle; }
	~pipelineLayout() { DestroyHandleBy(vkDestroyPipelineLayout); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkPipelineLayoutCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkResult result = vkCreatePipelineLayout(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ pipelineLayout ] ERROR\nFailed to create a pipeline layout!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(arrayRef<const VkDescriptorSetLayout> descriptorSetLayouts, arrayRef<const VkPushConstantRange> pushConstantRanges, VkPipelineLayoutCreateFlags flags = 0) {
		VkPipelineLayoutCreateInfo createInfo = {
			.flags = flags,
			.setLayoutCount = uint32_t(descriptorSetLayouts.Count()),
			.pSetLayouts = descriptorSetLayouts,
			.pushConstantRangeCount = uint32_t(pushConstantRanges.Count()),
			.pPushConstantRanges = pushConstantRanges
		};
		return Create(createInfo);
	}
	result_t Create(arrayRef<const descriptorSetLayout> descriptorSetLayouts, arrayRef<const VkPushConstantRange> pushConstantRanges, VkPipelineLayoutCreateFlags flags = 0) {
		return Create({ descriptorSetLayouts[0].Address(), descriptorSetLayouts.Count() }, pushConstantRanges, flags);
	}
	result_t Create(emptyList, arrayRef<const VkPushConstantRange> pushConstantRanges, VkPipelineLayoutCreateFlags flags = 0) {
		return Create(arrayRef<const VkDescriptorSetLayout>{}, pushConstantRanges, flags);
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
	pipeline(VkComputePipelineCreateInfo& createInfo) {
		Create(createInfo);
	}
	pipeline(pipeline&& other) noexcept { MoveHandle; }
	~pipeline() { DestroyHandleBy(vkDestroyPipeline); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkGraphicsPipelineCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		VkResult result = vkCreateGraphicsPipelines(graphicsBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ pipeline ] ERROR\nFailed to create a graphics pipeline!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkComputePipelineCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		VkResult result = vkCreateComputePipelines(graphicsBase::Base().Device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ pipeline ] ERROR\nFailed to create a compute pipeline!\nError code: {}\n", int32_t(result));
		return result;
	}
};
#pragma endregion

#pragma region RenderPass & Framebuffer
//Done++
class renderPass {
	VkRenderPass handle = VK_NULL_HANDLE;
	//Static
	static constexpr const VkExtent2D& windowSize = graphicsBase::Base().SwapchainCreateInfo().imageExtent;
public:
	renderPass() = default;
	renderPass(VkRenderPassCreateInfo& createInfo) {
		Create(createInfo);
	}
	renderPass(arrayRef<const VkAttachmentDescription> attachmentDescriptions, arrayRef<const VkSubpassDescription> subpassDescriptions,
		arrayRef<const VkSubpassDependency> subpassDependencies, VkRenderPassCreateFlags flags = 0) {
		Create(attachmentDescriptions, subpassDescriptions, subpassDependencies, flags);
	}
	renderPass(renderPass&& other) noexcept { MoveHandle; }
	~renderPass() { DestroyHandleBy(vkDestroyRenderPass); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	void CmdBegin(VkCommandBuffer commandBuffer, VkRenderPassBeginInfo& beginInfo, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.renderPass = handle;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
	}
	void CmdBegin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkRect2D renderArea = { {}, windowSize }, arrayRef<const VkClearValue> clearValues = {}, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
		VkRenderPassBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = handle,
			.framebuffer = framebuffer,
			.renderArea = renderArea,
			.clearValueCount = uint32_t(clearValues.Count()),
			.pClearValues = clearValues
		};
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, subpassContents);
	}
	void CmdNext(VkCommandBuffer commandBuffer, VkSubpassContents subpassContents = VK_SUBPASS_CONTENTS_INLINE) const {
		vkCmdNextSubpass(commandBuffer, subpassContents);
	}
	void CmdEnd(VkCommandBuffer commandBuffer) const {
		vkCmdEndRenderPass(commandBuffer);
	}
	//Non-const Function
	result_t Create(VkRenderPassCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		VkResult result = vkCreateRenderPass(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ renderPass ] ERROR\nFailed to create a render pass!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(arrayRef<const VkAttachmentDescription> attachmentDescriptions, arrayRef<const VkSubpassDescription> subpassDescriptions,
		arrayRef<const VkSubpassDependency> subpassDependencies, VkRenderPassCreateFlags flags = 0) {
		VkRenderPassCreateInfo createInfo = {
			.flags = flags,
			.attachmentCount = uint32_t(attachmentDescriptions.Count()),
			.pAttachments = attachmentDescriptions,
			.subpassCount = uint32_t(subpassDescriptions.Count()),
			.pSubpasses = subpassDescriptions,
			.dependencyCount = uint32_t(subpassDependencies.Count()),
			.pDependencies = subpassDependencies
		};
		return Create(createInfo);
	}
};

//Done++
class framebuffer {
	VkFramebuffer handle = VK_NULL_HANDLE;
public:
	framebuffer() = default;
	framebuffer(VkFramebufferCreateInfo& createInfo) {
		Create(createInfo);
	}
	framebuffer(VkRenderPass renderPass, arrayRef<const VkImageView> attachments, VkExtent2D extent, uint32_t layerCount, VkFramebufferCreateFlags flags = 0) {
		Create(renderPass, attachments, extent, layerCount, flags);
	}
	framebuffer(VkRenderPass renderPass, arrayRef<const imageView> attachments, VkExtent2D extent, uint32_t layerCount, VkFramebufferCreateFlags flags = 0) {
		Create(renderPass, attachments, extent, layerCount, flags);
	}
	framebuffer(framebuffer&& other) noexcept { MoveHandle; }
	~framebuffer() { DestroyHandleBy(vkDestroyFramebuffer); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Non-const Function
	result_t Create(VkFramebufferCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		VkResult result = vkCreateFramebuffer(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ framebuffer ] ERROR\nFailed to create a framebuffer!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkRenderPass renderPass, arrayRef<const VkImageView> attachments, VkExtent2D extent, uint32_t layerCount, VkFramebufferCreateFlags flags = 0) {
		VkFramebufferCreateInfo createInfo = {
			.flags = flags,
			.renderPass = renderPass,
			.attachmentCount = uint32_t(attachments.Count()),
			.pAttachments = attachments,
			.width = extent.width,
			.height = extent.height,
			.layers = layerCount
		};
		return Create(createInfo);
	}
	result_t Create(VkRenderPass renderPass, arrayRef<const imageView> attachments, VkExtent2D extent, uint32_t layerCount, VkFramebufferCreateFlags flags = 0) {
		return Create(renderPass, { attachments[0].Address(), attachments.Count() }, extent, layerCount, flags);
	}
};
#pragma endregion

#pragma region Pool
//Done++
class commandBuffer {
	friend class commandPool;
	VkCommandBuffer handle = nullptr;//Dispatchable handle
public:
	commandBuffer() = default;
	commandBuffer(commandBuffer&& other) noexcept { MoveHandle; }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	result_t Begin(VkCommandBufferUsageFlags flags = 0, optionalRef<VkCommandBufferInheritanceInfo> inheritanceInfo = {}) const {
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = flags,
			.pInheritanceInfo = &inheritanceInfo
		};
		if (&inheritanceInfo)
			inheritanceInfo.Get().sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		VkResult result = vkBeginCommandBuffer(handle, &beginInfo);
		if (result)
			OutputMessage("[ commandBuffer ] ERROR\nFailed to begin a command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t End() const {
		VkResult result = vkEndCommandBuffer(handle);
		if (result)
			OutputMessage("[ commandBuffer ] ERROR\nFailed to end a command buffer!\nError code: {}\n", int32_t(result));
		return result;
	}
};
class commandPool {
	VkCommandPool handle = VK_NULL_HANDLE;
public:
	commandPool() = default;
	commandPool(VkCommandPoolCreateInfo& createInfo) {
		Create(createInfo);
	}
	commandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
		Create(queueFamilyIndex, flags);
	}
	commandPool(commandPool&& other) noexcept { MoveHandle; }
	~commandPool() { DestroyHandleBy(vkDestroyCommandPool); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	result_t AllocateBuffers(arrayRef<VkCommandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
		VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = handle,
			.level = level,
			.commandBufferCount = uint32_t(buffers.Count())
		};
		VkResult result = vkAllocateCommandBuffers(graphicsBase::Base().Device(), &allocateInfo, buffers);
		if (result)
			OutputMessage("[ commandPool ] ERROR\nFailed to allocate command buffers!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t AllocateBuffers(arrayRef<commandBuffer> buffers, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const {
		return AllocateBuffers({ &buffers[0].handle, buffers.Count() }, level);
	}
	void FreeBuffers(arrayRef<VkCommandBuffer> buffers) const {
		vkFreeCommandBuffers(graphicsBase::Base().Device(), handle, buffers.Count(), buffers);
		memset(buffers, 0, buffers.Count() * sizeof(VkCommandBuffer));
	}
	void FreeBuffers(arrayRef<commandBuffer> buffers) const {
		FreeBuffers({ &buffers[0].handle, buffers.Count() });
	}
	//Non-const Function
	result_t Create(VkCommandPoolCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		VkResult result = vkCreateCommandPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ commandPool ] ERROR\nFailed to create a command pool!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
		VkCommandPoolCreateInfo createInfo = {
			.flags = flags,
			.queueFamilyIndex = queueFamilyIndex
		};
		return Create(createInfo);
	}
};

//Done++
class descriptorSet {
	friend class descriptorPool;
	VkDescriptorSet handle = VK_NULL_HANDLE;
public:
	descriptorSet() = default;
	descriptorSet(descriptorSet&& other) noexcept { MoveHandle; }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	void Write(arrayRef<const VkDescriptorImageInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
		VkWriteDescriptorSet writeDescriptorSet = {
			.dstSet = handle,
			.dstBinding = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = uint32_t(descriptorInfos.Count()),
			.descriptorType = descriptorType,
			.pImageInfo = descriptorInfos
		};
		Update(writeDescriptorSet);
	}
	void Write(arrayRef<const VkDescriptorBufferInfo> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
		VkWriteDescriptorSet writeDescriptorSet = {
			.dstSet = handle,
			.dstBinding = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = uint32_t(descriptorInfos.Count()),
			.descriptorType = descriptorType,
			.pBufferInfo = descriptorInfos
		};
		Update(writeDescriptorSet);
	}
	void Write(arrayRef<const VkBufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
		VkWriteDescriptorSet writeDescriptorSet = {
			.dstSet = handle,
			.dstBinding = dstBinding,
			.dstArrayElement = dstArrayElement,
			.descriptorCount = uint32_t(descriptorInfos.Count()),
			.descriptorType = descriptorType,
			.pTexelBufferView = descriptorInfos
		};
		Update(writeDescriptorSet);
	}
	void Write(arrayRef<const bufferView> descriptorInfos, VkDescriptorType descriptorType, uint32_t dstBinding = 0, uint32_t dstArrayElement = 0) const {
		Write({ descriptorInfos[0].Address(), descriptorInfos.Count() }, descriptorType, dstBinding, dstArrayElement);
	}
	//Static Function
	static void Update(arrayRef<VkWriteDescriptorSet> writes, arrayRef<VkCopyDescriptorSet> copies = {}) {
		for (auto& i : writes)
			i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		for (auto& i : copies)
			i.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
		vkUpdateDescriptorSets(graphicsBase::Base().Device(), writes.Count(), writes, copies.Count(), copies);
	}
};
class descriptorPool {
	VkDescriptorPool handle = VK_NULL_HANDLE;
public:
	descriptorPool() = default;
	descriptorPool(VkDescriptorPoolCreateInfo& createInfo) {
		Create(createInfo);
	}
	descriptorPool(uint32_t maxSetCount, arrayRef<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0) {
		Create(maxSetCount, poolSizes, flags);
	}
	descriptorPool(descriptorPool&& other) noexcept { MoveHandle; }
	~descriptorPool() { DestroyHandleBy(vkDestroyDescriptorPool); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	result_t AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const {
		if (sets.Count() != setLayouts.Count())
			if (sets.Count() < setLayouts.Count()) {
				OutputMessage("[ descriptorPool ] ERROR\nFor each descriptor set, must provide a corresponding layout!\n");
				return VK_RESULT_MAX_ENUM;//No proper VkResult enum value, don't use VK_ERROR_UNKNOWN
			}
			else
				OutputMessage("[ descriptorPool ] WARNING\nProvided layouts are more than sets!\n");
		VkDescriptorSetAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = handle,
			.descriptorSetCount = uint32_t(sets.Count()),
			.pSetLayouts = setLayouts
		};
		VkResult result = vkAllocateDescriptorSets(graphicsBase::Base().Device(), &allocateInfo, sets);
		if (result)
			OutputMessage("[ descriptorPool ] ERROR\nFailed to allocate descriptor sets!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t AllocateSets(arrayRef<VkDescriptorSet> sets, arrayRef<const descriptorSetLayout> setLayouts) const {
		return AllocateSets(sets, { setLayouts[0].Address(), setLayouts.Count() });
	}
	result_t AllocateSets(arrayRef<descriptorSet> sets, arrayRef<const VkDescriptorSetLayout> setLayouts) const {
		return AllocateSets({ &sets[0].handle, sets.Count() }, setLayouts);
	}
	result_t AllocateSets(arrayRef<descriptorSet> sets, arrayRef<const descriptorSetLayout> setLayouts) const {
		return AllocateSets({ &sets[0].handle, sets.Count() }, { setLayouts[0].Address(), setLayouts.Count() });
	}
	result_t FreeSets(arrayRef<VkDescriptorSet> sets) const {
		VkResult result = vkFreeDescriptorSets(graphicsBase::Base().Device(), handle, sets.Count(), sets);
		memset(sets, 0, sets.Count() * sizeof(VkDescriptorSet));
		return result;//Though vkFreeDescriptorSets(...) can only return VK_SUCCESS
	}
	result_t FreeSets(arrayRef<descriptorSet> sets) const {
		return FreeSets({ &sets[0].handle, sets.Count() });
	}
	//Non-const Function
	result_t Create(VkDescriptorPoolCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		VkResult result = vkCreateDescriptorPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ descriptorPool ] ERROR\nFailed to create a descriptor pool!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(uint32_t maxSetCount, arrayRef<const VkDescriptorPoolSize> poolSizes, VkDescriptorPoolCreateFlags flags = 0) {
		VkDescriptorPoolCreateInfo createInfo = {
			.flags = flags,
			.maxSets = maxSetCount,
			.poolSizeCount = uint32_t(poolSizes.Count()),
			.pPoolSizes = poolSizes
		};
		return Create(createInfo);
	}
};

//Done+
class queryPool {
	VkQueryPool handle = VK_NULL_HANDLE;
public:
	queryPool() = default;
	queryPool(VkQueryPoolCreateInfo& createInfo) {
		Create(createInfo);
	}
	queryPool(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics = 0 /*reserved for future use*/) {
		Create(queryType, queryCount, pipelineStatistics);
	}
	queryPool(queryPool&& other) noexcept { MoveHandle; }
	~queryPool() { DestroyHandleBy(vkDestroyQueryPool); }
	//Getter
	DefineHandleTypeOperator;
	DefineAddressFunction;
	//Const Function
	void CmdReset(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount) const {
		vkCmdResetQueryPool(commandBuffer, handle, firstQueryIndex, queryCount);
	}
	void CmdBegin(VkCommandBuffer commandBuffer, uint32_t queryIndex, VkQueryControlFlags flags = 0) const {
		vkCmdBeginQuery(commandBuffer, handle, queryIndex, flags);
	}
	void CmdEnd(VkCommandBuffer commandBuffer, uint32_t queryIndex) const {
		vkCmdEndQuery(commandBuffer, handle, queryIndex);
	}
	void CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, uint32_t queryIndex) const {
		vkCmdWriteTimestamp(commandBuffer, pipelineStage, handle, queryIndex);
	}
	void CmdCopyResults(VkCommandBuffer commandBuffer, uint32_t firstQueryIndex, uint32_t queryCount,
		VkBuffer buffer_dst, VkDeviceSize offset_dst, VkDeviceSize stride, VkQueryResultFlags flags = 0) const {
		vkCmdCopyQueryPoolResults(commandBuffer, handle, firstQueryIndex, queryCount, buffer_dst, offset_dst, stride, flags);
	}
	result_t GetResults(uint32_t firstQueryIndex, uint32_t queryCount, size_t dataSize, void* pData_dst, VkDeviceSize stride, VkQueryResultFlags flags = 0) const {
		VkResult result = vkGetQueryPoolResults(graphicsBase::Base().Device(), handle, firstQueryIndex, queryCount, dataSize, pData_dst, stride, flags);
		if (result)
			result > 0 ?
			OutputMessage("[ queryPool ] WARNING\nNot all queries are available!\nError code: {}\n", int32_t(result)) :
			OutputMessage("[ queryPool ] ERROR\nFailed to get query pool results!\nError code: {}\n", int32_t(result));
		return result;
	}
	/*Provided by VK_API_VERSION_1_2*/
	void Reset(uint32_t firstQueryIndex, uint32_t queryCount) {
		vkResetQueryPool(graphicsBase::Base().Device(), handle, firstQueryIndex, queryCount);
	}
	//Non-const Function
	result_t Create(VkQueryPoolCreateInfo& createInfo) {
		createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		VkResult result = vkCreateQueryPool(graphicsBase::Base().Device(), &createInfo, nullptr, &handle);
		if (result)
			OutputMessage("[ queryPool ] ERROR\nFailed to create a query pool!\nError code: {}\n", int32_t(result));
		return result;
	}
	result_t Create(VkQueryType queryType, uint32_t queryCount, VkQueryPipelineStatisticFlags pipelineStatistics = 0 /*reserved for future use*/) {
		VkQueryPoolCreateInfo createInfo = {
			.queryType = queryType,
			.queryCount = queryCount,
			.pipelineStatistics = pipelineStatistics
		};
		return Create(createInfo);
	}
};
#pragma endregion
NAMESPACE_END