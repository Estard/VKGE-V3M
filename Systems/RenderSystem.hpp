#pragma once
#include "../Entities/Entities.hpp"
#include "StandardSystem.hpp"

class RenderSystem: public ISystem
{
	public:
		RenderSystem(PresentationSystem *presentationSystem,
				vkjob::ThreadPool *tpool)
		{
			this->prSystem = presentationSystem;
			this->tPool = tpool;
			this->sysFlag = VGE_SYSTEM_RENDERER;
			synchro = &prSystem->synchronizers;

		}
		RenderSystem()
		{
			player.transform.position = glm::vec3(10, 10, 2.5);
		}

		float frameTimer = 0.f;
		int fps = 0;

		~RenderSystem()
		{
			for (uint32_t i = 0; i < trd.size(); i++)
			{
				vkDestroyCommandPool(prSystem->logicalDevice, trd[i].cmdPool,
						nullptr);
			}
		}

		void setEntities(std::vector<SceneEntity> entities)
		{
			this->entities = entities;
		}

		void OnUpdate()
		{
			if (!prepared)
				prepare();

			if (frameTimer <= 1.f)
			{
				frameTimer += deltaTime;
				fps++;
			}
			else
			{
				std::cout << fps << " fps" << std::endl;
				frameTimer = 0.f;
				fps = 0;

			}
			draw();
		}

		void addEntity(SceneEntity toAdd)
		{
		}

		void removeEntity(SceneEntity toRemove)
		{
		}

		std::vector<SceneEntity> entities;

	private:
		vksc::Synchronizers *synchro = nullptr;
		bool prepared = false;
		vkjob::ThreadPool *tPool = nullptr;
		float r = 41 / 255.0;
		float g = 104 / 255.0;
		float b = 233 / 255.0;
		glm::mat4 m4 = glm::mat4(1);
		uint32_t addToNext = 0;

		struct ThreadRenderData
		{
				VkCommandPool cmdPool;
				std::vector<VkCommandBuffer> commandBuffers = { };
				std::vector<SceneEntity*> objects { };
		};
		std::vector<ThreadRenderData> trd = { };

