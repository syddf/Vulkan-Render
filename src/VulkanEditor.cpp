#include "VulkanEditor.h"

void VulkanEditor::OnUpdateImgui( )
{
	size_t object_count = objects_.size();
	ImGui::Checkbox("Show Editor" , &GlobalData.ShowEditor);
	if (ImGui::CollapsingHeader("Objects"))
	{
		for (int i = 0; i < object_count; i++)
		{
			std::string name = std::to_string(objects_[i]->object_entry_.ind) + " " + objects_[i]->object_entry_.name;
			if (ImGui::TreeNode(name.c_str()))
			{
				float position[3] = { objects_[i]->object_entry_.position.x , objects_[i]->object_entry_.position.y , objects_[i]->object_entry_.position.z };
				float rotation[3] = { objects_[i]->object_entry_.rotation.x , objects_[i]->object_entry_.rotation.y , objects_[i]->object_entry_.rotation.z };
				float scale[3] = { objects_[i]->object_entry_.scale.x , objects_[i]->object_entry_.scale.y , objects_[i]->object_entry_.scale.z };

				ImGui::DragFloat3("position", position , 0.01f);
				ImGui::DragFloat3("rotation", rotation , 0.01f );
				ImGui::DragFloat3("scale", scale , 0.01f);

				objects_[i]->object_entry_.position.x = position[0]; objects_[i]->object_entry_.position.y = position[1]; objects_[i]->object_entry_.position.z = position[2];
				objects_[i]->object_entry_.rotation.x = rotation[0]; objects_[i]->object_entry_.rotation.y = rotation[1]; objects_[i]->object_entry_.rotation.z = rotation[2];
				objects_[i]->object_entry_.scale.x = scale[0]; objects_[i]->object_entry_.scale.y = scale[1]; objects_[i]->object_entry_.scale.z = scale[2];

				const char* mesh_items[] = { "sphere" , "chalet" , "cube" , "cerberus" };
				const char** mesh_item_current = &mesh_items[objects_[i]->object_entry_.meshIndex];
				if (ImGui::BeginCombo("Mesh ", *mesh_item_current, 0))
				{
					for (int n = 0; n < IM_ARRAYSIZE(mesh_items); n++)
					{
						bool is_selected = (*mesh_item_current == mesh_items[n]);
						if (ImGui::Selectable(mesh_items[n], is_selected))
							mesh_item_current = &mesh_items[n];
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				objects_[i]->object_entry_.meshIndex = mesh_item_current - mesh_items;

				const char* pipeline_items[] = { "No Pipeline" , "Forward Plus Pipeline" , "PBR Forward Pipeline" };
				const char** pipeline_item_current = &pipeline_items[objects_[i]->object_entry_.pipelineType];
				if (ImGui::BeginCombo("Pipeline ", *pipeline_item_current, 0))
				{
					for (int n = 0; n < IM_ARRAYSIZE(pipeline_items); n++)
					{
						bool is_selected = (pipeline_item_current == &pipeline_items[n]);
						if (ImGui::Selectable(pipeline_items[n], is_selected))
							pipeline_item_current = &pipeline_items[n];
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				objects_[i]->object_entry_.pipelineType = (PipelineType)(pipeline_item_current - pipeline_items);
				objects_[i]->UpdateImguI();
				ImGui::Button("Locate Object");
				ImGui::TreePop();
			}
		}
	}

	if (ImGui::Button("Add Object"))
	{
		AddNewEmptyObject();
	}
}

void VulkanEditor::MouseEvent(const MouseStateType & mouseState)
{
	static float lastMovePosition[2] = { 0.0f , 0.0f } ;
	if ( mouseState.LeftMouseButtonDown )
	{
		int axisInd = PushConstantData.axisIndex;

		if (PushConstantData.axisIndex == 0)
		{
			VulkanBuffer *stagingBuffer = RenderResource.vulkanDevice->CreateVulkanBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, RenderResource.axisIndexAttachmentImage->image_size_);
			RenderResource.axisIndexAttachmentImage->MapMemory(RenderResource.commandQueue, stagingBuffer);
			stagingBuffer->Map();
			int * axisIndexData = (int*)stagingBuffer->GetMappedMemory();
			int pixelInd = mouseState.mouse_position_.y * screen_width_ + mouseState.mouse_position_.x;
			axisInd = *(axisIndexData + pixelInd);
			lastMovePosition[0] = PushConstantData.position[0];
			lastMovePosition[1] = PushConstantData.position[1];
			PushConstantData.axisIndex = axisInd;
			stagingBuffer->Unmap();
			delete stagingBuffer;
		}

		const float speed = 0.002f;

		if (axisInd != 0)
		{
			glm::vec2 deltaPos = mouseState.mouse_position_ - mouseState.last_left_button_down_position_;
			if (axisInd == 1)
			{
				PushConstantData.position[0] =  lastMovePosition[0] + speed * deltaPos.x;
			}
			else if (axisInd == 2)
			{
				PushConstantData.position[1] = lastMovePosition[1] + speed * deltaPos.y;
			}
			else if (axisInd == 3)
			{

			}
		}
	}
	else {
		PushConstantData.axisIndex = 0;
	}
}

void VulkanEditor::AddNewEmptyObject()
{
	int ind = objects_.size();
	std::string name = std::to_string(ind) + " object ";
	VulkanObject * obj = new VulkanObject(ind, name);
	IMaterial * emptyMaterial = new EmptyMaterial();
	obj->SetPipelineType(PIPELINE_EMPTY);
	obj->ResetMaterial(emptyMaterial, PIPELINE_EMPTY);
	obj->SetMeshIndex(0);
	objects_.push_back(obj);
}

void VulkanEditor::UpdateEditAxis( int commandBufferInd )
{
	std::vector<VkClearValue> clearValues;
	VkClearValue clear;
	clear.color = { 0 , 0 , 0 , 0 };
	clearValues.push_back(clear);
	clearValues.push_back(clear);
	VkBuffer vertBuffer = RenderResource.vertexBuffer->GetDesc().buffer;
	VkDeviceSize offset = 0;
	RenderResource.axisRenderCommandBufferVec.resize(RenderResource.swapChain->GetImageCount());

	VkCommandBufferBeginInfo commandBufferBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo(0);
	VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(RenderResource.renderPass, RenderResource.frameBufferVec[commandBufferInd], clearValues, screen_width_, screen_height_);
	vkBeginCommandBuffer(RenderResource.axisRenderCommandBufferVec[commandBufferInd], &commandBufferBeginInfo);
	vkCmdPushConstants(RenderResource.axisRenderCommandBufferVec[commandBufferInd], RenderResource.axisRenderPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &PushConstantData);
	vkCmdPushConstants(RenderResource.axisRenderCommandBufferVec[commandBufferInd], RenderResource.axisRenderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &PushConstantData);
	vkCmdBeginRenderPass(RenderResource.axisRenderCommandBufferVec[commandBufferInd], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = VulkanInitializer::InitViewport(editor_x_, editor_y_, editor_width_, editor_height_, 0.0f, 1.0f);
	VkRect2D scissor = { 0 , 0 , screen_width_ , screen_height_ };
	vkCmdSetViewport(RenderResource.axisRenderCommandBufferVec[commandBufferInd], 0, 1, &viewport);
	vkCmdSetScissor(RenderResource.axisRenderCommandBufferVec[commandBufferInd], 0, 1, &scissor);
	vkCmdBindDescriptorSets(RenderResource.axisRenderCommandBufferVec[commandBufferInd], VK_PIPELINE_BIND_POINT_GRAPHICS, RenderResource.axisRenderPipelineLayout,
			0, 1, &RenderResource.axisRenderDescriptorSet, 0, NULL);
	vkCmdBindVertexBuffers(RenderResource.axisRenderCommandBufferVec[commandBufferInd], 0, 1, &vertBuffer, &offset);
	vkCmdBindIndexBuffer(RenderResource.axisRenderCommandBufferVec[commandBufferInd], RenderResource.indexBuffer->GetDesc().buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(RenderResource.axisRenderCommandBufferVec[commandBufferInd], VK_PIPELINE_BIND_POINT_GRAPHICS, RenderResource.axisRenderPipeline);
	vkCmdDrawIndexed(RenderResource.axisRenderCommandBufferVec[commandBufferInd], 6, 1, 0, 0, 0);
	vkCmdEndRenderPass(RenderResource.axisRenderCommandBufferVec[commandBufferInd]);
	vkEndCommandBuffer(RenderResource.axisRenderCommandBufferVec[commandBufferInd]);



}

void VulkanEditor::PrepareEditUIPass(   )
{
	auto buildRenderPipeline = [&]()->void
	{
		std::vector<VkPushConstantRange> constRange = {
			VulkanInitializer::InitVkPushConstantRange(0 , sizeof(float) * 2 , VK_SHADER_STAGE_VERTEX_BIT) ,
			VulkanInitializer::InitVkPushConstantRange(sizeof(float) * 2  , sizeof(int) , VK_SHADER_STAGE_FRAGMENT_BIT)
		};

		VkDescriptorSetLayoutBinding descriptorBinding = {};
		descriptorBinding.binding = 0;
		descriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorBinding.descriptorCount = 1;
		descriptorBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = VulkanInitializer::InitDescSetLayoutCreateInfo(1, &descriptorBinding);
		VULKAN_SUCCESS(vkCreateDescriptorSetLayout(RenderResource.vulkanDevice->GetDevice(), &descriptorSetLayoutCreateInfo, NULL, &RenderResource.axisRenderDescriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayout = VulkanInitializer::InitPipelineLayoutCreateInfo(constRange.size() , constRange.data(), 1, &RenderResource.axisRenderDescriptorSetLayout);
		VULKAN_SUCCESS(vkCreatePipelineLayout(RenderResource.vulkanDevice->GetDevice(), &pipelineLayout, NULL, &RenderResource.axisRenderPipelineLayout));

		RenderResource.axisIndexAttachmentImage = new VulkanImage(
			RenderResource.vulkanDevice,
			VK_FORMAT_R32_SINT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED ,
			screen_width_,
			screen_height_,
			VK_IMAGE_ASPECT_COLOR_BIT
		);

		RenderResource.axisImage = new Texture2D(
			GetAssetPath() + "textures/axis.png",	VK_FORMAT_R8G8B8A8_UNORM, RenderResource.vulkanDevice, RenderResource.commandQueue , 4 , true );

		VkImageView attachments[2] = { {} , RenderResource.axisIndexAttachmentImage->image_view_ };

		std::vector<VkAttachmentDescription> attachmentsDesc;
		attachmentsDesc.push_back(VulkanInitializer::InitAttachmentDescription(RenderResource.swapChain->GetColorFormat(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_LOAD , VK_ATTACHMENT_STORE_OP_STORE));
		attachmentsDesc.push_back(VulkanInitializer::InitAttachmentDescription(RenderResource.axisIndexAttachmentImage->image_format_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE));

		std::vector<VkAttachmentReference> attachmentReference;
		attachmentReference.push_back(VulkanInitializer::InitAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
		attachmentReference.push_back(VulkanInitializer::InitAttachmentReference(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
		std::vector<VkSubpassDescription> subpassDesc;
		subpassDesc.push_back(VulkanInitializer::InitSubpassDescription(attachmentReference));

		std::vector<VkSubpassDependency> subpassDependency = VulkanInitializer::GetDefaultSubpassDependency();
		VkRenderPassCreateInfo renderPassCreateInfo = VulkanInitializer::InitRenderPassCreateInfo(attachmentsDesc, subpassDesc, subpassDependency);
		VULKAN_SUCCESS(vkCreateRenderPass(RenderResource.vulkanDevice->GetDevice(), &renderPassCreateInfo, NULL, &RenderResource.renderPass));

		VkFramebufferCreateInfo frameBufferCreateInfo = VulkanInitializer::InitFrameBufferCreateInfo(screen_width_, screen_height_, 1, 2, attachments, RenderResource.renderPass);
		RenderResource.frameBufferVec.resize( RenderResource.swapChain->GetImageCount() );

		for (int i = 0; i < RenderResource.swapChain->GetImageCount(); i++)
		{
			attachments[0] = RenderResource.swapChain->GetImageView(i);
			vkCreateFramebuffer(RenderResource.vulkanDevice->GetDevice(), &frameBufferCreateInfo, NULL, &RenderResource.frameBufferVec[i]);
		}

		VkVertexInputBindingDescription binding = VulkanInitializer::InitVertexInputBindingDescription(0, VK_VERTEX_INPUT_RATE_VERTEX, sizeof(Vertex));

		VkVertexInputAttributeDescription attribute[2];
		attribute[0] = VulkanInitializer::InitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, 0);
		attribute[1] = VulkanInitializer::InitVertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2);

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = VulkanInitializer::InitVertexInputState(1, &binding, 2, attribute);
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = VulkanInitializer::GetNormalInputAssembly();
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = VulkanInitializer::InitRasterizationStateCreateInfo(VK_CULL_MODE_NONE);

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState[2] = {
			VulkanInitializer::InitColorBlendAttachmentState(VK_TRUE),
			VulkanInitializer::InitColorBlendAttachmentState(VK_FALSE)
		};

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = VulkanInitializer::InitPipelineColorBlendState(2, colorBlendAttachmentState);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = VulkanInitializer::InitMultiSampleState(VK_SAMPLE_COUNT_1_BIT);
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = VulkanInitializer::InitViewportState(1, 1);
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = VulkanInitializer::InitDepthStencilState(VK_FALSE, VK_FALSE, VK_COMPARE_OP_ALWAYS);
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = VulkanInitializer::GetDefaultDynamicState();
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfo =
		{
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT , GetAssetPath() + "shaders/axisUIVertex.spv" , RenderResource.vulkanDevice),
			VulkanInitializer::InitShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT , GetAssetPath() + "shaders/axisUIFrag.spv" , RenderResource.vulkanDevice)
		};
		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
			VulkanInitializer::InitGraphicsPipelineCreateInfo(RenderResource.axisRenderPipelineLayout, RenderResource.renderPass,
				&vertexInputStateCreateInfo, &inputAssemblyStateCreateInfo, &rasterizationStateCreateInfo, &colorBlendStateCreateInfo, &multisampleStateCreateInfo, &viewportStateCreateInfo,
				&depthStencilStateCreateInfo, &dynamicStateCreateInfo, pipelineShaderStageCreateInfo, 0);

		VULKAN_SUCCESS(vkCreateGraphicsPipelines(RenderResource.vulkanDevice->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &RenderResource.axisRenderPipeline));
	};

	auto buildRenderDescriptorSet = [&]() ->void
	{
		std::vector<VkDescriptorType> descType = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
		std::vector<VkDescriptorPoolSize> descPoolSizeVec = VulkanInitializer::InitDescriptorPoolSizeVec( descType );
		VkDescriptorPoolCreateInfo descPoolCreateInfo = VulkanInitializer::InitDescriptorPoolCreateInfo(descPoolSizeVec, 1);
		VULKAN_SUCCESS( vkCreateDescriptorPool( RenderResource.vulkanDevice->GetDevice() ,  &descPoolCreateInfo , NULL , &RenderResource.axisRenderDescriptorPool) );
		VkDescriptorSetAllocateInfo descSetAllocateInfo = VulkanInitializer::InitDescriptorSetAllocateInfo(1, RenderResource.axisRenderDescriptorPool, &RenderResource.axisRenderDescriptorSetLayout);
		VULKAN_SUCCESS( vkAllocateDescriptorSets( RenderResource.vulkanDevice->GetDevice() , &descSetAllocateInfo, &RenderResource.axisRenderDescriptorSet) );
		VkWriteDescriptorSet writeDescSet = VulkanInitializer::InitWriteImageDescriptorSet(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, RenderResource.axisRenderDescriptorSet, &RenderResource.axisImage->image_info_);
		vkUpdateDescriptorSets(RenderResource.vulkanDevice->GetDevice(), 1, &writeDescSet, 0, NULL);
	};

	auto buildVertexBuffer = [&]() -> void
	{
		float axisSizeX = 0.15f;
		float axisSizeY = axisSizeX * editor_width_ / editor_height_;
		const int vertsSize = 4;
		const int indicesSize = 6;
		Vertex verts[vertsSize] = {
			{ { -axisSizeX , axisSizeY } , { 0.0f , 1.0f } },
			{ { axisSizeX, axisSizeY } , { 1.0f , 1.0f } },
			{ { axisSizeX , -axisSizeY } , { 1.0f , 0.0f } },
			{ { -axisSizeX , -axisSizeY } , { 0.0f , 0.0f } } , 
		};

		uint32_t indices[indicesSize] = { 0 , 1 ,  2 ,  0 , 2 , 3   };

		RenderResource.vertexBuffer = RenderResource.vulkanDevice->CreateVulkanBuffer( 
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT , sizeof(verts) , verts ) ;

		RenderResource.indexBuffer = RenderResource.vulkanDevice->CreateVulkanBuffer(
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(indices), indices);
	};

	auto buildCommandBuffer = [&]() -> void
	{
		RenderResource.axisRenderCommandBufferVec.resize(RenderResource.swapChain->GetImageCount());
		for (int i = 0; i < RenderResource.swapChain->GetImageCount(); i++)
		{
			RenderResource.vulkanDevice->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &RenderResource.axisRenderCommandBufferVec[i]);
		}
	};


	buildRenderPipeline();
	buildRenderDescriptorSet();
	buildVertexBuffer();
	buildCommandBuffer();
}

VkCommandBuffer VulkanEditor::GetCommandBuffer(int index) const
{
	return RenderResource.axisRenderCommandBufferVec[index];
}
