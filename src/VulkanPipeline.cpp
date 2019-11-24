#include "VulkanPipeline.h"
#include <glm/gtc/random.hpp>

VkPipeline CullLightComputePipeline::CreateComputePipeline()
{
	InitResources();
	InitDesc();

	VkPushConstantRange constantRange;
	constantRange.size = sizeof(PushConstantData);
	constantRange.offset = 0;
	constantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VulkanInitializer::InitPipelineLayoutCreateInfo(1 , &constantRange , 1, &desc_set_layout_);
	VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayoutCreateInfo, NULL, &pipeline_layout_));

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, GetAssetPath() + "shaders/CullLight.spv", device_);
	VkComputePipelineCreateInfo computePipelineCreateInfo = VulkanInitializer::InitComputePipelineCreateInfo(pipeline_layout_, shaderStageCreateInfo);
	VULKAN_SUCCESS(vkCreateComputePipelines(device_->GetDevice(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, NULL, &compute_pipeline_));

	return compute_pipeline_;
}

VkCommandBuffer CullLightComputePipeline::SetupCommandBuffer()
{
	VkWriteDescriptorSet writeDescs[3] = {
		VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 0 , desc_set_ , &tile_light_visible_buffer_->GetDesc()),
		VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 , desc_set_ , &light_uniform_buffer_->GetDesc()),
		VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 2 , desc_set_ , &pre_depth_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
	};

	vkUpdateDescriptorSets(device_->GetDevice(), 3, writeDescs, 0, NULL);

	VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	vkBeginCommandBuffer(compute_command_buffer_, &commandBufferBeginInfo);
	vkCmdBindPipeline(compute_command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);
	vkCmdBindDescriptorSets(compute_command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1, &desc_set_, 0, NULL);
	vkCmdPushConstants(compute_command_buffer_, pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstantData), &PushConstantData);
	vkCmdDispatch(compute_command_buffer_, tile_count_x_, tile_count_y_, 1);
	vkEndCommandBuffer(compute_command_buffer_);
	return compute_command_buffer_;
}

void CullLightComputePipeline::UpdateData()
{
	light_uniform_buffer_->Map();
	LightUniformData * lightUniformData = (LightUniformData*)(light_uniform_buffer_->GetMappedMemory());
	for (int i = 0; i < lightUniformData->pointLightCount; i++)
	{
		lightUniformData->pointLights[i].pos.y += 0.01f;
		if (lightUniformData->pointLights[i].pos.y >= light_max_pos_.y)
		{
			lightUniformData->pointLights[i].pos.y = light_min_pos_.y;
		}
	}
	light_uniform_buffer_->Unmap();
}

void CullLightComputePipeline::SetPushConstantData(int viewportSizeX, int viewportSizeY, float zNear , float zFar ,  glm::mat4 & projMatrix, glm::mat4 & viewMatrix)
{
	PushConstantData.viewportSize[0] = viewportSizeX;
	PushConstantData.viewportSize[1] = viewportSizeY;
	PushConstantData.viewMatrix = viewMatrix;
	PushConstantData.projMatrix = projMatrix;
	PushConstantData.zNear = zNear;
	PushConstantData.zFar = zFar;
}

void CullLightComputePipeline::SetPreDepth(VulkanImage * preDepthImage)
{
	pre_depth_image_ = preDepthImage;
}

VulkanBuffer* CullLightComputePipeline::GetTileLightVisibleBuffer() const
{
	return tile_light_visible_buffer_;
}

VulkanBuffer * CullLightComputePipeline::GetLightUniformBuffer() const
{
	return light_uniform_buffer_;
}

void CullLightComputePipeline::InitResources( )
{
	auto InitLight = [&]()->void
	{
		LightUniformBufferData.pointLightCount = light_count_;
		for (int i = 0; i < LightUniformBufferData.pointLightCount; i++)
		{
			glm::vec3 color;
			do { color = { glm::linearRand(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)) }; } while (color.length() < 0.8f);
			glm::vec3 pos = glm::linearRand(light_min_pos_, light_max_pos_);
			LightUniformBufferData.pointLights[i].intensity = color;
			LightUniformBufferData.pointLights[i].radius = light_radius_;
			LightUniformBufferData.pointLights[i].pos = pos;
		}
	};

	InitLight();

	tile_light_visible_buffer_ = 
		device_->CreateVulkanBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT , tile_count_x_ * tile_count_y_ *  (sizeof(LightVisible)) );

	size_t lightBufferSize = sizeof(PointLight)*light_count_ + sizeof(uint32_t) * 4;
	light_uniform_buffer_ = device_->CreateVulkanBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightBufferSize, &LightUniformBufferData);

	device_->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &compute_command_buffer_);
}

