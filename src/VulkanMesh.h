#ifndef _VULKAN_MESH_H_
#define _VULKAN_MESH_H_

#include "Utility.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>


typedef enum Component {
	VERTEX_COMPONENT_POSITION = 0x0,
	VERTEX_COMPONENT_NORMAL = 0x1,
	VERTEX_COMPONENT_COLOR = 0x2,
	VERTEX_COMPONENT_UV = 0x3,
	VERTEX_COMPONENT_TANGENT = 0x4,
	VERTEX_COMPONENT_BITANGENT = 0x5,
	VERTEX_COMPONENT_DUMMY_FLOAT = 0x6,
	VERTEX_COMPONENT_DUMMY_VEC4 = 0x7
} Component;

struct VertexLayout {
public:
	std::vector<Component> components;

	VertexLayout(std::vector<Component> components)
	{
		this->components = std::move(components);
	}


	uint32_t stride()
	{
		uint32_t res = 0;
		for (auto& component : components)
		{
			switch (component)
			{
			case VERTEX_COMPONENT_UV:
				res += 2 * sizeof(float);
				break;
			case VERTEX_COMPONENT_DUMMY_FLOAT:
				res += sizeof(float);
				break;
			case VERTEX_COMPONENT_DUMMY_VEC4:
				res += 4 * sizeof(float);
				break;
			default:
				res += 3 * sizeof(float);
			}
		}
		return res;
	}
};

struct ModelCreateInfo {
	glm::vec3 center;
	glm::vec3 scale;
	glm::vec2 uvscale;
	VkMemoryPropertyFlags memoryPropertyFlags = 0;

	ModelCreateInfo() : center(glm::vec3(0.0f)), scale(glm::vec3(1.0f)), uvscale(glm::vec2(1.0f)) {};

	ModelCreateInfo(glm::vec3 scale, glm::vec2 uvscale, glm::vec3 center)
	{
		this->center = center;
		this->scale = scale;
		this->uvscale = uvscale;
	}

	ModelCreateInfo(float scale, float uvscale, float center)
	{
		this->center = glm::vec3(center);
		this->scale = glm::vec3(scale);
		this->uvscale = glm::vec2(uvscale);
	}

};

struct Model {
	VkDevice device = nullptr;
	VulkanBuffer *vertices;
	VulkanBuffer *indices;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;

	/** @brief Stores vertex and index base and counts for each part of a model */
	struct ModelPart {
		uint32_t vertexBase;
		uint32_t vertexCount;
		uint32_t indexBase;
		uint32_t indexCount;
	};
	std::vector<ModelPart> parts;

	static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

	struct Dimension
	{
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size;
	} dim;

