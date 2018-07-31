#pragma once
#include "RenderSystem.hpp"

class TurretSystem: public ISystem
{
	public:
		TurretSystem(PresentationSystem *presentationSystem,
				vkjob::ThreadPool *tpool, RenderSystem *renderSystem)
		{
			this->prSystem = presentationSystem;
			this->tPool = tpool;
			this->renderSystem = renderSystem;
			this->sysFlag = VGE_SYSTEM_TURRET;
		}

		TurretSystem()
		{
			this->prSystem = nullptr;
			this->tPool = nullptr;
			this->renderSystem = renderSystem;
			this->sysFlag = VGE_SYSTEM_TURRET;
		}

		~TurretSystem()
		{
		}

		void OnUpdate()
		{
			if (!prepared)
				prepare();

			if (cockpit != nullptr)
			{
				cockpit->transform.position = player.transform.position;
				cockpit->transform.rotation = player.transform.rotation;
				cockpit->transform.scale = glm::vec3(0.01, 0.01, 0.0125);

				if (cannonLeft != nullptr)
				{
					cannonLeft->transform.position =
							cockpit->transform.position;
					cannonLeft->transform.scale = glm::vec3(0.01, 0.01, 0.01);
					cannonLeft->transform.rotation =
							cockpit->transform.rotation;
					//+ glm::vec3(pi / 2.0, 0, 0);
				}
				if (cannonRight != nullptr)
				{
					cannonRight->transform.position =
							cockpit->transform.position;
					cannonRight->transform.scale = glm::vec3(0.01, 0.01, 0.01);
					cannonRight->transform.rotation =
							cockpit->transform.rotation;
				}
			}
		}

		SceneEntity *cockpit = nullptr;
		SceneEntity *cannonLeft = nullptr;
		SceneEntity *cannonRight = nullptr;

	private:
		RenderSystem *renderSystem = nullptr;
		bool prepared = false;

		void prepare()
		{

			for (uint32_t i = 0; i < renderSystem->entities.size(); i++)
			{
				if (renderSystem->entities.at(i).material->name.compare(
						"Cockpit") == 0)
				{
					cockpit = &renderSystem->entities.at(i);
				}
				if (renderSystem->entities.at(i).material->name.compare(
						"CannonLeft") == 0)
				{
					cannonLeft = &renderSystem->entities.at(i);
				}
				if (renderSystem->entities.at(i).material->name.compare(
						"CannonRight") == 0)
				{
					cannonRight = &renderSystem->entities.at(i);
				}
			}
			prepared = true;
		}

		void ThreadEnemyCode()
		{

		}

};
