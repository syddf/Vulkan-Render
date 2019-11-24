#ifndef _VULKAN_MATERIAL_H_
#define _VULKAN_MATERIAL_H_

#include "Utility.h"
#include "VulkanPipeline.h"
#include "PbrLightPipeline.h"
#include "GBufferPipeline.h"
#include "imgui.h"
#include <unordered_map>

class IMaterial
{
public:
	virtual ~IMaterial() {};
	virtual void UpdatePipelineData() = 0;
	virtual void UpdateImgui() = 0 ;
	virtual void SetupCommandBuffer( VkCommandBuffer & commandBuffer ) = 0 ;
	virtual void SetPipeline(IRenderingPipeline * renderingPipeline ) = 0 ;
};

class EmptyMaterial : public IMaterial
{
public:
	~EmptyMaterial() {};
	void UpdatePipelineData() {};
	void UpdateImgui() {};
	void SetupCommandBuffer(VkCommandBuffer & commandBuffer) {};
	void SetPipeline(IRenderingPipeline * renderingPipeline) {};
};

class ForwardLightPassMaterial : public IMaterial 
{
public:
	ForwardLightPassMaterial( Texture2D * albedo , Texture2D * normal , ForwardPlusLightPassPipeline * pipeline  , VulkanDevice * device  )
	{
		albedo_image_ = albedo;
		normal_image_ = normal;
		pipeline_ = pipeline;
		device_ = device;
		CreateDescriptorSet(device);
	}

	void CreateDescriptorSet( VulkanDevice * device  )
	{
		VkDescriptorSetLayoutBinding matBinding = VulkanInitializer::InitBinding(0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
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

		std::vector<VkDescriptorType> descTypeVec =
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER  ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
		};
		desc_set_vec_.resize(descSetLayoutCreateInfo.size());
		std::vector<VkDescriptorPoolSize> descPoolSize = VulkanInitializer::InitDescriptorPoolSizeVec(descTypeVec);
		VkDescriptorPoolCreateInfo descPoolCreateInfo = VulkanInitializer::InitDescriptorPoolCreateInfo(descPoolSize, descSetLayoutCreateInfo.size());
		VULKAN_SUCCESS(vkCreateDescriptorPool(device->GetDevice(), &descPoolCreateInfo, NULL, &desc_pool_));

		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VkDescriptorSetAllocateInfo allocateInfo = VulkanInitializer::InitDescriptorSetAllocateInfo(1, desc_pool_, &pipeline_->desc_set_layout_vec_[i]);
			VULKAN_SUCCESS(vkAllocateDescriptorSets(device->GetDevice(), &allocateInfo, &desc_set_vec_[i]));
		}

	}

	~ForwardLightPassMaterial()
	{

	}

	void UpdatePipelineData() 
	{
		pipeline_->SetAlbedoTexture(albedo_image_);
		pipeline_->SetNormalTexture(normal_image_);
	}

	void UpdateImgui()
	{

	}

	void SetupCommandBuffer(VkCommandBuffer & commandBuffer)
	{
		VkWriteDescriptorSet writeDescs[5] = {
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 0 , desc_set_vec_[0] , &pipeline_->transform_mat_uniform_buffer_->GetDesc()),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 0 , desc_set_vec_[1] , &albedo_image_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 1 , desc_set_vec_[1] , &normal_image_->image_info_),
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 0 , desc_set_vec_[2] , &pipeline_->light_visible_buffer_->GetDesc()),
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 , desc_set_vec_[2] , &pipeline_->pointlight_uniform_buffer_->GetDesc())
		};
		vkUpdateDescriptorSets(device_->GetDevice(), 5, writeDescs, 0, NULL);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline_layout_, 0, desc_set_vec_.size(), desc_set_vec_.data(), 0, NULL);
	}

	void SetPipeline(IRenderingPipeline * renderingPipeline)
	{
		pipeline_ = dynamic_cast<ForwardPlusLightPassPipeline*>(renderingPipeline);
		if (pipeline_ == NULL)
		{
			throw "Unexpected pipeline . ";
		}
	}

private:
	Texture2D * albedo_image_;
	Texture2D * normal_image_;
	ForwardPlusLightPassPipeline * pipeline_;
	VkDescriptorPool desc_pool_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	VulkanDevice * device_;
};

class PbrLightPassMaterial : public IMaterial
{
public:
	const char* albedoTexName[2] = { "treeAlbedo" , "cerberusAlbedo" };
	const char* aoTexName[2] = { "treeAo" , "cerberusAo"};
	const char* metallicTexName[2] = { "treeMetallic" , "cerberusMetallic" };
	const char* roughnessTexName[2] = { "treeRoughness" , "cerberusRoughness" };
	const char* normalTexName[2] = { "treeNormal", "cerberusNormal" };

