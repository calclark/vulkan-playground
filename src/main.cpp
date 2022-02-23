#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include <memory>
#include <optional>
#include <set>
#include <span>
#include <vector>
#include <cstdio>
#include <cstdlib>

constexpr auto g_application_name = "vulkan-demo";
constexpr auto g_window_width = 800;
constexpr auto g_window_height = 600;

void glfw_error_callback(int error, const char* description) {
	fmt::print(stderr, "GLFW error {}: {}\n", error, description);
	std::terminate();
}

auto VKAPI_PTR vk_diagnostic_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
		VkDebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* /*pUserData*/) -> VkBool32 {
	fmt::print(stderr, "{}\n", pCallbackData->pMessage);
	return VK_FALSE;
}

void glfw_key_callback(
		GLFWwindow* window,
		int key,
		int /*scancode*/,
		int /*action*/,
		int /*mods*/) {
	switch (key) {
		case GLFW_KEY_Q:
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

auto vk_func(const VkInstance& instance, const char* func_name)
		-> PFN_vkVoidFunction {
	auto func = vkGetInstanceProcAddr(instance, func_name);
	if (func == nullptr) {
		fmt::print(stderr, "Vulkan function not found: {}\n", func_name);
		std::terminate();
	}
	return func;
}

auto main() -> int {
	glfwSetErrorCallback(glfw_error_callback);
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	auto* window = glfwCreateWindow(
			g_window_width,
			g_window_height,
			g_application_name,
			nullptr,
			nullptr);

	auto application_info = VkApplicationInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext = VK_NULL_HANDLE,
			.pApplicationName = g_application_name,
			.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
			.pEngineName = VK_NULL_HANDLE,
			.engineVersion = VK_MAKE_VERSION(0, 0, 0),
			.apiVersion = VK_API_VERSION_1_0};

	auto extension_count = uint32_t{};
	auto* required_extensions =
			glfwGetRequiredInstanceExtensions(&extension_count);
	auto span = std::span(required_extensions, extension_count);
	auto extensions = std::vector<const char*>(extension_count);
	extensions.assign(span.begin(), span.end());

#ifdef USE_VALIDATION_LAYERS
	extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	auto debug_info = VkDebugUtilsMessengerCreateInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = VK_NULL_HANDLE,
			.flags = 0,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vk_diagnostic_callback,
			.pUserData = VK_NULL_HANDLE};

	auto validation_layers =
			std::vector<const char*>{"VK_LAYER_KHRONOS_validation"};
#endif

	auto instance_info = VkInstanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef USE_VALIDATION_LAYERS
			.pNext = &debug_info,
#else
			.pNext = VK_NULL_HANDLE,
#endif
			.flags = 0,
			.pApplicationInfo = &application_info,
#ifdef USE_VALIDATION_LAYERS
			.enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
			.ppEnabledLayerNames = validation_layers.data(),
#else
			.enabledLayerCount = 0,
			.ppEnabledLayerNames = VK_NULL_HANDLE,
#endif
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()};

	auto* instance = VkInstance{};
	if (vkCreateInstance(&instance_info, VK_NULL_HANDLE, &instance) !=
			VK_SUCCESS) {
		fmt::print(stderr, "Failed to create vulkan instance\n");
		std::terminate();
	}

#ifdef USE_VALIDATION_LAYERS
	auto vkCreateDebugDebugUtilsMessengerExt =
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
					vk_func(instance, "vkCreateDebugUtilsMessengerEXT"));
	auto* messenger = VkDebugUtilsMessengerEXT{};
	if (vkCreateDebugDebugUtilsMessengerExt(
					instance,
					&debug_info,
					nullptr,
					&messenger) != VK_SUCCESS) {
		fmt::print(stderr, "Failed to setup vulkan debug callback\n");
		std::terminate();
	}
