#include "Definitions.hpp"
#include "Systems/Systems.hpp"
#include "Entities/Entities.hpp"

class VGE_V3M
{
	public:

		VGE_V3M()
		{
			//Lade Scene
			//->Lade Modelle
			//->Lade Materialien
			//->Erstelle Entities
			loadStandardScene(&prefabs, &sceneEntities, &materials);
			//Lade Vulkan
			//Erstelle WorkerThreads
			threadPool.setThreadCount(threadCount);
			//->Lade Systeme
			//->Verteile Aufgaben
			//Update Loop

			//UniformBuffer, DescriptorPool, DescriptorSet
			createPlayerMovement();

			loadMaterials();
			loadScene();

			renderSystem = RenderSystem(&prSystem, &threadPool);
			enemySystem = EnemySystem(&prSystem, &threadPool, &renderSystem);
			turretSystem = TurretSystem(&prSystem, &threadPool, &renderSystem);
			physikSystem = PhysikSystem(&prSystem, &threadPool, &renderSystem);
			cannonSystem = CannonSystem(&prSystem, &threadPool, &renderSystem);
		}

		~VGE_V3M()
		{
			unloadMaterials();
		}

		void play()
		{
			prepareSystems();
			while (!glfwWindowShouldClose(prSystem.window))
			{
				glfwPollEvents();
				updateView();
				for (auto system : systems)
				{
					system->OnUpdate();
				}
			}
		}

	private:

		PresentationSystem prSystem;
		vkjob::ThreadPool threadPool;
		RenderSystem renderSystem = { };
		EnemySystem enemySystem = { };
		TurretSystem turretSystem = { };
		PhysikSystem physikSystem = { };
		CannonSystem cannonSystem = { };
		std::vector<ISystem*> systems = { &turretSystem, &cannonSystem,
				&physikSystem, &enemySystem, &renderSystem };
		std::vector<vksc::ThreadData> threadData;

		std::vector<IEntity> prefabs;
		std::vector<SceneEntity> sceneEntities;
		std::vector<vk3d::Material> materials;
		std::vector<vk3d::Vertex> vertices;
		std::vector<uint32_t> indices;

		void createPlayerMovement()
		{
			//Uniform Buffer
			VkDeviceSize bufferSize = sizeof(vksc::UniformBufferData);
			prSystem.createBuffer(bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
							| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&prSystem.buffers.uniformBuffer,
					&prSystem.buffers.uniformBufferMemory);

			//DescriptorPool
			uint32_t descriptorCount = materials.size();
			std::array<VkDescriptorPoolSize, 2> poolSizes = { };
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = descriptorCount;
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = descriptorCount;

			VkDescriptorPoolCreateInfo poolInfo = vk::DescriptorPoolCreateInfo(
					{ }, descriptorCount + 1,
					static_cast<uint32_t>(poolSizes.size()));
			poolInfo.pPoolSizes = poolSizes.data();

			vkCheck(
					vkCreateDescriptorPool(prSystem.logicalDevice, &poolInfo,
							nullptr, &prSystem.descriptors.dPool),
					"Beim Descriptor Pool ist was schief gelaufen");

			//Scene DescriptorSet
			prSystem.descriptors.sceneSet = createDescriptorSet(
					prSystem.logicalDevice, prSystem.descriptors.sceneLayout,
					prSystem.descriptors.dPool);

		}

