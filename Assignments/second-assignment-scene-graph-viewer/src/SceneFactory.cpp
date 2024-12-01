#include "SceneFactory.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <iostream>
using namespace gims;

namespace
{
/// <summary>
/// Converts the index buffer required for D3D12 rendering from an aiMesh.
/// </summary>
/// <param name="mesh">The ai mesh containing an index buffer.</param>
/// <returns></returns>
std::vector<ui32v3> getTriangleIndicesFromAiMesh(aiMesh const* const mesh)
{
  std::cout << "An attempt was made to extract the indices!" << std::endl;
  const auto          numberOfFaces = mesh->mNumFaces;
  const auto         faces    = mesh->mFaces;
  std::vector<ui32v3> indicesGroupedInFaces;
  indicesGroupedInFaces.resize(numberOfFaces);

  for (ui32 i = 0; i < numberOfFaces; i++)
  {
    indicesGroupedInFaces.at(i) = (ui32v3(faces[i].mIndices[0], faces[i].mIndices[1], faces[i].mIndices[2]));
  }

  // Assignment 3
  return indicesGroupedInFaces;
}

void addTextureToDescriptorHeap(const ComPtr<ID3D12Device>& device, aiTextureType aiTextureTypeValue,
                                i32 offsetInDescriptors, aiMaterial const* const inputMaterial,
                                const std::vector<Texture2DD3D12>& m_textures, Scene::Material& material,
                                std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex,
                                ui32                                            defaultTextureIndex)
{
  if (inputMaterial->GetTextureCount(aiTextureTypeValue) == 0)
  {
    m_textures[defaultTextureIndex].addToDescriptorHeap(device, material.srvDescriptorHeap, offsetInDescriptors);
  }
  else
  {
    aiString path;
    inputMaterial->GetTexture((aiTextureType)aiTextureTypeValue, 0, &path);
    m_textures[textureFileNameToTextureIndex[path.C_Str()]].addToDescriptorHeap(device, material.srvDescriptorHeap,
                                                                                offsetInDescriptors);
  }
}

std::unordered_map<std::filesystem::path, ui32> textureFilenameToIndex(aiScene const* const inputScene)
{
  std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex;

  ui32 textureIdx = 3;
  for (ui32 mIdx = 0; mIdx < inputScene->mNumMaterials; mIdx++)
  {
    for (ui32 textureType = aiTextureType_NONE; textureType < aiTextureType_UNKNOWN; textureType++)
    {
      for (ui32 i = 0; i < inputScene->mMaterials[mIdx]->GetTextureCount((aiTextureType)textureType); i++)
      {
        aiString path;
        inputScene->mMaterials[mIdx]->GetTexture((aiTextureType)textureType, i, &path);

        const auto texturePathCstr = path.C_Str();
        const auto textureIter     = textureFileNameToTextureIndex.find(texturePathCstr);
        if (textureIter == textureFileNameToTextureIndex.end())
        {
          textureFileNameToTextureIndex.emplace(texturePathCstr, static_cast<ui32>(textureIdx));
          textureIdx++;
        }
      }
    }
  }
  return textureFileNameToTextureIndex;
}
/// <summary>
/// Reads the color from the Asset Importer specific (pKey, type, idx) triple.
/// Use the Asset Importer Macros AI_MATKEY_COLOR_AMBIENT, AI_MATKEY_COLOR_DIFFUSE, etc. which map to these arguments
/// correctly.
///
/// If that key does not exist a null vector is returned.
/// </summary>
/// <param name="pKey">Asset importer specific parameter</param>
/// <param name="type"></param>
/// <param name="idx"></param>
/// <param name="material">The material from which we wish to extract the color.</param>
/// <returns>Color or 0 vector if no color exists.</returns>
f32v4 getColor(char const* const pKey, unsigned int type, unsigned int idx, aiMaterial const* const material)
{
  aiColor3D color;
  if (material->Get(pKey, type, idx, color) == aiReturn_SUCCESS)
  {
    return f32v4(color.r, color.g, color.b, 0.0f);
  }
  else
  {
    return f32v4(0.0f);
  }
}


i32 getTexture(aiTextureType textureType, unsigned int textureIndex, aiMaterial const* const material,
              std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex)
{
  i32 defaultTextureIndexToReturn = 0;
  aiString textureName("");
  aiReturn textureRetrievingResult = material->GetTexture(textureType, textureIndex, &textureName);
  if (textureRetrievingResult == aiReturn_FAILURE)
  {
    if (textureType == aiTextureType_AMBIENT)
    {
      defaultTextureIndexToReturn = 1;
    }
    else if (textureType == aiTextureType_DIFFUSE)
    {
      defaultTextureIndexToReturn = 0;
    }
    else if (textureType == aiTextureType_SPECULAR)
    {
      defaultTextureIndexToReturn = 0;
    }
    else if (textureType == aiTextureType_EMISSIVE)
    {
      defaultTextureIndexToReturn = 1;
    }
    else if (textureType == aiTextureType_HEIGHT)
    {
      defaultTextureIndexToReturn = 2;
    }
  }
  else
  {
    defaultTextureIndexToReturn = textureFileNameToTextureIndex.find(textureName.C_Str())->second;
  }
  return defaultTextureIndexToReturn;
}

} // namespace