#endif

	auto* surface = VkSurfaceKHR{};
	if (glfwCreateWindowSurface(instance, window, VK_NULL_HANDLE, &surface) !=
			VK_SUCCESS) {
		fmt::print(stderr, "Failed to create a window surface\n");
		std::terminate();
	}

	auto device_count = uint32_t{};
	vkEnumeratePhysicalDevices(instance, &device_count, VK_NULL_HANDLE);
	auto physical_devices = std::vector<VkPhysicalDevice>(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data());

	struct PhysicalDeviceInfo {
		VkPhysicalDevice device{};
		std::optional<uint32_t> graphics_family_idx;
		std::optional<uint32_t> present_family_idx;
		bool discrete{};
	};

	auto vkGetPhysicalDeviceSurfaceSupportKHR =
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
					vk_func(instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));

	auto devices_info = std::vector<PhysicalDeviceInfo>(device_count);
	for (auto& candidate_device : physical_devices) {
		auto physical_device_info = PhysicalDeviceInfo{};
		physical_device_info.device = candidate_device;
		auto queue_family_count = uint32_t{};
		vkGetPhysicalDeviceQueueFamilyProperties(
				candidate_device,
				&queue_family_count,
				nullptr);
		auto queue_families =
				std::vector<VkQueueFamilyProperties>(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(
				candidate_device,
				&queue_family_count,
				queue_families.data());
		auto idx = uint32_t{};
		for (auto& queue_family : queue_families) {
			if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
				physical_device_info.graphics_family_idx = idx;
			}
			auto supports_present = VkBool32{};
			vkGetPhysicalDeviceSurfaceSupportKHR(
					candidate_device,
					idx,
					surface,
					&supports_present);
			if (supports_present == VK_TRUE) {
				physical_device_info.present_family_idx = idx;
			}
			idx++;
		}
		auto props = VkPhysicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(candidate_device, &props);
		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			physical_device_info.discrete = true;
		}
		devices_info.emplace_back(physical_device_info);
	}

	auto physical_device_info = PhysicalDeviceInfo{};
	for (auto& device : devices_info) {
		if (device.graphics_family_idx.has_value() &&
				device.present_family_idx.has_value() &&
				!physical_device_info.discrete) {
			physical_device_info = device;
		}
	}
	if (physical_device_info.device == nullptr) {
		fmt::print(stderr, "Failed to find a suitable physical device\n");
		std::terminate();
	}

	auto queue_create_infos = std::vector<VkDeviceQueueCreateInfo>{};
	auto unique_queue_families = std::set<uint32_t>{
			*physical_device_info.present_family_idx,
			*physical_device_info.graphics_family_idx};
	auto queue_priority = 1.0F;
	for (auto queue_family : unique_queue_families) {
		auto device_queue_info = VkDeviceQueueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = VK_NULL_HANDLE,
				.flags = 0,
				.queueFamilyIndex = queue_family,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority};
		queue_create_infos.emplace_back(device_queue_info);
	}
	auto device_info = VkDeviceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = VK_NULL_HANDLE,
			.flags = 0,
			.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size()),
			.pQueueCreateInfos = queue_create_infos.data(),
			.enabledLayerCount = 0,  // ignored
			.ppEnabledLayerNames = VK_NULL_HANDLE,  // ignored
			.enabledExtensionCount = 0,
			.ppEnabledExtensionNames = VK_NULL_HANDLE,
			.pEnabledFeatures = VK_NULL_HANDLE};
	auto* device = VkDevice{};
	if (vkCreateDevice(
					physical_device_info.device,
					&device_info,
					VK_NULL_HANDLE,
					&device) != VK_SUCCESS) {
		fmt::print(stderr, "Failed to create a logical device\n");
		std::terminate();
	}

	auto* graphics_queue = VkQueue{};
	vkGetDeviceQueue(
			device,
			*physical_device_info.graphics_family_idx,
			0,
			&graphics_queue);

	auto* present_queue = VkQueue{};
	vkGetDeviceQueue(
			device,
			*physical_device_info.present_family_idx,
			0,
			&present_queue);

	glfwSetKeyCallback(window, glfw_key_callback);
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		glfwPollEvents();
	}

	vkDestroyDevice(device, VK_NULL_HANDLE);
	vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
#ifdef USE_VALIDATION_LAYERS
	auto vkDestroyDebugUtilsMessengerExt =
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
					vk_func(instance, "vkDestroyDebugUtilsMessengerEXT"));
	vkDestroyDebugUtilsMessengerExt(instance, messenger, nullptr);
#endif
	vkDestroyInstance(instance, VK_NULL_HANDLE);
	glfwTerminate();
	return EXIT_SUCCESS;
}