		void loadMaterials()
		{
			for (vk3d::Material &m : materials)
			{
				m.pipeline = &prSystem.pipelines.solid;

				for (size_t i = 0; i < m.fileNames.size(); i++)
				{
					switch (i + 1)
					{
						case 1:
							m.textures.diffuseTop = m.textures.diffuseSide =
									m.textures.diffuseFront = createTexture(
											prSystem.logicalDevice,
											prSystem.physicalDevice,
											prSystem.cmdPool,
											prSystem.queues.graphics,
											m.fileNames[i]);
							break;
						case 2:
							m.textures.normalTop = m.textures.normalSide =
									m.textures.normalFront = createTexture(
											prSystem.logicalDevice,
											prSystem.physicalDevice,
											prSystem.cmdPool,
											prSystem.queues.graphics,
											m.fileNames[i]);
							break;
						case 3:
							m.textures.specularTop = m.textures.specularSide =
									m.textures.specularFront = createTexture(
											prSystem.logicalDevice,
											prSystem.physicalDevice,
											prSystem.cmdPool,
											prSystem.queues.graphics,
											m.fileNames[i]);
							break;
						case 4:
							m.textures.diffuseSide = m.textures.diffuseFront =
									createTexture(prSystem.logicalDevice,
											prSystem.physicalDevice,
											prSystem.cmdPool,
											prSystem.queues.graphics,
											m.fileNames[i]);
							break;
						case 5:
							m.textures.normalSide = m.textures.normalFront =
									createTexture(prSystem.logicalDevice,
											prSystem.physicalDevice,
											prSystem.cmdPool,
											prSystem.queues.graphics,
											m.fileNames[i]);
							break;
						case 6:
							m.textures.specularSide = m.textures.specularFront =
									createTexture(prSystem.logicalDevice,
											prSystem.physicalDevice,
											prSystem.cmdPool,
											prSystem.queues.graphics,
											m.fileNames[i]);
							break;
						case 7:
							m.textures.diffuseFront = createTexture(
									prSystem.logicalDevice,
									prSystem.physicalDevice, prSystem.cmdPool,
									prSystem.queues.graphics, m.fileNames[i]);
							break;
						case 8:
							m.textures.normalFront = createTexture(
									prSystem.logicalDevice,
									prSystem.physicalDevice, prSystem.cmdPool,
									prSystem.queues.graphics, m.fileNames[i]);
							break;
						case 9:
							m.textures.specularFront = createTexture(
									prSystem.logicalDevice,
									prSystem.physicalDevice, prSystem.cmdPool,
									prSystem.queues.graphics, m.fileNames[i]);
							break;
					}
				}
				m.mDSet = createDescriptorSet(prSystem.logicalDevice,
						prSystem.descriptors.materialLayout,
						prSystem.descriptors.dPool);

				createWriteDescriptorSet(prSystem.logicalDevice, m.mDSet,
				VK_NULL_HANDLE, m.textures.diffuseTop.view,
						m.textures.diffuseTop.sampler);
			}
			createWriteDescriptorSet(prSystem.logicalDevice,
					prSystem.descriptors.sceneSet,
					prSystem.buffers.uniformBuffer,
					VK_NULL_HANDLE,
					VK_NULL_HANDLE);
		}

		void loadScene()
		{

			uint32_t indexBase = 0;
			uint32_t vertexOffset = 0;
			for (SceneEntity &re : sceneEntities)
			{

				re.entity->indexBase = indexBase;
				re.entity->vertexOffset = vertexOffset;

				re.entity->indexCount = re.entity->mesh.preIndexBuffer.size();

				for (vk3d::Vertex v : re.entity->mesh.preVertexBuffer)
				{
					vertices.push_back(v);
				}

				for (uint32_t i : re.entity->mesh.preIndexBuffer)
				{
					indices.push_back(i);
				}

				indexBase += re.entity->mesh.preIndexBuffer.size();
				vertexOffset += re.entity->mesh.preVertexBuffer.size();
				if (re.material == nullptr)
					re.material = &materials[0];

			}
			createVertexAndIndexBuffer();
		}

