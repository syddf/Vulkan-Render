#ifndef  _VULKAN_DEVICE_HPP_
#define _VULKAN_DEVICE_HPP_

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <fstream>

#include "VulkanBuffer.hpp"

class VulkanDevice
{
public:
	struct
	{
		uint32_t graphics = -1;
		uint32_t compute = -1;
		uint32_t transfer = -1;
	}QueueFamilyIndices;

private:
	VkPhysicalDevice physical_device_;
	VkDevice logical_device_;
	VkPhysicalDeviceProperties device_properties_;
	VkPhysicalDeviceFeatures device_features_;
	VkPhysicalDeviceFeatures device_enabled_features;
	VkPhysicalDeviceMemoryProperties device_memory_properties_;

	std::vector<VkQueueFamilyProperties> queue_family_properties_;
	std::vector<std::string> supported_extensions_name_;
	VkCommandPool command_pool_;

public:
	VulkanDevice(const VkPhysicalDevice physical_device)
	{
		this->physical_device_ = physical_device;

		vkGetPhysicalDeviceProperties(physical_device_, &device_properties_);
		vkGetPhysicalDeviceFeatures(physical_device_, &device_features_);
		vkGetPhysicalDeviceMemoryProperties(physical_device_, &device_memory_properties_);

		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, NULL);
		if (queue_family_count > 0)
		{
			queue_family_properties_.resize(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_family_properties_.data());
		}

		uint32_t extension_count;
		vkEnumerateDeviceExtensionProperties(physical_device_, NULL, &extension_count, NULL);
		if (extension_count > 0)
		{
			std::vector<VkExtensionProperties> extensions;
			extensions.resize(extension_count);
			vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, extensions.data());

