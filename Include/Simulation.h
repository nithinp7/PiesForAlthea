#pragma once

#include <Pies/Solver.h>

#include <Althea/Application.h>
#include <Althea/DrawContext.h>
#include <Althea/DynamicVertexBuffer.h>
#include <Althea/GraphicsPipeline.h>
#include <Althea/IndexBuffer.h>
#include <Althea/SingleTimeCommandBuffer.h>
#include <Althea/InputManager.h>

#include <vector>

using namespace Pies;
using namespace AltheaEngine;

namespace PiesForAlthea {
class Simulation {
public:
  static void initInputBindings(InputManager& inputManager);
  static void buildPipeline(GraphicsPipelineBuilder& builder);

  Simulation(
      const Application& app,
      SingleTimeCommandBuffer& commandBuffer);

  void tick(
      const Application& app,
      float deltaTime);
  void draw(const DrawContext& context) const;

private:
  Solver _solver;
  DynamicVertexBuffer<glm::vec3> _vertexBuffer;
  IndexBuffer _indexBuffer;
};
} // namespace PiesForAlthea