namespace gims
{
Scene SceneGraphFactory::createFromAssImpScene(const std::filesystem::path       pathToScene,
                                               const ComPtr<ID3D12Device>&       device,
                                               const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  Scene outputScene;

  const auto absolutePath = std::filesystem::weakly_canonical(pathToScene);
  if (!std::filesystem::exists(absolutePath))
  {
    throw std::exception((absolutePath.string() + std::string(" does not exist.")).c_str());
  }

  const auto arguments = aiPostProcessSteps::aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                         aiProcess_GenUVCoords | aiProcess_ConvertToLeftHanded | aiProcess_OptimizeMeshes |
                         aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
                         aiProcess_FindInvalidData | aiProcess_FindDegenerates /*| aiProcess_FlipWindingOrder */;

  Assimp::Importer imp;
  imp.SetPropertyBool(AI_CONFIG_PP_FD_REMOVE, true);
  auto inputScene = imp.ReadFile(absolutePath.string(), arguments);
  if (!inputScene)
  {
    throw std::exception((absolutePath.string() + std::string(" can't be loaded. with Assimp.")).c_str());
  }
  const auto textureFileNameToTextureIndex = textureFilenameToIndex(inputScene);

  createMeshes(inputScene, device, commandQueue, outputScene);

  createNodes(inputScene, outputScene, inputScene->mRootNode);

  computeSceneAABB(outputScene, outputScene.m_aabb, 0, glm::identity<f32m4>());
  createTextures(textureFileNameToTextureIndex, absolutePath.parent_path(), device, commandQueue, outputScene);
  createMaterials(inputScene, textureFileNameToTextureIndex, device, outputScene);

  return outputScene;
}

void SceneGraphFactory::createMeshes(aiScene const* const inputScene, const ComPtr<ID3D12Device>& device,
                                     const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{
  // Assignment 3
  
  // Acquiring a pointer to the array of meshes available in the scene
  aiMesh**   meshesInsideTheScene     = inputScene->mMeshes;

  // Getting the number of meshes available in the input scene
  const ui32 numberOfMeshesInTheScene = inputScene->mNumMeshes;
  outputScene.m_meshes.resize(numberOfMeshesInTheScene);

  // Iterating over the meshes, creating a triangle mesh and adding it to the vector of meshes
  for (ui32 i = 0; i < numberOfMeshesInTheScene; i++)
  {
    // Acquiring a pointer to the mesh which should be added
    const aiMesh* meshToAdd       = meshesInsideTheScene[i];
    const ui32    meshPrimitiveTypes = meshToAdd->mPrimitiveTypes;
    const bool    hasPoints          = meshPrimitiveTypes & aiPrimitiveType_POINT;
    const bool    hasLines           = meshPrimitiveTypes & aiPrimitiveType_LINE;
    const bool    hasTriangles       = meshPrimitiveTypes & aiPrimitiveType_TRIANGLE;
    const bool    hasPolygons        = meshPrimitiveTypes & aiPrimitiveType_POLYGON;

    // Getting id of the material
    const auto materialIndex = meshToAdd->mMaterialIndex;

    // Acquiring a pointer to the array containing positions
    const aiVector3D* positionsArray = meshToAdd->mVertices;

    // Getting the number of positions
    const ui32 sizeOfPositionsArray = meshToAdd->mNumVertices;

    // Acquiring a pointer to the normal vectors
    const aiVector3D* normalVectorsArray = meshToAdd->mNormals;

    // Converting the array containing positions into a vector
    std::vector<f32v3> positions((f32v3*)positionsArray, (f32v3*)positionsArray + sizeOfPositionsArray);

    // Converting the array containing normal vectors into a vector
    std::vector<f32v3> normalVectors((f32v3*)normalVectorsArray, (f32v3*)normalVectorsArray + sizeOfPositionsArray);

    std::vector<f32v3> textureCoordinates((f32v3*)meshToAdd->mTextureCoords[0],
                                          (f32v3*)meshToAdd->mTextureCoords[0] + sizeOfPositionsArray);
   // const ui32         numberOfUVChannelsAvailable = meshToAdd->GetNumUVChannels();
     
    const std::vector<ui32v3> indicesGroupdInFaces = getTriangleIndicesFromAiMesh(meshToAdd);

    std::cout << "Mesh Information:\n"
              << "-----------------\n"
              << "Primitive Types:\n"
              << "    Points: " << (hasPoints ? "Yes" : "No") << "\n"
              << "    Lines: " << (hasLines ? "Yes" : "No") << "\n"
              << "    Triangles: " << (hasTriangles ? "Yes" : "No") << "\n"
              << "    Polygons: " << (hasPolygons ? "Yes" : "No") << "\n"
              << "Material ID: " << materialIndex << "\n"
              << "Number of Positions: " << positions.size() << "\n"
              << "Number of Normal Vectors: " << normalVectors.size() << "\n"
              << "Number of Texture Coordinates: " << textureCoordinates.size() << "\n"
              << "Number of Faces: " << indicesGroupdInFaces.size() << "\n"
              << "  Number of Indices: " << indicesGroupdInFaces.size() * 3 << std::endl;

    TriangleMeshD3D12 triangleMeshToAdd(positions.data(), normalVectors.data(), textureCoordinates.data(),
                                   sizeOfPositionsArray, indicesGroupdInFaces.data(),
                                   (ui32)(indicesGroupdInFaces.size() * 3), materialIndex, device, commandQueue);


   outputScene.m_meshes.at(i) = triangleMeshToAdd;

  }
}


static inline glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from)
{
  //glm::mat4 to {};

  //to[0][0] = f32(from->a1);
  //to[0][1] = f32(from->b1);
  //to[0][2] = f32(from->c1);
  //to[0][3] = f32(from->d1);
  //to[1][0] = f32(from->a2);
  //to[1][1] = f32(from->b2);
  //to[1][2] = f32(from->c2);
  //to[1][3] = f32(from->d2);
  //to[2][0] = f32(from->a3);
  //to[2][1] = f32(from->b3);
  //to[2][2] = f32(from->c3);
  //to[2][3] = f32(from->d3);
  //to[3][0] = f32(from->a4);
  //to[3][1] = f32(from->b4);
  //to[3][2] = f32(from->c4);
  //to[3][3] = f32(from->d4);

  //return to;

   return glm::transpose(glm::make_mat4(&from.a1));
}


ui32 SceneGraphFactory::createNodes(aiScene const* const inputScene, Scene& outputScene, aiNode const* const inputNode)
{
  (void)inputScene;
  inputNode->mChildren;
  const ui32 numberOfChildrenOfInputNode = inputNode->mNumChildren;
  
  const ui32        numberOfMeshesOfInputNode   = inputNode->mNumMeshes;
  const auto        meshesOfInputNode           = inputNode->mMeshes;


  const aiMatrix4x4 parentRelativeTransformation = inputNode->mTransformation;
  glm::mat4         convertedParentRelativeTransformation = aiMatrix4x4ToGlm(parentRelativeTransformation);

   Scene::Node nodeToAdd;
   nodeToAdd.transformation = convertedParentRelativeTransformation;
   outputScene.m_nodes.push_back(nodeToAdd);

   ui32 nodeToAddId = ui32(outputScene.m_nodes.size() - 1);
   
   for (ui32 i = 0; i < numberOfMeshesOfInputNode; i++)
   {
     outputScene.m_nodes.at(nodeToAddId).meshIndices.emplace_back(meshesOfInputNode[i]);
   }

  if (numberOfChildrenOfInputNode == 0)
  {
    return nodeToAddId;
  }

  aiNode**  childrenOfInputNode = inputNode->mChildren;
  for (ui32 i = 0; i < numberOfChildrenOfInputNode; i++)
  {
    const ui32 childNodeCreated = createNodes(inputScene, outputScene, childrenOfInputNode[i]);
    outputScene.m_nodes.at(nodeToAddId).childIndices.emplace_back(childNodeCreated);
  }

  // Assignment 4
  return nodeToAddId;
}

void SceneGraphFactory::computeSceneAABB(Scene& scene, AABB& aabb, ui32 nodeIdx, f32m4 transformation)
{


  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  Scene::Node currentNode               = scene.getNode(nodeIdx);
  f32m4       accumulatedTransformation = transformation * currentNode.transformation;


  for (ui32 i = 0; i < currentNode.meshIndices.size(); i++)
  {
    aabb = aabb.getUnion(scene.getMesh(currentNode.meshIndices[i]).getAABB().getTransformed(accumulatedTransformation));
  }

  for (const ui32& nodeIndex : currentNode.childIndices)
  {
    computeSceneAABB(scene, aabb, nodeIndex, accumulatedTransformation);
  }

  // Assignment 5
}

void SceneGraphFactory::createTextures(
    const std::unordered_map<std::filesystem::path, ui32>& textureFileNameToTextureIndex,
    std::filesystem::path parentPath, const ComPtr<ID3D12Device>& device,
    const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{
  
  ui8v4 defaultWhiteTextureData(255, 255, 255, 255);
  ui8v4 defaultBlackTextureData(0, 0, 0, 255);
  ui8v4 defaultNormalMapTextureData(0, 0, 255, 255);

  Texture2DD3D12 defaultWhiteTexture(&defaultWhiteTextureData, 1, 1, device, commandQueue);
  Texture2DD3D12 defaultBlackTexture(&defaultBlackTextureData, 1, 1, device, commandQueue);
  Texture2DD3D12 defaultNormalMapTexture(&defaultNormalMapTextureData, 1, 1, device, commandQueue);

  outputScene.m_textures.resize(textureFileNameToTextureIndex.size() + 3);
  outputScene.m_textures.at(0) = defaultWhiteTexture;
  outputScene.m_textures.at(1) = defaultBlackTexture;
  outputScene.m_textures.at(2) = defaultNormalMapTexture;


  for (const auto& [textureRelativePath, textureIndex] : textureFileNameToTextureIndex)
  {
    Texture2DD3D12 textureToAdd((parentPath / textureRelativePath), device, commandQueue); 
    outputScene.m_textures.at(textureIndex) = textureToAdd;
  }

  // Assignment 9
}

void SceneGraphFactory::createMaterials(aiScene const* const                            inputScene,
                                        std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex,
                                        const ComPtr<ID3D12Device>& device, Scene& outputScene)
{
  const ui32 numberOfMaterialsInTheScene= inputScene->mNumMaterials;
  const auto materialsInTheScene         = inputScene->mMaterials;
  outputScene.m_materials.resize(numberOfMaterialsInTheScene);
  for (ui32 i = 0; i < numberOfMaterialsInTheScene; i++)
  {
    ComPtr<ID3D12DescriptorHeap> srv;
    D3D12_DESCRIPTOR_HEAP_DESC   desc = {};
    desc.Type                         = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors               = 5;
    desc.NodeMask                     = 0;
    desc.Flags                        = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    throwIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv)));


