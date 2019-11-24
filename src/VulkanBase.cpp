#include "VulkanBase.h"
#include "VulkanDebug.h"
#include "VulkanPipeline.h"
#include "VulkanMesh.h"
#include "VulkanObject.h"
#define DEBUG_LAYER 
void VulkanBase::SetupWindow(HINSTANCE hinstance, WNDPROC wndproc, int width, int height)
{
	width_ = width;
	height_ = height;

	window_instance_ = hinstance;
	app_name_ = "Vulkan Render";
	WNDCLASSEX wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = wndproc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbWndExtra = 0;
	wc.cbClsExtra = 0;
	wc.hInstance = hinstance;
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"Vulkan Render";
	wc.lpszMenuName = NULL;

	RegisterClassEx(&wc);
	int fs_width = GetSystemMetrics(SM_CXSCREEN);
	int fs_height = GetSystemMetrics(SM_CYSCREEN);

	int posX = (fs_width - width) / 2;
	int posY = (fs_height - height) / 2;

	window_hwnd_ = CreateWindowEx(
		WS_EX_APPWINDOW, 
		L"Vulkan Render", 
		L"Vulkan Render",
		WS_OVERLAPPEDWINDOW,
		posX, 
		posY, 
		width, 
		height, 
		NULL, 
		NULL, 
		window_instance_, 
		NULL
	);

	ShowWindow(window_hwnd_, SW_SHOW);
	SetForegroundWindow(window_hwnd_);
	SetFocus(window_hwnd_);
	ShowCursor(true);
}

void VulkanBase::HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;

	case WM_LBUTTONDOWN:
		MouseState.LeftMouseButtonDown = true;
		MouseState.last_left_button_down_position_ = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		break;

	case WM_MOUSEMOVE:
		MouseState.mouse_position_ = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		break;

	case WM_LBUTTONUP:
		MouseState.LeftMouseButtonDown = false;
		break;

	case WM_KEYDOWN:
		OnKeyDownEvent(wParam) ;
		break;

	case WM_KEYUP:
		OnKeyUpEvent(wParam);
		break;
		
	}
}

void VulkanBase::Loop()
{
	MSG msg;
	bool quitMessageReceived = false;

	while (!quitMessageReceived) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessageReceived = true;
				break;
			}
		}

		if (!IsIconic(window_hwnd_)) {
			Render();
			Update();
		}

	}
}

void VulkanBase::Init()
{
	global_state_ = SetGlobalRenderState();
	CreateInstance();
	CreateDevice();
	CreateBaseResource();
	SetImguiDrawCommandBuffer();

}

void VulkanBase::CreateDepthStencil()
{
	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.arrayLayers = 1;
	image_create_info.mipLevels = 1;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.extent = { (uint32_t)width_ , (uint32_t)height_ , 1 };
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.format = VK_FORMAT_D24_UNORM_S8_UINT;

	VkResult res = vkCreateImage(vulkan_device_->GetDevice(), &image_create_info, NULL, &DepthStencil.image);
	if (res != VK_SUCCESS)
	{
		throw " create image fault . ";
	}

	VkMemoryRequirements memory_require = {};
	vkGetImageMemoryRequirements(vulkan_device_->GetDevice(), DepthStencil.image, &memory_require);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_require.size;
	alloc_info.memoryTypeIndex = vulkan_device_->GetMemoryType(memory_require.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	res = vkAllocateMemory(vulkan_device_->GetDevice(), &alloc_info, NULL, &DepthStencil.memory);
	if (res != VK_SUCCESS)
	{
		throw " allocate memory fault . ";
	}
	vkBindImageMemory(vulkan_device_->GetDevice(), DepthStencil.image, DepthStencil.memory, 0);

	VkImageViewCreateInfo image_view_create_info = {};
	image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_create_info.format = image_create_info.format;
	image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_create_info.image = DepthStencil.image;
	image_view_create_info.subresourceRange.baseArrayLayer = 0;
	image_view_create_info.subresourceRange.layerCount = 1;
	image_view_create_info.subresourceRange.baseMipLevel = 0;
	image_view_create_info.subresourceRange.levelCount = 1;
	image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	res = vkCreateImageView(vulkan_device_->GetDevice(), &image_view_create_info, NULL, &DepthStencil.image_view);
	if (res != VK_SUCCESS)
	{
		throw " create image view fault . ";
	}
}

void VulkanBase::CreateSyncPrimitives()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkResult res = vkCreateSemaphore(vulkan_device_->GetDevice(), &semaphoreCreateInfo, nullptr, &DrawSyncs.render_semaphore);
	if (res != VK_SUCCESS) throw " create semaphore fault .";
	res = vkCreateSemaphore(vulkan_device_->GetDevice(), &semaphoreCreateInfo, nullptr, &DrawSyncs.present_semaphore);
	if (res != VK_SUCCESS) throw " create semaphore fault .";

	DrawSyncs.fences.resize(swap_chain_->GetImageCount());
	for (int i = 0; i < DrawSyncs.fences.size(); i++)
	{
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		res = vkCreateFence(vulkan_device_->GetDevice(), &fenceCreateInfo, NULL, &DrawSyncs.fences[i]);
		if (res != VK_SUCCESS) throw " create fence fault .";
	}
}

