#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fmt/core.h>
#include <vector>
#include <span>
#include <cstdio>
#include <cstdlib>

constexpr auto g_window_width = 800;
constexpr auto g_window_height = 600;

const auto g_validation_layers =
		std::vector<const char*>{"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const auto g_enable_validation_layers = false;
#else
const auto g_enable_validation_layers = true;
#endif

auto check_validation_layer_support() -> bool {
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
			return false;
		}
	}
	return true;
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

VKAPI_ATTR auto VKAPI_CALL vk_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
		VkDebugUtilsMessageTypeFlagsEXT /*type*/,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void* /*user_data*/) -> VkBool32 {
	fmt::print(stderr, "{}\n", data->pMessage);
	return VK_FALSE;
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
				"vulkan-demo",
				nullptr,
				nullptr);
	}

	void init_vulkan() {
		create_instance();
		setup_debug_messenger();
	}

	void create_instance() {
		if (g_enable_validation_layers && !check_validation_layer_support()) {
			fmt::print(stderr, "Failed to find validation layers\n");
			std::terminate();
		}
		auto create_info = VkInstanceCreateInfo{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		auto extensions = required_extensions();
		create_info.enabledExtensionCount = extensions.size();
		create_info.ppEnabledExtensionNames = extensions.data();
		if (g_enable_validation_layers) {
			create_info.enabledLayerCount =
					static_cast<uint32_t>(g_validation_layers.size());
			create_info.ppEnabledLayerNames = g_validation_layers.data();
		}
		if (vkCreateInstance(&create_info, nullptr, &_instance) != VK_SUCCESS) {
			fmt::print(stderr, "Failed to create vulkan instance\n");
			std::terminate();
		}
	}

	void setup_debug_messenger() {
		if (!g_enable_validation_layers) {
			return;
		}
		auto info = VkDebugUtilsMessengerCreateInfoEXT{};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
													 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
											 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
											 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		info.pfnUserCallback = vk_debug_callback;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT"));
		if (func == nullptr) {
			fmt::print(
					stderr,
					"Vulkan extension vkCreateDebugUtilsMessengerEXT not found\n");
			std::terminate();
		}
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
				vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (func == nullptr) {
			fmt::print(
					stderr,
					"Vulkan extension vkDestroyDebugUtilsMessengerExt not found\n");
			std::terminate();
		}
		func(_instance, _debug_messenger, nullptr);
	}
};

auto main() -> int {
	auto app = VulkanApplication{};
	app.run();
	return EXIT_SUCCESS;
}
