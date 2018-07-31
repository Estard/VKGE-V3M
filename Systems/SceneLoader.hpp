#pragma once
#include "../Definitions.hpp"
#include "../Entities/Entities.hpp"

IEntity createCube()
{
	IEntity cube = { };
	cube.mesh.preIndexBuffer =
	{	0,3,1,1,3,5, 1,5,4,4,5,7, 0,1,2,2,1,4, 6,2,4,4,7,6, 0,2,3,3,2,6, 3,6,5,5,6,7};

	cube.mesh.preVertexBuffer =
	{
		{
			{	0,0,0},
			{	-0.5,-0.5,0.5},
			{	0,0}},
		{
			{	0,0,1},
			{	-0.5,-0.5,0.5},
			{	1,1}},
		{
			{	0,1,0},
			{	-0.5,0.5,0.5},
			{	0,1}},
		{
			{	1,0,0},
			{	0.5,-0.5,0.5},
			{	1,0}},
		{
			{	0,1,1},
			{	-0.5,0.5,0.5},
			{	1,2}},
		{
			{	1,0,1},
			{	0.5,-0.5,0.5},
			{	2,1}},
		{
			{	1,1,0},
			{	0.5,0.5,0.5},
			{	1,1}},
		{
			{	1,1,1},
			{	0.5,0.5,0.5},
			{	2,2}}
	};

	return cube;
}

IEntity makeSimplexNoiseTerrain()
{
	IEntity terrain = { };

	init();

	double uvScale = 1.0;
	double xyScale = 80.0;
	double zScale = 6.0;
	int width = 768;
	int height = 768;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{

			double n = (octaveNoise((x / (((double) width) / 2.0)) + 0.5,
					y / (((double) height) / 2.0), 8, 0.5));

			n = (octaveNoise(n, n, 4, 0.25));

			vk3d::Vertex v = { { (x / ((float) width / xyScale)) - 1.f, (y
					/ ((float) height / xyScale)) - 1.f, n * zScale },
					{ 0, 0, 0 }, { uvScale * x / float(width), uvScale * y
							/ float(height) } };
			terrain.mesh.preVertexBuffer.push_back(v);
		}
	}

	for (int y = 0; y < height - 1; y++)
	{
		for (int x = 0; x < width - 1; x++)
		{
			terrain.mesh.preIndexBuffer.push_back((x + y * width));
			terrain.mesh.preIndexBuffer.push_back((x + 1 + y * width));
			terrain.mesh.preIndexBuffer.push_back((x + (y + 1) * width));

			terrain.mesh.preIndexBuffer.push_back((x + 1 + y * width));
			terrain.mesh.preIndexBuffer.push_back((x + 1 + (y + 1) * width));
			terrain.mesh.preIndexBuffer.push_back((x + (y + 1) * width));
		}
	}

	for (unsigned int i = 0; i < terrain.mesh.preIndexBuffer.size(); i += 3)
	{
		glm::vec3 A =
				terrain.mesh.preVertexBuffer[terrain.mesh.preIndexBuffer[i]].pos;
		glm::vec3 B = terrain.mesh.preVertexBuffer[terrain.mesh.preIndexBuffer[i
				+ 1]].pos;
		glm::vec3 C = terrain.mesh.preVertexBuffer[terrain.mesh.preIndexBuffer[i
				+ 2]].pos;

		glm::vec3 faceNormal = glm::cross((B - A), (C - A));
		faceNormal.x *= -1.0;

		terrain.mesh.preVertexBuffer[terrain.mesh.preIndexBuffer[i]].normal +=
				faceNormal;
		terrain.mesh.preVertexBuffer[terrain.mesh.preIndexBuffer[i + 1]].normal +=
				faceNormal;
		terrain.mesh.preVertexBuffer[terrain.mesh.preIndexBuffer[i + 2]].normal +=
				faceNormal;
	}

	return terrain;
}