void CullLightComputePipeline::InitDesc()
{
	VkDescriptorSetLayoutBinding binding[3] = { {} , {} , {} };
	binding[0].binding = 0;
	binding[0].descriptorCount = 1;
	binding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	binding[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	binding[1].binding = 1;
	binding[1].descriptorCount = 1;
	binding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	binding[2].binding = 2;
	binding[2].descriptorCount = 1;
	binding[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo = VulkanInitializer::InitDescSetLayoutCreateInfo(3, binding);
	VULKAN_SUCCESS(vkCreateDescriptorSetLayout(device_->GetDevice(), &descSetLayoutCreateInfo, NULL, &desc_set_layout_));

	std::vector<VkDescriptorType> descTypeVec = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER  ,  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
	std::vector<VkDescriptorPoolSize> descPoolSize = VulkanInitializer::InitDescriptorPoolSizeVec(descTypeVec);
	VkDescriptorPoolCreateInfo descPoolCreateInfo = VulkanInitializer::InitDescriptorPoolCreateInfo(descPoolSize, 1);
	VULKAN_SUCCESS(vkCreateDescriptorPool(device_->GetDevice(), &descPoolCreateInfo, NULL, &desc_pool_));

	VkDescriptorSetAllocateInfo allocateInfo = VulkanInitializer::InitDescriptorSetAllocateInfo(1, desc_pool_, &desc_set_layout_);
	VULKAN_SUCCESS(vkAllocateDescriptorSets(device_->GetDevice(), &allocateInfo , &desc_set_));

	VkWriteDescriptorSet writeDescs[3] = {
		VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 0 , desc_set_ , &tile_light_visible_buffer_->GetDesc()),
		VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 , desc_set_ , &light_uniform_buffer_->GetDesc()),
		VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 2 , desc_set_ , &pre_depth_image_->GetDescriptorImageInfo( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
	};

	vkUpdateDescriptorSets(device_->GetDevice(), 3, writeDescs, 0, NULL);


}

VkPipeline PreDepthRenderingPipeline::CreateGraphicsPipeline()
{
	VkVertexInputBindingDescription binding = VulkanInitializer::InitVertexInputBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex));

	VkVertexInputAttributeDescription attribute[4];
	attribute[0] = VulkanInitializer::InitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	attribute[1] = VulkanInitializer::InitVertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, 0);
	attribute[2] = VulkanInitializer::InitVertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, 0);
	attribute[3] = VulkanInitializer::InitVertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32_SFLOAT, 0);

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
		VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/preDepth.spv" , device_),
	};

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		VulkanInitializer::InitGraphicsPipelineCreateInfo(pipeline_layout_, render_pass_,
			&vertexInputStateCreateInfo, &inputAssemblyStateCreateInfo, &rasterizationStateCreateInfo, &colorBlendStateCreateInfo, &multisampleStateCreateInfo, &viewportStateCreateInfo,
			&depthStencilStateCreateInfo, &dynamicStateCreateInfo, pipelineShaderStageCreateInfo, 0);

	VULKAN_SUCCESS(vkCreateGraphicsPipelines(device_->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &pipeline_));

	return pipeline_;
}

void PreDepthRenderingPipeline::SetupCommandBuffer(VkCommandBuffer & commandBuffer, bool startRenderPass , bool endRenderPass )
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
		VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(render_pass_, frame_buffer_, clearValues, render_width_, render_height);
		vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertBuffer, &offset);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, offset, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
	vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvpMat);
	vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
	if (endRenderPass)
	{
		vkCmdEndRenderPass(commandBuffer);
	}
}

VkRenderPass PreDepthRenderingPipeline::CreateRenderPass()
{
	std::vector<VkPushConstantRange> constRange = {
	VulkanInitializer::InitVkPushConstantRange(0 , sizeof(glm::mat4) , VK_SHADER_STAGE_VERTEX_BIT) ,
	};

	VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size(), constRange.data(), 0, NULL);
	VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayout, NULL, &pipeline_layout_));

	VkImageView attachments = pre_depth_image_->image_view_;
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

	VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(render_width_, render_height, 1, 1,&attachments, render_pass_);
	vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &frame_buffer_);
	
	return render_pass_;
}

void PreDepthRenderingPipeline::PrepareResources()
{
	pre_depth_image_ = new VulkanImage(
		device_,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		render_width_,
		render_height,
		VK_IMAGE_ASPECT_DEPTH_BIT ,1,1,
		true 
	);

	CreateRenderPass();
	CreateGraphicsPipeline();
}

VertexLayout PreDepthRenderingPipeline::GetVertexLayout(std::string & layoutName)
{
	layoutName = "Position";
	VertexLayout layout({ VERTEX_COMPONENT_POSITION });
	return layout;
}

