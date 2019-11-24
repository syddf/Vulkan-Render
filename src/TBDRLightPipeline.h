#ifndef _TBDR_LIGHT_PIPELINE_H_
#define _TBDR_LIGHT_PIPELINE_H_

#include "VulkanPipeline.h"

class TBDRLightPipeline : public IRenderingPipeline 
{
public:
	TBDRLightPipeline(
		VulkanBuffer * tileLightVisibleBuffer,
		VulkanBuffer * lightUniformBuffer,
		VkImageView depthBuffer,
		VulkanImage * albedoImage,
		VulkanImage * positionImage,
		VulkanImage * normalImage,
		VulkanImage * pbrImage,
		VulkanImage * irradianceImage,
		VulkanImage * prefilteredImage,
		Texture2D * brdflutTexture,
		VulkanDevice* device,
		VulkanSwapChain * swapChain,
		uint32_t sWidth,
		uint32_t sHeight
	)
	{
		tile_light_visible_buffer_ = tileLightVisibleBuffer;
		light_uniform_buffer_ = lightUniformBuffer;
		depth_buffer_ = depthBuffer;
		albedo_image_ = albedoImage;
		position_image_ = positionImage;
		normal_image_ = normalImage;
		pbr_image_ = pbrImage;
		irradiance_image_ = irradianceImage;
		prefiltered_image_ = prefilteredImage;
		brdflut_texture_ = brdflutTexture;
		device_ = device;
		swap_chain_ = swapChain;
		screen_width_ = sWidth;
		screen_height_ = sHeight;

		PrepareResources();
	}

	~TBDRLightPipeline()
	{

	}

	VkPipeline CreateGraphicsPipeline()
	{
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = VulkanInitializer::InitVertexInputState(0, NULL , 0 , NULL );
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = VulkanInitializer::GetNormalInputAssembly();
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = VulkanInitializer::InitRasterizationStateCreateInfo(VK_CULL_MODE_BACK_BIT);

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState[1] = {
			VulkanInitializer::InitColorBlendAttachmentState(VK_TRUE)
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = VulkanInitializer::InitPipelineColorBlendState(1, colorBlendAttachmentState);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = VulkanInitializer::InitMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = VulkanInitializer::InitViewportState(1, 1);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = VulkanInitializer::InitDepthStencilState(VK_FALSE,VK_FALSE, VK_COMPARE_OP_LESS);
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = VulkanInitializer::GetDefaultDynamicState();
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfo =
		{
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/FullScreenVert.spv" , device_),
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT , GetAssetPath() + "shaders/TBDRLightPassFrag.spv" , device_)
		};

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
			VulkanInitializer::InitGraphicsPipelineCreateInfo(pipeline_layout_, render_pass_,
				&vertexInputStateCreateInfo, &inputAssemblyStateCreateInfo, &rasterizationStateCreateInfo, &colorBlendStateCreateInfo, &multisampleStateCreateInfo, &viewportStateCreateInfo,
				&depthStencilStateCreateInfo, &dynamicStateCreateInfo, pipelineShaderStageCreateInfo, 0);

		VULKAN_SUCCESS(vkCreateGraphicsPipelines(device_->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &pipeline_));

		return pipeline_;
	}

