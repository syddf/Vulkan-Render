#ifndef _VULKAN_OBJECT_H_
#define _VULKAN_OBJECT_H_

#include "VulkanMaterial.h"
#include <vector>
#include "VulkanMesh.h"
#include "VulkanImage.h"
#include <glm/gtx/hash.hpp>

template <class T>
void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
};

struct ObjectEntry
{
	int ind;
	std::string name;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	PipelineType pipelineType;
	int meshIndex;
	bool isStaticMesh;
};

class VulkanObject
{
public:
	VulkanObject(int ind, std::string & name , VulkanMesh * staticMesh = NULL )
	{
		object_entry_.ind = ind;
		object_entry_.name = name;
		object_entry_.position = glm::vec3(0.0f, 0.0f, 0.0f);
		object_entry_.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		object_entry_.scale = glm::vec3(1.0f, 1.0f, 1.0f);
		object_entry_.meshIndex = -1;
		object_entry_.isStaticMesh = staticMesh != NULL ;

		static_mesh_ = staticMesh;
		prev_pipeline_type = PIPELINE_EMPTY;
		material_ = NULL;
	}

	glm::mat4 GetWorldMatrix() const;

	void UpdatePipeline()
	{
		material_->UpdatePipelineData();
	}
	void ResetMaterial(IMaterial * material , PipelineType pipelineType )
	{
		if (material_ != NULL) delete material_;
		material_ = material;
		prev_pipeline_type = pipelineType;
	}
	void SetPosition(glm::vec3 & position) { object_entry_.position = position; };
	void SetRotation(glm::vec3 & rotation) { object_entry_.rotation = rotation; };
	void SetScale(glm::vec3 & scale) { object_entry_.scale = scale; };
	void SetPipelineType(PipelineType pipelineType) { 
		object_entry_.pipelineType = pipelineType;  
		if (prev_pipeline_type == PIPELINE_EMPTY)
		{
			prev_pipeline_type = pipelineType;
		}
	}
	void SetMeshIndex(int index) { object_entry_.meshIndex = index;  };
	void SetPipeline(IRenderingPipeline * pipeline) { material_->SetPipeline(pipeline); };
	bool GetStaticMesh(VulkanMesh *& staticMesh) const { staticMesh = static_mesh_; return object_entry_.isStaticMesh; }
	void UpdateImguI() { material_->UpdateImgui(); };
	int GetMeshIndex() const { return object_entry_.meshIndex; }
	PipelineType GetPipelineType() const { return object_entry_.pipelineType; }
	PipelineType GetPrevPipelineType() const { return prev_pipeline_type; }
	bool IsStaticMesh() const { return object_entry_.isStaticMesh;  }

	void SetupCommandBuffer(VkCommandBuffer & commandBuffer)
	{
		material_->SetupCommandBuffer(commandBuffer);
	}

private:
	ObjectEntry object_entry_;
	IMaterial * material_;
	VulkanMesh * static_mesh_;
	PipelineType prev_pipeline_type;
	friend class VulkanEditor;
};

class VulkanSceneObjectsGroup
{
public:
	VulkanSceneObjectsGroup( std::string & file , std::string & folder , VulkanDevice * device , VkQueue queue ) 
	{
		LoadObjectFromFile(file, folder, device, queue);
	};
	~VulkanSceneObjectsGroup() {} ;

public:
	struct Material
	{
		VulkanBuffer * vertBuffer;
		VulkanBuffer * indicesBuffer;
		Texture2D * albedoImage = NULL ;
		Texture2D * normalIamge = NULL ;
		size_t vertSize;
		size_t indexSize;
	};

	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;
		glm::vec3 normal;
		bool operator == ( const Vertex & other ) const 
		{
			return other.pos == pos && other.color == color && other.texCoord == texCoord && other.normal == normal;
		}

		size_t hash() const
		{
			size_t seed = 0;
			hash_combine(seed, pos);
			hash_combine(seed, color);
			hash_combine(seed, texCoord);
			hash_combine(seed, normal);
			return seed;
		}
	};

	struct VertexHasher
	{
		size_t operator()(const Vertex & vert) const 
		{
			return vert.hash();
		}
	};


public:
	std::vector<Material> LoadObjectFromFile(std::string & file, std::string & folder, VulkanDevice * device, VkQueue queue);
	std::vector<VulkanObject*> GetObjectsVecFromMaterial(ForwardPlusLightPassPipeline * pipeline , Texture2D * dummyImage , VulkanDevice * device);

private:
	std::vector<Material> material_vec_;
};

#endif