	void destroy()
	{
		assert(device);
		vkDestroyBuffer(device, vertices->GetDesc().buffer, nullptr);
		vkFreeMemory(device, vertices->GetBufferMemory(), nullptr);
		if (indices->GetDesc().buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, indices->GetDesc().buffer, nullptr);
			vkFreeMemory(device, indices->GetBufferMemory(), nullptr);
		}
	}

	bool loadFromFile(const std::string& filename, VertexLayout layout, ModelCreateInfo *createInfo, VulkanDevice *device, VkQueue copyQueue)
	{
		this->device = device->GetDevice();

		Assimp::Importer Importer;
		const aiScene* pScene;

		pScene = Importer.ReadFile(filename.c_str(), defaultFlags);
		if (!pScene) {
			std::string error = Importer.GetErrorString();
			throw error + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.";
		}

		if (pScene)
		{
			parts.clear();
			parts.resize(pScene->mNumMeshes);

			glm::vec3 scale(1.0f);
			glm::vec2 uvscale(1.0f);
			glm::vec3 center(0.0f);
			if (createInfo)
			{
				scale = createInfo->scale;
				uvscale = createInfo->uvscale;
				center = createInfo->center;
			}

			std::vector<float> vertexBuffer;
			std::vector<uint32_t> indexBuffer;

			vertexCount = 0;
			indexCount = 0;

			// Load meshes
			for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
			{
				const aiMesh* paiMesh = pScene->mMeshes[i];

				parts[i] = {};
				parts[i].vertexBase = vertexCount;
				parts[i].indexBase = indexCount;

				vertexCount += pScene->mMeshes[i]->mNumVertices;

				aiColor3D pColor(0.f, 0.f, 0.f);
				pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

				const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

				for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
				{
					const aiVector3D* pPos = &(paiMesh->mVertices[j]);
					const aiVector3D* pNormal = &(paiMesh->mNormals[j]);
					const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
					const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
					const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

					for (auto& component : layout.components)
					{
						switch (component) {
						case VERTEX_COMPONENT_POSITION:
							vertexBuffer.push_back(pPos->x * scale.x + center.x);
							vertexBuffer.push_back(-pPos->y * scale.y + center.y);
							vertexBuffer.push_back(pPos->z * scale.z + center.z);
							break;
						case VERTEX_COMPONENT_NORMAL:
							vertexBuffer.push_back(pNormal->x);
							vertexBuffer.push_back(-pNormal->y);
							vertexBuffer.push_back(pNormal->z);
							break;
						case VERTEX_COMPONENT_UV:
							vertexBuffer.push_back(pTexCoord->x * uvscale.s);
							vertexBuffer.push_back(pTexCoord->y * uvscale.t);
							break;
						case VERTEX_COMPONENT_COLOR:
							vertexBuffer.push_back(pColor.r);
							vertexBuffer.push_back(pColor.g);
							vertexBuffer.push_back(pColor.b);
							break;
						case VERTEX_COMPONENT_TANGENT:
							vertexBuffer.push_back(pTangent->x);
							vertexBuffer.push_back(pTangent->y);
							vertexBuffer.push_back(pTangent->z);
							break;
						case VERTEX_COMPONENT_BITANGENT:
							vertexBuffer.push_back(pBiTangent->x);
							vertexBuffer.push_back(pBiTangent->y);
							vertexBuffer.push_back(pBiTangent->z);
							break;
							// Dummy components for padding
						case VERTEX_COMPONENT_DUMMY_FLOAT:
							vertexBuffer.push_back(0.0f);
							break;
						case VERTEX_COMPONENT_DUMMY_VEC4:
							vertexBuffer.push_back(0.0f);
							vertexBuffer.push_back(0.0f);
							vertexBuffer.push_back(0.0f);
							vertexBuffer.push_back(0.0f);
							break;
						};
					}

					dim.max.x = fmax(pPos->x, dim.max.x);
					dim.max.y = fmax(pPos->y, dim.max.y);
					dim.max.z = fmax(pPos->z, dim.max.z);

					dim.min.x = fmin(pPos->x, dim.min.x);
					dim.min.y = fmin(pPos->y, dim.min.y);
					dim.min.z = fmin(pPos->z, dim.min.z);
				}

				dim.size = dim.max - dim.min;

				parts[i].vertexCount = paiMesh->mNumVertices;

				uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
				for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
				{
					const aiFace& Face = paiMesh->mFaces[j];
					if (Face.mNumIndices != 3)
						continue;
					indexBuffer.push_back(indexBase + Face.mIndices[0]);
					indexBuffer.push_back(indexBase + Face.mIndices[1]);
					indexBuffer.push_back(indexBase + Face.mIndices[2]);
					parts[i].indexCount += 3;
					indexCount += 3;
				}
			}


			uint32_t vBufferSize = static_cast<uint32_t>(vertexBuffer.size()) * sizeof(float);
			uint32_t iBufferSize = static_cast<uint32_t>(indexBuffer.size()) * sizeof(uint32_t);

			// Use staging buffer to move vertex and index buffer to device local memory
			// Create staging buffers
			VulkanBuffer* vertexStaging, *indexStaging;

			// Vertex buffer
			vertexStaging = device->CreateVulkanBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				vBufferSize,
				vertexBuffer.data());

			// Index buffer
			indexStaging = device->CreateVulkanBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				iBufferSize,
				indexBuffer.data());

			// Create device local target buffers
			// Vertex buffer
			vertices = device->CreateVulkanBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo->memoryPropertyFlags,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vBufferSize);

			// Index buffer
			indices = device->CreateVulkanBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo->memoryPropertyFlags,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				iBufferSize);

			// Copy from staging buffers
			VkCommandBuffer copyCmd;
			device->CreateCommandBuffer(1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, &copyCmd);
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			vkBeginCommandBuffer(copyCmd, &beginInfo);
			VkBufferCopy copyRegion{};

			copyRegion.size = vBufferSize;
			vkCmdCopyBuffer(copyCmd, vertexStaging->GetDesc().buffer, vertices->GetDesc().buffer, 1, &copyRegion);

			copyRegion.size = iBufferSize;
			vkCmdCopyBuffer(copyCmd, indexStaging->GetDesc().buffer, indices->GetDesc().buffer, 1, &copyRegion);

			vkEndCommandBuffer(copyCmd);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &copyCmd;

			// Create fence to ensure that the command buffer has finished executing
			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = 0;
			VkFence fence;
			vkCreateFence(this->device, &fenceInfo, nullptr, &fence);

			// Submit to the queue
			vkQueueSubmit(copyQueue, 1, &submitInfo, fence);
			// Wait for the fence to signal that command buffer has finished executing
			vkWaitForFences(this->device, 1, &fence, VK_TRUE, 1e10);

			vkDestroyFence(this->device, fence, nullptr);
			device->DestroyCommandBuffer(&copyCmd, 1);

			// Destroy staging resources
			vkDestroyBuffer(this->device, vertexStaging->GetDesc().buffer, nullptr);
			vkFreeMemory(this->device, vertexStaging->GetBufferMemory(), nullptr);
			vkDestroyBuffer(this->device, indexStaging->GetDesc().buffer, nullptr);
			vkFreeMemory(this->device, indexStaging->GetBufferMemory(), nullptr);

			return true;
		}
		else
		{
			printf("Error parsing '%s': '%s'\n", filename.c_str(), Importer.GetErrorString());
			return false;
		}
	};

	bool loadFromFile(const std::string& filename, VertexLayout layout, float scale, VulkanDevice *device, VkQueue copyQueue)
	{
		ModelCreateInfo modelCreateInfo(scale, 1.0f, 0.0f);
		return loadFromFile(filename, layout, &modelCreateInfo, device, copyQueue);
	}

	void loadFromExitBuffer(VulkanBuffer * vertBuffer , size_t vertSize , VulkanBuffer * indicesBuffer , size_t indicesSize  )
	{
		vertices = vertBuffer;
		indices = indicesBuffer;
		vertexCount = vertSize;
		indexCount= indicesSize;
	}
};

struct MeshEntry
{
	std::string name;
	VulkanBuffer * vertexBuffer;
	VulkanBuffer * indexBuffer;
	size_t vertCount;
	size_t indicesCount;
};

class VulkanMesh
{
public:
	VulkanMesh(const std::string& filename, VertexLayout layout, float scale, VulkanDevice *device, VkQueue copyQueue , std::string name )
	{
		model_.loadFromFile(filename, layout, scale, device, copyQueue);
		name_ = name;
	}

	VulkanMesh(VulkanBuffer * vertBuffer, size_t vertSize, VulkanBuffer * indicesBuffer, size_t indicesSize , std::string name )
	{
		model_.loadFromExitBuffer(vertBuffer, vertSize, indicesBuffer, indicesSize);
		name_ = name;
	}

	~VulkanMesh()
	{

	}

public:
	MeshEntry GetMeshEntry() const 
	{
		MeshEntry entry;
		entry.vertCount = model_.vertexCount;
		entry.vertexBuffer = model_.vertices;
		entry.indicesCount = model_.indexCount;
		entry.indexBuffer = model_.indices;
		entry.name = name_;
		return entry;
	}

private:
	Model model_;
	std::string name_;
};

#endif