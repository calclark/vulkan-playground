#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include <memory>
#include <span>
#include <vector>
#include <cstdio>
#include <cstdlib>

#ifdef NDEBUG
#else
#define DEBUG
#endif

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

#ifdef DEBUG
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
#ifdef DEBUG
			.pNext = &debug_info,
#else
			.pNext = VK_NULL_HANDLE,
#endif
			.flags = 0,
			.pApplicationInfo = &application_info,
#ifdef DEBUG
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

#ifdef DEBUG
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

	glfwSetKeyCallback(window, glfw_key_callback);
	while (glfwWindowShouldClose(window) == GLFW_FALSE) {
		glfwPollEvents();
	}

#ifdef DEBUG
	auto bkDestroyDebugUtilsMessengerExt =
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
					vk_func(instance, "vkDestroyDebugUtilsMessengerEXT"));
	bkDestroyDebugUtilsMessengerExt(instance, messenger, nullptr);
#endif
	vkDestroyInstance(instance, VK_NULL_HANDLE);
	glfwTerminate();
	return EXIT_SUCCESS;
}
