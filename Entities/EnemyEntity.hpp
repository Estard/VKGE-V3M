#pragma once
#include "SceneEntity.hpp"

struct EnemyEntity: public SceneEntity
{
		EnemyEntity()
		{
			this->speed = 0.9f;
		}
		~EnemyEntity()
		{
		}
};
