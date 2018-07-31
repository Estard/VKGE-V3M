#pragma once
#include "../Definitions.hpp"
#include "../Components/Vk3DComponents.hpp"
class IEntity
{
	public:
		uint64_t SystemFlags;
		IEntity(uint64_t SystemFlags  = 0)
		{
			this->SystemFlags = SystemFlags;
		}
		~IEntity()
		{
			mesh.preIndexBuffer.clear();
			mesh.preVertexBuffer.clear();
		}
		vk3d::Mesh mesh = { };
		uint32_t indexBase = 0;
		uint32_t indexCount = 0;
		uint32_t vertexOffset = 0;
};
