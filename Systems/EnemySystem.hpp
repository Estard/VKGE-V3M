#pragma once
#include "../Entities/Entities.hpp"
#include "RenderSystem.hpp"

class EnemySystem: public ISystem
{
	public:
		EnemySystem(PresentationSystem *presentationSystem,
				vkjob::ThreadPool *tpool, RenderSystem *renderSystem)
		{
			this->prSystem = presentationSystem;
			this->tPool = tpool;
			this->renderSystem = renderSystem;
			this->sysFlag = VGE_SYSTEM_ENEMY;
		}

		EnemySystem()
		{
			this->prSystem = nullptr;
			this->tPool = nullptr;
			this->renderSystem = renderSystem;
			this->sysFlag = VGE_SYSTEM_ENEMY;
		}

		~EnemySystem()
		{
		}

		void OnUpdate()
		{
			if (!prepared)
				prepare();

			updateEnemies();

		}

		std::vector<SceneEntity*> enemies;

	private:
		RenderSystem *renderSystem = nullptr;
		bool prepared = false;

		struct EnemieRenderData
		{
				uint32_t count;
				uint32_t offset;
		};

		void prepare()
		{
			for (uint32_t i = 0; i < renderSystem->entities.size(); i++)
			{
				if (renderSystem->entities.at(i).entity->SystemFlags
						& this->sysFlag)
				{
					SceneEntity *enemy = &renderSystem->entities.at(i);
					enemies.push_back(enemy);
				}
			}

			std::cout << "So many Enemies : " << enemies.size() << std::endl;
			prepared = true;
		}

		void updateEnemies()
		{
			uint32_t offset = 0;
			uint32_t rest = enemies.size() % threadCount;
			for (uint32_t t = 0; t < threadCount; t++)
			{
				EnemieRenderData erd;
				erd.count = (enemies.size() / threadCount)
						+ (t == 0 ? rest : 0);
				erd.offset = offset;
				offset += erd.count;

				if (erd.count > 0)
				{
					tPool->threads[t]->addJob([=]
					{
						ThreadEnemyCode(erd);});
				}

			}
		}

		void ThreadEnemyCode(EnemieRenderData erd)
		{

			if (player.active)
				for (uint32_t i = 0; i < erd.count; i++)
				{
					SceneEntity *enemy = enemies.at(i + erd.offset);
					glm::vec3 direction = player.transform.position
							- enemy->transform.position;

					enemy->vel += deltaTime
							* (enemy->ace
									+ enemy->speed * glm::normalize(direction)
									+ (direction.z >= 0 ?
											glm::vec3(0, 0, 9.81) :
											glm::vec3(0, 0, 4.905)));

					glm::vec4 rotation = glm::vec4(1)
							* glm::lookAt(player.transform.position,
									enemy->transform.position,
									glm::vec3(0, 0, 1));

					enemy->transform.position += deltaTime * enemy->vel;
					if (enemy->transform.position.z < -2)
						enemy->transform.position.z = -2;
					enemy->transform.rotation = glm::vec3(rotation.x,
							rotation.y, rotation.z);
				}
		}
};
