#include <Windows.h>
#include "VulkanBase.h"
#include "VulkanEditor.h"
#include "SkyBoxPipeline.h"

VulkanBase * base = NULL;

float f;
float pos[3];

class ForwardSponzaTest : public VulkanBase
{
public:
	ForwardSponzaTest()
	{

	};

	RenderGlobalState SetGlobalRenderState()
	{
		RenderGlobalState renderGlobalState;
		renderGlobalState.usingSponzaScene = true;
		renderGlobalState.sponzaPipelineType = PIPELINE_FORWARD_PLUS;
		renderGlobalState.cameraPosition = glm::vec3(12.7101822f, 1.87933588f, -0.0333303586f);
		renderGlobalState.cameraYaw = -3.1415926535 / 2.0f;
		renderGlobalState.cameraPitch = 0;

		return renderGlobalState;
	}

	void Init()
	{
		VulkanBase::Init();

	}

	void UpdateMouseEvent()
	{
		if (MouseState.mouse_position_.x > width_ || MouseState.mouse_position_.y > height_) return;
		editor_->MouseEvent(MouseState);
		render_scene_->MouseEvent(MouseState);
	}

	void OnKeyDownEvent(WPARAM wParam)
	{
		render_scene_->KeyDownEvent(wParam);
	}

	void OnKeyUpEvent(WPARAM wParam)
	{
		render_scene_->KeyUpEvent(wParam);
	}

	void OnUpdateImgui()
	{
	}
};
class SkyboxTest : public VulkanBase
{
public:
	SkyboxTest()
	{

	};

	RenderGlobalState SetGlobalRenderState()
	{
		RenderGlobalState renderGlobalState;
		renderGlobalState.usingSponzaScene = false;
		renderGlobalState.cameraPosition = glm::vec3(8, 0, 0);
		renderGlobalState.cameraYaw = -3.1415926535 / 2.0f;
		renderGlobalState.cameraPitch = 0;

		return renderGlobalState;
	}

	void Init()
	{
		VulkanBase::Init();

	}

	void UpdateMouseEvent()
	{
		if (MouseState.mouse_position_.x > width_ || MouseState.mouse_position_.y > height_) return;
		editor_->MouseEvent(MouseState);
		render_scene_->MouseEvent(MouseState);
	}

	void OnKeyDownEvent(WPARAM wParam)
	{
		render_scene_->KeyDownEvent(wParam);
	}

	void OnKeyUpEvent(WPARAM wParam)
	{
		render_scene_->KeyUpEvent(wParam);
	}

	void OnUpdateImgui()
	{
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
	base = new SkyboxTest();
	base->SetupWindow(hInstance, WndProc, 1440, 880);
	base->Init();
	base->Loop();
	return 0;
}