    aiMaterial* materialExtracted = materialsInTheScene[i];
    ai_real           exponentPropertyValue(0.0f);
    f32v4 emissiveFactors = getColor(AI_MATKEY_COLOR_EMISSIVE, materialExtracted);
    f32v4 ambientColor = getColor(AI_MATKEY_COLOR_AMBIENT, materialExtracted);
    f32v4 diffuseColor = getColor(AI_MATKEY_COLOR_DIFFUSE, materialExtracted);
    f32v4 specularColor = getColor(AI_MATKEY_COLOR_SPECULAR, materialExtracted);
    aiGetMaterialFloat(materialExtracted, AI_MATKEY_SHININESS, &exponentPropertyValue);
    
    i32 ambientTextureIndex = getTexture(aiTextureType_AMBIENT, 0, materialExtracted, textureFileNameToTextureIndex);
    i32 diffuseTextureIndex = getTexture(aiTextureType_DIFFUSE, 0, materialExtracted, textureFileNameToTextureIndex);
    i32 specularTextureIndex = getTexture(aiTextureType_SPECULAR, 0, materialExtracted, textureFileNameToTextureIndex);
    i32 emissiveTextureIndex = getTexture(aiTextureType_EMISSIVE, 0, materialExtracted, textureFileNameToTextureIndex);
    i32 normalMapTextureIndex = getTexture(aiTextureType_HEIGHT, 0, materialExtracted, textureFileNameToTextureIndex);


