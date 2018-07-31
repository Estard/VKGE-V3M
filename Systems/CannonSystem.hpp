#pragma once
#include "../Entities/Entities.hpp"
#include "StandardSystem.hpp"
#include "RenderSystem.hpp"

class CannonSystem: public ISystem
{
	public:
		CannonSystem(PresentationSystem *presentationSystem,
				vkjob::ThreadPool *tpool, RenderSystem *renderSystem)
		{
			this->prSystem = presentationSystem;
			this->tPool = tpool;
			this->renderSystem = renderSystem;
			this->sysFlag = VGE_SYSTEM_CANNON;
		}

		CannonSystem()
		{
			this->prSystem = nullptr;
			this->tPool = nullptr;
			this->renderSystem = renderSystem;
			this->sysFlag = VGE_SYSTEM_CANNON;
		}

		~CannonSystem()
		{
		}

		void OnUpdate()
		{
			if (!prepared)
				prepare();

			if (!player.active)
				return;

			fire();
			updateAmmo();
			hitEnemies();
		}

		std::vector<SceneEntity*> impulsAmmo = { };
		std::vector<SceneEntity*> targets = { };
		std::vector<float> impulsCooldown = { };
		uint32_t nextShot = 0;

		SceneEntity* impulsCannon = nullptr;

	private:
		RenderSystem *renderSystem = nullptr;
		bool prepared = false;

		void prepare()
		{
			for (uint32_t i = 0; i < renderSystem->entities.size(); i++)
			{
				if (renderSystem->entities.at(i).entity->SystemFlags
						& this->sysFlag)
				{
					SceneEntity *ammo = &renderSystem->entities.at(i);
					impulsAmmo.push_back(ammo);
				}

				if (renderSystem->entities.at(i).entity->SystemFlags
						& VGE_SYSTEM_ENEMY)
				{
					SceneEntity *enemy = &renderSystem->entities.at(i);
					targets.push_back(enemy);
				}

				if (renderSystem->entities.at(i).material->name.compare(
						"CannonRight") == 0)
				{
					impulsCannon = &renderSystem->entities.at(i);
				}
			}

			impulsCooldown.resize(impulsAmmo.size());

			for (float &f : impulsCooldown)
			{
				f = 0.f;
			}

			prepared = true;
		}

		int oldFireState = GLFW_RELEASE;
		void fire()
		{
			int newFireState = glfwGetMouseButton(prSystem->window,
			GLFW_MOUSE_BUTTON_1);
			if (newFireState == GLFW_PRESS && oldFireState == GLFW_RELEASE)
			{
				if (impulsCooldown.at(nextShot) <= 0.f)
				{

					glm::vec3 direction = glm::rotateZ(glm::vec3(0, 10, 0),
							player.transform.rotation.z);

					SceneEntity *ammo = impulsAmmo.at(nextShot);
					ammo->transform.position = impulsCannon->transform.position
							+ glm::normalize(direction);

					direction += glm::vec3(0, 0,
							15 * (player.transform.rotation.x - pi / 2));
					ammo->ace = direction;
					direction *= 500;
					ammo->vel = direction;
					impulsCooldown.at(nextShot) = 1.f;
					nextShot = (nextShot + 1) % impulsAmmo.size();
				}
			}

			for (float &f : impulsCooldown)
			{
				if (f <= 0.f)
					f = 0.f;
				else
					f -= deltaTime;
			}

			oldFireState = newFireState;
		}

		void updateAmmo()
		{
			for (size_t i = 0; i < impulsAmmo.size(); i++)
			{
				SceneEntity *shot = impulsAmmo.at(i);

				shot->transform.position += deltaTime * shot->ace;
				shot->vel = shot->ace;
				shot->vel *= 2;
			}
		}

		void hitEnemies()
		{
			uint32_t count = targets.size() / threadCount;
			uint32_t rest = targets.size() - count * threadCount;

			for (uint32_t t = 0; t < threadCount; t++)
			{

				tPool->threads[t]->addJob(
						[=]
						{
							ThreadCompareCode(count+(t==threadCount-1?rest:0),count*t);});
			}
		}

		void ThreadCompareCode(uint32_t count, uint32_t offset)
		{
			for (auto ammo : impulsAmmo)
				for (uint32_t i = 0; i < count; i++)
				{
					SceneEntity *target = targets.at(i + offset);
					if (target->extent + ammo->extent
							>= glm::distance(ammo->transform.position,
									target->transform.position))
					{
						target->vel += ammo->vel;
						ammo->ace /= 2;
						ammo->ace += glm::vec3(0, 0, -1);
					}
				}
		}
};
