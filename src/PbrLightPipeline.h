#ifndef _PBR_LIGHT_PIPELINE_H_
#define _PBR_LIGHT_PIPELINE_H_

#include "VulkanPipeline.h"

class PBRLightPipeline : public IRenderingPipeline 
{
public:
	PBRLightPipeline(VulkanDevice* device, VulkanSwapChain * swapChain, VulkanCamera * camera ,  uint32_t sWidth, uint32_t sHeight , VkImageView depthStencilImage,
		VulkanBuffer * cascadeTransformBuffer, VulkanImage * shadowMapImage)
	{
		device_ = device;
		swap_chain_ = swapChain;
		camera_ = camera;
		screen_width_ = sWidth;
		screen_height_ = sHeight;
		depth_buffer_ = depthStencilImage;
		PrepareResources();

		cascade_transform_buffer_ = cascadeTransformBuffer;
		shadow_map_image_ = shadowMapImage;
	};
	~PBRLightPipeline() {};

public:
	VkPipeline CreateGraphicsPipeline()
	{
		VkVertexInputBindingDescription binding = VulkanInitializer::InitVertexInputBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex));

		VkVertexInputAttributeDescription attribute[3];
		attribute[0] = VulkanInitializer::InitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT,0);
		attribute[1] = VulkanInitializer::InitVertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float) );
		attribute[2] = VulkanInitializer::InitVertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float) );

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = VulkanInitializer::InitVertexInputState(1, &binding, 3, attribute);
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = VulkanInitializer::GetNormalInputAssembly();
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = VulkanInitializer::InitRasterizationStateCreateInfo(VK_CULL_MODE_NONE);

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState[1] = {
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE)
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = VulkanInitializer::InitPipelineColorBlendState(1, colorBlendAttachmentState);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = VulkanInitializer::InitMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = VulkanInitializer::InitViewportState(1, 1);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = VulkanInitializer::InitDepthStencilState(VK_TRUE, VK_TRUE , VK_COMPARE_OP_LESS);
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = VulkanInitializer::GetDefaultDynamicState();
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfo =
		{
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/pbrLightVert.spv" , device_),
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT , GetAssetPath() + "shaders/pbrLightFrag.spv" , device_)
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
		vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &PushConstantData);
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
				VulkanInitializer::InitVkPushConstantRange(0 , sizeof(PushConstantData) , VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT )
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
			VulkanInitializer::InitAttachmentReference(1 , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
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
			VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(screen_width_, screen_height_,1, 2, attachments, render_pass_);
			vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &(frame_buffer_vec_[i]));
		}

		return render_pass_;
	}

	void PrepareResources()
	{
		uniform_buffer_ = device_->CreateVulkanBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(UniformBufferData));
		InitDesc();
		CreateRenderPass();
		CreateGraphicsPipeline();
	}

	VertexLayout GetVertexLayout(std::string & layoutName) {
		layoutName = "PositionNormalUv";
		VertexLayout layout(
			{
				{ VERTEX_COMPONENT_POSITION },
				{ VERTEX_COMPONENT_NORMAL },
				{VERTEX_COMPONENT_UV}
			}
		);
		return layout;
	};
	void SetMesh(VulkanMesh * mesh) { mesh_ = mesh; };

	void SetPushConstantData(glm::mat4 model, glm::mat4 view, glm::mat4 proj , glm::vec3 camera , glm::vec3 lightPos  )
	{
		PushConstantData.model = model;
		PushConstantData.view = view;
		PushConstantData.projection = proj;
		PushConstantData.camerapos = camera;
		PushConstantData.lightDir = glm::normalize(lightPos);
	}

	void SetFrameBufferIndex(int ind) { frame_index_ = ind; };

	void UpdateData() 
	{
		uniform_buffer_->Map();
		UniformBufferStruct * uniformBuffer = (UniformBufferStruct*)uniform_buffer_->GetMappedMemory();
		uniformBuffer->gamma = 2.2f;
		uniformBuffer->exposure = 4.5f;
		const float p = 15.0f;
		uniformBuffer->lightPos[0] = glm::vec4(-p, -p * 0.5f, -p, 1.0f);
		uniformBuffer->lightPos[1] = glm::vec4(-p, -p * 0.5f, p, 1.0f);
		uniformBuffer->lightPos[2] = glm::vec4(p, -p * 0.5f, p, 1.0f);
		uniformBuffer->lightPos[3] = glm::vec4(p, -p * 0.5f, -p, 1.0f);
		uniform_buffer_->Unmap();
	};

	void InitDesc()
	{
		VkDescriptorSetLayoutBinding binding[9] =
		{
			VulkanInitializer::InitBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(5, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(6, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(7, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(8, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),

		};
		VkDescriptorSetLayoutBinding shadowBinding[2] =
		{
			VulkanInitializer::InitBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(1, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
		};


		std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutCreateInfo = {
			VulkanInitializer::InitDescSetLayoutCreateInfo(9 , binding),
			VulkanInitializer::InitDescSetLayoutCreateInfo(2 , shadowBinding)
		};
		desc_set_layout_vec_.resize(descSetLayoutCreateInfo.size());
		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VULKAN_SUCCESS(vkCreateDescriptorSetLayout(device_->GetDevice(), &descSetLayoutCreateInfo[i], NULL, &desc_set_layout_vec_[i]));
		}
	}

private:
	struct
	{
		glm::mat4 projection;
		glm::mat4 model;
		glm::mat4 view;
		glm::vec3 camerapos;
		float padding;
		glm::vec3 lightDir;
	}PushConstantData;

	struct UniformBufferStruct 
	{
		glm::vec4 lightPos[4];
		float exposure;
		float gamma;
	}UniformBufferData;

	struct Vertex
	{
		glm::vec3 postion;
		glm::vec3 normal;
		glm::vec2 uv;
	};

private:
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VkRenderPass render_pass_;
	VulkanDevice* device_;
	VulkanSwapChain * swap_chain_;
	uint32_t screen_width_;
	uint32_t screen_height_;
	uint32_t frame_index_;
	VulkanMesh * mesh_;
	VulkanBuffer * uniform_buffer_;
	VulkanCamera * camera_;
	VkImageView depth_buffer_;

	VulkanBuffer * cascade_transform_buffer_;
	VulkanImage * shadow_map_image_;
private:
	std::vector<VkFramebuffer> frame_buffer_vec_;
	std::vector<VkDescriptorSetLayout> desc_set_layout_vec_;

	friend class PbrLightPassMaterial;
};

#endif