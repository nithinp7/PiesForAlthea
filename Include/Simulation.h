#pragma once

#include <Althea/Application.h>
#include <Althea/DrawContext.h>
#include <Althea/DynamicVertexBuffer.h>
#include <Althea/GraphicsPipeline.h>
#include <Althea/IndexBuffer.h>
#include <Althea/InputManager.h>
#include <Althea/SingleTimeCommandBuffer.h>
#include <Pies/Solver.h>
#include <vulkan/vulkan.h>

#include <vector>
#include <cstdint>

using namespace Pies;
using namespace AltheaEngine;

namespace PiesForAlthea {
class Simulation {
public:
  static void buildPipelineLines(GraphicsPipelineBuilder& builder);
  static void buildPipelineTriangles(GraphicsPipelineBuilder& builder);
  static void buildPipelineNodes(GraphicsPipelineBuilder& builder);

  Simulation();

  void initInputBindings(InputManager& inputManager);
  void tick(Application& app, float deltaTime);
  void preDraw(Application& app, VkCommandBuffer commandBuffer);
  void drawLines(const DrawContext& context) const;
  void drawTriangles(const DrawContext& context) const;
  void drawNodes(const DrawContext& context) const;

  void createRenderState(Application& app);
  void destroyRenderState(Application& app);

  void setCameraTransform(const glm::mat4& transform);

private:
  void _createRenderState(Application& app, VkCommandBuffer commandBuffer);
  void _deferredDestroyRenderState(Application& app);

  enum class ViewMode {
    TRIANGLES,
    NODES,
    LINES
  };

  ViewMode _viewMode = ViewMode::TRIANGLES;

  glm::mat4 _cameraTransform;

  Solver _solver;
  DynamicVertexBuffer<Solver::Vertex> _vertexBuffer;
  IndexBuffer _linesIndexBuffer;
  IndexBuffer _trianglesIndexBuffer;
  
  // For visualizing nodes with instancing
  struct Sphere {
    VertexBuffer<glm::vec3> vertexBuffer;
    IndexBuffer indexBuffer;

    Sphere() = default;
    Sphere(Application& app, VkCommandBuffer commandBuffer);
  };
  Sphere _sphere{};

  struct StaticGeometry {
    VertexBuffer<Solver::Vertex> vertexBuffer;

    StaticGeometry() = default;
    StaticGeometry(Application& app, VkCommandBuffer commandBuffer);
  };
  StaticGeometry _staticGeometry{};
};
} // namespace PiesForAlthea