		void createVertexAndIndexBuffer()
		{
			//Vertices
			VkDeviceSize bufferSizeV = sizeof(vertices[0]) * vertices.size();

			VkBuffer stagingBufferV;
			VkDeviceMemory stagingBufferMemoryV;

			prSystem.createBuffer(bufferSizeV, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
							| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&stagingBufferV, &stagingBufferMemoryV);
			void* dataV;
			vkMapMemory(prSystem.logicalDevice, stagingBufferMemoryV, 0,
					bufferSizeV, 0, &dataV);
			memcpy(dataV, vertices.data(), (size_t) bufferSizeV);
			vkUnmapMemory(prSystem.logicalDevice, stagingBufferMemoryV);

			prSystem.createBuffer(bufferSizeV,
					VK_BUFFER_USAGE_TRANSFER_DST_BIT
							| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&prSystem.buffers.vertexBuffer,
					&prSystem.buffers.vertexBufferMemory);

			copyBuffer(prSystem.logicalDevice, prSystem.cmdPool,
					prSystem.queues.graphics, stagingBufferV,
					prSystem.buffers.vertexBuffer, bufferSizeV);
			vkDestroyBuffer(prSystem.logicalDevice, stagingBufferV, nullptr);
			vkFreeMemory(prSystem.logicalDevice, stagingBufferMemoryV, nullptr);

			//Indices
			VkDeviceSize bufferSizeI = sizeof(indices[0]) * indices.size();

			VkBuffer stagingBufferI;
			VkDeviceMemory stagingBufferMemoryI;
			prSystem.createBuffer(bufferSizeI, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
							| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					&stagingBufferI, &stagingBufferMemoryI);

			void *dataI;
			vkMapMemory(prSystem.logicalDevice, stagingBufferMemoryI, 0,
					bufferSizeI, 0, &dataI);
			memcpy(dataI, indices.data(), (size_t) bufferSizeI);
			vkUnmapMemory(prSystem.logicalDevice, stagingBufferMemoryI);

			prSystem.createBuffer(bufferSizeI,
					VK_BUFFER_USAGE_TRANSFER_DST_BIT
							| VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					&prSystem.buffers.indexBuffer,
					&prSystem.buffers.indexBufferMemory);

			copyBuffer(prSystem.logicalDevice, prSystem.cmdPool,
					prSystem.queues.graphics, stagingBufferI,
					prSystem.buffers.indexBuffer, bufferSizeI);

			vkDestroyBuffer(prSystem.logicalDevice, stagingBufferI, nullptr);
			vkFreeMemory(prSystem.logicalDevice, stagingBufferMemoryI, nullptr);

		}

