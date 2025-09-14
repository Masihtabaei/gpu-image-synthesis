#include "Scene.hpp"
#include <d3dx12/d3dx12.h>
#include <unordered_map>

using namespace gims;

namespace
{
void addToCommandListImpl(Scene& scene, ui32 nodeIdx, f32m4 transformation,
                          const ComPtr<ID3D12GraphicsCommandList6>& commandList, ui32 modelViewRootParameterIdx,
                          ui32 materialConstantsRootParameterIdx, ui32 srvRootParameterIdx, ui32 pipelineState)
{

  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  Scene::Node currentNode               = scene.getNode(nodeIdx);
  f32m4       accumulatedTransformation =
      transformation * currentNode.transformation;

  for (ui32 i = 0; i < currentNode.meshIndices.size(); i++)
  {
    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 16, &accumulatedTransformation, 0);
    commandList->SetGraphicsRootConstantBufferView(
        materialConstantsRootParameterIdx,
        scene.getMaterial(scene.getMesh(currentNode.meshIndices[i]).getMaterialIndex())
                                 .materialConstantBuffer.getResource()
                                 ->GetGPUVirtualAddress());

    commandList->SetDescriptorHeaps(1, scene.getMaterial(scene.getMesh(currentNode.meshIndices[i]).getMaterialIndex()).srvDescriptorHeap.GetAddressOf());
    commandList->SetGraphicsRootDescriptorTable(
        srvRootParameterIdx, scene.getMaterial(scene.getMesh(currentNode.meshIndices[i]).getMaterialIndex())
               .srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    // Checking whether bounding boxes should be rendered
    if (pipelineState == 1)
    {
      const AABBPoints meshAABBPoints = scene.m_sceneCalculatedAABBPoints.at(currentNode.meshIndices[i]);
      commandList->SetGraphicsRoot32BitConstants(4, 32, &meshAABBPoints, 0);
    }

    scene.getMesh(currentNode.meshIndices[i]).addToCommandList(commandList, pipelineState);
  }

  for (const ui32& nodeIndex : currentNode.childIndices)
  {
    addToCommandListImpl(scene, nodeIndex, accumulatedTransformation, commandList, modelViewRootParameterIdx,
                         materialConstantsRootParameterIdx, srvRootParameterIdx, pipelineState);
  }

  // Assignemt 6
  // Assignment 10
}
} // namespace

namespace gims
{


const Scene::Node& Scene::getNode(ui32 nodeIdx) const
{
  return m_nodes[nodeIdx];
}

Scene::Node& Scene::getNode(ui32 nodeIdx)
{
  return m_nodes[nodeIdx];
}

const ui32 Scene::getNumberOfNodes() const
{
  return static_cast<ui32>(m_nodes.size());
}

const ui32 Scene::getNumberOfMeshesAvailable()
{
  return static_cast<ui32>(m_meshes.size());
}

const ui32 Scene::getNumberOfMaterialsAvailable()
{
  return static_cast<ui32>(m_materials.size());
}

const ui32 Scene::getNumberOfTexturesAvailable()
{
  return static_cast<ui32>(m_textures.size());
}

const TriangleMeshD3D12& Scene::getMesh(ui32 meshIdx) const
  {
  return m_meshes[meshIdx];
}

const Scene::Material& Scene::getMaterial(ui32 materialIdx) const
{
  return m_materials[materialIdx];
}

const AABB& Scene::getAABB() const
{
  return m_aabb;
}

void Scene::addToCommandList(const ComPtr<ID3D12GraphicsCommandList6>& commandList, const f32m4 transformation,
                             ui32 modelViewRootParameterIdx, ui32 materialConstantsRootParameterIdx,
                             ui32 srvRootParameterIdx, ui32 pipelineState)
{
  addToCommandListImpl(*this, 0, transformation, commandList, modelViewRootParameterIdx,
                       materialConstantsRootParameterIdx, srvRootParameterIdx, pipelineState);
}
} // namespace gims
