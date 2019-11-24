#ifndef _VULKAN_CAMERA_H_
#define _VULKAN_CAMERA_H_

#include "Utility.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "KEYS.hpp"

class VulkanCamera
{
private:
	float fov;
	float znear, zfar;

	void updateViewMatrix()
	{
		matrices.view = glm::lookAtLH(position, position + viewDir, glm::vec3(0.0f, 1.0f, 0.0f));
		matrices.viewRotation = matrices.view;
		matrices.viewRotation[3][0] = 0;
		matrices.viewRotation[3][1] = 0;
		matrices.viewRotation[3][2] = 0;
		matrices.viewRotation[3][3] = 1;
	};

public:
	float yaw, pitch ;
	glm::vec3 position = glm::vec3();
	glm::vec3 viewDir = glm::vec3();

	float rotationSpeed = 1.0f;
	float movementSpeed = 1.0f;

	bool updated = false;

	struct
	{
		glm::mat4 perspective;
		glm::mat4 view;
		glm::mat4 viewRotation;
	} matrices;

	struct
	{
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;

	VulkanCamera()
	{
		yaw = 0 ;
		pitch = 0;
		rotationSpeed = 0.001f;
		UpdateViewDirection();
	}

	bool moving()
	{
		return keys.left || keys.right || keys.up || keys.down;
	}

	float getNearClip() {
		return znear;
	}

	float getFarClip() {
		return zfar;
	}

	void setPerspective(float fov, float aspect, float znear, float zfar)
	{
		float fax = 1.0f / (float)tan(glm::radians(fov) * 0.5f);
		this->fov = fov;
		this->znear = znear;
		this->zfar = zfar;
		
		matrices.perspective = glm::mat4(0.0f);
		matrices.perspective[0][0] = (float)(fax / aspect);
		matrices.perspective[1][1] = (float)(fax);
		matrices.perspective[2][2] = zfar / (zfar - znear);
		matrices.perspective[2][3] = -znear * zfar / (zfar - znear);
		matrices.perspective[3][2] = 1;

		matrices.perspective = glm::transpose(matrices.perspective);
		matrices.perspective[1][1] *= -1;
	};
	void setViewParameters(float vyaw, float vpitch)
	{
		yaw = vyaw;
		pitch = vpitch;
		UpdateViewDirection();
	}

	void setPosition(glm::vec3 position)
	{
		this->position = position;
		updateViewMatrix();
	}

	void UpdateViewDirection()
	{
		float yaw2 = yaw;
		float pitch2 = pitch;
		float xDirection = sin(yaw2) * cos(pitch2);
		float yDirection = sin(pitch2);
		float zDirection = cos(yaw2) * cos(pitch2);
		viewDir = glm::vec3(xDirection, yDirection, zDirection);
		updateViewMatrix();
	}

	void Update(float deltaTime)
	{
		updated = false;

    	if (moving())
		{
			float moveSpeed = deltaTime * movementSpeed;

			if (keys.up)
				position += viewDir * moveSpeed;
			if (keys.down)
				position -= viewDir * moveSpeed;
			if (keys.left)
				position -= glm::normalize(glm::cross(viewDir, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
			if (keys.right)
				position += glm::normalize(glm::cross(viewDir, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

			updateViewMatrix();
		}

	};

	void MouseEvent(const MouseStateType & mouseState)
	{
		static float lastYaw, lastPitch;
		static bool move = false;
		if ( mouseState.LeftMouseButtonDown )
		{
			if (move == false)
			{
				lastYaw = yaw;
				lastPitch = pitch;
				move = true;
			}
			int32_t dx = (int32_t)mouseState.mouse_position_.x - mouseState.last_left_button_down_position_.x;
			int32_t dy = (int32_t)mouseState.mouse_position_.y - mouseState.last_left_button_down_position_.y;
			yaw = lastYaw + dx * 1.25f * rotationSpeed;
			pitch = lastPitch - dy * 1.25f * rotationSpeed;
		}
		else {
			move = false;
		}
		UpdateViewDirection();
	}
	void KeyDownEvent(WPARAM wParam)
	{
		switch (wParam)
		{
		case KEY_W:
			keys.up = true;
			break;
		case KEY_S:
			keys.down = true;
			break;
		case KEY_A:
			keys.left = true;
			break;
		case KEY_D:
			keys.right = true;
			break;
		}
	}
	void KeyUpEvent(WPARAM wParam)
	{
		switch (wParam)
		{
		case KEY_W:
			keys.up = false;
			break;
		case KEY_S:
			keys.down = false;
			break;
		case KEY_A:
			keys.left = false;
			break;
		case KEY_D:
			keys.right = false;
			break;
		}
	}
};
#endif