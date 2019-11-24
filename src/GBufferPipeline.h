#ifndef _GBUFFER_PIPELINE_H_
#define _GBUFFER_PIPELINE_H_

#include "VulkanPipeline.h"

class GBufferPipeline : public IRenderingPipeline
{
public:
	GBufferPipeline(int renderWidth, int renderHeight, VulkanDevice * device_) :
		render_width_(renderWidth), render_height_(renderHeight), device_(device_)
	{
		PrepareResources();
	}

public:
	VkPipeline CreateGraphicsPipeline()
	{
		VkVertexInputBindingDescription binding = VulkanInitializer::InitVertexInputBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex));

		VkVertexInputAttributeDescription attribute[4];
		attribute[0] = VulkanInitializer::InitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
		attribute[1] = VulkanInitializer::InitVertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float));
		attribute[2] = VulkanInitializer::InitVertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, 6 * sizeof(float));
		attribute[3] = VulkanInitializer::InitVertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32_SFLOAT, 8 * sizeof(float));

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = VulkanInitializer::InitVertexInputState(1, &binding, 4, attribute);
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = VulkanInitializer::GetNormalInputAssembly();
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = VulkanInitializer::InitRasterizationStateCreateInfo(VK_CULL_MODE_BACK_BIT);

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState[4] = {
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE),
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE),
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE),
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE)
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = VulkanInitializer::InitPipelineColorBlendState(4, colorBlendAttachmentState);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = VulkanInitializer::InitMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = VulkanInitializer::InitViewportState(1, 1);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = VulkanInitializer::InitDepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = VulkanInitializer::GetDefaultDynamicState();
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfo =
		{
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/GBufferVert.spv" , device_),
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT , GetAssetPath() + "shaders/GBufferFrag.spv" , device_)
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
		VkClearValue depthClearValue;
		depthClearValue.depthStencil = { 1 , 0 };
		VkClearValue colorClearValue;
		colorClearValue.color = { 0 };
		std::vector<VkClearValue> clearValues = { colorClearValue , colorClearValue , colorClearValue , colorClearValue , depthClearValue  };
		VkBuffer vertBuffer = mesh_->GetMeshEntry().vertexBuffer->GetDesc().buffer;
		VkBuffer indexBuffer = mesh_->GetMeshEntry().indexBuffer->GetDesc().buffer;
		size_t vertsCount = mesh_->GetMeshEntry().vertCount;
		size_t indicesCount = mesh_->GetMeshEntry().indicesCount;

		VkDeviceSize offset = 0;

		if (startRenderPass)
		{
			VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(render_pass_, frame_buffer_, clearValues, render_width_, render_height_);
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, offset, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
		vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &PushConstantData);
		vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
		if (endRenderPass)
		{
			vkCmdEndRenderPass(commandBuffer);
		}
	}

	VkRenderPass CreateRenderPass()
	{
		std::vector<VkPushConstantRange> constRange = {
				VulkanInitializer::InitVkPushConstantRange(0 , sizeof(PushConstantData) , VK_SHADER_STAGE_VERTEX_BIT) ,
		};

		VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size(), constRange.data(), 1 ,&desc_set_layout_);
		VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayout, NULL, &pipeline_layout_));

		VkImageView attachments[5] = {
			albedo_image_->image_view_,
			position_image_->image_view_, 
			normal_image_->image_view_,
			pbr_image_->image_view_ , 
			pre_depth_image_->image_view_
		};

		std::vector<VkAttachmentDescription> attachmentsDesc =
		{
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_R8G8B8A8_UNORM , VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE),
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE),
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_R32G32B32A32_SFLOAT , VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE),
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_R8G8B8A8_UNORM , VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE),
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_D32_SFLOAT_S8_UINT , VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		};

		std::vector<VkAttachmentReference> depthAttachmentReference =
		{
			VulkanInitializer::InitAttachmentReference(4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		};

		std::vector<VkAttachmentReference> colorAttachmentReference = 
		{
			VulkanInitializer::InitAttachmentReference(0 , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ),
			VulkanInitializer::InitAttachmentReference(1 , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ),
			VulkanInitializer::InitAttachmentReference(2 , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ),
			VulkanInitializer::InitAttachmentReference(3 , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL )
		};

		std::vector<VkSubpassDescription> subpassDesc =
		{
			VulkanInitializer::InitSubpassDescription(colorAttachmentReference , &depthAttachmentReference[0])
		};

		std::vector<VkSubpassDependency> subpassDependency;
		VkRenderPassCreateInfo renderPassCreateInfo = VulkanInitializer::InitRenderPassCreateInfo(attachmentsDesc, subpassDesc, subpassDependency);
		VULKAN_SUCCESS(vkCreateRenderPass(device_->GetDevice(), &renderPassCreateInfo, NULL, &render_pass_));

		VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(render_width_, render_height_, 1, 5, attachments, render_pass_);
		vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &frame_buffer_);
		return render_pass_;
	}

	void PrepareResources()
	{
		pre_depth_image_ = new VulkanImage(device_, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_UNDEFINED, render_width_, render_height_, VK_IMAGE_ASPECT_DEPTH_BIT, 1 , 1 , true );
		albedo_image_ = new VulkanImage(device_, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_UNDEFINED , render_width_ , render_height_ , VK_IMAGE_ASPECT_COLOR_BIT,  1, 1, true);
		normal_image_ = new VulkanImage(device_, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_UNDEFINED , render_width_ , render_height_ , VK_IMAGE_ASPECT_COLOR_BIT,  1, 1, true);
		position_image_ = new VulkanImage(device_, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_UNDEFINED , render_width_ , render_height_ , VK_IMAGE_ASPECT_COLOR_BIT,  1, 1, true);
		pbr_image_ = new VulkanImage(device_, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_UNDEFINED , render_width_ , render_height_ , VK_IMAGE_ASPECT_COLOR_BIT,  1, 1, true);

		InitDesc();
		CreateRenderPass();
		CreateGraphicsPipeline();
	}

	VertexLayout GetVertexLayout(std::string & layoutName)
	{
		layoutName = "PositionColorTexcoordNormal";
		VertexLayout layout(
			{
				{ VERTEX_COMPONENT_POSITION } ,
				{ VERTEX_COMPONENT_COLOR },
				{ VERTEX_COMPONENT_UV },
				{ VERTEX_COMPONENT_NORMAL },
			}
		);
		return layout;
	}

	VulkanImage* GetDepthImage() const
	{
		return pre_depth_image_;
	}

	void SetMesh(VulkanMesh * mesh)
	{
		mesh_ = mesh;
	}

	void SetPushConstantData(glm::mat4 model , glm::mat4 mvp )
	{
		PushConstantData.model = model;
		PushConstantData.mvp = mvp;
	}

	void UpdateData()
	{

	}

	void InitDesc()
	{
		VkDescriptorSetLayoutBinding bindings[5] = {
			VulkanInitializer::InitBinding(0 , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(1 , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(2 , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(3 , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(4 , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayout = {
				VulkanInitializer::InitDescSetLayoutCreateInfo(5 , bindings)
		};
		VULKAN_SUCCESS(vkCreateDescriptorSetLayout(device_->GetDevice(), descSetLayout.data(), NULL, &desc_set_layout_));
	}
	VulkanImage* GetAlbedoImage() const { return albedo_image_; };
	VulkanImage* GetNormalImage() const { return normal_image_; };
	VulkanImage* GetPositionImage() const { return position_image_; };
	VulkanImage* GetPBRImage() const { return pbr_image_; };
	VulkanImage* GetPredepthImage() const { return pre_depth_image_; }
private:
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VkDescriptorSetLayout desc_set_layout_;
	VulkanMesh * mesh_;
	VkRenderPass render_pass_;
	VkFramebuffer frame_buffer_;

private:
	VulkanDevice * device_;
	int render_width_;
	int render_height_;
	
private:
	VulkanImage * pre_depth_image_;
	VulkanImage * albedo_image_;
	VulkanImage * normal_image_;
	VulkanImage * position_image_;
	VulkanImage * pbr_image_;

private:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 uv;
		glm::vec3 normal;
	};

	struct {
		glm::mat4 model;
		glm::mat4 mvp;
	}PushConstantData;

	friend class TBDRMaterial;
};

#endif