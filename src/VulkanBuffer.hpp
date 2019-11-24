#ifndef _VULKAN_BUFFER_HPP_H_
#define _VULKAN_BUFFER_HPP_H_

#include <vulkan/vulkan.h>

class VulkanBuffer
{
public:
	VulkanBuffer(
		VkBuffer buffer,
		VkDeviceMemory device_memory,
		VkDevice logical_device
	) : buffer_(buffer),
		buffer_memory_(device_memory),
		logical_device_(logical_device)

	{
		mapped_memory_ = NULL;
		desc_info_.buffer = buffer;
		desc_info_.range = VK_WHOLE_SIZE;
		desc_info_.offset = 0;
	}

	~VulkanBuffer()
	{
		Destroy();
	}

public:
	void Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		VkResult res = vkMapMemory(logical_device_, buffer_memory_, offset, size, 0, &mapped_memory_);
		if (res != VK_SUCCESS)
		{
			throw " map memory fault . ";
		}
	}

	void Unmap()
	{
		if (mapped_memory_ == NULL)
		{
			throw " the memory has not been mapped . ";
		}
		vkUnmapMemory(logical_device_, buffer_memory_);
		mapped_memory_ = nullptr;
	}

	void FillDescriptorInfo(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		desc_info_.offset = offset;
		desc_info_.range = size;
		desc_info_.buffer = buffer_;
	}

	void FlushMemory(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = buffer_memory_;
		memoryRange.size = size;
		memoryRange.offset = offset;
		VkResult res = vkFlushMappedMemoryRanges(logical_device_, 1, &memoryRange);
		if (res != VK_SUCCESS)
		{
			throw " flush memory fault . ";
		}
	}

	void InvalidateMemory(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		VkMappedMemoryRange memoryRange = {};
		memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryRange.memory = buffer_memory_;
		memoryRange.size = size;
		memoryRange.offset = offset;
		VkResult res = vkInvalidateMappedMemoryRanges(logical_device_, 1, &memoryRange);
		if (res != VK_SUCCESS)
		{
			throw " invalidate memory fault . ";
		}
	}

	void Destroy()
	{
		if (buffer_)
		{
			vkDestroyBuffer(logical_device_, buffer_, NULL);
		}

		if (buffer_memory_)
		{
			vkFreeMemory(logical_device_, buffer_memory_, NULL);
		}
	}

	VkDescriptorBufferInfo& GetDesc() 
	{
		return desc_info_;
	}

	void* GetMappedMemory()
	{
		return mapped_memory_;
	}

	VkDeviceMemory GetBufferMemory() const
	{
		return buffer_memory_;
	}

private:
	VkDevice logical_device_;
	VkBuffer buffer_;
	VkDeviceMemory buffer_memory_;
	void* mapped_memory_;
	VkDescriptorBufferInfo desc_info_;

};

#endif