#ifndef _SHADOW_DEPTH_PIPELINE_H_
#define _SHADOW_DEPTH_PIPELINE_H_

#include "VulkanPipeline.h"

class ShadowDepthPipeline : public IRenderingPipeline
{
public:
	ShadowDepthPipeline(VulkanDevice * device, VulkanCamera * camera, glm::vec3 lightPos ) :
		device_(device) , camera_(camera) ,light_pos_(lightPos)
	{
		light_pos_ = glm::normalize(lightPos);
		PrepareResources();
	}

	~ShadowDepthPipeline()
	{

	};

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

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState[1] = {
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE)
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = VulkanInitializer::InitPipelineColorBlendState(1, colorBlendAttachmentState);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = VulkanInitializer::InitMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = VulkanInitializer::InitViewportState(1, 1);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = VulkanInitializer::InitDepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = VulkanInitializer::GetDefaultDynamicState();
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfo =
		{
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/shadowDepthVert.spv" , device_)
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
		VkClearValue depthClearValue = {};
		depthClearValue.depthStencil = { 1 , 0 };
		std::vector<VkClearValue> clearValues = { depthClearValue };
		VkBuffer vertBuffer = mesh_->GetMeshEntry().vertexBuffer->GetDesc().buffer;
		VkBuffer indexBuffer = mesh_->GetMeshEntry().indexBuffer->GetDesc().buffer;
		size_t vertsCount = mesh_->GetMeshEntry().vertCount;
		size_t indicesCount = mesh_->GetMeshEntry().indicesCount;

		VkDeviceSize offset = 0;

		if (startRenderPass)
		{
			VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(render_pass_, frame_buffer_[PushConstantData.cascadeIndex], clearValues, 4096 , 4096);
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		}
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, desc_set_vec_.data(), 0, NULL);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, offset, VK_INDEX_TYPE_UINT32);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
		vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData) , &PushConstantData);
		vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
		if (endRenderPass)
		{
			vkCmdEndRenderPass(commandBuffer);
		}
	}
	VkRenderPass CreateRenderPass()
	{
		std::vector<VkPushConstantRange> constRange = {
				VulkanInitializer::InitVkPushConstantRange(0 , sizeof(PushConstantData) , VK_SHADER_STAGE_VERTEX_BIT)
		};

		VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size(), constRange.data(), desc_set_vec_.size() , desc_set_layout_vec_.data() );
		VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayout, NULL, &pipeline_layout_));

		std::vector<VkAttachmentDescription> attachmentsDesc =
		{
			VulkanInitializer::InitAttachmentDescription(VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
		};

		std::vector<VkAttachmentReference> depthAttachmentReference =
		{
			VulkanInitializer::InitAttachmentReference(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		};

		std::vector<VkAttachmentReference> colorAttachmentReference = {};
		std::vector<VkSubpassDescription> subpassDesc =
		{
			VulkanInitializer::InitSubpassDescription(colorAttachmentReference , &depthAttachmentReference[0])
		};
		std::vector<VkSubpassDependency> subpassDependency;
		VkRenderPassCreateInfo renderPassCreateInfo = VulkanInitializer::InitRenderPassCreateInfo(attachmentsDesc, subpassDesc, subpassDependency);
		VULKAN_SUCCESS(vkCreateRenderPass(device_->GetDevice(), &renderPassCreateInfo, NULL, &render_pass_));

		for (int i = 0; i < 4; i++)
		{
			VkImageView attachments = image_views_[i];
			VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(4096 , 4096 , 1, 1, &attachments, render_pass_);
			vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &frame_buffer_[i]);
		}
		return render_pass_;

	}
	void PrepareResources()
	{
		cascade_matrix_buffer_ = device_->CreateVulkanBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(UniformBufferData));
		shadow_map_image_ = new VulkanImage(device_, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_UNDEFINED, 4096, 4096, VK_IMAGE_ASPECT_DEPTH_BIT, 4, 1, true);
		for (int i = 0; i < 4; i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo = VulkanInitializer::InitImageViewCreateInfo(VK_FORMAT_D32_SFLOAT_S8_UINT, shadow_map_image_->image_, VK_IMAGE_ASPECT_DEPTH_BIT, i, 1, 0, 1, VK_IMAGE_VIEW_TYPE_2D);
			vkCreateImageView(device_->GetDevice(), &imageViewCreateInfo, NULL, &image_views_[i]);
		}
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
	void InitDesc()
	{
		VkDescriptorSetLayoutBinding bufferBinding[1] = {
			VulkanInitializer::InitBinding(0 , 1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , VK_SHADER_STAGE_VERTEX_BIT)
		};


		std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutCreateInfo = {
			VulkanInitializer::InitDescSetLayoutCreateInfo(1 , bufferBinding),
		};

		desc_set_layout_vec_.resize(descSetLayoutCreateInfo.size());
		desc_set_vec_.resize(descSetLayoutCreateInfo.size());
		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VULKAN_SUCCESS(vkCreateDescriptorSetLayout(device_->GetDevice(), &descSetLayoutCreateInfo[i], NULL, &desc_set_layout_vec_[i]));
		}

		std::vector<VkDescriptorType> descTypeVec =
		{
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

		VkWriteDescriptorSet writeDescs[1] = {
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 0 ,desc_set_vec_[0] , &cascade_matrix_buffer_->GetDesc()),
		};

		vkUpdateDescriptorSets(device_->GetDevice(), 1, writeDescs, 0, NULL);
	}

	VulkanImage* GetShadowMapImage() const
	{
		return shadow_map_image_;
	}
	VulkanBuffer * GetCascadeTransform() const {
		return cascade_matrix_buffer_;
	}
	void SetMesh(VulkanMesh * mesh)
	{
		mesh_ = mesh;
	}
	void SetPushConstantData(glm::mat4 model , uint32_t cascadeIndex)
	{
		PushConstantData.cascadeIndex = cascadeIndex;
		PushConstantData.model = model;
		VkDescriptorImageInfo imageInfo;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imageInfo.imageView = image_views_[cascadeIndex];
		imageInfo.sampler = shadow_map_image_->sampler_;
	}

	void updateCascades()
	{
		float cascadeSplitLambda = 0.5f;
		float cascadeSplits[4];

		float nearClip = camera_->getNearClip();
		float farClip = camera_->getFarClip();
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		for (uint32_t i = 0; i < 4; i++) {
			float p = (i + 1) / static_cast<float>(4);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < 4; i++) {
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};
			glm::mat4 invCam = glm::inverse(camera_->matrices.perspective * camera_->matrices.view);
			for (uint32_t i = 0; i < 8; i++) {
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++) {
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++) {
				frustumCenter += frustumCorners[i];
			}
			frustumCenter /= 8.0f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++) {
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = (glm::max)(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = normalize(-light_pos_);
			glm::mat4 lightViewMatrix = glm::lookAtLH(frustumCenter - lightDir * minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = getOrthoMatrix(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
		
			cascades_[i].splitDepth = (camera_->getNearClip() + splitDist * clipRange) ;
			cascades_[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}

	glm::mat4 getOrthoMatrix( float minX , float maxX , float minY , float maxY , float minZ , float maxZ )
	{
		glm::mat4 mat(0.0f);
		mat[0][0] = 2.0f / (maxX - minX);
		mat[1][1] = 2.0f / (maxY - minY);
		mat[2][2] = 1.0f / (maxZ - minZ);
		mat[2][3] = minZ / (minZ - maxZ);
		mat[3][3] = 1.0f;
		return mat;
	}

	void UpdateData()
	{
		updateCascades();
		for (int i = 0; i < 4; i++)
		{
			UniformBufferData.splitDepth[i] = cascades_[i].splitDepth;
			UniformBufferData.cascadeTransform[i] = cascades_[i].viewProjMatrix;
		}

		cascade_matrix_buffer_->Map();
		UniformBufferStruct* data = (UniformBufferStruct*)cascade_matrix_buffer_->GetMappedMemory();
		memcpy(data, &UniformBufferData, sizeof(UniformBufferData));
		cascade_matrix_buffer_->Unmap();
	}

private:
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VulkanMesh * mesh_;
	VkRenderPass render_pass_;
	VkFramebuffer frame_buffer_[4];
	Cascade cascades_[4];
	VulkanImage * shadow_map_image_;
	VulkanBuffer * cascade_matrix_buffer_;
	VkImageView image_views_[4];
	VkDescriptorPool desc_pool_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	std::vector<VkDescriptorSetLayout> desc_set_layout_vec_;

private:
	VulkanDevice * device_;
	VulkanCamera * camera_;
	glm::vec3 light_pos_;
	
private:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 world;
	};
	struct
	{
		glm::mat4 model;
		uint32_t cascadeIndex;
	}PushConstantData;

	struct UniformBufferStruct {
		glm::mat4 cascadeTransform[4];
		float splitDepth[4];
	}UniformBufferData;
};

#endif