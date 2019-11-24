#ifndef _VULKAN_DEBUG_H_
#define _VULKAN_DEBUG_H_

#include <Windows.h>
#include <vulkan/vulkan.h>

VKAPI_ATTR VkBool32 VKAPI_CALL messageCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t srcObject,
	size_t location,
	int32_t msgCode,
	const char* pLayerPrefix,
	const char* pMsg,
	void* pUserData);

class VulkanDebug
{
public:
	static const int validation_layer_count = 1;
	static const char * validation_layer_names[1];
	static VulkanDebug* Instance()
	{
		if (instance_ == NULL)
			instance_ = new VulkanDebug();
		return instance_;
	}

private:
	static VulkanDebug* instance_;

public:

	void setupDebugging(
		VkInstance instance,
		VkDebugReportFlagsEXT flags,
		VkDebugReportCallbackEXT callBack);

	void freeDebugCallback(VkInstance instance);
};



#endif