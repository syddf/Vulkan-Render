#ifndef _VULKAN_BASE_H_
#define _VULKAN_BASE_H_

#include "VulkanDevice.hpp"
#include "VulkanImgui.h"
#include "VulkanSwapChain.hpp"
#include "VulkanEditor.h"
#include "Utility.h"
#include "VulkanRenderScene.h"

class VulkanBase
{
public:
	VulkanBase()
	{
	}

	virtual ~VulkanBase()
	{

	}

public:
	void SetupWindow(HINSTANCE hinstance, WNDPROC wndproc, int width, int height);
	void HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Loop();
	virtual void Init();

	void CreateInstance();
	void CreateDevice();
	void CreateDepthStencil();
	void CreateSyncPrimitives();
	virtual void UpdateImgui();
	void CreateBaseResource();
	void Update();
	virtual void UpdateMouseEvent() = 0;
	virtual void OnUpdateImgui() = 0;
	virtual void OnKeyDownEvent(WPARAM wParam) = 0;
	virtual void OnKeyUpEvent(WPARAM wParam) = 0;
	virtual RenderGlobalState SetGlobalRenderState() = 0;

private:
	VkRenderPass imgui_render_pass_;
	std::vector<VkFramebuffer> imgui_frame_buffer_;
	std::vector<VkCommandBuffer> imgui_command_buffer_;
	void PrepareImguiPass();
	void SetImguiDrawCommandBuffer();
	void RenderImgui(uint32_t image_index);
	void RenderScene(uint32_t image_index);
	void Render();

protected:
	HINSTANCE window_instance_;
	HWND window_hwnd_;
	std::string app_name_;
	int width_;
	int height_;

	VkInstance instance_;
	VulkanDevice * vulkan_device_;
	VulkanImgui* imgui_;
	VulkanSwapChain * swap_chain_;
	RenderGlobalState global_state_;
	VkQueue queue_;
	VkPhysicalDeviceFeatures device_enabled_features_;
	std::vector<const char*> device_extensions_name_;
	VkQueueFlags queue_flag_;
	std::vector<const char*> instance_extensions_name_;


private:
	struct
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView image_view;
	}DepthStencil;

	struct
	{
		VkSemaphore present_semaphore;
		VkSemaphore render_semaphore;
		std::vector<VkFence> fences;
	}DrawSyncs;

private:
	int frame_count = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
	float frame_timer = 0;

protected:
	VulkanEditor * editor_;
	VulkanRenderScene * render_scene_;
	MouseStateType MouseState;

};

#endif