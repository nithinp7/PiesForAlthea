#include "Simulation.h"

#include <Althea/FrameContext.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace Pies;
using namespace AltheaEngine;

namespace PiesForAlthea {
/*static*/
void Simulation::buildPipelineLines(GraphicsPipelineBuilder& builder) {
  builder.setPrimitiveType(PrimitiveType::LINES)
      .addVertexInputBinding<Solver::Vertex>()
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Solver::Vertex, position))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, radius))
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
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, radius))
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Solver::Vertex, baseColor))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, roughness))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, metallic))

      // TODO: This is a hack to workaround incorrect winding of sphere
      // triangle indices - fix that instead of disabling backface culling
      .setCullMode(VK_CULL_MODE_NONE)

      .addVertexShader(GProjectDirectory + "/Shaders/Triangles.vert")
      .addFragmentShader(GProjectDirectory + "/Shaders/Triangles.frag");
}

/*static*/
void Simulation::buildPipelineNodes(GraphicsPipelineBuilder& builder) {
  builder
      .setPrimitiveType(PrimitiveType::TRIANGLES)

      // TODO: This is a hack to workaround incorrect winding of sphere
      // triangle indices - fix that instead of disabling backface culling
      .setCullMode(VK_CULL_MODE_NONE)

      // Instance buffer (Solver::Vertex, which corresponds to individual nodes)
      .addVertexInputBinding<Solver::Vertex>(VK_VERTEX_INPUT_RATE_INSTANCE)
      .addVertexAttribute(
          VertexAttributeType::VEC3,
          offsetof(Solver::Vertex, position))
      .addVertexAttribute(
          VertexAttributeType::FLOAT,
          offsetof(Solver::Vertex, radius))
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

      // Vertex buffer (sphere verts)
      .addVertexInputBinding<glm::vec3>(VK_VERTEX_INPUT_RATE_VERTEX)
      .addVertexAttribute(VertexAttributeType::VEC3, 0)

      .addVertexShader(GProjectDirectory + "/Shaders/Nodes.vert")

      // TODO: Maybe can re-use Triangles.frag here
      .addFragmentShader(GProjectDirectory + "/Shaders/Nodes.frag");
}

Simulation::Simulation() {
  SolverOptions solverOptions{};
  solverOptions.floorHeight = -8.0f;
  solverOptions.gridSpacing = 1.0f;

  this->_solver = Solver(solverOptions);
  this->_solver.createBox(glm::vec3(-10.0f, 5.0f, 0.0f), 1.0f, 1.0f);
}

void Simulation::initInputBindings(InputManager& inputManager) {
  inputManager.addKeyBinding({GLFW_KEY_C, GLFW_PRESS, 0}, [this]() {
    this->_solver.clear();
  });

  inputManager.addKeyBinding({GLFW_KEY_B, GLFW_PRESS, 0}, [this]() {
    glm::vec3 cameraPos = glm::vec3(this->_cameraTransform[3]);
    glm::vec3 cameraForward = -glm::vec3(this->_cameraTransform[2]);
    this->_solver.createTetBox(
        cameraPos + 10.0f * cameraForward,
        1.0f,
        10.0f * cameraForward,
        0.85f);
  });

  inputManager.addKeyBinding({GLFW_KEY_N, GLFW_PRESS, 0}, [this]() {
    glm::vec3 cameraPos = glm::vec3(this->_cameraTransform[3]);
    glm::vec3 cameraForward = -glm::vec3(this->_cameraTransform[2]);
    this->_solver.createSheet(cameraPos + 10.0f * cameraForward, 1.0f, 0.85f);
  });

  inputManager.addKeyBinding({GLFW_KEY_1, GLFW_PRESS, 0}, [this]() {
    this->_viewMode = ViewMode::TRIANGLES;
  });

  inputManager.addKeyBinding({GLFW_KEY_2, GLFW_PRESS, 0}, [this]() {
    this->_viewMode = ViewMode::NODES;
  });
}

