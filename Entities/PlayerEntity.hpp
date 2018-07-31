#pragma once
#include "StandardEntity.hpp"

struct PlayerEntity
{
		bool active = true;
		int oldState = GLFW_RELEASE;
		//glm::vec3 position = glm::vec3(2.0, 2.0, 0.5);
		//float horizontalAngle = 0.0f; //pi;
		//float verticalAngle = 0.0f;
		float initialFoV = 55.0f;
		float speed = 1.0f;
		float mouseSpeed = 0.5f;
		vk3d::Transform transform = { };
};