	PbrLightPassMaterial(Texture2D * albedoTex , Texture2D * normalTex, Texture2D * aoTex ,  Texture2D * metallicTex , 
	Texture2D * roughnessTex, VulkanImage * irradianceTex, Texture2D * brdfLutTex , VulkanImage * prefilterTex ,
		VulkanDevice * device , PBRLightPipeline * lightPipeline , std::unordered_map<std::string, Texture*> & textures) : textures_(textures)
	{
		albedo_texture_ = albedoTex;
		normal_texture_ = normalTex;
		ao_texture_ = aoTex;
		metallic_texture_ = metallicTex;
		roughness_texture_ = roughnessTex;
		irradiance_texture_ = irradianceTex;
		brdf_lut_texture_ = brdfLutTex;
		prefileter_texture_ = prefilterTex;
		device_ = device;
		pipeline_ = lightPipeline;

		tex_entry_.albedo_texture_name_ = &albedoTexName[0];
		tex_entry_.ao_texture_name_ = &aoTexName[0];
		tex_entry_.metallic_texture_name_ = &metallicTexName[0];
		tex_entry_.roughness_texture_name_ = &roughnessTexName[0];
		tex_entry_.normal_texture_name_ = &normalTexName[0];

		CreateDescriptorSet(device);
	}

	void CreateDescriptorSet(VulkanDevice * device)
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
		std::vector<VkDescriptorType> descTypeVec =
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER  ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER  ,
		};
		desc_set_vec_.resize(2);
		std::vector<VkDescriptorPoolSize> descPoolSize = VulkanInitializer::InitDescriptorPoolSizeVec(descTypeVec);
		VkDescriptorPoolCreateInfo descPoolCreateInfo = VulkanInitializer::InitDescriptorPoolCreateInfo(descPoolSize, descSetLayoutCreateInfo.size());
		VULKAN_SUCCESS(vkCreateDescriptorPool(device->GetDevice(), &descPoolCreateInfo, NULL, &desc_pool_));

		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VkDescriptorSetAllocateInfo allocateInfo = VulkanInitializer::InitDescriptorSetAllocateInfo(1, desc_pool_, &pipeline_->desc_set_layout_vec_[i]);
			VULKAN_SUCCESS(vkAllocateDescriptorSets(device->GetDevice(), &allocateInfo, &desc_set_vec_[i]));
		}

	}

	~PbrLightPassMaterial()
	{

	}

	void UpdatePipelineData()
	{

	}

	void UpdateImgui()
	{
		if (ImGui::TreeNode("PBR Material"))
		{
			auto BuildCombo = [&](const char * paramName , const char **& param ,const char ** paramList , int paramSize  )->void
			{
				if (ImGui::BeginCombo( paramName , *param, 0))
				{
					for (int n = 0; n < paramSize; n++)
					{
						bool is_selected = (param == &paramList[n]);
						if (ImGui::Selectable(paramList[n], is_selected))
							param = &paramList[n];
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			};

			BuildCombo(&"Albedo"[0], tex_entry_.albedo_texture_name_, &albedoTexName[0] , sizeof(albedoTexName) / sizeof(albedoTexName[0]));
			BuildCombo(&"Ao"[0], tex_entry_.ao_texture_name_, &aoTexName[0], sizeof(aoTexName) / sizeof(aoTexName[0]));
			BuildCombo(&"Normal"[0], tex_entry_.normal_texture_name_, &normalTexName[0], sizeof(normalTexName) / sizeof(normalTexName[0]));
			BuildCombo(&"Metallic"[0], tex_entry_.metallic_texture_name_, &metallicTexName[0], sizeof(metallicTexName) / sizeof(metallicTexName[0]));
			BuildCombo(&"Roughness"[0], tex_entry_.roughness_texture_name_, &roughnessTexName[0], sizeof(roughnessTexName) / sizeof(roughnessTexName[0]));

			albedo_texture_ = dynamic_cast<Texture2D*>( (*textures_.find(*tex_entry_.albedo_texture_name_)).second );
			ao_texture_ = dynamic_cast<Texture2D*>((*textures_.find(*tex_entry_.ao_texture_name_)).second);
			normal_texture_ = dynamic_cast<Texture2D*>((*textures_.find(*tex_entry_.normal_texture_name_)).second);
			metallic_texture_ = dynamic_cast<Texture2D*>((*textures_.find(*tex_entry_.metallic_texture_name_)).second);
			roughness_texture_ = dynamic_cast<Texture2D*>((*textures_.find(*tex_entry_.roughness_texture_name_)).second);

			ImGui::TreePop();
		}
	}

	void SetupCommandBuffer(VkCommandBuffer & commandBuffer)
	{
		VkWriteDescriptorSet writeDescs[11] = {
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 0 , desc_set_vec_[0] , &pipeline_->uniform_buffer_->GetDesc()),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 1 , desc_set_vec_[0] , &irradiance_texture_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 2 , desc_set_vec_[0] , &brdf_lut_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 3 , desc_set_vec_[0] , &prefileter_texture_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 4 , desc_set_vec_[0] , &albedo_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 5 , desc_set_vec_[0] , &normal_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 6 , desc_set_vec_[0] , &ao_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 7 , desc_set_vec_[0] , &metallic_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 8 , desc_set_vec_[0] , &roughness_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 0 , desc_set_vec_[1] , &pipeline_->shadow_map_image_->GetDescriptorImageInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)),
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 , desc_set_vec_[1] , &pipeline_->cascade_transform_buffer_->GetDesc())
		};
		vkUpdateDescriptorSets(device_->GetDevice(), 11, writeDescs, 0, NULL);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline_layout_, 0, desc_set_vec_.size(), desc_set_vec_.data(), 0, NULL);
	}

	void SetPipeline(IRenderingPipeline * renderingPipeline)
	{
		pipeline_ = dynamic_cast<PBRLightPipeline*>(renderingPipeline);
		if (pipeline_ == NULL)
		{
			throw "Unexpected pipeline . ";
		}
	}

