#ifndef _SKY_BOX_PIPELINE_H_
#define _SKY_BOX_PIPELINE_H_

#include "Utility.h"
#include "VulkanPipeline.h"

class SkyBoxPipeline : public IRenderingPipeline
{
public:
	SkyBoxPipeline(TextureCube * skyBoxTexture , VulkanDevice* device, VulkanSwapChain * swapChain , uint32_t sWidth , uint32_t sHeight)
	{
		skybox_texture_ = skyBoxTexture;
		device_ = device;
		swap_chain_ = swapChain;
		screen_width_ = sWidth;
		screen_height_ = sHeight;
		PrepareResources();
	}
	~SkyBoxPipeline()
	{

	}

public:
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
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT , GetAssetPath() + "shaders/SkyboxFrag.spv" , device_)
		};

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
			VulkanInitializer::InitGraphicsPipelineCreateInfo(pipeline_layout_, render_pass_,
				&vertexInputStateCreateInfo, &inputAssemblyStateCreateInfo, &rasterizationStateCreateInfo, &colorBlendStateCreateInfo, &multisampleStateCreateInfo, &viewportStateCreateInfo,
				&depthStencilStateCreateInfo, &dynamicStateCreateInfo, pipelineShaderStageCreateInfo, 0);

		VULKAN_SUCCESS(vkCreateGraphicsPipelines(device_->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &pipeline_));

		return pipeline_;
	};

	void SetupCommandBuffer(VkCommandBuffer & commandBuffer, bool startRenderPass, bool endRenderPass)
	{
		VkClearValue colorClearValue = {};
		VkClearValue depthClearValue = {};
		depthClearValue.depthStencil.depth = 1;
		std::vector<VkClearValue> clearValues = { colorClearValue , depthClearValue };
		VkBuffer vertBuffer = mesh_->GetMeshEntry().vertexBuffer->GetDesc().buffer;
		VkBuffer indexBuffer = mesh_->GetMeshEntry().indexBuffer->GetDesc().buffer;
		size_t vertsCount = mesh_->GetMeshEntry().vertCount;
		size_t indicesCount = mesh_->GetMeshEntry().indicesCount;
		VkDeviceSize offset = 0;
		if (startRenderPass)
		{
			VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(render_pass_, frame_buffer_vec_[frame_index_], clearValues, screen_width_, screen_height_);
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, offset, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
		vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT , 0, sizeof(PushConstantData), &PushConstantData);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, desc_set_vec_.data(), 0, NULL);
		vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
		if (endRenderPass)
		{
			vkCmdEndRenderPass(commandBuffer);
		}
	}

	VkRenderPass CreateRenderPass()
	{
		// Layout 
		std::vector<VkPushConstantRange> constRange = {
				VulkanInitializer::InitVkPushConstantRange(0 , sizeof(PushConstantData) , VK_SHADER_STAGE_VERTEX_BIT)
		};
		VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size(), constRange.data(), 1, desc_set_layout_vec_.data());
		VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayout, NULL, &pipeline_layout_));

		//Render Pass
		std::vector<VkAttachmentDescription> attachmentsDesc =
		{
			VulkanInitializer::InitAttachmentDescription(swap_chain_->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE),
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
		size_t swapChainImageSize = swap_chain_->GetImageCount();
		frame_buffer_vec_.resize(swapChainImageSize);
		for (int i = 0; i < swapChainImageSize; i++)
		{
			VkImageView attachments[2] = {
				swap_chain_->GetImageView(i) 
			};
			VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(screen_width_, screen_height_, 1, 1, attachments, render_pass_);
			vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &(frame_buffer_vec_[i]));
		}

		return render_pass_;
	}

	void PrepareResources()
	{
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
	} ;
	void SetMesh(VulkanMesh * mesh) { mesh_ = mesh; };

	void SetPushConstantData(glm::mat4 & mvp) 
	{
		PushConstantData.mvp = mvp;
	}

	void SetFrameBufferIndex(int ind) { frame_index_ = ind; };

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

private:
	struct Vertex
	{
		glm::vec3 position;
	};

	struct {
		glm::mat4 mvp;
	} PushConstantData;

private:
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VkRenderPass render_pass_;
	VulkanDevice* device_;
	VulkanSwapChain * swap_chain_;
	std::vector<VkFramebuffer> frame_buffer_vec_;
	VkDescriptorPool desc_pool_;
	std::vector<VkDescriptorSetLayout> desc_set_layout_vec_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	uint32_t screen_width_;
	uint32_t screen_height_;
	uint32_t frame_index_;
	VulkanMesh * mesh_;
	TextureCube * skybox_texture_;
};

#endif