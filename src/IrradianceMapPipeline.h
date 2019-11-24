#ifndef _IRRADIANCE_MAP_PIPELINE_H_
#define _IRRADIANCE_MAP_PIPELINE_H_

#include "VulkanPipeline.h"

class IrradianceMapPipeline : public IRenderingPipeline
{
public:
	IrradianceMapPipeline(VulkanDevice* device, VkQueue queue, VulkanMesh * mesh, TextureCube * skyboxTexture )
	{
		device_ = device;
		queue_ = queue;
		mesh_ = mesh;
		skybox_texture_ = skyboxTexture;

		PrepareResources();
		
	};

	~IrradianceMapPipeline() 
	{
		delete irradiance_map_image_;
		delete offscreen_tmp_image_;
	};

	VkPipeline CreateGraphicsPipeline()
	{
		VkVertexInputBindingDescription binding = VulkanInitializer::InitVertexInputBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex));

		VkVertexInputAttributeDescription attribute[1];
		attribute[0] = VulkanInitializer::InitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = VulkanInitializer::InitVertexInputState(1, &binding, 1, attribute);
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = VulkanInitializer::GetNormalInputAssembly();
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = VulkanInitializer::InitRasterizationStateCreateInfo(VK_CULL_MODE_NONE);

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState[1] = {
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE)
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = VulkanInitializer::InitPipelineColorBlendState(1, colorBlendAttachmentState);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = VulkanInitializer::InitMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = VulkanInitializer::InitViewportState(1, 1);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = VulkanInitializer::InitDepthStencilState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = VulkanInitializer::GetDefaultDynamicState();
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfo =
		{
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/SkyboxVert.spv" , device_),
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT , GetAssetPath() + "shaders/IrradianceMapFrag.spv" , device_)
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
		std::vector<glm::mat4> matrices = {
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};
		VkBuffer vertBuffer = mesh_->GetMeshEntry().vertexBuffer->GetDesc().buffer;
		VkBuffer indexBuffer = mesh_->GetMeshEntry().indexBuffer->GetDesc().buffer;
		size_t vertsCount = mesh_->GetMeshEntry().vertCount;
		size_t indicesCount = mesh_->GetMeshEntry().indicesCount;
		VkDeviceSize offset = 0;
		std::vector<VkClearValue> clearValues;
		VkClearValue value;
		value.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		clearValues.push_back(value);
		VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(render_pass_, frame_buffer_, clearValues, dim_ , dim_);
		
		VkViewport viewport = VulkanInitializer::InitViewport(0,0,dim_,dim_,0.0f , 1.0f );
		VkRect2D scissor = {};
		scissor.extent.width = dim_;
		scissor.extent.height = dim_;

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		for (int mipLevel = 0; mipLevel < mip_levels_; mipLevel++)
		{
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			for (int arrayLayer = 0; arrayLayer < 6; arrayLayer++)
			{
				vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuffer, &offset);
				vkCmdBindIndexBuffer(commandBuffer, indexBuffer, offset, VK_INDEX_TYPE_UINT32);
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
				SetPushConstantData(glm::perspective((float)(PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[arrayLayer], PI * 2.0f / 180.0f, PI * 0.5f / 90.0f);
				vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantDataUniform), &PushConstantDataUniform);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, desc_set_vec_.data(), 0, NULL);
				vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
				vkCmdEndRenderPass(commandBuffer);
				irradiance_map_image_->CopyImageToImage(queue_, offscreen_tmp_image_, irradiance_map_image_, viewport.width, viewport.height, 0, 1, 0, 1, arrayLayer, 1, mipLevel, mip_levels_, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer);
			}
			viewport.width = viewport.width * 0.5f;
			viewport.height = viewport.height * 0.5f;
		}

		irradiance_map_image_->TranslateImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, queue_);
	}

	VkRenderPass CreateRenderPass()
	{
		// Layout 
		std::vector<VkPushConstantRange> constRange = {
				VulkanInitializer::InitVkPushConstantRange(0 , sizeof(PushConstantDataUniform) , VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT )
		};
		VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size(), constRange.data(), 1, desc_set_layout_vec_.data());
		VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayout, NULL, &pipeline_layout_));

		//Render Pass
		std::vector<VkAttachmentDescription> attachmentsDesc =
		{
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE),
		};
		std::vector<VkAttachmentReference> colorAttachmentReference =
		{
			VulkanInitializer::InitAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		};
		std::vector<VkSubpassDescription> subpassDesc =
		{
			VulkanInitializer::InitSubpassDescription(colorAttachmentReference)
		};
		std::vector<VkSubpassDependency> subpassDependency;
		VkRenderPassCreateInfo renderPassCreateInfo = VulkanInitializer::InitRenderPassCreateInfo(attachmentsDesc, subpassDesc, subpassDependency);
		VULKAN_SUCCESS(vkCreateRenderPass(device_->GetDevice(), &renderPassCreateInfo, NULL, &render_pass_));

		// Attachment Framebuffer 
		VkImageView attachments[1] = {
			offscreen_tmp_image_->image_view_
		};
		VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(dim_, dim_, 1, 1, attachments, render_pass_);
		vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &frame_buffer_);
		return render_pass_;
	}

	void PrepareResources()
	{
		dim_ = 64;
		mip_levels_ = static_cast<uint32_t>(floor(log2(dim_))) + 1;
		offscreen_tmp_image_ = new VulkanImage(device_, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_UNDEFINED, dim_, dim_, VK_IMAGE_ASPECT_COLOR_BIT, 1, 1, false);
		offscreen_tmp_image_->TranslateImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, queue_);
		irradiance_map_image_ = new VulkanImage(device_, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED, dim_, dim_, VK_IMAGE_ASPECT_COLOR_BIT, 6, mip_levels_, true);
		irradiance_map_image_->TranslateImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, queue_, VK_NULL_HANDLE, 6, mip_levels_);
		InitDesc();
		CreateRenderPass();
		CreateGraphicsPipeline();
	}

	VertexLayout GetVertexLayout(std::string & layoutName) {
		layoutName = "Position";
		VertexLayout layout(
			{
				VERTEX_COMPONENT_POSITION
			}
		);
		return layout;
	};
	void SetMesh(VulkanMesh * mesh) { mesh_ = mesh; };
	void SetPushConstantData(glm::mat4 mvp, float deltaPhi, float deltaTheta)
	{
		PushConstantDataUniform.mvp = mvp;
		PushConstantDataUniform.deltaPhi = deltaPhi;
		PushConstantDataUniform.deltaTheta = deltaTheta;
	}
	void UpdateData() {};

	void InitDesc()
	{
		VkDescriptorSetLayoutBinding matBinding = VulkanInitializer::InitBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

		std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutCreateInfo = {
			VulkanInitializer::InitDescSetLayoutCreateInfo(1 , &matBinding),
		};
		desc_set_layout_vec_.resize(descSetLayoutCreateInfo.size());
		desc_set_vec_.resize(descSetLayoutCreateInfo.size());
		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VULKAN_SUCCESS(vkCreateDescriptorSetLayout(device_->GetDevice(), &descSetLayoutCreateInfo[i], NULL, &desc_set_layout_vec_[i]));
		}

		std::vector<VkDescriptorType> descTypeVec =
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
		};

		std::vector<VkDescriptorPoolSize> descPoolSize = VulkanInitializer::InitDescriptorPoolSizeVec(descTypeVec);
		VkDescriptorPoolCreateInfo descPoolCreateInfo = VulkanInitializer::InitDescriptorPoolCreateInfo(descPoolSize, descSetLayoutCreateInfo.size());
		VULKAN_SUCCESS(vkCreateDescriptorPool(device_->GetDevice(), &descPoolCreateInfo, NULL, &desc_pool_));

		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VkDescriptorSetAllocateInfo allocateInfo = VulkanInitializer::InitDescriptorSetAllocateInfo(1, desc_pool_, &desc_set_layout_vec_[i]);
			VULKAN_SUCCESS(vkAllocateDescriptorSets(device_->GetDevice(), &allocateInfo, &desc_set_vec_[i]));
		}
		VkWriteDescriptorSet writeDescs[1] = {
				VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 0 ,desc_set_vec_[0] , &skybox_texture_->image_info_)
		};

		vkUpdateDescriptorSets(device_->GetDevice(), 1, writeDescs, 0, NULL);
	}

	VulkanImage* GetIrradianceMapImage() const { return irradiance_map_image_; }

private:
	struct
	{
		glm::mat4 mvp;
		float deltaPhi;
		float deltaTheta;
	} PushConstantDataUniform;
	struct Vertex
	{
		glm::vec3 position;
	};

private:
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VkRenderPass render_pass_;
	VkDescriptorPool desc_pool_;
	std::vector<VkDescriptorSetLayout> desc_set_layout_vec_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	VkFramebuffer frame_buffer_;

private:
	VulkanDevice* device_;
	VkQueue queue_;
	VulkanMesh * mesh_;
	TextureCube * skybox_texture_;

private:
	uint32_t dim_;
	uint32_t mip_levels_;
	VulkanImage * irradiance_map_image_;
	VulkanImage * offscreen_tmp_image_;
};

#endif