void VulkanBase::UpdateImgui()
{
	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2((float)width_, (float)height_);
	io.DeltaTime = frame_timer;

	io.MousePos = ImVec2(MouseState.mouse_position_.x, MouseState.mouse_position_.y);
	io.MouseDown[0] = MouseState.LeftMouseButtonDown;
	io.MouseDown[1] = MouseState.RightMouseButtonDown;
	ImGui::NewFrame();
	ImGui::StyleColorsDark();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Vulkan", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(app_name_.c_str());
	
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 5.0f * UIOverlay.scale));
#endif
	ImGui::PushItemWidth(110.0f);

	editor_->OnUpdateImgui();
	OnUpdateImgui();

	ImGui::PopItemWidth();
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	ImGui::PopStyleVar();
#endif

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::Render();

	if (imgui_->update() || imgui_->updated) {
		SetImguiDrawCommandBuffer();
		imgui_->updated = false;
	}

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	if (mouseButtons.left) {
		mouseButtons.left = false;
	}
#endif
}

void VulkanBase::CreateBaseResource()
{
	swap_chain_ = new VulkanSwapChain(instance_, vulkan_device_->GetPhysicalDevice(), vulkan_device_->GetDevice(), window_instance_, window_hwnd_, (uint32_t * )&width_, (uint32_t *)&height_);
	imgui_command_buffer_.resize(swap_chain_->GetImageCount());
	vulkan_device_->CreateCommandBuffer(imgui_command_buffer_.size(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, imgui_command_buffer_.data());
	CreateDepthStencil();
	CreateSyncPrimitives();
	PrepareImguiPass();
	imgui_ = new VulkanImgui(vulkan_device_, queue_);
	imgui_->prepareResources();
	imgui_->preparePipeline(VK_NULL_HANDLE, imgui_render_pass_);

	render_scene_ = new VulkanRenderScene(vulkan_device_, queue_, swap_chain_, DepthStencil.image_view, width_ - 320, height_, 320, 0, width_, height_, global_state_ );

	editor_ = new VulkanEditor(vulkan_device_, width_ - 320 , height_, 320, 0, width_ , height_ , queue_, swap_chain_, render_scene_->objects_);
	
}

void VulkanBase::Update()
{
	render_scene_->Update(frame_timer);
	UpdateMouseEvent();
	UpdateImgui();
}

void VulkanBase::PrepareImguiPass()
{
	std::vector<VkAttachmentDescription> attachment_desc;
	attachment_desc.resize(2);

	// Color Attachment 
	attachment_desc[0].flags = 0;
	attachment_desc[0].format = swap_chain_->GetColorFormat();
	attachment_desc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_desc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachment_desc[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_desc[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	attachment_desc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_desc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_desc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// DepthStencil Attachment 
	attachment_desc[1].flags = 0;
	attachment_desc[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
	attachment_desc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_desc[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment_desc[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachment_desc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_desc[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_desc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_desc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

	std::vector<VkAttachmentReference> attachment_reference;
	attachment_reference.resize(2);
	attachment_reference[0].attachment = 0;
	attachment_reference[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment_reference[1].attachment = 1;
	attachment_reference[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_desc = {};
	subpass_desc.colorAttachmentCount = 1;
	subpass_desc.pColorAttachments = &attachment_reference[0];
	subpass_desc.pDepthStencilAttachment = &attachment_reference[1];
	subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	std::vector<VkSubpassDependency> subpass_dependency;
	subpass_dependency.resize(2);
	subpass_dependency[0].dependencyFlags = 0;
	subpass_dependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpass_dependency[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpass_dependency[0].dstSubpass = 0;
	subpass_dependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	subpass_dependency[1].dependencyFlags = 0;
	subpass_dependency[1].srcSubpass = 0;
	subpass_dependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpass_dependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpass_dependency[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass_desc;
	render_pass_create_info.dependencyCount = 2;
	render_pass_create_info.pDependencies = subpass_dependency.data();
	render_pass_create_info.attachmentCount = 2;
	render_pass_create_info.pAttachments = attachment_desc.data();

	VkResult res = vkCreateRenderPass(vulkan_device_->GetDevice(), &render_pass_create_info, NULL, &imgui_render_pass_);
	if (res != VK_SUCCESS)
	{
		throw " create vulkan render pass fault . ";
	}


	VkImageView attachments[2];

	// Depth/Stencil attachment is the same for all frame buffers
	attachments[1] = DepthStencil.image_view;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = imgui_render_pass_;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = width_;
	frameBufferCreateInfo.height = height_;
	frameBufferCreateInfo.layers = 1;

	imgui_frame_buffer_.resize(swap_chain_->GetImageCount());
	for (uint32_t i = 0; i < imgui_frame_buffer_.size(); i++)
	{
		attachments[0] = swap_chain_->GetImageView(i);
		vkCreateFramebuffer(vulkan_device_->GetDevice(), &frameBufferCreateInfo, nullptr, &imgui_frame_buffer_[i]);
	}
}

void VulkanBase::SetImguiDrawCommandBuffer()
{
	VkCommandBufferBeginInfo commandBeginInfo = VulkanInitializer::InitCommandBufferBeginInfo( 0 , NULL);
	for (int i = 0; i < swap_chain_->GetImageCount(); i++)
	{
		vkBeginCommandBuffer(imgui_command_buffer_[i], &commandBeginInfo);

		std::vector<VkClearValue> clearValues = { {} , {} };
		clearValues[0].color = { { 0.0f , 0.0f , 0.0f , 0.0f } };
		clearValues[1].depthStencil = { 1 , 0 };
		VkRenderPassBeginInfo renderPassBeginInfo = VulkanInitializer::InitRenderPassBeginInfo(imgui_render_pass_, imgui_frame_buffer_[i], clearValues, width_, height_);
		vkCmdBeginRenderPass(imgui_command_buffer_[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		VkViewport viewport = VulkanInitializer::InitViewport( 0 , 0 , width_ , height_ , 0.0f , 1.0f );
		vkCmdSetViewport(imgui_command_buffer_[i] , 0 , 1 , &viewport);
		VkRect2D scissor = { width_ , height_ ,  0 , 0 };
		vkCmdSetScissor(imgui_command_buffer_[i], 0, 1, &scissor);
		imgui_->draw(imgui_command_buffer_[i], width_ , height_ );
		vkCmdEndRenderPass(imgui_command_buffer_[i]);

		vkEndCommandBuffer(imgui_command_buffer_[i]);
	}

}

void VulkanBase::RenderImgui(uint32_t image_index)
{
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	editor_->UpdateEditAxis(image_index);

	VkCommandBuffer commandBuffers[2] = {
		imgui_command_buffer_[image_index],
		editor_->GetCommandBuffer(image_index)
	};

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = &waitStageMask;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = NULL;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &DrawSyncs.render_semaphore;
	submit_info.commandBufferCount = 2;
	submit_info.pCommandBuffers = commandBuffers;
	vkQueueSubmit(queue_, 1, &submit_info, DrawSyncs.fences[image_index]);
	swap_chain_->queuePresent(queue_, image_index, DrawSyncs.render_semaphore);
	vkQueueWaitIdle(queue_);
}

void VulkanBase::RenderScene(uint32_t image_index)
{
	std::vector<VkCommandBuffer> commandBuffers;
	render_scene_->SetupCommandBuffers(commandBuffers , image_index);
	if (commandBuffers.size() == 0)return;
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = &waitStageMask;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &DrawSyncs.present_semaphore;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = NULL;
	submit_info.commandBufferCount = commandBuffers.size();
	submit_info.pCommandBuffers = commandBuffers.data();

	vkQueueSubmit(queue_, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue_);

}

void VulkanBase::Render()
{
	uint32_t image_index;
	swap_chain_->acquireNextImage(DrawSyncs.present_semaphore, &image_index);
	vkWaitForFences(vulkan_device_->GetDevice(), 1, &DrawSyncs.fences[image_index], VK_TRUE, UINT64_MAX);
	vkResetFences(vulkan_device_->GetDevice(), 1, &DrawSyncs.fences[image_index]);

	auto tStart = std::chrono::high_resolution_clock::now();

	RenderScene(image_index);
	RenderImgui(image_index);

	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart);
	auto tDiffToLastTime = (float)(std::chrono::duration<double, std::milli>((tEnd - last_time)).count());
	float frameTime = tDiffToLastTime / 1000.0f;
	frame_timer = frameTime;
	// logical update using the frame Time .
	// ...

	// set window title to show fps . 
	frame_count++;
	if (tDiffToLastTime > 1000.0f)
	{
		float fps = frame_count * 1000.0f / tDiffToLastTime;
		frame_count = 0;
		last_time = tEnd;
	}

}

void VulkanBase::CreateInstance()
{
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_VERSION_1_0;
	appInfo.pEngineName = app_name_.c_str();
	appInfo.pApplicationName = app_name_.c_str();

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME , VK_KHR_WIN32_SURFACE_EXTENSION_NAME , VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
	for (auto iter : instance_extensions_name_)
	{
		instanceExtensions.push_back(iter);
	}

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledExtensionCount = instanceExtensions.size();
	instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();

#ifdef DEBUG_LAYER
	instanceInfo.enabledLayerCount = VulkanDebug::validation_layer_count;
	instanceInfo.ppEnabledLayerNames = VulkanDebug::validation_layer_names;
#endif

	VkResult res = vkCreateInstance(&instanceInfo, NULL, &instance_);
	if (res != VK_SUCCESS)
	{
		throw " create instance fault . ";
	}
#ifdef DEBUG_LAYER 
	VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	VulkanDebug::Instance()->setupDebugging(instance_, debugReportFlags, VK_NULL_HANDLE);
#endif

}

void VulkanBase::CreateDevice()
{
	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(instance_, &gpu_count, NULL);
	std::vector<VkPhysicalDevice> physical_devices(gpu_count);
	vkEnumeratePhysicalDevices(instance_, &gpu_count, physical_devices.data());

	uint32_t selectedDevice = 0;
	vulkan_device_ = new VulkanDevice(physical_devices[selectedDevice]);

	device_enabled_features_ = {};
	device_extensions_name_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	queue_flag_ = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
	vulkan_device_->CreateLogicalDevice(device_enabled_features_, device_extensions_name_, queue_flag_);

	vkGetDeviceQueue(vulkan_device_->GetDevice(), vulkan_device_->GetGraphicsQueue(), 0, &queue_);

}