		void draw()
		{
			vkQueueWaitIdle(prSystem->queues.present);
			VkSemaphore imageAvailableSemaphore =
					prSystem->synchronizers.imageAvailableSemas[0];
			VkSemaphore renderFinishedSemaphore =
					prSystem->synchronizers.renderFinishedSemas[0];
			uint32_t imageIndex;
			VkResult result = vkAcquireNextImageKHR(prSystem->logicalDevice,
					prSystem->swapChain.swapChain,
					std::numeric_limits<uint64_t>::max(),
					imageAvailableSemaphore,
					VK_NULL_HANDLE, &imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				prSystem->recreateSwapChain();
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
				throw std::runtime_error(
						"Swapchain Image konnte nicht erhalten werden");

			updateCmdBuffer(
					prSystem->swapChain.swapChainFramebuffers[imageIndex]);

			VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
			VkPipelineStageFlags waitStages[] = {
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

			VkSubmitInfo submitInfo = { };
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &prSystem->primaryCmdBuffer;

			VkSemaphore signaleSemaphores[] = { renderFinishedSemaphore };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signaleSemaphores;

			vkCheck(
					vkQueueSubmit(prSystem->queues.graphics, 1, &submitInfo,
							prSystem->synchronizers.renderFence),
					"Failed to submit Draw Command Buffer");

			VkResult fenceRes;
			do
			{
				fenceRes = vkWaitForFences(prSystem->logicalDevice, 1,
						&synchro->renderFence, VK_TRUE,
						std::numeric_limits<uint64_t>::max());
			} while (fenceRes == VK_TIMEOUT);
			vkResetFences(prSystem->logicalDevice, 1, &synchro->renderFence);

			VkPresentInfoKHR presentInfo = { };
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signaleSemaphores;

			VkSwapchainKHR swapChains[] = { prSystem->swapChain.swapChain };
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = swapChains;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			result = vkQueuePresentKHR(prSystem->queues.present, &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR
					|| result == VK_SUBOPTIMAL_KHR)
				prSystem->recreateSwapChain();
			else if (result != VK_SUCCESS)
				throw std::runtime_error("Bildpräsentation gescheitert!");



		}

		void updateCmdBuffer(VkFramebuffer framebuffer)
		{
			VkCommandBufferBeginInfo cmdBufferBeginInfo =
					vk::CommandBufferBeginInfo(
							vk::CommandBufferUsageFlagBits::eRenderPassContinue
									| vk::CommandBufferUsageFlagBits::eSimultaneousUse);

			std::array<VkClearValue, 2> clearValues = { };
			clearValues[0].color =
			{	r,g,b};

			clearValues[1].depthStencil =
			{	1.0f,0};
			VkRenderPassBeginInfo renderPassInfo = { };
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = prSystem->renderPass;
			renderPassInfo.renderArea.offset =
			{	0,0};
			renderPassInfo.renderArea.extent =
					prSystem->swapChain.swapChainExtent;
			renderPassInfo.clearValueCount =
					static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();
			renderPassInfo.framebuffer = framebuffer;

			vkCheck(
					vkBeginCommandBuffer(prSystem->primaryCmdBuffer,
							&cmdBufferBeginInfo),
					"PrimaryCmdBuffer went wrong");

			vkCmdBeginRenderPass(prSystem->primaryCmdBuffer, &renderPassInfo,
					VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

			VkCommandBufferInheritanceInfo erbschaft =
					vk::CommandBufferInheritanceInfo();
			erbschaft.renderPass = prSystem->renderPass;
			erbschaft.framebuffer = framebuffer;

			std::vector<VkCommandBuffer> cmdBuffers;

			for (uint32_t t = 0; t < threadCount; t++)
			{
				for (uint32_t i = 0; i < trd[t].commandBuffers.size(); i++)
				{
					//ThreadRenderCode(t, i, erbschaft);
					tPool->threads[t]->addJob([=]
					{
						ThreadRenderCode(t,i,erbschaft);});
				}
			}
			tPool->wait();

			for (uint32_t t = 0; t < threadCount; t++)
			{
				for (uint32_t i = 0; i < trd[t].commandBuffers.size(); i++)
				{
					cmdBuffers.push_back(trd[t].commandBuffers[i]);
				}
			}

			vkCmdExecuteCommands(prSystem->primaryCmdBuffer, cmdBuffers.size(),
					cmdBuffers.data());
			vkCmdEndRenderPass(prSystem->primaryCmdBuffer);
			vkCheck(vkEndCommandBuffer(prSystem->primaryCmdBuffer),
					"Primäerer Cmd Buffer konnte nicht geschlossen werden");
		}

		void prepare()
		{
			VkCommandBufferAllocateInfo cmdBufAllocInfo =
					vk::CommandBufferAllocateInfo(prSystem->cmdPool,
							vk::CommandBufferLevel::ePrimary, 1);
			vkCheck(
					vkAllocateCommandBuffers(prSystem->logicalDevice,
							&cmdBufAllocInfo, &prSystem->primaryCmdBuffer),
					"Primärer CmdBuffer konnte nicht erstellt werden");

			trd.resize(threadCount);
			uint32_t objectCountPT = entities.size() / threadCount;
			uint32_t rest = entities.size() - objectCountPT * threadCount;
			uint32_t objectsDone = 0;
			for (uint32_t i = 0; i < threadCount; i++)
			{
				ThreadRenderData *thread = &trd[i];
				VkCommandPoolCreateInfo poolInfo = vk::CommandPoolCreateInfo(
						vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
						prSystem->queues.indices.graphicsFamily);
				vkCheck(
						vkCreateCommandPool(prSystem->logicalDevice, &poolInfo,
								nullptr, &thread->cmdPool),
						"CommandPool konnte nicht gefüllt werden");

				uint32_t threadObjects =
						i == 0 ? objectCountPT + rest : objectCountPT;

				if (threadObjects == 0)
					continue;

				thread->commandBuffers.resize(threadObjects);

				VkCommandBufferAllocateInfo secondaryCmdBufAllocInfo =
						vk::CommandBufferAllocateInfo(thread->cmdPool,
								vk::CommandBufferLevel::eSecondary,
								thread->commandBuffers.size());

				vkCheck(
						vkAllocateCommandBuffers(prSystem->logicalDevice,
								&secondaryCmdBufAllocInfo,
								thread->commandBuffers.data()),
						"Thread CommandBuffers konnten nicht allociert werden");

				thread->objects.resize(threadObjects);
				for (uint32_t j = 0; j < threadObjects; j++, objectsDone++)
				{
					thread->objects[j] = &entities.at(objectsDone);
				}
			}

			prepared = true;
		}

		void ThreadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex,
				VkCommandBufferInheritanceInfo erbschaft)
		{
			ThreadRenderData *thread = &trd.at(threadIndex);
			vk3d::Transform *transform =
					&thread->objects[cmdBufferIndex]->transform;

			VkCommandBufferBeginInfo cmdBufBeginInfo =
					vk::CommandBufferBeginInfo(
							vk::CommandBufferUsageFlagBits::eRenderPassContinue
									| vk::CommandBufferUsageFlagBits::eSimultaneousUse);
			cmdBufBeginInfo.pInheritanceInfo = &erbschaft;

			VkCommandBuffer cmdBuffer = thread->commandBuffers[cmdBufferIndex];

			vkCheck(vkBeginCommandBuffer(cmdBuffer, &cmdBufBeginInfo),
					"Secondäerer CmdBuffer will nicht starten");

			VkViewport viewport = vk::Viewport(0, 0,
					(float) prSystem->swapChain.swapChainExtent.width,
					(float) prSystem->swapChain.swapChainExtent.height, 0, 1);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			VkRect2D scissor = { };
			scissor.offset =
			{	0,0};
			scissor.extent = prSystem->swapChain.swapChainExtent;
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

			std::array<VkDescriptorSet, 2> descriptorSets = {
					prSystem->descriptors.sceneSet,
					thread->objects[cmdBufferIndex]->material->mDSet };

			VkDeviceSize offsets[1] = { 0 };

			vkCmdBindVertexBuffers(cmdBuffer, 0, 1,
					&prSystem->buffers.vertexBuffer, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, prSystem->buffers.indexBuffer, 0,
					VK_INDEX_TYPE_UINT32);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					prSystem->pipelines.solid);

			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
					prSystem->pipelines.layout, 0,
					static_cast<uint32_t>(descriptorSets.size()),
					descriptorSets.data(), 0, nullptr);

			if (!thread->objects[cmdBufferIndex]->replaceAble)
			{

				if (thread->objects[cmdBufferIndex]->entity->indexCount < 100)
				{
					transform->rotation.x = pi / 2.f;
					//transform->rotation.y += deltaTime;
					transform->rotation.z += deltaTime;
				}

				transform->pushBlock.model = glm::translate(m4,
						transform->position);

				transform->pushBlock.model *= glm::toMat4(
						glm::quat(transform->rotation));

				transform->pushBlock.model = glm::scale(
						transform->pushBlock.model, transform->scale);

				vkCmdPushConstants(cmdBuffer, prSystem->pipelines.layout,
						VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(vk3d::PushBlock),
						&transform->pushBlock);

				vkCmdDrawIndexed(cmdBuffer,
						thread->objects[cmdBufferIndex]->entity->indexCount, 1,
						thread->objects[cmdBufferIndex]->entity->indexBase,
						thread->objects[cmdBufferIndex]->entity->vertexOffset,
						0);
			}
			vkCheck(vkEndCommandBuffer(cmdBuffer),
					"CmdBuffer konnte nicht geschlossen werden");

		}
};
