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
void Simulation::buildPipeline(GraphicsPipelineBuilder& builder) {
  builder.setPrimitiveType(PrimitiveType::LINES)
      .addVertexInputBinding<glm::vec3>()
      .addVertexAttribute(VertexAttributeType::VEC3, 0)

      .addVertexShader(GProjectDirectory + "/Shaders/Lines.vert")
      .addFragmentShader(GProjectDirectory + "/Shaders/Lines.frag");
}

Simulation::Simulation(
    const Application& app,
    SingleTimeCommandBuffer& commandBuffer)
    : _solver() {

  std::vector<uint32_t> indices = this->_solver.getDistanceConstraintLines();
  this->_indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  this->_vertexBuffer = DynamicVertexBuffer<glm::vec3>(
      app,
      commandBuffer,
      this->_solver.getVertices().size());
}

void Simulation::tick(const Application& app, float /*deltaTime*/) {
  this->_solver.tick(0.05f);

  this->_vertexBuffer.updateVertices(
      app.getCurrentFrameRingBufferIndex(),
      this->_solver.getVertices());
}

void Simulation::draw(const DrawContext& context) const {
  context.bindDescriptorSets();
  context.bindIndexBuffer(this->_indexBuffer);
  this->_vertexBuffer.bind(
      context.getFrame().frameRingBufferIndex,
      context.getCommandBuffer());
  vkCmdDrawIndexed(
      context.getCommandBuffer(),
      static_cast<uint32_t>(this->_indexBuffer.getIndexCount()),
      1,
      0,
      0,
      0);
}
} // namespace PiesForAlthea