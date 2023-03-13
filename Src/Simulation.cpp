#include "Simulation.h"

#include <Althea/FrameContext.h>

using namespace Pies;
using namespace AltheaEngine;

namespace PiesForAlthea {
static bool releaseHinge = false;

/*static*/
void Simulation::initInputBindings(InputManager& inputManager) {
  inputManager.addKeyBinding({GLFW_KEY_B, GLFW_PRESS, 0}, []() {
    releaseHinge = true;
  });
}

/*static*/
void Simulation::buildPipelineLines(GraphicsPipelineBuilder& builder) {
  builder.setPrimitiveType(PrimitiveType::LINES)
      .addVertexInputBinding<glm::vec3>()
      .addVertexAttribute(VertexAttributeType::VEC3, 0)

      .addVertexShader(GProjectDirectory + "/Shaders/Lines.vert")
      .addFragmentShader(GProjectDirectory + "/Shaders/Lines.frag");
}

/*static*/
void Simulation::buildPipelineTriangles(GraphicsPipelineBuilder& builder) {
  builder.setPrimitiveType(PrimitiveType::TRIANGLES)
      .addVertexInputBinding<glm::vec3>()
      .addVertexAttribute(VertexAttributeType::VEC3, 0)
      .setCullMode(VK_CULL_MODE_NONE)

      .addVertexShader(GProjectDirectory + "/Shaders/Triangles.vert")
      .addFragmentShader(GProjectDirectory + "/Shaders/Triangles.frag");
}

Simulation::Simulation(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer)
    : _solver() {

  std::vector<uint32_t> indices = this->_solver.getDistanceConstraintLines();
  this->_linesIndexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));

  std::vector<uint32_t> triIndices = this->_solver.getTriangles();
  this->_trianglesIndexBuffer =
      IndexBuffer(app, commandBuffer, std::move(triIndices));
  this->_vertexBuffer = DynamicVertexBuffer<glm::vec3>(
      app,
      commandBuffer,
      this->_solver.getVertices().size());
}

void Simulation::tick(const Application& app, float /*deltaTime*/) {
  if (releaseHinge) {
    releaseHinge = false;
    this->_solver.releaseHinge = true;
  }

  this->_solver.tick(0.05f);

  this->_vertexBuffer.updateVertices(
      app.getCurrentFrameRingBufferIndex(),
      this->_solver.getVertices());
}

void Simulation::drawLines(const DrawContext& context) const {
  context.bindDescriptorSets();
  context.bindIndexBuffer(this->_linesIndexBuffer);
  this->_vertexBuffer.bind(
      context.getFrame().frameRingBufferIndex,
      context.getCommandBuffer());
  vkCmdDrawIndexed(
      context.getCommandBuffer(),
      static_cast<uint32_t>(this->_linesIndexBuffer.getIndexCount()),
      1,
      0,
      0,
      0);
}

void Simulation::drawTriangles(const DrawContext& context) const {
  context.bindDescriptorSets();
  context.bindIndexBuffer(this->_trianglesIndexBuffer);
  this->_vertexBuffer.bind(
      context.getFrame().frameRingBufferIndex,
      context.getCommandBuffer());
  vkCmdDrawIndexed(
      context.getCommandBuffer(),
      static_cast<uint32_t>(this->_trianglesIndexBuffer.getIndexCount()),
      1,
      0,
      0,
      0);
}
} // namespace PiesForAlthea