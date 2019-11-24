#ifndef _VULKAN_PIPELINE_H_
#define _VULKAN_PIPELINE_H_

#include "Utility.h"
#include "VulkanImage.h"
#include "VulkanMesh.h"
#include "VulkanSwapChain.hpp"
#include "VulkanCamera.h"

class IRenderingPipeline
{
public:
	virtual ~IRenderingPipeline() {};
	virtual VkPipeline CreateGraphicsPipeline() = 0;
	virtual void SetupCommandBuffer(VkCommandBuffer & commandBuffer, bool startRenderPass, bool endRenderPass) = 0;
	virtual VkRenderPass CreateRenderPass() = 0;
	virtual void PrepareResources() = 0;
	virtual VertexLayout GetVertexLayout( std::string & layoutName ) = 0;
	virtual void SetMesh(VulkanMesh * mesh) = 0;
	virtual void UpdateData() = 0;
};

class IComputePipeline
{
public:
	virtual ~IComputePipeline() {};
	virtual VkPipeline CreateComputePipeline() = 0 ;
	virtual VkCommandBuffer SetupCommandBuffer() = 0 ;
	virtual void UpdateData() = 0;
};

class PreDepthRenderingPipeline : public IRenderingPipeline
{
public:
	PreDepthRenderingPipeline(int renderWidth, int renderHeight, VulkanDevice * device_ ) :
		render_width_(renderWidth), render_height(renderHeight), device_(device_) 
	{
		PrepareResources();
	}

public:
	VkPipeline CreateGraphicsPipeline();
	void SetupCommandBuffer(VkCommandBuffer & commandBuffer , bool startRenderPass, bool endRenderPass);
	VkRenderPass CreateRenderPass() ;
	void PrepareResources();
	VertexLayout GetVertexLayout(std::string & layoutName);

	VulkanImage* GetDepthImage() const
	{
		return pre_depth_image_;
	}
	void SetMesh(VulkanMesh * mesh)
	{
		mesh_ = mesh;
	}
	void SetMVP(glm::mat4 mvp)
	{
		mvpMat = mvp;
	}

	void UpdateData()
	{

	}

private:
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VulkanMesh * mesh_;
	VkRenderPass render_pass_;
	VkFramebuffer frame_buffer_;
	VulkanCamera * camera_;
private:
	glm::mat4 mvpMat;
	VulkanDevice * device_;
	VulkanImage * pre_depth_image_;
	int render_width_;
	int render_height;

private:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 world;
	};

};

class CullLightComputePipeline : public IComputePipeline 
{
public:
	VkPipeline CreateComputePipeline();
	VkCommandBuffer SetupCommandBuffer();
	void UpdateData();

public:
	CullLightComputePipeline(int tileSizeX, int tileSizeY, int tileCountX, int tileCountY, int lightCount, glm::vec3 & lightMinPos, glm::vec3 & lightMaxPos, float lightRadius , VulkanImage * preDepthImage , VulkanDevice * device  )
		: tile_size_x_(tileSizeX), tile_size_y_(tileSizeY), tile_count_x_(tileCountX), tile_count_y_(tileCountY), light_count_(lightCount), light_min_pos_(lightMinPos), light_max_pos_(lightMaxPos), light_radius_(lightRadius) , 
		pre_depth_image_(preDepthImage) , device_(device)
	{
		InitResources();
		InitDesc();
		CreateComputePipeline();
		light_radius_ = lightRadius;
		PushConstantData.tileNum[0] = tileCountX;
		PushConstantData.tileNum[1] = tileCountY;
	}

	void SetPushConstantData( int viewportSizeX , int viewportSizeY  , float zNear , float zFar , glm::mat4 & projMatrix , glm::mat4 & viewMatrix );
	void SetPreDepth(VulkanImage * preDepthImage );

	VulkanBuffer* GetTileLightVisibleBuffer() const ;
	VulkanBuffer* GetLightUniformBuffer() const ;

private:
	void InitResources( );
	void InitDesc();

private:
	VulkanBuffer * tile_light_visible_buffer_;
	VulkanBuffer * light_uniform_buffer_;
	VulkanImage * pre_depth_image_;
	VkDescriptorPool desc_pool_;
	VkDescriptorSet desc_set_;
	VkDescriptorSetLayout desc_set_layout_;
	VkPipelineLayout pipeline_layout_;
	VkPipeline compute_pipeline_;
	VkCommandBuffer compute_command_buffer_;

private:
	int tile_size_x_;
	int tile_size_y_;
	int tile_count_x_;
	int tile_count_y_;
	int light_count_;
	glm::vec3 light_min_pos_;
	glm::vec3 light_max_pos_;
	float light_radius_;
	VulkanDevice * device_;

	struct LightUniformData
	{
		uint32_t pointLightCount;
		float padding[3];
		PointLight pointLights[1000];
	} LightUniformBufferData;

