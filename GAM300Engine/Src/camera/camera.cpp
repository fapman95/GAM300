#include "camera/camera.h"
#include "camera/Camerasystem/CameraSystem.h"
#include "Rendering/GraphicsManager.h"
#include "Input/InputSystem.h"

#include<iostream>

namespace TDS
{
	TDSCamera::TDSCamera(float Yaw, float pitch, /*float aspectRatio,*/ float zNear, float zFar, Vec3 position, Vec3 up)
	{

		m_Position = position;
		m_WorldUp = up;
		m_Yaw = Yaw;
		m_Pitch = pitch;
		//m_ProjMatrix = Mat4::Perspective(m_Fov

		updateViewMatrix();
	}


	Mat4 TDSCamera::GetViewMatrix() const
	{
		return Mat4::LookAt(m_Position, m_Position + m_Front, m_Up);
	}


	void TDSCamera::updateViewMatrix()
	{
		// calculate the new Front vector
		Vec3 front;
		front.x = Mathf::Cos(m_Yaw * Mathf::Deg2Rad) * Mathf::Cos(m_Pitch * Mathf::Deg2Rad);
		front.y = Mathf::Sin(m_Pitch * Mathf::Deg2Rad);
		front.z = Mathf::Sin(m_Yaw * Mathf::Deg2Rad) * Mathf::Cos(m_Pitch * Mathf::Deg2Rad);
		m_Front = Vec3::Normalize(front);
		// also re-calculate the Right and Up vector
		m_Right = Vec3::Normalize(Vec3::Cross(m_Front, m_WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		m_Up = Vec3::Normalize(Vec3::Cross(m_Right, m_Front));

		m_Down = -m_Up;

		//Input::wheelDelta = 0;
	}

	void TDSCamera::UpdateCamera(float deltaTime, bool gameIsPlaying)
	{
		if (!gameIsPlaying)
		{
			static Vec2 mouse = Vec2(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
			if (getEditorCamera() && getScrollWheel())
			{
				//translate(static_cast<float>(Input::wheelDelta * 0.05f));
				translate(static_cast<float>(InputSystem::GetInstance()->getWheelDelta() * 0.05f));
				if (moving())
				{
					float CameraSpeed = m_Speed;
					if (keys.up)
					{
						m_Position += m_Front * CameraSpeed;
					}
					if (keys.down)
						m_Position -= m_Front * CameraSpeed;
					if (keys.left)
						m_Position -= m_Right * CameraSpeed;
					if (keys.right)
						m_Position += m_Right * CameraSpeed;
				}

				//if (Input::isMouseButtonPressed(TDS_MOUSE_RIGHT))
				if (InputSystem::GetInstance()->isMouseDown(VK_RBUTTON))
				{
					if (mouse.x == std::numeric_limits<int>::max() && mouse.y == std::numeric_limits<int>::max())
					{
						//mouse = Input::getMousePosition();
						mouse = InputSystem::GetInstance()->getLocalMousePos();
					}

					//if (!Input::isMouseButtonReleased(TDS_MOUSE_RIGHT))
					if (!InputSystem::GetInstance()->isMouseReleased(VK_RBUTTON))
					{
						float GetMousex = static_cast<float>(mouse.x);
						float GetMousey = static_cast<float>(mouse.y);

						//float getNewMousex = static_cast<float>(Input::getMousePosition().x);
						//float getNewMousey = static_cast<float>(Input::getMousePosition().y);
						float getNewMousex = static_cast<float>(InputSystem::GetInstance()->getLocalMousePos().x);
						float getNewMousey = static_cast<float>(InputSystem::GetInstance()->getLocalMousePos().y);

						float offsetx = getNewMousex - GetMousex;
						float offsety = GetMousey - getNewMousey;

						ProcessMouseMovement(offsetx, offsety);

						//mouse = Input::getMousePosition();
						mouse = InputSystem::GetInstance()->getLocalMousePos();
					}

				}
				else
				{
					//mouse = Input::mousePosition(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
					mouse = Vec2(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
				}
			}

			/*else // TODO: refactor so that we dont copy and paste similar function
			{
				if (CameraSystem::GetIsPlaying())
				{
					if (moving())
					{
						float CameraSpeed = m_Speed;
						if (keys.up)
						{
							m_Position += m_Front * CameraSpeed;
						}
						if (keys.down)
							m_Position -= m_Front * CameraSpeed;
						if (keys.left)
							m_Position -= m_Right * CameraSpeed;
						if (keys.right)
							m_Position += m_Right * CameraSpeed;
					}

					if (mouse.x == std::numeric_limits<int>::max() && mouse.y == std::numeric_limits<int>::max())
					{
						mouse = Input::getMousePosition();
					}

					float GetMousex = static_cast<float>(mouse.x);
					float GetMousey = static_cast<float>(mouse.y);

					float getNewMousex = static_cast<float>(Input::getMousePosition().x);
					float getNewMousey = static_cast<float>(Input::getMousePosition().y);

					float offsetx = getNewMousex - GetMousex;
					float offsety = GetMousey - getNewMousey;

					ProcessMouseMovement(offsetx, offsety);

					mouse = Input::getMousePosition();
				}
			}*/

			updateViewMatrix();
		}
		else // gameIsPlaying
		{
			// for exe to get its local coordinate if imgui is not running 
			RECT win;
			GetClientRect(GraphicsManager::getInstance().GetWindow()->getWindowHandler(), &win);
			float Width = (win.right - win.left);
			float Height = (win.bottom - win.top);
			//Input::mousePosition globalMousePos = Input::getMousePosition();
			Vec2 globalMousePos = InputSystem::GetInstance()->getRawMouseInput();
			
			float normalizedLocalMouseX = (globalMousePos.x / Width) * 2 - 1;
			float normalizedLocalMouseY = (globalMousePos.y / Height) * 2 - 1;
			Vec2 localMousePos = { normalizedLocalMouseX, normalizedLocalMouseY };

			//Input::setLocalMousePos(localMousePos);
		}
	}


	bool TDSCamera::moving()
	{
		auto& inputSystem = InputSystem::GetInstance();
		if (/*inputSystem->isKeyPressed('W') ||*/ inputSystem->isKeyDown('W'))
		{
			keys.up = true;
		}
		else
		{
			keys.up = false;
		}

		if (/*inputSystem->isKeyPressed('A') ||*/ inputSystem->isKeyDown('A'))
		{
			keys.left = true;
		}
		else
		{
			keys.left = false;
		}

		if (/*inputSystem->isKeyPressed('S') ||*/ inputSystem->isKeyDown('S'))
		{
			keys.down = true;
		}
		else
		{
			keys.down = false;
		}

		if (/*inputSystem->isKeyPressed('D') ||*/ inputSystem->isKeyDown('D'))
		{
			keys.right = true;
		}
		else
		{
			keys.right = false;
		}

		return keys.up || keys.left || keys.down || keys.right;

		//if (Input::keystatus == Input::KeyStatus::PRESSED || Input::keystatus == Input::KeyStatus::REPEATED)
		//{
		//	switch (Input::keyCode)
		//	{
		//	case TDS_W:
		//		return keys.up = true;
		//		break;

		//	case TDS_A:
		//		return keys.left = true;
		//		break;

		//	case TDS_S:
		//		return keys.down = true;
		//		break;

		//	case TDS_D:
		//		return keys.right = true;

		//	default:
		//		return false;
		//	}
		//}
		//else
		//{
		//	keys.up = false;
		//	keys.left = false;
		//	keys.down = false;
		//	keys.right = false;
		//}
		//return false;
	}

	void TDSCamera::translate(const float& deltaWheel)
	{
		m_Fov -= deltaWheel;
		if (m_Fov < 1.f)
			m_Fov = 1.f;
		if (m_Fov > 45.f)
			m_Fov = 45.f;
	}

	void TDSCamera::ProcessMouseMovement(float  offsetX, float offsetY)
	{
		offsetX *= m_mouseSensitivity;
		offsetY *= m_mouseSensitivity;

		m_Yaw += offsetX;
		m_Pitch += offsetY;

		//prevent from out of bound
		if (m_Pitch > 89.f)
			m_Pitch = 89.f;
		if (m_Pitch < -89.f)
			m_Pitch = -89.f;

		updateViewMatrix();

	}
}