		void updateView()
		{
			auto currentTime = std::chrono::high_resolution_clock::now();
			deltaTime =
					std::chrono::duration<float, std::chrono::seconds::period>(
							currentTime - startTime).count();

			startTime = currentTime;

			int newState = glfwGetKey(prSystem.window, GLFW_KEY_ESCAPE);

			if (newState == GLFW_PRESS && player.oldState == GLFW_RELEASE)
			{
				player.active = !player.active;
				if (player.active)
				{
					glfwSetCursorPos(prSystem.window,
							prSystem.swapChain.swapChainExtent.width / 2.0,
							prSystem.swapChain.swapChainExtent.height / 2.0);
					std::cout << "active" << std::endl;
					glfwSetInputMode(prSystem.window, GLFW_CURSOR,
					GLFW_CURSOR_HIDDEN);
				}
				else
				{
					std::cout << "inactive" << std::endl;
					glfwSetInputMode(prSystem.window, GLFW_CURSOR,
					GLFW_CURSOR_NORMAL);
				}
			}

			player.oldState = newState;
			if (!player.active)
				return;

			double xpos, ypos;
			glfwGetCursorPos(prSystem.window, &xpos, &ypos);

			glfwSetCursorPos(prSystem.window,
					prSystem.swapChain.swapChainExtent.width / 2.0,
					prSystem.swapChain.swapChainExtent.height / 2.0);

			if (xpos == 0.0 && ypos == 0.0)
				return;

			glm::vec3 moveDir = glm::vec3(0);
			if (glfwGetKey(prSystem.window, GLFW_KEY_W) == GLFW_PRESS)
				moveDir.y = 1;
			else if (glfwGetKey(prSystem.window,
			GLFW_KEY_S) == GLFW_PRESS)
				moveDir.y = -1;

			if (glfwGetKey(prSystem.window, GLFW_KEY_D) == GLFW_PRESS)
				moveDir.x = 1;
			else if (glfwGetKey(prSystem.window,
			GLFW_KEY_A) == GLFW_PRESS)
				moveDir.x = -1;

			if (glfwGetKey(prSystem.window,
			GLFW_KEY_SPACE) == GLFW_PRESS)
				moveDir.z = 1;
			else if (glfwGetKey(prSystem.window,
			GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
				moveDir.z = -1;

			if (glfwGetKey(prSystem.window,
			GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(prSystem.window,
			GLFW_KEY_R) == GLFW_PRESS)
				player.speed = 3.0f;
			else
				player.speed = 1.0f;

			player.transform.rotation += glm::vec3(
					player.mouseSpeed * deltaTime
							* (prSystem.swapChain.swapChainExtent.height / 2.0
									- ypos), 0,
					player.mouseSpeed * deltaTime
							* (prSystem.swapChain.swapChainExtent.width / 2.0
									- xpos));

			if (player.transform.rotation.x > pi)
				player.transform.rotation.x = pi;

			if (player.transform.rotation.x < 0)
				player.transform.rotation.x = 0;

			vksc::UniformBufferData ubo = { };
			glm::mat4 model = glm::mat4(1);
			glm::mat4 proj = glm::perspective(glm::radians(player.initialFoV),
					prSystem.swapChain.swapChainExtent.width
							/ (float) prSystem.swapChain.swapChainExtent.height,
					0.1f, 50.0f);
			proj[1][1] *= -1;

			moveDir = glm::rotateZ(moveDir, player.transform.rotation.z);
			player.transform.position += player.speed * deltaTime * moveDir;

			glm::mat4 rotation = glm::toMat4(
					glm::quat(player.transform.rotation));

			glm::mat4 cameraTransform = glm::translate(model,
					player.transform.position) * rotation;

			glm::mat4 view = glm::inverse(cameraTransform);

			ubo.model = model;
			ubo.view = view;
			ubo.MVP = proj * view * model;

			void *data;
			vkMapMemory(prSystem.logicalDevice,
					prSystem.buffers.uniformBufferMemory, 0, sizeof(ubo), 0,
					&data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(prSystem.logicalDevice,
					prSystem.buffers.uniformBufferMemory);

			//	std::cout << player.position.x << " " << player.position.y << " "
			//			<< player.position.z << std::endl;
		}

		void destroyTexture(vk3d::Texture texture)
		{
			vkDestroySampler(prSystem.logicalDevice, texture.sampler, nullptr);
			vkDestroyImageView(prSystem.logicalDevice, texture.view, nullptr);

			vkDestroyImage(prSystem.logicalDevice, texture.image, nullptr);
			vkFreeMemory(prSystem.logicalDevice, texture.memory, nullptr);
		}

		void unloadMaterials()
		{
			for (vk3d::Material m : materials)
			{
				if (m.textures.diffuseTop.image)
				{
					destroyTexture(m.textures.diffuseTop);
					if (m.textures.diffuseFront.image
							!= m.textures.diffuseTop.image)
						destroyTexture(m.textures.diffuseFront);
					if (m.textures.diffuseSide.image
							!= m.textures.diffuseTop.image)
						destroyTexture(m.textures.diffuseSide);
				}
				if (m.textures.normalTop.image)
				{
					destroyTexture(m.textures.normalTop);
					if (m.textures.normalFront.image
							!= m.textures.normalTop.image)
						destroyTexture(m.textures.normalFront);
					if (m.textures.normalSide.image
							!= m.textures.normalTop.image)
						destroyTexture(m.textures.normalSide);
				}
				if (m.textures.specularTop.image)
				{
					destroyTexture(m.textures.specularTop);
					if (m.textures.specularFront.image
							!= m.textures.specularTop.image)
						destroyTexture(m.textures.specularFront);
					if (m.textures.specularSide.image
							!= m.textures.specularTop.image)
						destroyTexture(m.textures.specularSide);
				}
			}
		}

		void prepareSystems()
		{
			renderSystem.setEntities(sceneEntities);
		}

};
