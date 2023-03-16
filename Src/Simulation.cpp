#include "Simulation.h"

#include <Althea/FrameContext.h>

using namespace Pies;
using namespace AltheaEngine;

namespace PiesForAlthea {
static bool releaseHinge = false;

/*static*/
void Simulation::buildPipelineLines(GraphicsPipelineBuilder& builder) {
  builder.setPrimitiveType(PrimitiveType::LINES)
      .addVertexInputBinding<Solver::Vertex>()
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Solver::Vertex, position))
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Solver::Vertex, baseColor))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, roughness))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, metallic))

      .addVertexShader(GProjectDirectory + "/Shaders/Lines.vert")
      .addFragmentShader(GProjectDirectory + "/Shaders/Lines.frag");
}

/*static*/
void Simulation::buildPipelineTriangles(GraphicsPipelineBuilder& builder) {
  builder.setPrimitiveType(PrimitiveType::TRIANGLES)
      .addVertexInputBinding<Solver::Vertex>()
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Solver::Vertex, position))
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Solver::Vertex, baseColor))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, roughness))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, metallic))
      .setCullMode(VK_CULL_MODE_NONE)

      .addVertexShader(GProjectDirectory + "/Shaders/Triangles.vert")
      .addFragmentShader(GProjectDirectory + "/Shaders/Triangles.frag");
}

Simulation::Simulation() {
  SolverOptions solverOptions{};
  this->_solver = Solver(solverOptions);
  this->_solver.createBox(glm::vec3(-10.0f, 5.0f, 0.0f), 0.5f, 1.0f);
}

void Simulation::initInputBindings(InputManager& inputManager) {
  inputManager.addKeyBinding({GLFW_KEY_B, GLFW_PRESS, 0}, [this]() {
    // this->_solver.createBox(glm::vec3(-10.0f, 5.0f, 0.0f), 0.5f, 0.85f);
    glm::vec3 cameraPos = glm::vec3(this->_cameraTransform[3]);
    glm::vec3 cameraForward = -glm::vec3(this->_cameraTransform[2]);
    this->_solver.createTetBox(cameraPos + 10.0f * cameraForward, 0.5f, 0.85f);
  });
}

void Simulation::tick(Application& app, float /*deltaTime*/) {
  if (releaseHinge) {
    releaseHinge = false;
    this->_solver.releaseHinge = true;
  }

  this->_solver.tick(0.05f);
}

void Simulation::preDraw(Application& app, VkCommandBuffer commandBuffer) {
  if (this->_solver.renderStateDirty) {
    this->_deferredDestroyRenderState(app);
    this->_createRenderState(app, commandBuffer);
    this->_solver.renderStateDirty = false;
  }

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

void Simulation::setCameraTransform(const glm::mat4& transform) {
  this->_cameraTransform = transform;
}

void Simulation::createRenderState(Application& app) {
  SingleTimeCommandBuffer commandBuffer(app);
  this->_createRenderState(app, commandBuffer);
  this->_solver.renderStateDirty = false;
}

void Simulation::destroyRenderState(Application& app) {
  this->_vertexBuffer = {};
  this->_linesIndexBuffer = {};
  this->_trianglesIndexBuffer = {};
}

void Simulation::_createRenderState(
    Application& app,
    VkCommandBuffer commandBuffer) {
  std::vector<uint32_t> lineIndices = this->_solver.getLines();
  BufferAllocation* pLinesStagingBuffer =
      new BufferAllocation(BufferUtilities::createStagingBuffer(
          app,
          commandBuffer,
          gsl::span<const std::byte>(
              reinterpret_cast<const std::byte*>(lineIndices.data()),
              lineIndices.size() * sizeof(uint32_t))));
  this->_linesIndexBuffer = IndexBuffer(
      app,
      commandBuffer,
      pLinesStagingBuffer->getBuffer(),
      0,
      lineIndices.size() * sizeof(uint32_t));

  std::vector<uint32_t> triIndices = this->_solver.getTriangles();
  BufferAllocation* pTriStagingBuffer =
      new BufferAllocation(BufferUtilities::createStagingBuffer(
          app,
          commandBuffer,
          gsl::span<const std::byte>(
              reinterpret_cast<const std::byte*>(triIndices.data()),
              triIndices.size() * sizeof(uint32_t))));
  this->_trianglesIndexBuffer = IndexBuffer(
      app,
      commandBuffer,
      pTriStagingBuffer->getBuffer(),
      0,
      triIndices.size() * sizeof(uint32_t));

  this->_vertexBuffer = DynamicVertexBuffer<Solver::Vertex>(
      app,
      commandBuffer,
      this->_solver.getVertices().size());

  // Delete staging buffers once the transfer is complete
  app.addDeletiontask(
      {[pLinesStagingBuffer, pTriStagingBuffer]() {
         delete pLinesStagingBuffer;
         delete pTriStagingBuffer;
       },
       app.getCurrentFrameRingBufferIndex()});
}

void Simulation::_deferredDestroyRenderState(Application& app) {
  // Move old resources to heap to prepare for deferred deletion
  IndexBuffer* pOldLinesIndexBuffer =
      new IndexBuffer(std::move(this->_linesIndexBuffer));
  this->_linesIndexBuffer = {};
  IndexBuffer* pOldTriIndexBuffer =
      new IndexBuffer(std::move(this->_trianglesIndexBuffer));
  this->_trianglesIndexBuffer = {};
  DynamicVertexBuffer<Solver::Vertex>* pOldVertexBuffer =
      new DynamicVertexBuffer<Solver::Vertex>(std::move(this->_vertexBuffer));
  this->_vertexBuffer = {};

  app.addDeletiontask(
      {[pOldLinesIndexBuffer, pOldTriIndexBuffer, pOldVertexBuffer]() {
         delete pOldLinesIndexBuffer;
         delete pOldTriIndexBuffer;
         delete pOldVertexBuffer;
       },
       app.getCurrentFrameRingBufferIndex()});
}
} // namespace PiesForAlthea