	struct
	{
		glm::mat4 projMatrix;
		glm::mat4 viewMatrix;
		int tileNum[2];
		int viewportSize[2];
		float zFar;
		float zNear;
	} PushConstantData;
};

class ForwardPlusLightPassPipeline : public IRenderingPipeline 
{
public:
	VkPipeline CreateGraphicsPipeline();
	void SetupCommandBuffer(VkCommandBuffer & commandBuffer, bool startRenderPass, bool endRenderPass);
	VkRenderPass CreateRenderPass();
	void PrepareResources();
	VertexLayout GetVertexLayout(std::string & layoutName);

	void InitDesc();

	ForwardPlusLightPassPipeline(
		VulkanDevice * device,
		VulkanSwapChain * swapChain,
		VulkanBuffer * lightVisibleBuffer,
		VulkanBuffer * pointLightUniformBuffer,
		int screenWidth,
		int screenHeight , 
		int tileNumX , 
		int tileNumY , 
		glm::vec3 cameraPos ,
		VkImageView depthStencilImage , 
		VulkanCamera * camera ) :
		device_(device), swap_chain_(swapChain), light_visible_buffer_(lightVisibleBuffer),
		pointlight_uniform_buffer_(pointLightUniformBuffer), screen_width_(screenWidth), screen_height_(screenHeight) , 
		depth_stencil_image_( depthStencilImage  ) , camera_(camera)
	{
		PushConstantData.tileNum[0] = tileNumX;
		PushConstantData.tileNum[1] = tileNumY;
		TransformUniformBuffer.cameraPosition = cameraPos;

		PrepareResources();
	}

	void SetAlbedoTexture(Texture2D * albedo)
	{
		albedo_image_ = albedo;
	}

	void SetNormalTexture(Texture2D * normal)
	{
		normal_sampler_ = normal;
	}

	void SetMesh(VulkanMesh * mesh)
	{
		mesh_ = mesh;
	}

	void SetFramebufferIndex(uint32_t ind)
	{
		frame_index_ = ind;
	}

	void SetPushConstantValue( glm::mat4 model , glm::mat4 mvp , int viewportOffsetX , int viewportOffsetY  )
	{
		PushConstantData.model = model;
		PushConstantData.mvp = mvp;
		PushConstantData.viewportOffset[0] = viewportOffsetX;
		PushConstantData.viewportOffset[1] = viewportOffsetY;

	}

	void UpdateDescriptorSet()
	{
		VkWriteDescriptorSet writeDescs[5] = {
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 0 , desc_set_vec_[0] , &transform_mat_uniform_buffer_->GetDesc()),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 0 , desc_set_vec_[1] , &albedo_image_->image_info_),
			VulkanInitializer::InitWriteImageDescriptorSet(1 , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER , 1 , desc_set_vec_[1] , &normal_sampler_->image_info_),
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , 0 , desc_set_vec_[2] , &light_visible_buffer_->GetDesc()),
			VulkanInitializer::InitWriteBufferDescriptorSet(1 , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER , 1 , desc_set_vec_[2] , &pointlight_uniform_buffer_->GetDesc())
		};

		vkUpdateDescriptorSets(device_->GetDevice(), 5, writeDescs, 0, NULL);
	}

	void UpdateData()
	{
		transform_mat_uniform_buffer_->Map();
		TransformData * data = (TransformData * )(transform_mat_uniform_buffer_->GetMappedMemory());
		data->cameraPosition = camera_->position;
		transform_mat_uniform_buffer_->Unmap();
	}

private:
	VkRenderPass render_pass_;
	VkPipeline pipeline_;
	VkPipelineLayout pipeline_layout_;
	VulkanCamera * camera_;
	VkDescriptorPool desc_pool_;
	std::vector<VkFramebuffer> frame_buffer_vec_;
	std::vector<VkDescriptorSetLayout> desc_set_layout_vec_;
	std::vector<VkDescriptorSet> desc_set_vec_;
	
	struct TransformData{
		glm::vec3 cameraPosition;
	}TransformUniformBuffer;

	struct {
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 texCoord;
		glm::vec3 normal;
	}Vertex;

	struct
	{
		int tileNum[2];
		int viewportOffset[2];
		glm::mat4 model;
		glm::mat4 mvp;
	} PushConstantData;

private:
	VulkanDevice* device_;
	VulkanSwapChain * swap_chain_;
	Texture2D * albedo_image_;
	Texture2D * normal_sampler_;
	VulkanBuffer * light_visible_buffer_;
	VulkanBuffer * pointlight_uniform_buffer_;
	VulkanBuffer * transform_mat_uniform_buffer_;
	VkImageView depth_stencil_image_;
	VulkanMesh * mesh_;
	uint32_t screen_width_;
	uint32_t screen_height_;
	uint32_t frame_index_;

	friend class ForwardLightPassMaterial;
};

#endif