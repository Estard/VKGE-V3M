#pragma once
#include "RenderSystem.hpp"

class PhysikSystem: public ISystem
{
	public:

		PhysikSystem(PresentationSystem *presentationSystem,
				vkjob::ThreadPool *tpool, RenderSystem *renderSystem)
		{
			this->prSystem = presentationSystem;
			this->tPool = tpool;
			this->renderSystem = renderSystem;
			this->sysFlag = VGE_SYSTEM_PHYSIK;
		}

		PhysikSystem()
		{
			this->prSystem = nullptr;
			this->tPool = nullptr;
			this->sysFlag = VGE_SYSTEM_PHYSIK;
		}

		~PhysikSystem()
		{
		}

		void OnUpdate()
		{
			if (!prepared)
				prepare();

			updateMovement();
			hitdetect();

		}

	private:
		RenderSystem *renderSystem = nullptr;
		bool prepared = false;

		int areaResMul = 4;

		struct ChunkData
		{
				bool override = false;
				bool isStatic = false;
				glm::vec3 staticPos = glm::vec3(0);
				std::vector<SceneEntity*> hitting = { };
		};

		struct AreaIndex
		{
				size_t x, y, z;
		};

		struct Area
		{
				size_t ex, ey, ez;

				Area(size_t x = 0, size_t y = 0, size_t z = 0)
				{
					ex = x;
					ey = y;
					ez = z;
					data.resize(x * y * z);
				}

				ChunkData *at(size_t x, size_t y, size_t z)
				{
					if (x <= ex && y <= ey && z <= ez)
						return &data.at(x * ey * ez + y * ez + z);
					else
						return nullptr;
				}

				ChunkData *at(glm::vec3 pos)
				{
					size_t x = static_cast<size_t>(pos.x);
					size_t y = static_cast<size_t>(pos.y);
					size_t z = static_cast<size_t>(pos.z);
					if (x <= ex && y <= ey && z <= ez)
						return &data.at(x * ey * ez + y * ez + z);
					else
						return nullptr;
				}

				ChunkData *at(AreaIndex ai)
				{
					if (ai.x <= ex && ai.y <= ey && ai.z <= ez)
						return &data.at(ai.x * ey * ez + ai.y * ez + ai.z);
					else
						return nullptr;
				}

			private:
				std::vector<ChunkData> data = { };
		};

		Area area = Area(256, 256, 128);
		std::vector<AreaIndex> searchHere = { };

		std::vector<SceneEntity*> scene = { };

		void prepare()
		{
			scene.resize(renderSystem->entities.size());
			for (size_t i = 0; i < renderSystem->entities.size(); i++)
			{
				scene[i] = &renderSystem->entities.at(i);
				scene[i]->ace = glm::vec3(0, 0, -9.81);
				if (scene[i]->material->name.compare("Terrain") == 0)
				{
					SceneEntity *terrain = scene[i];

					for (auto vertex : terrain->entity->mesh.preVertexBuffer)
					{
						AreaIndex ai = areaIndex(vertex.pos);
						if (!area.at(ai)->isStatic)
							area.at(ai)->isStatic = true;
						area.at(ai)->staticPos += vertex.pos;
						area.at(ai)->staticPos /= 2.0;
					}
				}
			}

			prepared = true;
		}

		void updateMovement()
		{
			uint32_t count = scene.size() / threadCount;
			uint32_t rest = scene.size() - count * threadCount;

			for (uint32_t t = 0; t < threadCount; t++)
			{
				tPool->threads[t]->addJob(
						[=]
						{
							ThreadMovementCode(count+(t==threadCount-1?rest:0),count*t,t);});
			}

		}

		void ThreadMovementCode(uint32_t count, uint32_t offset,
				uint32_t threadIndex)
		{
			for (uint32_t i = offset; i < offset + count; i++)
			{
				SceneEntity* se = scene.at(i);
				se->vel *= 1.0 - (0.1 * deltaTime);
			}
		}

		void hitdetect()
		{
			searchHere.clear();
			for (uint32_t i = 0; i < scene.size(); i++)
			{
				SceneEntity* se = scene.at(i);
				AreaIndex pos = areaIndex(se->transform.position); // + se->vel * deltaTime);
				ChunkData *field = area.at(pos);
				if (field != nullptr
						&& (se->entity->SystemFlags & VGE_SYSTEM_PHYSIK))
				{
					field->hitting.push_back(se);
					if (!field->override
							&& (field->hitting.size() > 1 || field->isStatic))
					{
						field->override = true;
						searchHere.push_back(pos);
					}
				}
			}

			uint32_t count = searchHere.size() / threadCount;
			uint32_t rest = searchHere.size() - count * threadCount;

			for (uint32_t t = 0; t < threadCount; t++)
			{

				tPool->threads[t]->addJob(
						[=]
						{
							ThreadImpactCode(count+(t==threadCount-1?rest:0),count*t);});
			}

		}

		void ThreadImpactCode(size_t count, size_t offset)
		{
			for (size_t i = 0; i < count; i++)
			{
				AreaIndex ai = searchHere.at(i + offset);
				ChunkData *field = area.at(ai);
				if (field->override)
				{
					if (field->isStatic)
					{
						for (auto entity : field->hitting)
						{
							if (glm::length(
									entity->transform.position
											- field->staticPos) < 2.0)
								; //Suspicious O_o
							entity->vel.z *= entity->vel.z > 0 ? 1 : -0.5;

						}
					}
					else
					{
						for (size_t i = 0; i < field->hitting.size(); i++)
						{
							auto e1 = field->hitting.at(i);

							for (size_t j = i + 1; j < field->hitting.size();
									j++)
							{
								auto e2 = field->hitting.at(j);

								if (e1 == e2)
									return;

								if (glm::distance(e1->transform.position,
										e2->transform.position)
										<= e1->extent + e2->extent)
								{

									if (glm::length(e1->vel)
											> glm::length(e2->vel))
									{
										e2->vel += e1->vel;
										e1->vel /= 2.0;
									}
									else
									{
										e1->vel += e2->vel;
										e2->vel /= 2.0;
									}

								}
							}

						}
					}

					field->hitting.clear();
					field->override = false;
				}

			}
		}

		AreaIndex areaIndex(glm::vec3 pos)
		{
			size_t x = pos.x >= 0 ? static_cast<size_t>(pos.x * areaResMul) : 0;
			size_t y = pos.y >= 0 ? static_cast<size_t>(pos.y * areaResMul) : 0;
			size_t z = pos.z >= 0 ? static_cast<size_t>(pos.z * areaResMul) : 0;

			x = x >= area.ex ? area.ex - 1 : x + 0;
			y = y >= area.ey ? area.ey - 1 : y + 0;
			z = z >= area.ez ? area.ez - 1 : z + 0;

			return AreaIndex { x, y, z };
		}

};
