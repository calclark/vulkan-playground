#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include <memory>
#include <span>
#include <vector>
#include <cstdio>
#include <cstdlib>

constexpr auto g_application_name = "vulkan-demo";
constexpr auto g_window_width = 800;
constexpr auto g_window_height = 600;

const auto g_validation_layers =
		std::vector<const char*>{"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const auto g_debug = false;
#else
const auto g_debug = true;
#endif

VKAPI_ATTR auto VKAPI_CALL vk_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
		VkDebugUtilsMessageTypeFlagsEXT /*type*/,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* /*user_data*/) -> VkBool32 {
	fmt::print(stderr, "{}\n", data->pMessage);
	return VK_FALSE;
}

auto get_vk_func(const VkInstance& instance, const char* func_name)
		-> PFN_vkVoidFunction {
	auto func = vkGetInstanceProcAddr(instance, func_name);
	if (func == nullptr) {
		fmt::print(stderr, "Vulkan extension {} not found\n", func_name);
		std::terminate();
	}
	return func;
}

void verify_validation_layers_supported() {
	auto layer_count = uint32_t{};
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	auto available_layers = std::vector<VkLayerProperties>(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
	for (const auto* layer_name : g_validation_layers) {
		auto layer_found = false;
		for (const auto& layer_properties : available_layers) {
			if (strcmp(
							layer_name,
							static_cast<const char*>(layer_properties.layerName)) == 0) {
				layer_found = true;
				break;
			}
		}
		if (!layer_found) {
			fmt::print(stderr, "Failed to find validation layer: {}", layer_name);
			std::terminate();
		}
	}
}

auto required_extensions() -> std::vector<const char*> {
	auto extension_count = uint32_t{};
	const auto** glfw_extensions =
			glfwGetRequiredInstanceExtensions(&extension_count);
	auto span = std::span(glfw_extensions, extension_count);
	auto extensions = std::vector<const char*>(
			extension_count + static_cast<unsigned int>(g_debug));
	extensions.assign(span.begin(), span.end());
	if (g_debug) {
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

constexpr auto g_application_info = VkApplicationInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = g_application_name,
		.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
		.pEngineName = nullptr,
		.engineVersion = VK_MAKE_VERSION(0, 0, 0),
		.apiVersion = VK_API_VERSION_1_2};

constexpr auto g_debug_messenger_info = VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
		.pfnUserCallback = vk_debug_callback,
		.pUserData = nullptr};

auto instance_info(const std::span<const char*>& extensions)
		-> VkInstanceCreateInfo {
	if (g_debug) {
		verify_validation_layers_supported();
	}
	return VkInstanceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = g_debug ? &g_debug_messenger_info : nullptr,
			.flags = 0,
			.pApplicationInfo = &g_application_info,
			.enabledLayerCount =
					g_debug ? static_cast<uint32_t>(g_validation_layers.size()) : 0,
			.ppEnabledLayerNames = g_debug ? g_validation_layers.data() : nullptr,
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()};
}

auto init_window() -> GLFWwindow* {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	auto* window = glfwCreateWindow(
			g_window_width,
			g_window_height,
			g_application_name,
			nullptr,
			nullptr);
	return window;
}

auto create_instance() -> VkInstance {
	auto extensions = required_extensions();
	auto inst_info = instance_info(extensions);
	auto* instance = VkInstance{};
	if (vkCreateInstance(&inst_info, nullptr, &instance) != VK_SUCCESS) {
		fmt::print(stderr, "Failed to create vulkan instance\n");
		std::terminate();
	}
	return instance;
}

auto setup_debug_messenger(const VkInstance& instance)
		-> VkDebugUtilsMessengerEXT {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			get_vk_func(instance, "vkCreateDebugUtilsMessengerEXT"));
	auto* messenger = VkDebugUtilsMessengerEXT{};
	if (func(instance, &g_debug_messenger_info, nullptr, &messenger) !=
			VK_SUCCESS) {
		fmt::print(stderr, "Failed to setup vulkan debug callback\n");
		std::terminate();
	}
	return messenger;
}

void init_vulkan(VkInstance& instance, VkDebugUtilsMessengerEXT& messenger) {
	instance = create_instance();
	messenger = setup_debug_messenger(instance);
}

void loop(GLFWwindow* window) {
	while (glfwWindowShouldClose(window) == 0) {
		glfwPollEvents();
	}
}

void cleanup_debug_messenger(
		const VkInstance& instance,
		const VkDebugUtilsMessengerEXT& messenger) {
	if (!g_debug) {
		return;
	}
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
			get_vk_func(instance, "vkDestroyDebugUtilsMessengerEXT"));
	func(instance, messenger, nullptr);
}

void cleanup(
		GLFWwindow* window,
		const VkInstance& instance,
		const VkDebugUtilsMessengerEXT& messenger) {
	cleanup_debug_messenger(instance, messenger);
	vkDestroyInstance(instance, nullptr);
	glfwDestroyWindow(window);
	glfwTerminate();
}

void run() {
	auto* window = init_window();
	auto* instance = VkInstance{};
	auto* messenger = VkDebugUtilsMessengerEXT{};
	init_vulkan(instance, messenger);
	loop(window);
	cleanup(window, instance, messenger);
}

auto main() -> int {
	run();
	return EXIT_SUCCESS;
}