    outputScene.m_textures.at(ambientTextureIndex).addToDescriptorHeap(device, srv, 0);
    outputScene.m_textures.at(diffuseTextureIndex).addToDescriptorHeap(device, srv, 1);
    outputScene.m_textures.at(specularTextureIndex).addToDescriptorHeap(device, srv, 2);
    outputScene.m_textures.at(emissiveTextureIndex).addToDescriptorHeap(device, srv, 3);
    outputScene.m_textures.at(normalMapTextureIndex).addToDescriptorHeap(device, srv, 4);
    


    Scene::MaterialConstantBuffer materialToAddConstantBuffer(
        ambientColor + f32v4(emissiveFactors.r, emissiveFactors.g, emissiveFactors.b, emissiveFactors.a), diffuseColor,
        f32v4(specularColor.x, specularColor.y, specularColor.z, exponentPropertyValue));

    ConstantBufferD3D12 materialToAddConstantBufferCreated(materialToAddConstantBuffer, device);

    Scene::Material     materialToAdd(materialToAddConstantBufferCreated, srv);
    outputScene.m_materials.at(i) = materialToAdd;

    std::cout << materialExtracted->GetName().C_Str() << std::endl;
    std::cout << "Ambient:" << ambientColor.x << ambientColor.y << ambientColor.z << std::endl;
    std::cout << "Diffuse:" << diffuseColor.x << diffuseColor.y << diffuseColor.z << std::endl;
    std::cout << "Specular:" << specularColor.x << specularColor.y << specularColor.z << exponentPropertyValue << std::endl;
  }

  // Assignment 7
  // Assignment 9
  // Assignment 10
}

} // namespace gims
