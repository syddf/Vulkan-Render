#ifndef _UTILITY_H_
#define _UTILITY_H_



#include <string>
#include <glm/glm.hpp>
#include <Windows.h>
#include <string>
#include <chrono>
#include "VulkanInitializer.hpp"
#include <vulkan/vulkan.h>

#define USING_IMGUI
#define USING_VALIDATION_LAYER

#define VULKAN_SUCCESS(s) \
{ \
VkResult res = s;\
if(res != VK_SUCCESS) \
{\
throw " Unexpected error . ";\
}\
}

struct MouseStateType
{
	glm::vec2 mouse_position_;
	glm::vec2 last_left_button_down_position_;
	bool LeftMouseButtonDown = false;
	bool RightMouseButtonDown = false;
};

static std::string GetAssetPath()
{
	std::string path = "Resource/";
	return path;
}

enum Type
{
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_VEC2,
	TYPE_VEC3,
	TYPE_VEC4
};

union TypeVal
{
	int ival;
	float fval;
	glm::vec2 vec2val;
	glm::vec3 vec3val;
	glm::vec4 vec4val;
};

enum PipelineType
{
	PIPELINE_EMPTY, 
	PIPELINE_FORWARD_PLUS,
	PIPELINE_FORWARD_PBR, 
	PIPELINE_TBDR
};

struct PointLight
{
	glm::vec3 pos;
	float radius;
	glm::vec3 intensity;
	float padding;
};

struct LightVisible
{
	uint32_t count;
	uint32_t light_indices[1023];
};

struct Cascade
{
	float splitDepth;
	glm::mat4 viewProjMatrix;
};

struct RenderGlobalState
{
	bool usingSponzaScene;
	PipelineType sponzaPipelineType;
	glm::vec3 cameraPosition;
	float cameraYaw;
	float cameraPitch;
};

#define PI 3.1415926535f

#endif