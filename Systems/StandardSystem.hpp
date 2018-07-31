#pragma once
#include "../Definitions.hpp"
#include "PresentationSystem.hpp"

class ISystem
{

	public:
		virtual void OnUpdate() = 0;
		virtual ~ISystem();
		SystemFlag sysFlag = VGE_SYSTEM_STANDARD;

	protected:
		PresentationSystem *prSystem = nullptr;
		vkjob::ThreadPool  *tPool = nullptr;

};

ISystem::~ISystem()
{
}
