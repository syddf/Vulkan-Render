#include "VulkanObject.h"
#include <glm/gtx/transform.hpp>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

glm::mat4 VulkanObject::GetWorldMatrix() const
{
	glm::mat4 eularRotation = glm::rotate(object_entry_.rotation.x, glm::vec3(1, 0, 0)) *
												glm::rotate(object_entry_.rotation.y, glm::vec3(0, 1, 0)) *
												glm::rotate(object_entry_.rotation.z, glm::vec3(0, 0, 1));
	return 	glm::translate(object_entry_.position) * eularRotation * glm::scale(object_entry_.scale) ;
}

std::vector<VulkanSceneObjectsGroup::Material> VulkanSceneObjectsGroup::LoadObjectFromFile(std::string & file, std::string & folder, VulkanDevice * device, VkQueue queue)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn , err;
	std::string fullFile = folder + file;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn , &err , fullFile.c_str() , folder.c_str()  ))
	{
		throw " can't load model ";
	}
	bool has_normal = attrib.normals.size() > 0;
	if (!has_normal)
	{
		throw " not a legal obj data ";
	}
	std::vector<Material> groups( materials.size() + 1 );

	std::vector<std::unordered_map<Vertex, size_t , VertexHasher>> verticesPerGroup(materials.size() + 1);
	std::vector<std::vector<Vertex>> verticesData(materials.size() + 1);
	std::vector<std::vector<uint32_t>> indicesData(materials.size() + 1);
	
	auto appendVertex = [&](int material_id, const Vertex& vertex)-> void
	{
		auto & unique_vertices = verticesPerGroup[material_id + 1];
		auto & group = groups[material_id + 1];
		auto & vertices = verticesData[material_id + 1];
		auto & indices = indicesData[material_id + 1];

		if (unique_vertices.count(vertex) == 0)
		{
			unique_vertices[vertex] = vertices.size();
			vertices.push_back(vertex);
		}
		indices.push_back(unique_vertices[vertex]);
	};

	for (auto & shape : shapes)
	{
		size_t offset = 0;
		for (size_t i = 0; i < shape.mesh.num_face_vertices.size(); i++)
		{
			auto ngon = shape.mesh.num_face_vertices[i];
			auto material_id = shape.mesh.material_ids[i];
			for (size_t f = 0; f < ngon; f++)
			{
				const auto& index = shape.mesh.indices[offset + f];
				Vertex vertex;
				vertex.pos =
				{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
				vertex.texCoord =
				{
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				appendVertex(material_id, vertex);

			}
			offset += ngon;
		}
	}

	for (size_t i = 0; i < materials.size() + 1 ; i++)
	{
		if (indicesData[i].size() == 0)
		{
			groups[i].vertBuffer = NULL;
			groups[i].indicesBuffer = NULL;
		}
		if (i < materials.size())
		{
			if (materials[i].diffuse_texname != "")
			{
				groups[i + 1].albedoImage = new Texture2D(folder + materials[i].diffuse_texname, VK_FORMAT_R8G8B8A8_UNORM, device, queue);
			}
			if (materials[i].normal_texname != "")
			{
				groups[i + 1].normalIamge = new Texture2D(folder + materials[i].normal_texname, VK_FORMAT_R8G8B8A8_UNORM, device, queue);
			}
			else if (materials[i].bump_texname != "")
			{
				groups[i + 1].normalIamge = new Texture2D(folder + materials[i].bump_texname, VK_FORMAT_R8G8B8A8_UNORM, device, queue);
			}
		}
		if (indicesData[i].size() == 0)
		{
			continue;
		}

		groups[i].vertBuffer = device->CreateVulkanBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(Vertex) * verticesData[i].size(), verticesData[i].data());
		groups[i].indicesBuffer = device->CreateVulkanBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uint32_t) * indicesData[i].size(), indicesData[i].data());
		groups[i].vertSize = verticesData[i].size();
		groups[i].indexSize = indicesData[i].size();
	}
	
	material_vec_ = groups;
	return groups;

}

std::vector<VulkanObject*> VulkanSceneObjectsGroup::GetObjectsVecFromMaterial(ForwardPlusLightPassPipeline * pipeline , Texture2D * dummyImage , VulkanDevice * device  )
{
	std::vector<VulkanObject*> objectsVec;
	std::string name = "static object ";
	for (auto material : material_vec_)
	{
		if (material.vertBuffer == NULL) continue;
		VulkanMesh * newMesh = new VulkanMesh(material.vertBuffer, material.vertSize, material.indicesBuffer, material.indexSize, "StaticMesh");
		VulkanObject * newObj = new VulkanObject(objectsVec.size(), name , newMesh );
		IMaterial * forwardLightMaterial = new ForwardLightPassMaterial(
			material.albedoImage == NULL ? dummyImage : material.albedoImage, 
			material.normalIamge == NULL ? dummyImage : material.normalIamge, 
			pipeline, 
			device );
		newObj->ResetMaterial(forwardLightMaterial , PIPELINE_FORWARD_PLUS);
		objectsVec.push_back(newObj);
	}
	return objectsVec;
}