	void SetupCommandBuffer(VkCommandBuffer & commandBuffer, bool startRenderPass, bool endRenderPass)
	{
		std::vector<VkClearValue> clearValues = { };
		VkDeviceSize offset = 0;
		if (startRenderPass)
		{
			VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(render_pass_, frame_buffer_vec_[frame_index_], clearValues, screen_width_, screen_height_);
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
		vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &PushConstantData);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 2 , desc_set_vec_.data(), 0, NULL);
		vkCmdDraw(commandBuffer,3,1,0,0);
		if (endRenderPass)
		{
			vkCmdEndRenderPass(commandBuffer);
		}
	}

	VkRenderPass CreateRenderPass()
	{
		// Layout 
		std::vector<VkPushConstantRange> constRange = {
				VulkanInitializer::InitVkPushConstantRange(0 , sizeof(PushConstantData) , VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size(), constRange.data(), desc_set_layout_vec_.size(), desc_set_layout_vec_.data());
		VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayout, NULL, &pipeline_layout_));

		//Render Pass
		std::vector<VkAttachmentDescription> attachmentsDesc =
		{
			VulkanInitializer::InitAttachmentDescription(swap_chain_->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE),
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_D24_UNORM_S8_UINT , VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL , VK_ATTACHMENT_LOAD_OP_LOAD , VK_ATTACHMENT_STORE_OP_STORE)
		};
		std::vector<VkAttachmentReference> colorAttachmentReference =
		{
			VulkanInitializer::InitAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		};
		std::vector<VkAttachmentReference> depthAttachmentReference =
		{
			VulkanInitializer::InitAttachmentReference(1 , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		};

		std::vector<VkSubpassDescription> subpassDesc =
		{
			VulkanInitializer::InitSubpassDescription(colorAttachmentReference, depthAttachmentReference.data())
		};
		std::vector<VkSubpassDependency> subpassDependency;
		VkRenderPassCreateInfo renderPassCreateInfo = VulkanInitializer::InitRenderPassCreateInfo(attachmentsDesc, subpassDesc, subpassDependency);
		VULKAN_SUCCESS(vkCreateRenderPass(device_->GetDevice(), &renderPassCreateInfo, NULL, &render_pass_));

		// Attachment Framebuffer 
		size_t swapChainImageSize = swap_chain_->GetImageCount();
		frame_buffer_vec_.resize(swapChainImageSize);
		for (int i = 0; i < swapChainImageSize; i++)
		{
			VkImageView attachments[2] = {
				swap_chain_->GetImageView(i) ,
				depth_buffer_
			};
			VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(screen_width_, screen_height_, 1, 2, attachments, render_pass_);
			vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &(frame_buffer_vec_[i]));
		}

		return render_pass_;
	}
	void PrepareResources()
	{
		InitDesc();
		CreateRenderPass();
		CreateGraphicsPipeline();
		UpdateDescriptorSet();
	}
	VertexLayout GetVertexLayout(std::string & layoutName)
	{
		layoutName = "";
		VertexLayout empty = { {} };
		return empty;
	}
	void SetFrameIndex(uint32_t frameInd)
	{
		frame_index_ = frameInd;
	}

	void InitDesc()
	{
		VkDescriptorSetLayoutBinding samplerBinding[7] = {
			VulkanInitializer::InitBinding(0 , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(1  , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(2  , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(3  , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(4  , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(5  , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(6  , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT)
		};
		VkDescriptorSetLayoutBinding lightBuffer[2] = {
				VulkanInitializer::InitBinding(0 , 1 , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , VK_SHADER_STAGE_FRAGMENT_BIT),
				VulkanInitializer::InitBinding(1 , 1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutCreateInfo = {
			VulkanInitializer::InitDescSetLayoutCreateInfo(7 , samplerBinding),
			VulkanInitializer::InitDescSetLayoutCreateInfo(2 , lightBuffer)
		};

		desc_set_layout_vec_.resize(descSetLayoutCreateInfo.size());
		desc_set_vec_.resize(descSetLayoutCreateInfo.size());
		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VULKAN_SUCCESS(vkCreateDescriptorSetLayout(device_->GetDevice(), &descSetLayoutCreateInfo[i], NULL, &desc_set_layout_vec_[i]));
		}

		std::vector<VkDescriptorType> descTypeVec =
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		};

		std::vector<VkDescriptorPoolSize> descPoolSize = VulkanInitializer::InitDescriptorPoolSizeVec(descTypeVec);
		VkDescriptorPoolCreateInfo descPoolCreateInfo = VulkanInitializer::InitDescriptorPoolCreateInfo(descPoolSize, descSetLayoutCreateInfo.size());
		VULKAN_SUCCESS(vkCreateDescriptorPool(device_->GetDevice(), &descPoolCreateInfo, NULL, &desc_pool_));

		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VkDescriptorSetAllocateInfo allocateInfo = VulkanInitializer::InitDescriptorSetAllocateInfo(1, desc_pool_, &desc_set_layout_vec_[i]);
			VULKAN_SUCCESS(vkAllocateDescriptorSets(device_->GetDevice(), &allocateInfo, &desc_set_vec_[i]));
		}


	}

	void SetPushConstantData(glm::mat4 model, glm::mat4 view, glm::mat4 proj, glm::vec3 cameraPos, int viewportOffsetX, int viewportOffsetY , int tileNumX , int tileNumY  )
	{
		PushConstantData.modelMatrix = model;
		PushConstantData.viewMatrix = view;
		PushConstantData.projMatrix = proj;
		PushConstantData.cameraPosition = cameraPos;
		PushConstantData.viewportSize[0] = viewportOffsetX;
		PushConstantData.viewportSize[1] = viewportOffsetY;
		PushConstantData.tileNum[0] = tileNumX;
		PushConstantData.tileNum[1] = tileNumY;
		PushConstantData.exposure = 4.5f;
		PushConstantData.gamma = 2.2f;
	}
	void SetMesh(VulkanMesh * mesh)
	{
	}
	void UpdateData()
	{
	}

	void UpdateDescriptorSet()
	{
		VkWriteDescriptorSet writeDescs[9] = {
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 0 , desc_set_vec_[0] , &albedo_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 1 , desc_set_vec_[0] , &position_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 2 , desc_set_vec_[0] , &normal_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 3 , desc_set_vec_[0] , &pbr_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 4 , desc_set_vec_[0] , &irradiance_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 5 , desc_set_vec_[0] , &brdflut_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 6 , desc_set_vec_[0] , &prefiltered_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 0 , desc_set_vec_[1] , &tile_light_visible_buffer_->GetDesc()),
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 , desc_set_vec_[1] , &light_uniform_buffer_->GetDesc())
		};

		vkUpdateDescriptorSets(device_->GetDevice(), 9, writeDescs, 0, NULL);
	}

private:
	VulkanBuffer * tile_light_visible_buffer_;
	VulkanBuffer * light_uniform_buffer_;
	VkImageView depth_buffer_;
	VulkanImage * albedo_image_;
	VulkanImage * position_image_;
	VulkanImage * normal_image_;
	VulkanImage * pbr_image_;
	VulkanImage * irradiance_image_;
	VulkanImage * prefiltered_image_;
	Texture2D * brdflut_texture_;
	VulkanDevice* device_;
	VulkanSwapChain * swap_chain_;
	uint32_t screen_width_;
	uint32_t screen_height_;
	uint32_t frame_index_;

private:
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VkDescriptorSetLayout desc_set_layout_;
	VkRenderPass render_pass_;
	VkFramebuffer frame_buffer_;

private:	
	std::vector<VkFramebuffer> frame_buffer_vec_;
	VkDescriptorPool desc_pool_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	std::vector<VkDescriptorSetLayout> desc_set_layout_vec_;
	struct
	{
		int tileNum[2];
		int viewportSize[2];
		glm::mat4 projMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
		glm::vec3 cameraPosition;
		float exposure;
		float gamma;
	} PushConstantData;
};

#endif