			for (auto iter : extensions)
			{
				supported_extensions_name_.push_back(iter.extensionName);
			}
		}

	}

	int GetQueueFamilyIndex(VkQueueFlagBits queue_flag)
	{
		if ((queue_flag & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT ||
			(queue_flag & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
		{
			for (int i = 0; i < queue_family_properties_.size(); i++)
			{
				if ((queue_family_properties_[i].queueFlags & queue_flag) && (queue_family_properties_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
				{
					return i;
				}
			}
		}

		for (int i = 0; i < queue_family_properties_.size(); i++)
		{
			if ((queue_family_properties_[i].queueFlags & queue_flag) == queue_flag)
			{
				return i;
			}
		}

		return -1;
	}

	void CreateLogicalDevice(VkPhysicalDeviceFeatures physical_device_features, std::vector<const char*> enabledExtensions, VkQueueFlags init_queue_bits)
	{
		const float defaultQueuePriority(0.0f);
		std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;

		VkDeviceCreateInfo device_create_info = {};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.pNext = NULL;
		device_create_info.pEnabledFeatures = &physical_device_features;

		if (init_queue_bits & VK_QUEUE_GRAPHICS_BIT)
		{
			int ind = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
			QueueFamilyIndices.graphics = ind;
			VkDeviceQueueCreateInfo queue_create_info = {};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.pQueuePriorities = &defaultQueuePriority;
			queue_create_info.queueCount = 1;
			queue_create_info.queueFamilyIndex = ind;

			device_queue_create_infos.push_back(queue_create_info);
		}

		if (init_queue_bits & VK_QUEUE_COMPUTE_BIT)
		{
			int ind = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
			QueueFamilyIndices.compute = ind;
			if (ind != QueueFamilyIndices.graphics)
			{
				VkDeviceQueueCreateInfo queue_create_info = {};
				queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_info.pQueuePriorities = &defaultQueuePriority;
				queue_create_info.queueCount = 1;
				queue_create_info.queueFamilyIndex = ind;

				device_queue_create_infos.push_back(queue_create_info);
			}
		}

		if (init_queue_bits & VK_QUEUE_TRANSFER_BIT)
		{
			int ind = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
			QueueFamilyIndices.transfer = ind;
			if (ind != QueueFamilyIndices.graphics && ind != QueueFamilyIndices.compute)
			{
				VkDeviceQueueCreateInfo queue_create_info = {};
				queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queue_create_info.pQueuePriorities = &defaultQueuePriority;
				queue_create_info.queueCount = 1;
				queue_create_info.queueFamilyIndex = ind;

				device_queue_create_infos.push_back(queue_create_info);
			}
		}

		device_create_info.queueCreateInfoCount = device_queue_create_infos.size();
		device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
		device_create_info.enabledExtensionCount = enabledExtensions.size();
		device_create_info.ppEnabledExtensionNames = enabledExtensions.data();

		if (vkCreateDevice(physical_device_, &device_create_info, NULL, &logical_device_) != VK_SUCCESS)
		{
			throw "create device error . ";
		}

		device_enabled_features = physical_device_features;
		VkCommandPoolCreateInfo command_pool_create_info = {};
		command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_create_info.queueFamilyIndex = QueueFamilyIndices.graphics;
		command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		vkCreateCommandPool(logical_device_, &command_pool_create_info, NULL, &command_pool_);
	}

	bool SupportExtension(std::string extension_name)
	{
		return std::find(supported_extensions_name_.begin(), supported_extensions_name_.end(), extension_name) != supported_extensions_name_.end();
	}

	VkPhysicalDevice GetPhysicalDevice() const
	{
		return physical_device_;
	}

	VkDevice GetDevice() const
	{
		return logical_device_;
	}

	void CreateCommandBuffer(int size, VkCommandBufferLevel command_buffer_level, VkCommandBuffer * dst)
	{
		VkCommandBufferAllocateInfo alloc_info;
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.level = command_buffer_level;
		alloc_info.commandPool = command_pool_;
		alloc_info.commandBufferCount = size;

		VkResult res = vkAllocateCommandBuffers(logical_device_, &alloc_info, dst);
		if (res != VK_SUCCESS)
		{
			throw " allocate command buffer fault . ";
		}
	}

	void DestroyCommandBuffer(VkCommandBuffer *commandBuffer, int size)
	{
		vkFreeCommandBuffers(logical_device_, command_pool_, size, commandBuffer
		);
	}

	uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr)
	{
		for (uint32_t i = 0; i < device_memory_properties_.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((device_memory_properties_.memoryTypes[i].propertyFlags & properties) == properties)
				{
					if (memTypeFound)
					{
						*memTypeFound = true;
					}
					return i;
				}
			}
			typeBits >>= 1;
		}

		if (memTypeFound)
		{
			*memTypeFound = false;
			return 0;
		}
		else
		{
			throw std::runtime_error("Could not find a matching memory type");
		}
	}

	VulkanBuffer* CreateVulkanBuffer(VkBufferUsageFlags usage_bits, VkMemoryPropertyFlags memory_property, uint32_t buffer_size, void * data = NULL)
	{
		VkBufferCreateInfo buffer_create_info = {};
		buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_create_info.size = buffer_size;
		buffer_create_info.usage = usage_bits;

		VkBuffer buffer;
		VkDeviceMemory memory;
		VkResult res;
		res = vkCreateBuffer(logical_device_, &buffer_create_info, NULL, &buffer);
		if (res != VK_SUCCESS)
		{
			throw " create buffer fault . ";
		}

		VkMemoryRequirements memory_require;
		vkGetBufferMemoryRequirements(logical_device_, buffer, &memory_require);
		VkMemoryAllocateInfo memory_alloc_info = {};
		memory_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memory_alloc_info.allocationSize = memory_require.size;
		memory_alloc_info.memoryTypeIndex = GetMemoryType(memory_require.memoryTypeBits, memory_property);
		res = vkAllocateMemory(logical_device_, &memory_alloc_info, NULL, &memory);
		if (res != VK_SUCCESS)
		{
			throw " allocate memory fault . ";
		}

		if (data != nullptr)
		{
			void * mapped_memory;
			vkMapMemory(logical_device_, memory, 0, buffer_size, 0, &mapped_memory);
			memcpy(mapped_memory, data, buffer_size);

			if ((memory_property & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT == 0))
			{
				VkMappedMemoryRange memoryRange = {};
				memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				memoryRange.size = buffer_size;
				memoryRange.offset = 0;
				memoryRange.memory = memory;
				vkFlushMappedMemoryRanges(logical_device_, 1, &memoryRange);
			}

			vkUnmapMemory(logical_device_, memory);
		}

		res = vkBindBufferMemory(logical_device_, buffer, memory, 0);
		if (res != VK_SUCCESS)
		{
			throw " bind buffer memory fault . ";
		}

		VulkanBuffer * vulkan_buffer = new VulkanBuffer(buffer, memory, logical_device_);

		return vulkan_buffer;
	}

	VkShaderModule LoadShader(std::string file_name)
	{
		std::ifstream fstream;
		fstream.open(file_name.c_str(), std::ios::binary);
		if (fstream.fail())
		{
			throw " load shader module fault . ";
		}
		fstream.seekg(0, std::ios::end);
		uint32_t code_size = fstream.tellg();
		fstream.seekg(0, std::ios::beg);
		char * code_data = new char[code_size];
		fstream.read(code_data, code_size);
		fstream.close();

		VkShaderModuleCreateInfo shader_module_ci = {};
		shader_module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_ci.codeSize = code_size;
		shader_module_ci.pCode = (uint32_t*)code_data;

		VkShaderModule shader_module;
		VkResult res = vkCreateShaderModule(logical_device_, &shader_module_ci, NULL, &shader_module);

		if (res != VK_SUCCESS)
		{
			throw " load shader module fault . ";
		}
		delete[] code_data;
		code_data = NULL;

		return shader_module;
	}

	uint32_t GetGraphicsQueue() const
	{
		return QueueFamilyIndices.graphics;
	}
};

#endif 