private:
	VulkanImage * irradiance_texture_;
	Texture2D * brdf_lut_texture_;
	VulkanImage * prefileter_texture_;
	Texture2D * albedo_texture_;
	Texture2D * normal_texture_;
	Texture2D * ao_texture_;
	Texture2D * metallic_texture_;
	Texture2D * roughness_texture_;

	PBRLightPipeline * pipeline_;
	VkDescriptorPool desc_pool_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	VulkanDevice * device_;
	const std::unordered_map<std::string, Texture*> & textures_;

	struct PBRTextureEntry
	{
		const char ** albedo_texture_name_;
		const char ** normal_texture_name_;
		const char ** ao_texture_name_;
		const char ** metallic_texture_name_;
		const char ** roughness_texture_name_;
	}tex_entry_;
};

class TBDRMaterial : public IMaterial
{
public:
	TBDRMaterial(Texture2D * albedoTex, Texture2D * normalTex, Texture2D * aoTex, Texture2D * metallicTex , Texture2D * roughnessTex, VulkanDevice * device , GBufferPipeline * pipeline)
	{
		albedo_texture_ = albedoTex;
		normal_texture_ = normalTex;
		ao_texture_ = aoTex;
		metallic_texture_ = metallicTex;
		roughness_texture_ = roughnessTex;
		device_ = device;
		pipeline_ = pipeline;
		CreateDescriptorSet(device);
	}

	void CreateDescriptorSet(VulkanDevice * device)
	{
		VkDescriptorSetLayoutBinding binding[5] =
		{
			VulkanInitializer::InitBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
			VulkanInitializer::InitBinding(4, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
		};

		std::vector<VkDescriptorSetLayoutCreateInfo> descSetLayoutCreateInfo = {
			VulkanInitializer::InitDescSetLayoutCreateInfo(5, binding)
		};
		std::vector<VkDescriptorType> descTypeVec =
		{
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER  ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER 
		};
		desc_set_vec_.resize(1);
		std::vector<VkDescriptorPoolSize> descPoolSize = VulkanInitializer::InitDescriptorPoolSizeVec(descTypeVec);
		VkDescriptorPoolCreateInfo descPoolCreateInfo = VulkanInitializer::InitDescriptorPoolCreateInfo(descPoolSize, descSetLayoutCreateInfo.size());
		VULKAN_SUCCESS(vkCreateDescriptorPool(device->GetDevice(), &descPoolCreateInfo, NULL, &desc_pool_));

		for (int i = 0; i < descSetLayoutCreateInfo.size(); i++)
		{
			VkDescriptorSetAllocateInfo allocateInfo = VulkanInitializer::InitDescriptorSetAllocateInfo(1, desc_pool_, &pipeline_->desc_set_layout_);
			VULKAN_SUCCESS(vkAllocateDescriptorSets(device->GetDevice(), &allocateInfo, &desc_set_vec_[i]));
		}

	}

	~TBDRMaterial()
	{

	}

	void UpdatePipelineData()
	{

	}

	void UpdateImgui()
	{
	}

	void SetupCommandBuffer(VkCommandBuffer & commandBuffer)
	{
		VkWriteDescriptorSet writeDescs[5] = {
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 0 , desc_set_vec_[0] , &albedo_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 1 , desc_set_vec_[0] , &normal_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 2 , desc_set_vec_[0] , &ao_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 3 , desc_set_vec_[0] , &metallic_texture_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 4 , desc_set_vec_[0] , &roughness_texture_->image_info_)
		};

		vkUpdateDescriptorSets(device_->GetDevice(), 5, writeDescs, 0, NULL);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline_layout_, 0, desc_set_vec_.size(), desc_set_vec_.data(), 0, NULL);
	}

	void SetPipeline(IRenderingPipeline * renderingPipeline)
	{
		pipeline_ = dynamic_cast<GBufferPipeline*>(renderingPipeline);
		if (pipeline_ == NULL)
		{
			throw "Unexpected pipeline . ";
		}
	}

private:
	Texture2D * albedo_texture_;
	Texture2D * normal_texture_;
	Texture2D * ao_texture_;
	Texture2D * metallic_texture_;
	Texture2D * roughness_texture_;

	GBufferPipeline * pipeline_;
	VkDescriptorPool desc_pool_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	VulkanDevice * device_;
};



#endif