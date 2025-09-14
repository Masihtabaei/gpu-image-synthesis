#pragma once
#include "Scene.hpp"
#include <filesystem>
#include <unordered_map>





namespace gims
{




class SceneGraphFactory
{
public:


  static Scene createFromAssImpScene(const std::filesystem::path pathToScene,
                                     const ComPtr<ID3D12GraphicsCommandList6> commandList,
                                     const ComPtr<ID3D12Device2>&             device,
                                     const ComPtr<ID3D12CommandQueue>&        commandQueue,
                                     ComPtr<ID3D12Resource>& outputOBBReadBack, ComPtr<ID3D12Resource>& inputAABB,
                                     ComPtr<ID3D12Resource>& outputOBB);
  static void  createSceneAABBs(Scene& scene, ComPtr<ID3D12Resource>& outputOBBReadBack);

private:

  static void createMeshes(aiScene const* const inputScene, const ComPtr<ID3D12GraphicsCommandList6> commandList,
                           const ComPtr<ID3D12Device2>& device, const ComPtr<ID3D12CommandQueue>& commandQueue,
                           ComPtr<ID3D12Resource>& outputOBBReadBack, ComPtr<ID3D12Resource>& inputAABB,
                           ComPtr<ID3D12Resource>& outputOBB, Scene& outputScene);


  static ui32 createNodes(aiScene const* const inputScene, Scene& outputScene, aiNode const* const inputNode);

  static void computeSceneAABB(Scene& scene, AABB& aabb, ui32 nodeIdx, f32m4 transformation);

  static void createTextures(const std::unordered_map<std::filesystem::path, ui32>& textureFileNameToTextureIndex,
                             std::filesystem::path parentPath, const ComPtr<ID3D12Device2>& device,
                             const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene);

  static void createMaterials(aiScene const* const                            inputScene,
                              std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex,
                              const ComPtr<ID3D12Device2>& device, Scene& outputScene);


};
} // namespace gims
