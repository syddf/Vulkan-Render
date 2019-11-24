#ifndef _VULKAN_EDITOR_H_
#define _VULKAN_EDITOR_H_

#include <string>
#include "VulkanImgui.h"
#include "Utility.h"
#include "VulkanImage.h"
#include <vector>
#include "VulkanSwapChain.hpp"
#include "VulkanMaterial.h"
#include "VulkanObject.h"



class VulkanEditor
{
public:
	VulkanEditor( VulkanDevice * device , int eWidth , int eHeight , int eX , int eY  , int sWidth , int sHeight , VkQueue commandQueue , VulkanSwapChain * swapChain , std::vector<VulkanObject*> & objects) : objects_(objects)
	{
		RenderResource.vulkanDevice = device;
		editor_width_ = eWidth;
		editor_height_ = eHeight;
		editor_x_ = eX;
		editor_y_ = eY;
		screen_width_ = sWidth;
		screen_height_ = sHeight;
		RenderResource.commandQueue = commandQueue;
		RenderResource.swapChain = swapChain;

		PrepareEditUIPass();
	}

	~VulkanEditor()
	{

	}

public:
	void OnUpdateImgui();
	void MouseEvent( const MouseStateType & mouseState );

	void AddNewEmptyObject();	
	void UpdateEditAxis(int commandBufferInd);
	void PrepareEditUIPass();

	VkCommandBuffer GetCommandBuffer(int index) const;

private:
	std::vector<VulkanObject*> & objects_;

	struct
	{
		bool ShowEditor;
	}GlobalData;

private:
	int editor_width_;
	int editor_height_;
	int editor_x_;
	int editor_y_;
	int screen_width_;
	int screen_height_;

private:
	struct {
		VkDescriptorSetLayout axisRenderDescriptorSetLayout;
		VkPipelineLayout axisRenderPipelineLayout;
		VulkanDevice * vulkanDevice;
		Texture2D * axisImage;
		VulkanImage * axisIndexAttachmentImage;
		VkQueue commandQueue;
		VulkanSwapChain * swapChain;
		VkRenderPass renderPass;
		VkPipeline axisRenderPipeline;
		std::vector<VkCommandBuffer> axisRenderCommandBufferVec;
		VkDescriptorSet axisRenderDescriptorSet;
		VkDescriptorPool axisRenderDescriptorPool;
		std::vector<VkFramebuffer> frameBufferVec;
		VulkanBuffer * vertexBuffer;
		VulkanBuffer * indexBuffer;

		VkCommandBuffer axisConstantPushCommandBuffer;
	}RenderResource;

	struct
	{
		float position[2] = { 0.0f , 0.0f };
		int axisIndex = 0;
	}PushConstantData;

	struct Vertex {
		float pos[2];
		float uv[2];
	};
};

#endif