#pragma once

#include <memory>
#include <vector>
#include <ranges>

#include "scene_object.hpp"
#include "cube.hpp"

#include "material_factory.hpp"
#include "geometry_factory.hpp"
#include "simple_scene.hpp"

inline SimpleScene createCottageScene(MaterialFactory& aMaterialFactory, GeometryFactory& aGeometryFactory) {
	SimpleScene scene;
	{
		auto ground = std::make_shared<LoadedMeshObject>("./data/geometry/ground.obj");
		ground->addMaterial(
			"solid",
			MaterialParameters(
				"material_deffered",
				RenderStyle::Solid,
				{
					{ "u_diffuseTexture", TextureInfo("cottage/groundDif.png") },
				}
				)
		);
		ground->prepareRenderData(aMaterialFactory, aGeometryFactory);
		scene.addObject(ground);
	}
	for(int i = 0; i < 10; ++i) {
		auto oak = std::make_shared<LoadedMeshObject>("./data/geometry/oak.obj");
		oak->addMaterial(
			"solid",
			MaterialParameters(
				"material_deffered",
				RenderStyle::Solid,
				{
					{ "u_diffuseTexture", TextureInfo("cottage/OakDif.png") },
				}
				)
		);
		oak->prepareRenderData(aMaterialFactory, aGeometryFactory);

		oak->setPosition(glm::vec3{-5,0,i * 10 - 40});
		scene.addObject(oak);

		auto oak2 = std::make_shared<LoadedMeshObject>("./data/geometry/oak.obj");
		oak2->addMaterial(
			"solid",
			MaterialParameters(
				"material_deffered",
				RenderStyle::Solid,
				{
					{ "u_diffuseTexture", TextureInfo("cottage/OakDif.png") },
				}
				)
		);
		oak2->prepareRenderData(aMaterialFactory, aGeometryFactory);

		oak2->setPosition(glm::vec3{-15,0,i * 10 - 40});
		scene.addObject(oak2);
	}


	return scene;
}