VkPipeline ForwardPlusLightPassPipeline::CreateGraphicsPipeline()
{
	VkVertexInputBindingDescription binding = VulkanInitializer::InitVertexInputBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex));

	VkVertexInputAttributeDescription attribute[4];
	attribute[0] = VulkanInitializer::InitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
	attribute[1] = VulkanInitializer::InitVertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 );
	attribute[2] = VulkanInitializer::InitVertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6);
	attribute[3] = VulkanInitializer::InitVertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = VulkanInitializer::InitVertexInputState(1, &binding, 4, attribute);
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = VulkanInitializer::GetNormalInputAssembly();
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = VulkanInitializer::InitRasterizationStateCreateInfo(VK_CULL_MODE_BACK_BIT);

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState[1] = {
		VulkanInitializer::InitColorBlendAttachmentState(VK_TRUE)
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = VulkanInitializer::InitPipelineColorBlendState(1, colorBlendAttachmentState);
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = VulkanInitializer::InitMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = VulkanInitializer::InitViewportState(1, 1);
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = VulkanInitializer::InitDepthStencilState(VK_TRUE, VK_TRUE , VK_COMPARE_OP_LESS);
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = VulkanInitializer::GetDefaultDynamicState();
	std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfo =
	{
		VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/forwardLightPassVert.spv" , device_),
		VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT , GetAssetPath() + "shaders/forwardLightPassFrag.spv" , device_)
	};

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		VulkanInitializer::InitGraphicsPipelineCreateInfo(pipeline_layout_, render_pass_,
			&vertexInputStateCreateInfo, &inputAssemblyStateCreateInfo, &rasterizationStateCreateInfo, &colorBlendStateCreateInfo, &multisampleStateCreateInfo, &viewportStateCreateInfo,
			&depthStencilStateCreateInfo, &dynamicStateCreateInfo, pipelineShaderStageCreateInfo, 0);

	VULKAN_SUCCESS(vkCreateGraphicsPipelines(device_->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &pipeline_));

	return pipeline_;
}

void ForwardPlusLightPassPipeline::SetupCommandBuffer(VkCommandBuffer & commandBuffer, bool startRenderPass, bool endRenderPass)
{
	VkClearValue colorClearValue = {};
	VkClearValue depthClearValue = {};
	depthClearValue.depthStencil.depth = 1;
	std::vector<VkClearValue> clearValues = {  colorClearValue , depthClearValue };
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
	vkCmdPushConstants(commandBuffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData) , &PushConstantData);
	vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
	if (endRenderPass)
	{
		vkCmdEndRenderPass(commandBuffer);
	}
}

VkRenderPass ForwardPlusLightPassPipeline::CreateRenderPass()
{
	// Layout 
	std::vector<VkPushConstantRange> constRange = {
			VulkanInitializer::InitVkPushConstantRange(0 , sizeof(PushConstantData) , VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT )
	};
	VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size(), constRange.data(),desc_set_layout_vec_.size() , desc_set_layout_vec_.data() );
	VULKAN_SUCCESS(vkCreatePipelineLayout(device_->GetDevice(), &pipelineLayout, NULL, &pipeline_layout_));

	//Render Pass
	std::vector<VkAttachmentDescription> attachmentsDesc =
	{
		VulkanInitializer::InitAttachmentDescription(swap_chain_->GetColorFormat(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE),
		VulkanInitializer::InitAttachmentDescription(VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_LAYOUT_UNDEFINED , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL , VK_ATTACHMENT_LOAD_OP_CLEAR )
	};
	std::vector<VkAttachmentReference> colorAttachmentReference =
	{
		VulkanInitializer::InitAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
	};
	VkAttachmentReference depthStencilReference = VulkanInitializer::InitAttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	std::vector<VkSubpassDescription> subpassDesc =
	{
		VulkanInitializer::InitSubpassDescription(colorAttachmentReference , &depthStencilReference ) 
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
			depth_stencil_image_
		};
		VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(screen_width_, screen_height_, 1, 2,  attachments, render_pass_);
		vkCreateFramebuffer(device_->GetDevice(), &frameBufferCreateInfo, NULL, &(frame_buffer_vec_[i]));
	}


	return render_pass_;
}

void ForwardPlusLightPassPipeline::PrepareResources()
{
	transform_mat_uniform_buffer_ = device_->CreateVulkanBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		sizeof(TransformUniformBuffer) , 
		&TransformUniformBuffer
	);
	InitDesc();
	CreateRenderPass();
	CreateGraphicsPipeline();
}

VertexLayout ForwardPlusLightPassPipeline::GetVertexLayout(std::string & layoutName)
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

void ForwardPlusLightPassPipeline::InitDesc()
{
	VkDescriptorSetLayoutBinding matBinding = VulkanInitializer::InitBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT );
	VkDescriptorSetLayoutBinding samplerBinding[2] = {
		VulkanInitializer::InitBinding(0 , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT),
		VulkanInitializer::InitBinding(1  , 1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	VkDescriptorSetLayoutBinding lightBuffer[2] = {
		VulkanInitializer::InitBinding(0 , 1 , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , VK_SHADER_STAGE_FRAGMENT_BIT),
		VulkanInitializer::InitBinding(1 , 1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutCreateInfo = {
		VulkanInitializer::InitDescSetLayoutCreateInfo(1 , &matBinding),
		VulkanInitializer::InitDescSetLayoutCreateInfo(2 , samplerBinding),
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
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER  ,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
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
