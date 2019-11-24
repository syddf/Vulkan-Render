#ifndef _VULKAN_IMAGE_H_
#define _VULKAN_IMAGE_H_

#include "Utility.h"
#include "VulkanDevice.hpp"

class Texture
{
public:
	VulkanDevice * device_;
	VkImage image_;
	VkImageLayout image_layout_;
	VkImageView image_view_;
	VkDeviceMemory memory_;

	uint32_t width_, height_;
	uint32_t mip_levels;
	uint32_t layer_count_;
	VkDescriptorImageInfo image_info_;
	VkSampler sampler_;

	virtual ~Texture() {};

	void UpdateDescriptor()
	{
		image_info_.imageLayout = image_layout_;
		image_info_.imageView = image_view_;
		image_info_.sampler = sampler_;
	}

	void destroy()
	{
		vkDestroyImageView(device_->GetDevice(), image_view_, nullptr);
		vkDestroyImage(device_->GetDevice(), image_, nullptr);
		if (sampler_)
		{
			vkDestroySampler(device_->GetDevice(), sampler_, nullptr);
		}
		vkFreeMemory(device_->GetDevice(), memory_, nullptr);
	}
};

class Texture2D : public Texture
{
public:
	Texture2D(
		std::string filename,
		VkFormat format,
		VulkanDevice *device,
		VkQueue copyQueue,
		uint32_t formatSize = 4,
		bool generateMipMaps = false,
		bool ktxTex = false , 
		VkFilter filter = VK_FILTER_LINEAR,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VkAccessFlags dstAccessFlag = VK_ACCESS_SHADER_READ_BIT,
		bool forceLinear = false
	);
	void GenerateMipMaps(uint32_t mipLevels, uint32_t width, uint32_t height, VkFormat imageFormat, VkQueue copyQueue);
};
class TextureCube : public Texture
{
public:
	TextureCube(
		std::string filePrifix,
		std::string fileSuffix,
		VulkanDevice * device,
		VkQueue CopyQueue,
		VkFormat format,
		uint32_t formatSize = 4,
		VkFilter filter = VK_FILTER_LINEAR,
		VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VkAccessFlags dstAccessFlag = VK_ACCESS_SHADER_READ_BIT,
		bool forceLinear = false
	);

	void GenerateMipMaps(uint32_t mipLevels, uint32_t width, uint32_t height, VkFormat imageFormat, VkQueue copyQueue);
};

class VulkanImage
{
public:
	VkImage image_;
	VkImageView image_view_;
	VkFormat image_format_;
	VkDeviceMemory image_memory_;
	VkDescriptorImageInfo desc_image_info_;
	VkDeviceSize image_size_;
	VkImageLayout image_layout_;
	VkSampler sampler_;
	VkDescriptorImageInfo image_info_;
	bool HaveSampler;
	int width_;
	int height_;
	int layer_count_;
	int mip_levels_;

public:
	VulkanImage(
		VulkanDevice * device, 
		VkFormat format, 
		VkImageUsageFlags usage ,
		VkImageLayout imageLayout , 
		int width , 
		int height , 
		VkImageAspectFlags aspectFlag ,
		int layerCount = 1 ,
		int mipLevels = 1 ,
		bool haveSampler = false 
	)
	{
		device_ = device;
		image_format_ = format;
		image_layout_ = imageLayout;
		HaveSampler = haveSampler;
		layer_count_ = layerCount;
		mip_levels_ = mipLevels;
		uint32_t ind = device->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		VkImageCreateInfo imageCreateInfo = VulkanInitializer::InitImageCreateInfo(
			format , 
			VK_IMAGE_TILING_OPTIMAL , 
			width ,
			height ,
			1 ,
			layer_count_,
			mip_levels_ , 
			&ind , 
			imageLayout , 
			usage ,
			layerCount == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0 
		);

		VULKAN_SUCCESS( vkCreateImage( device_->GetDevice() , &imageCreateInfo , NULL , &image_ ) );

		VkMemoryAllocateInfo memoryAllocateInfo = VulkanInitializer::InitMemoryAllocateInfo(device_, image_);
		VULKAN_SUCCESS(vkAllocateMemory(device_->GetDevice(), &memoryAllocateInfo, NULL, &image_memory_));
		vkBindImageMemory(device_->GetDevice(), image_, image_memory_, 0);
		image_size_ = memoryAllocateInfo.allocationSize;

		VkImageViewType viewType = layerCount == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
		VkImageViewCreateInfo imageViewCreateInfo = VulkanInitializer::InitImageViewCreateInfo(
			format,
			image_,
			aspectFlag ,
			0 , layerCount, 0, mip_levels_, viewType
		);

		VULKAN_SUCCESS( vkCreateImageView( device_->GetDevice() , &imageViewCreateInfo , NULL , &image_view_  ));

		desc_image_info_.imageLayout = imageLayout;
		desc_image_info_.imageView = image_view_;
		desc_image_info_.sampler = VK_NULL_HANDLE;

		width_ = width;
		height_ = height;

		if (haveSampler)
		{
			VkSamplerCreateInfo samplerCreateInfo = {};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerCreateInfo.minLod = 0.0f;
			samplerCreateInfo.maxLod = 0.0f;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			vkCreateSampler(device->GetDevice(), &samplerCreateInfo, nullptr, &sampler_);
		}

		desc_image_info_.imageLayout = image_layout_;
		desc_image_info_.imageView = image_view_;
		desc_image_info_.sampler = sampler_;
	}


