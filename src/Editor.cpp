#include <Windows.h>
#include "VulkanBase.h"
#include "VulkanEditor.h"

VulkanBase * base = NULL;

float f;
float pos[3];

class Test : public VulkanBase
{
public:
	VulkanEditor *editor;
	Test()
	{

	}

	void Init()
	{
		VulkanBase::Init();
		editor = new VulkanEditor(vulkan_device_, 300, 300, 100, 100, queue_, swap_chain_);
		editor->AddNewEmptyObject();
		editor->AddNewEmptyObject();
		editor->AddNewEmptyObject();
	}

	void OnUpdateImgui()
	{
		editor->OnUpdateImgui();
	}
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (base != NULL)
	{
		base->HandleMessages(hWnd, uMsg, wParam, lParam);
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	base = new Test();
	base->SetupWindow(hInstance, WndProc, 1280, 720);
	base->Init();
	base->Loop();
	return 0;
}