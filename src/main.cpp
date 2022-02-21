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
const auto g_enable_validation_layers = false;
#else
const auto g_enable_validation_layers = true;
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
			extension_count + static_cast<unsigned int>(g_enable_validation_layers));
	extensions.assign(span.begin(), span.end());
	if (g_enable_validation_layers) {
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

auto application_info() -> VkApplicationInfo {
	auto info = VkApplicationInfo{};
	info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	info.pNext = nullptr;
	info.pApplicationName = g_application_name;
	info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	info.pEngineName = nullptr;
	info.engineVersion = VK_MAKE_VERSION(0, 0, 0);
	info.apiVersion = VK_API_VERSION_1_0;
	return info;
}

auto instance_info(
		const VkDebugUtilsMessengerCreateInfoEXT* debug_info,
		const std::span<const char*>& extensions,
		const VkApplicationInfo* app_info) -> VkInstanceCreateInfo {
	auto info = VkInstanceCreateInfo{};
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	info.flags = 0;
	info.pApplicationInfo = app_info;
	if (g_enable_validation_layers) {
		info.pNext = debug_info;
		verify_validation_layers_supported();
		info.enabledLayerCount = static_cast<uint32_t>(g_validation_layers.size());
		info.ppEnabledLayerNames = g_validation_layers.data();
	} else {
		info.pNext = nullptr;
		info.enabledLayerCount = 0;
		info.ppEnabledLayerNames = nullptr;
	}
	info.enabledExtensionCount = extensions.size();
	info.ppEnabledExtensionNames = extensions.data();
	return info;
}

auto debug_messenger_info() -> VkDebugUtilsMessengerCreateInfoEXT {
	auto info = VkDebugUtilsMessengerCreateInfoEXT{};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info.pNext = nullptr;
	info.flags = 0;
	info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	info.pfnUserCallback = vk_debug_callback;
	return info;
}

class VulkanApplication {
 public:
	void run() {
		init_window();
		init_vulkan();
		loop();
		cleanup();
	}

 private:
	GLFWwindow* _window;
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;

	void init_window() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(
				g_window_width,
				g_window_height,
				g_application_name,
				nullptr,
				nullptr);
	}

	void init_vulkan() {
		create_instance();
		setup_debug_messenger();
	}

	void create_instance() {
		auto app_info = application_info();
		auto debug_info = debug_messenger_info();
		auto extensions = required_extensions();
		auto inst_info = instance_info(&debug_info, extensions, &app_info);
		if (vkCreateInstance(&inst_info, nullptr, &_instance) != VK_SUCCESS) {
			fmt::print(stderr, "Failed to create vulkan instance\n");
			std::terminate();
		}
	}

	void setup_debug_messenger() {
		if (!g_enable_validation_layers) {
			return;
		}
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				get_vk_func(_instance, "vkCreateDebugUtilsMessengerEXT"));
		auto info = debug_messenger_info();
		if (func(_instance, &info, nullptr, &_debug_messenger) != VK_SUCCESS) {
			fmt::print(stderr, "Failed to setup vulkan debug callback\n");
			std::terminate();
		}
	}

	void loop() {
		while (glfwWindowShouldClose(_window) == 0) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		cleanup_debug_messenger();
		vkDestroyInstance(_instance, nullptr);
		glfwDestroyWindow(_window);
		glfwTerminate();
	}

	void cleanup_debug_messenger() {
		if (!g_enable_validation_layers) {
			return;
		}
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				get_vk_func(_instance, "vkDestroyDebugUtilsMessengerEXT"));
		func(_instance, _debug_messenger, nullptr);
	}
};

auto main() -> int {
	auto app = VulkanApplication{};
	app.run();
	return EXIT_SUCCESS;
}