	~VulkanImage()
	{
		vkFreeMemory(device_->GetDevice(), image_memory_, NULL);
		vkDestroyImage(device_->GetDevice(), image_, NULL);
		vkDestroyImageView(device_->GetDevice(), image_view_, NULL);
	}

	void MapMemory( VkQueue queue , VulkanBuffer * stagingBuffer  )
	{
		TranslateImageLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL , queue );
		CopyImageToBuffer( queue, stagingBuffer );
		TranslateImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, queue);
	}

	void TranslateImageLayout( VkImageLayout oldLayout ,  VkImageLayout newLayout , VkQueue queue , VkCommandBuffer cmdBuffer = VK_NULL_HANDLE  , int arrayLayerCount = 1 , int mipLevels = 1  )
	{
		bool submit = ( cmdBuffer == VK_NULL_HANDLE );
		VkCommandBuffer translateLayoutBuffer;
		if (submit)
		{
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &translateLayoutBuffer);
			VkCommandBufferBeginInfo beginInfo = VulkanInitializer::InitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			vkBeginCommandBuffer(translateLayoutBuffer, &beginInfo);
		}
		else {
			translateLayoutBuffer = cmdBuffer;
		}

		VkImageMemoryBarrier imageMemoryBarrier = VulkanInitializer::InitImageMemoryBarrier( VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT , VK_ACCESS_TRANSFER_READ_BIT , 
			oldLayout , newLayout  , VK_IMAGE_ASPECT_COLOR_BIT , 0 , arrayLayerCount , 0 , mipLevels , image_  );
		vkCmdPipelineBarrier(translateLayoutBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier);

		if (submit)
		{
			vkEndCommandBuffer(translateLayoutBuffer);
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &translateLayoutBuffer;
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			VkFence fence;
			vkCreateFence(device_->GetDevice(), &fenceCreateInfo, NULL, &fence);
			vkQueueSubmit(queue, 1, &submitInfo, fence);
			vkWaitForFences(device_->GetDevice(), 1, &fence, VK_TRUE, 1e8);
			vkDestroyFence(device_->GetDevice(), fence, NULL);
			device_->DestroyCommandBuffer(&translateLayoutBuffer, 1);
		}
		
		image_layout_ = newLayout;
	}

	void CopyImageToBuffer( VkQueue queue , VulkanBuffer * stagingBuffer , int mipMapLevel = 0 , VkCommandBuffer cmdBuffer = VK_NULL_HANDLE   )
	{
		VkBufferImageCopy copyRange = VulkanInitializer::InitBufferImageCopy(0, 0, 0, width_, height_, 1, { 0 , 0 , 0 }, VK_IMAGE_ASPECT_COLOR_BIT, 0, layer_count_, mipMapLevel );
		bool submit = ( cmdBuffer == VK_NULL_HANDLE );
		VkCommandBuffer copyImageBuffer;
		if (submit)
		{
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &copyImageBuffer);
			VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			vkBeginCommandBuffer(copyImageBuffer, &commandBufferBeginInfo);
		}
		else {
			copyImageBuffer = cmdBuffer;
		}

		vkCmdCopyImageToBuffer(copyImageBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer->GetDesc().buffer, 1, &copyRange);

		if (submit)
		{
			vkEndCommandBuffer(copyImageBuffer);
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &copyImageBuffer;
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			VkFence fence;
			vkCreateFence(device_->GetDevice(), &fenceCreateInfo, NULL, &fence);
			vkQueueSubmit(queue, 1, &submitInfo, fence);
			vkWaitForFences(device_->GetDevice(), 1, &fence, VK_TRUE, 1e8);
			vkDestroyFence(device_->GetDevice(), fence, NULL);
			device_->DestroyCommandBuffer(&copyImageBuffer, 1);
		}

	}

	void CopyImageToImage(
		VkQueue queue, 
		VulkanImage * srcImage, 
		VulkanImage * dstImage,
		int width ,
		int height , 
		int srcBaseArrayLayer,
		int srcArrayLayerCount,  
		int srcMipLevel, 
		int srcMipLevels,
		int dstBaseArrayLayer , 
		int dstArrayLayerCount,
		int dstMipLevel ,
		int dstMipLevels , 
		VkImageLayout srcImageLayout , 
		VkImageLayout dstImageLayout , 
		VkCommandBuffer cmdBuffer = VK_NULL_HANDLE)

	{
		bool submit = ( cmdBuffer == VK_NULL_HANDLE ) ;
		srcImage->TranslateImageLayout(srcImageLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL , queue , cmdBuffer , srcImage->layer_count_ , srcMipLevels  );
		dstImage->TranslateImageLayout(dstImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, queue , cmdBuffer , dstImage->layer_count_ , dstMipLevels );

		VkImageCopy copyRegion = {};
		copyRegion.extent.width = width;
		copyRegion.extent.height = height;
		copyRegion.extent.depth = 1;
		copyRegion.srcOffset = { 0 , 0 , 0 };
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.baseArrayLayer = srcBaseArrayLayer ;
		copyRegion.srcSubresource.layerCount = srcArrayLayerCount;
		copyRegion.srcSubresource.mipLevel = srcMipLevel;

		copyRegion.dstOffset = { 0, 0 , 0 };
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.baseArrayLayer = dstBaseArrayLayer;
		copyRegion.dstSubresource.layerCount = dstArrayLayerCount;
		copyRegion.dstSubresource.mipLevel = dstMipLevel;

		VkCommandBuffer copyImageBuffer;
		if (submit)
		{
			device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &copyImageBuffer);
			VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
			vkBeginCommandBuffer(copyImageBuffer, &commandBufferBeginInfo);
		}
		else {
			copyImageBuffer = cmdBuffer;
		}
		vkCmdCopyImage(copyImageBuffer, srcImage->image_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage->image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		if (submit)
		{
			vkEndCommandBuffer(copyImageBuffer);
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &copyImageBuffer;
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			VkFence fence;
			vkCreateFence(device_->GetDevice(), &fenceCreateInfo, NULL, &fence);
			vkQueueSubmit(queue, 1, &submitInfo, fence);
			vkWaitForFences(device_->GetDevice(), 1, &fence, VK_TRUE, 1e8);
			vkDestroyFence(device_->GetDevice(), fence, NULL);
			device_->DestroyCommandBuffer(&copyImageBuffer, 1);
		}

		srcImage->TranslateImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcImageLayout,  queue, cmdBuffer, srcImage->layer_count_, srcMipLevels );
		dstImage->TranslateImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dstImageLayout , queue , cmdBuffer, dstImage->layer_count_, dstMipLevels );
	}

	VkDescriptorImageInfo& GetDescriptorImageInfo( VkImageLayout imageLayout )
	{
		image_info_.imageLayout = imageLayout ;
		image_info_.imageView = image_view_;
		image_info_.sampler = HaveSampler ? sampler_ : VK_NULL_HANDLE;
		return image_info_;
	}

private:
	VulkanDevice * device_;
};

#endif