void Simulation::tick(Application& app, float /*deltaTime*/) {
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
  if (this->_linesIndexBuffer.getIndexCount() == 0) {
    return;
  }

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
  if (this->_viewMode == ViewMode::TRIANGLES &&
      this->_trianglesIndexBuffer.getIndexCount() > 0) {
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

  context.bindDescriptorSets();
  context.bindVertexBuffer(this->_staticGeometry.vertexBuffer);
  context.draw(this->_staticGeometry.vertexBuffer.getVertexCount());
}

void Simulation::drawNodes(const DrawContext& context) const {
  if (this->_viewMode != ViewMode::NODES ||
      this->_vertexBuffer.getVertexCount() == 0) {
    return;
  }

  context.bindDescriptorSets();
  context.bindIndexBuffer(this->_sphere.indexBuffer);

  VkBuffer vertexBuffers[2];
  VkDeviceSize offsets[2];

  vertexBuffers[0] = this->_vertexBuffer.getBuffer();
  offsets[0] = this->_vertexBuffer.getCurrentBufferOffset(
      context.getFrame().frameRingBufferIndex);

  vertexBuffers[1] = this->_sphere.vertexBuffer.getAllocation().getBuffer();
  offsets[1] = 0;

  vkCmdBindVertexBuffers(
      context.getCommandBuffer(),
      0,
      2,
      vertexBuffers,
      offsets);
  vkCmdDrawIndexed(
      context.getCommandBuffer(),
      static_cast<uint32_t>(this->_sphere.indexBuffer.getIndexCount()),
      this->_vertexBuffer.getVertexCount(),
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
  this->_sphere = {};
  this->_staticGeometry = {};

  // Kinda hacky
  this->_solver.renderStateDirty = true;
}

void Simulation::_createRenderState(
    Application& app,
    VkCommandBuffer commandBuffer) {
  this->_sphere = Sphere(app, commandBuffer);
  this->_staticGeometry = StaticGeometry(app, commandBuffer);

  std::vector<uint32_t> lineIndices = this->_solver.getLines();
  if (!lineIndices.empty()) {
    this->_linesIndexBuffer =
        IndexBuffer(app, commandBuffer, std::move(lineIndices));
  }

  std::vector<uint32_t> triIndices = this->_solver.getTriangles();
  if (!triIndices.empty()) {
    this->_trianglesIndexBuffer =
        IndexBuffer(app, commandBuffer, std::move(triIndices));
  }

  if (!this->_solver.getVertices().empty()) {
    this->_vertexBuffer = DynamicVertexBuffer<Solver::Vertex>(
        app,
        commandBuffer,
        this->_solver.getVertices().size());
  }
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

  Sphere* pOldSphere = new Sphere(std::move(this->_sphere));
  this->_sphere = {};

  StaticGeometry* pOldStaticGeom =
      new StaticGeometry(std::move(this->_staticGeometry));
  this->_staticGeometry = {};

  app.addDeletiontask(
      {[pOldLinesIndexBuffer,
        pOldTriIndexBuffer,
        pOldVertexBuffer,
        pOldSphere,
        pOldStaticGeom]() {
         delete pOldLinesIndexBuffer;
         delete pOldTriIndexBuffer;
         delete pOldVertexBuffer;
         delete pOldSphere;
         delete pOldStaticGeom;
       },
       app.getCurrentFrameRingBufferIndex()});
}

Simulation::Sphere::Sphere(Application& app, VkCommandBuffer commandBuffer) {
  constexpr uint32_t resolution = 50;
  constexpr float maxPitch = 0.499f * glm::pi<float>();

  auto sphereUvIndexToVertIndex = [resolution](uint32_t i, uint32_t j) {
    i = i % resolution;
    return i * resolution / 2 + j;
  };

  std::vector<glm::vec3> vertices;
  std::vector<uint32_t> indices;

  // Verts from the cylinder mapping
  uint32_t cylinderVertsCount = resolution * resolution / 2;
  // Will include cylinder mapped verts and 2 cap verts
  vertices.reserve(cylinderVertsCount + 2);
  indices.reserve(3 * resolution * resolution);
  for (uint32_t i = 0; i < resolution; ++i) {
    float theta = i * 2.0f * glm::pi<float>() / resolution;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);

    for (uint32_t j = 0; j < resolution / 2; ++j) {
      float phi = j * 2.0f * maxPitch / (resolution / 2) - maxPitch;
      float cosPhi = cos(phi);
      float sinPhi = sin(phi);

      vertices.emplace_back(cosPhi * cosTheta, sinPhi, -cosPhi * sinTheta);

      if (j < resolution / 2 - 1) {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));

        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j + 1));
        indices.push_back(sphereUvIndexToVertIndex(i, j + 1));
      } else {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
        indices.push_back(cylinderVertsCount);
      }

      if (j == 0) {
        indices.push_back(sphereUvIndexToVertIndex(i, j));
        indices.push_back(cylinderVertsCount + 1);
        indices.push_back(sphereUvIndexToVertIndex(i + 1, j));
      }
    }
  }

  // Cap vertices
  vertices.emplace_back(0.0f, 1.0f, 0.0f);
  vertices.emplace_back(0.0f, -1.0f, 0.0f);

  this->indexBuffer = IndexBuffer(app, commandBuffer, std::move(indices));
  this->vertexBuffer =
      VertexBuffer<glm::vec3>(app, commandBuffer, std::move(vertices));
}

Simulation::StaticGeometry::StaticGeometry(
    Application& app,
    VkCommandBuffer commandBuffer) {
  std::vector<Solver::Vertex> vertices;
  vertices.resize(6);

  float height = -8.0f;
  float halfWidth = 200.0f;

  vertices[0].position = glm::vec3(-halfWidth, height, -halfWidth);
  vertices[1].position = glm::vec3(halfWidth, height, -halfWidth);
  vertices[2].position = glm::vec3(halfWidth, height, halfWidth);

  vertices[3].position = glm::vec3(-halfWidth, height, -halfWidth);
  vertices[4].position = glm::vec3(halfWidth, height, halfWidth);
  vertices[5].position = glm::vec3(-halfWidth, height, halfWidth);

  for (uint32_t i = 0; i < vertices.size(); ++i) {
    vertices[i].baseColor = glm::vec3(1.0f, 0.0f, 0.0f);
    vertices[i].metallic = 0.0f;
    vertices[i].roughness = 0.25f;
  }

  this->vertexBuffer =
      VertexBuffer<Solver::Vertex>(app, commandBuffer, std::move(vertices));
}
} // namespace PiesForAlthea