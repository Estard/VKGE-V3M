#pragma once
#include "../Definitions.hpp"

namespace vk3d
{
	struct Vertex
	{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
			glm::vec3 color = { 1, 1, 1 };

			bool operator==(const Vertex &other) const
			{
				return pos == other.pos && color == other.color
						&& uv == other.uv && normal == other.normal;
			}
	};

	struct Image
	{
			VkImage image = VK_NULL_HANDLE;
			VkDeviceMemory memory;
			VkImageView view = VK_NULL_HANDLE;
	};

	struct Texture: Image
	{
			VkSampler sampler = VK_NULL_HANDLE;
	};

	struct Mesh
	{
			std::string filepath = "";
			std::vector<Vertex> preVertexBuffer = { };
			std::vector<uint32_t> preIndexBuffer = { };
	};

	struct MaterialPropertie
	{
			vk3d::Texture diffuseTop, diffuseSide, diffuseFront;
			vk3d::Texture normalTop, normalSide, normalFront;
			vk3d::Texture specularTop, specularSide, specularFront;
			//glm::vec3 ambientLight = {1,1,1}, specularLight{1,1,1};
	};

	struct Material
	{
			VkDescriptorSet mDSet;
			MaterialPropertie textures;
			VkPipeline *pipeline;
			std::string name = "Default";
			std::vector<std::string> fileNames;
	};

	struct PushBlock
	{
			glm::mat4 model = glm::mat4(1);
			glm::vec3 booleans = glm::vec3(0);
	};

	struct Transform
	{
			PushBlock pushBlock = { };
			glm::vec3 position = { 0, 0, 0 };
			glm::vec3 rotation = { 0, 0, 0 };
			glm::vec3 scale = { 1, 1, 1 };
	};
}

static VkVertexInputBindingDescription getBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription = { };
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(vk3d::Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescription()
{
	std::array<VkVertexInputAttributeDescription, 4> attributeDescription = { };
	attributeDescription[0].binding = 0;
	attributeDescription[0].location = 0;
	attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[0].offset = offsetof(vk3d::Vertex, pos);

	attributeDescription[1].binding = 0;
	attributeDescription[1].location = 1;
	attributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[1].offset = offsetof(vk3d::Vertex, normal);

	attributeDescription[2].binding = 0;
	attributeDescription[2].location = 2;
	attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescription[2].offset = offsetof(vk3d::Vertex, uv);

	attributeDescription[3].binding = 0;
	attributeDescription[3].location = 3;
	attributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[3].offset = offsetof(vk3d::Vertex, color);
	return attributeDescription;
}

namespace std
{
	template<> struct hash<vk3d::Vertex>
	{
			size_t operator()(vk3d::Vertex const &vertex) const
			{
				size_t h1 = hash<glm::vec3>()(vertex.pos);
				size_t h2 = hash<glm::vec3>()(vertex.color);
				size_t h3 = hash<glm::vec2>()(vertex.uv);
				size_t h4 = hash<glm::vec3>()(vertex.normal);
				return ((((h1 ^ (h2 << 1)) >> 1) ^ h3) << 1) ^ h4;
			}
	};
}
