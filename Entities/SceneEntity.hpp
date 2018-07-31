#pragma once
#include "StandardEntity.hpp"
struct SceneEntity
{
		IEntity *entity = nullptr;
		vk3d::Transform transform = { };
		vk3d::Material *material = nullptr;
		bool replaceAble = false;
		float speed = 0;
		glm::vec3 vel = glm::vec3(0);
		glm::vec3 ace = glm::vec3(0);
		float extent = 0;

		virtual ~SceneEntity();
};

SceneEntity::~SceneEntity()
{
}