IEntity loadFromFile(std::string filename)
{
	IEntity model = { };

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
			("Assets/models/" + filename).c_str()))
		throw std::runtime_error(err);

	std::unordered_map<vk3d::Vertex, uint32_t> uniqueVertices = { };

	for (const auto &shape : shapes)
	{
		for (const auto &index : shape.mesh.indices)
		{
			vk3d::Vertex vertex = { };

			vertex.pos =
			{
				attrib.vertices[3* index.vertex_index +0],
				attrib.vertices[3* index.vertex_index +1],
				attrib.vertices[3* index.vertex_index +2]
			};

			if (filename.compare("cockpit_o_low.obj") == 0)
				vertex.pos += glm::vec3(0, -17.0, -24.0);

			if (filename.compare("gatling_o_top.obj") == 0)
				vertex.pos += glm::vec3(-25, 0, -10.0);

			if (filename.compare("plasma_o_top.obj") == 0)
				vertex.pos += glm::vec3(25, 0, -28.0);

			vertex.uv =
			{
				attrib.texcoords[2 *index.texcoord_index + 0],
				1.0f- attrib.texcoords[2 *index.texcoord_index +1]
			};

			vertex.color =
			{	1.0f,1.0f,1.0f};

			if (!attrib.normals.empty())
			{
				vertex.normal =
				{
					attrib.normals[3*index.normal_index +0],
					attrib.normals[3*index.vertex_index +1],
					attrib.normals[3*index.vertex_index +2]
				};
			}
			else
			{
				vertex.normal = glm::vec3(0,0,0);
			}
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] =
						static_cast<uint32_t>(model.mesh.preVertexBuffer.size());
				model.mesh.preVertexBuffer.push_back(vertex);
			}

			model.mesh.preIndexBuffer.push_back(uniqueVertices[vertex]);
		}

		if (attrib.normals.empty())
		{
			for (unsigned int i = 0; i < model.mesh.preIndexBuffer.size(); i +=
					3)
			{
				glm::vec3 A =
						model.mesh.preVertexBuffer[model.mesh.preIndexBuffer[i]].pos;
				glm::vec3 B =
						model.mesh.preVertexBuffer[model.mesh.preIndexBuffer[i
								+ 1]].pos;
				glm::vec3 C =
						model.mesh.preVertexBuffer[model.mesh.preIndexBuffer[i
								+ 2]].pos;

				glm::vec3 faceNormal = glm::cross((B - A), (C - A));
				faceNormal.x *= -1.0;

				model.mesh.preVertexBuffer[model.mesh.preIndexBuffer[i]].normal +=
						faceNormal;
				model.mesh.preVertexBuffer[model.mesh.preIndexBuffer[i + 1]].normal +=
						faceNormal;
				model.mesh.preVertexBuffer[model.mesh.preIndexBuffer[i + 2]].normal +=
						faceNormal;
			}
		}
	}

	return model;
}
;
void loadStandardScene(std::vector<IEntity> *prefabs,
		std::vector<SceneEntity> *scene, std::vector<vk3d::Material> *materials)
{
	scene->clear();
	materials->clear();
	prefabs->clear();

	vk3d::Material terrainMa;
	terrainMa.fileNames =
	{	"MultiTexture.png"};
	terrainMa.name = "Terrain";
	materials->push_back(terrainMa);

	vk3d::Material cubeMa;
	cubeMa.fileNames =
	{	"sandy_plains_area01_tex001.png"};
	cubeMa.name = "Cube";
	materials->push_back(cubeMa);

	vk3d::Material chaletMa;
	chaletMa.fileNames =
	{	"chalet.jpg"};
	chaletMa.name = "Chalet";
	materials->push_back(chaletMa);

	vk3d::Material cockpitMa;
	cockpitMa.fileNames =
	{	"cockpit_m_diffuse.png","cockpit_m_normal.png","cockpit_m_specular.png"};
	cockpitMa.name = "Cockpit";
	materials->push_back(cockpitMa);

	vk3d::Material manSpaceshipMa;
	manSpaceshipMa.fileNames =
	{	"man_spaceship_diffuse.png","man_spaceship_normal.png","man_spaceship_specular.png"};
	manSpaceshipMa.name = "ManSpaceship";
	materials->push_back(manSpaceshipMa);

	vk3d::Material cannonLeftMa;
	cannonLeftMa.fileNames =
	{	"gatling_m_diffuse.png","gatling_m_normal.png","gatling_m_specular.png"};
	cannonLeftMa.name = "CannonLeft";
	materials->push_back(cannonLeftMa);

	vk3d::Material cannonRightMa;
	cannonRightMa.fileNames =
	{	"plasma_m_diffuse.png","plasma_m_normal.png","plasma_m_specular.png"};
	cannonRightMa.name = "CannonRight";
	materials->push_back(cannonRightMa);

	vk3d::Material impulseAmmoMa;
	impulseAmmoMa.fileNames =
	{	"rot.png"};
	impulseAmmoMa.name = "Ammo";
	materials->push_back(impulseAmmoMa);

	prefabs->push_back(makeSimplexNoiseTerrain());
	prefabs->push_back(createCube());
	prefabs->push_back(loadFromFile("chalet.obj"));
	prefabs->push_back(loadFromFile("cockpit_o_low.obj"));
	prefabs->push_back(loadFromFile("man_spaceship.obj"));
	prefabs->push_back(loadFromFile("gatling_o_top.obj"));
	prefabs->push_back(loadFromFile("plasma_o_top.obj"));
	prefabs->push_back(loadFromFile("sphere.obj"));

	SceneEntity terry = { };
	terry.entity = &prefabs->front();
	terry.transform.pushBlock.booleans = glm::vec3(1, 0, 0);
	terry.material = &materials->at(0);
	scene->push_back(terry);

	SceneEntity cube1 = { };
	cube1.entity = &prefabs->at(1);
	cube1.material = &materials->at(1);
	scene->push_back(cube1);

	SceneEntity cube2 = { };
	cube2.entity = &prefabs->at(1);
	cube2.transform.position = glm::vec3(2, 2, 2);
	cube2.material = &materials->at(1);
	scene->push_back(cube2);

	/*SceneEntity chalet = { };
	 chalet.entity = &prefabs->at(2);
	 chalet.transform.position = glm::vec3(5.5, 5.5, 0.5);
	 chalet.transform.rotation = glm::vec3(0, 0, -90);
	 chalet.transform.scale = glm::vec3(1, 1, 1);
	 chalet.material = &materials->at(2);
	 scene->push_back(chalet);
	 */

	SceneEntity cockpit = { };
	cockpit.entity = &prefabs->at(3);
	cockpit.transform.position = glm::vec3(3, 3, 3);
	cockpit.transform.scale = glm::vec3(0.025, 0.025, 0.025);
	cockpit.transform.rotation = glm::vec3(pi / 2.f, 0, 0);
	cockpit.material = &materials->at(3);
	scene->push_back(cockpit);

	prefabs->at(4).SystemFlags = VGE_SYSTEM_ENEMY | VGE_SYSTEM_AI
			| VGE_SYSTEM_PHYSIK;

	EnemyEntity manShip = { };
	manShip.entity = &prefabs->at(4);
	manShip.transform.position = glm::vec3(5, 5, 10);
	manShip.transform.scale = glm::vec3(0.0125, 0.0125, 0.0125);
	manShip.material = &materials->at(4);
	manShip.extent = 1.f;
	scene->push_back(manShip);
	for (int i = 0; i < 47; i++)
	{
		double x = octaveNoise(i, 1, 1, 1) + 1.0;
		double y = octaveNoise(1, i, 1, 1) + 1.0;
		manShip.transform.position = glm::vec3(x * 50, y * 50, 70);
		manShip.speed += (x + y - 2.0) / 4.0;
		scene->push_back(manShip);
	}

	SceneEntity cannonLeft = { };
	cannonLeft.entity = &prefabs->at(5);
	cannonLeft.material = &materials->at(5);
//	scene->push_back(cannonLeft);

	SceneEntity cannonRight = { };
	cannonRight.entity = &prefabs->at(6);
	cannonRight.material = &materials->at(6);
	scene->push_back(cannonRight);

	SceneEntity sphereAmmo = { };
	sphereAmmo.entity = &prefabs->at(7);
	sphereAmmo.material = &materials->at(7);
	sphereAmmo.transform.scale = glm::vec3(0.25);
	sphereAmmo.entity->SystemFlags = VGE_SYSTEM_PHYSIK | VGE_SYSTEM_CANNON;
	sphereAmmo.extent = 0.75f;
	for (uint32_t i = 0; i < 10; i++)
		scene->push_back(sphereAmmo);

}
