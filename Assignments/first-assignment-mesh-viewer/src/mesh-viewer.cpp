#include "mesh-viewer.h"
#include <d3dx12/d3dx12.h>
#include <format>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/sys/Event.hpp>
#include <imgui.h>
#include <iostream>
#include <vector>

using namespace gims;

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
{
  initializeCameraPosition();

  initializeUIData();

  // De-serializing our mesh
  CograBinaryMeshFile cbm("../../../data/bunny.cbm");

  loadMesh(&cbm);

  createRootSignature();

  // Creating our pipelines

  // Preparing the vector created to keep track of our pipeline
  m_pipelineStates.resize(4);
  // Creating the ordinary graphics pipeline
  createPipelineForRenderingMeshes(false, false);
  // Creating the ordinary graphics pipeline with back-face culling
  createPipelineForRenderingMeshes(true, false);
  // Creating the wire-frame overlay graphics pipeline
  createPipelineForRenderingMeshes(false, true);
  // Creating the wire-frame overlay graphics pipeline with back-face culling
  createPipelineForRenderingMeshes(true, true);

  createConstantBuffers();

  createTriangleMesh();

  createTexture();
}

MeshViewer::~MeshViewer()
{
}

void MeshViewer::createRootSignature()
{
  const ConstantBuffer   cb {};
  CD3DX12_ROOT_PARAMETER parameter[2] = {};
  parameter[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription;

  CD3DX12_DESCRIPTOR_RANGE range {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0};
  parameter[1].InitAsDescriptorTable(1, &range);

  D3D12_STATIC_SAMPLER_DESC sampler = {};
  sampler.Filter                    = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressV                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressW                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.MipLODBias                = 0;
  sampler.MaxAnisotropy             = 0;
  sampler.ComparisonFunc            = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor               = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD                    = 0.0f;
  sampler.MaxLOD                    = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister            = 0;
  sampler.RegisterSpace             = 0;
  sampler.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

  rootSignatureDescription.Init(2, parameter, 1, &sampler,
                                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void MeshViewer::printInformationOfMeshToLoad(const CograBinaryMeshFile* meshToLoad)
{
  ui32 numberOfAttributes = meshToLoad->getNumAttributes();
  std::cout << std::format(
                   "The mesh to load consists of {} vertices, resulting in {} position coordinates.\n"
                   "It also contains {} triangles, with a total of {} indices.\n"
                   "Additionally, there are {} constants and {}",
                   meshToLoad->getNumVertices(), meshToLoad->getNumVertices() * 3, meshToLoad->getNumTriangles(),
                   meshToLoad->getNumTriangles() * 3, meshToLoad->getNumConstants(),
                   numberOfAttributes == 0 ? "no attribute available!"
                                           : std::format("following {} attributes available:", numberOfAttributes))
            << std::endl;
  for (ui32 i = 0; i < numberOfAttributes; i++)
  {
    std::cout << std::format("Attribute Nr. {}: ", i + 1) << meshToLoad->getAttributeName(i) << std::endl;
  }
}

void MeshViewer::createPipelineForRenderingMeshes(bool backfaceCullingEnabled, bool wireFrameOverlayEnabled)
{

  const auto vertexShader =
      wireFrameOverlayEnabled
          ? compileShader(L"../../../assignments/first-assignment-mesh-viewer/shaders/mesh-viewer.hlsl",
                          L"VS_WireFrame_main", L"vs_6_0")
          : compileShader(L"../../../assignments/first-assignment-mesh-viewer/shaders/mesh-viewer.hlsl", L"VS_main",
                          L"vs_6_0");

  const auto pixelShader =
      wireFrameOverlayEnabled
          ? compileShader(L"../../../assignments/first-assignment-mesh-viewer/shaders/mesh-viewer.hlsl",
                          L"PS_WireFrame_main", L"ps_6_0")
          : compileShader(L"../../../assignments/first-assignment-mesh-viewer/shaders/mesh-viewer.hlsl", L"PS_main",
                          L"ps_6_0");

  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDescription = {};
  pipelineStateDescription.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
  pipelineStateDescription.pRootSignature                     = m_rootSignature.Get();
  pipelineStateDescription.VS                                 = HLSLCompiler::convert(vertexShader);
  pipelineStateDescription.PS                                 = HLSLCompiler::convert(pixelShader);
  pipelineStateDescription.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  pipelineStateDescription.RasterizerState.FillMode =
      wireFrameOverlayEnabled ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
  pipelineStateDescription.RasterizerState.CullMode =
      backfaceCullingEnabled ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE;
  pipelineStateDescription.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  pipelineStateDescription.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  pipelineStateDescription.SampleMask            = UINT_MAX;
  pipelineStateDescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pipelineStateDescription.NumRenderTargets      = 1;
  pipelineStateDescription.SampleDesc.Count      = 1;
  pipelineStateDescription.RTVFormats[0]                 = getDX12AppConfig().renderTargetFormat;
  pipelineStateDescription.DSVFormat                     = getDX12AppConfig().depthBufferFormat;
  pipelineStateDescription.DepthStencilState.DepthEnable = TRUE;
  pipelineStateDescription.DepthStencilState.DepthFunc =
      wireFrameOverlayEnabled ? D3D12_COMPARISON_FUNC_LESS : D3D12_COMPARISON_FUNC_LESS_EQUAL;
  pipelineStateDescription.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  pipelineStateDescription.DepthStencilState.StencilEnable = FALSE;
  if (wireFrameOverlayEnabled)
  {
    pipelineStateDescription.RasterizerState.DepthBias            = 1;
    pipelineStateDescription.RasterizerState.DepthBiasClamp       = -100.0f;
    pipelineStateDescription.RasterizerState.SlopeScaledDepthBias = -1.0f;
  }

  ComPtr<ID3D12PipelineState> pipeLineStateToCreate;
  throwIfFailed(
      getDevice()->CreateGraphicsPipelineState(&pipelineStateDescription, IID_PPV_ARGS(&pipeLineStateToCreate)));
  m_pipelineStates.at(static_cast<ui8>(backfaceCullingEnabled) | (wireFrameOverlayEnabled << 1)) =
      pipeLineStateToCreate;
}

void MeshViewer::initializeUIData()
{
  m_uiData.backgroundColor              = f32v3(0.25f, 0.25f, 0.25f);
  m_uiData.wireframeOverlayColor        = f32v3(0.0f, 0.0f, 0.0f);
  m_uiData.frameWorkWidth               = f32(getWidth());
  m_uiData.frameWorkHeight              = f32(getWidth());
  m_uiData.frameAspectRatio             = m_uiData.frameWorkWidth / m_uiData.frameWorkHeight;
  m_uiData.backFaceCullingEnabled       = false;
  m_uiData.wireFrameOverlayEnabled      = false;
  m_uiData.twoSidedLightingEnabled      = false;
  m_uiData.useTexture                   = false;
  m_uiData.flatShadingEnabled           = false;
  m_uiData.ambient                      = f32v3(0.0f, 0.0f, 0.0f);
  m_uiData.diffuse                      = f32v3(1.0f, 1.0f, 1.0f);
  m_uiData.specular                     = f32v3(1.0f, 1.0f, 1.0f);
  m_uiData.exponent                     = f32(128.0f);
  m_uiData.lightDirectionXCoordinate    = f32(0.0f);
  m_uiData.lightDirectionYCoordinate    = f32(0.0f);
  m_uiData.fieldOfView                  = f32(45.0f);
  m_uiData.nearPlane                    = f32(0.01f);
  m_uiData.farPlane                     = f32(1000.01f);
  m_uiData.cameraResetButtonClicked     = false;
  m_uiData.parametersResetButtonClicked = false;
}

void MeshViewer::initializeCameraPosition()
{
  m_examinerController.reset();
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));
}

void MeshViewer::createTriangleMesh()
{
  // Instantiating a upload helper to upload our buffers on the GPU
  UploadHelper uploadBuffer(getDevice(), std::max(m_vertexBufferOnCPUSizeInBytes, m_indexBufferOnCPUSizeInBytes));

  // Uploading the vertex buffer on the GPU
  const CD3DX12_RESOURCE_DESC   vertexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferOnCPUSizeInBytes);
  const CD3DX12_HEAP_PROPERTIES defaultHeapProperties   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = static_cast<ui32>(m_vertexBufferOnCPUSizeInBytes);
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);
  uploadBuffer.uploadBuffer(m_vertexBufferOnCPU.data(), m_vertexBuffer, m_vertexBufferOnCPUSizeInBytes,
                            getCommandQueue());

  // Uploading the index buffer on the GPU
  const CD3DX12_RESOURCE_DESC indexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferOnCPUSizeInBytes);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = static_cast<ui32>(m_indexBufferOnCPUSizeInBytes);
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  uploadBuffer.uploadBuffer(m_indexBufferOnCPU.data(), m_indexBuffer, m_indexBufferOnCPUSizeInBytes, getCommandQueue());
}

void MeshViewer::loadMesh(const CograBinaryMeshFile* meshToLoad)
{
  printInformationOfMeshToLoad(meshToLoad);

  // Default-initializing the vertex buffer residing on the system memory (RAM)
  for (ui32 i = 0; i < meshToLoad->getNumVertices(); i++)
  {
    Vertex emptyVertex {};
    m_vertexBufferOnCPU.push_back(emptyVertex);
  }

  // Zero-initializing the index buffer residing on the system memory (RAM)
  for (ui32 i = 0; i < meshToLoad->getNumTriangles() * 3; i++)
  {
    m_indexBufferOnCPU.push_back(0);
  }

  // Updating size of the vertex buffer residing on the system memory (RAM)
  m_vertexBufferOnCPUSizeInBytes = m_vertexBufferOnCPU.size() * sizeof(Vertex);

  loadVertices(meshToLoad);

  loadIndices(meshToLoad);

  loadNormals(meshToLoad);

  loadUVs(meshToLoad);

  calculateNormalizationTransformation();
}

void MeshViewer::loadVertices(const CograBinaryMeshFile* meshToLoad)
{
  auto positionsPointer = meshToLoad->getPositionsPtr();
  for (ui32 i = 0; i < meshToLoad->getNumVertices(); i++)
  {
    f32v3 currentPosition(positionsPointer[i * 3], positionsPointer[i * 3 + 1], positionsPointer[i * 3 + 2]);

    m_vertexBufferOnCPU.at(i).position = currentPosition;
  }
  m_vertexBufferOnCPUSizeInBytes = m_vertexBufferOnCPU.size() * sizeof(Vertex);
}

void MeshViewer::loadIndices(const CograBinaryMeshFile* meshToLoad)
{
  auto indicesPointer = meshToLoad->getTriangleIndices();
  for (ui32 i = 0; i < meshToLoad->getNumTriangles() * 3; i++)
  {
    m_indexBufferOnCPU.at(i) = indicesPointer[i];
  }
  m_indexBufferOnCPUSizeInBytes = m_indexBufferOnCPU.size() * sizeof(ui32);
}

void MeshViewer::loadNormals(const CograBinaryMeshFile* meshToLoad)
{
  void*                           normalsVoidPointer = meshToLoad->getAttributePtr(0);
  CograBinaryMeshFile::FloatType* normalsPointer     = static_cast<CograBinaryMeshFile::FloatType*>(normalsVoidPointer);
  for (ui32 i = 0; i < meshToLoad->getNumVertices(); i++)
  {
    f32v3 currentNormal(normalsPointer[i * 3], normalsPointer[i * 3 + 1], normalsPointer[i * 3 + 2]);

    m_vertexBufferOnCPU.at(i).normal = currentNormal;
  }

  m_vertexBufferOnCPUSizeInBytes = m_vertexBufferOnCPU.size() * sizeof(Vertex);
}

void MeshViewer::loadUVs(const CograBinaryMeshFile* meshToLoad)
{
  void*                           UVsVoidPointer = meshToLoad->getAttributePtr(1);
  CograBinaryMeshFile::FloatType* UVsPointer     = static_cast<CograBinaryMeshFile::FloatType*>(UVsVoidPointer);
  for (ui32 i = 0; i < meshToLoad->getNumVertices(); i++)
  {
    f32v2 currentUV(UVsPointer[i * 2], UVsPointer[i * 2 + 1]);

    m_vertexBufferOnCPU.at(i).UV = currentUV;
  }

  m_vertexBufferOnCPUSizeInBytes = m_vertexBufferOnCPU.size() * sizeof(Vertex);
}

void MeshViewer::createTexture()
{
  i32 textureWidth, textureHeight, textureComp;
  
  stbi_set_flip_vertically_on_load(1);
  std::unique_ptr<ui8, void (*)(void*)> image(
      stbi_load("../../../data/bunny.png", &textureWidth, &textureHeight, &textureComp, 4), &stbi_image_free);

  D3D12_RESOURCE_DESC textureDesc = {};
  textureDesc.MipLevels           = 1;
  textureDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.Width               = textureWidth;
  textureDesc.Height              = textureHeight;
  textureDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
  textureDesc.DepthOrArraySize    = 1;
  textureDesc.SampleDesc.Count    = 1;
  textureDesc.SampleDesc.Quality  = 0;
  textureDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  throwIfFailed(getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
                                                     D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_texture)));

  UploadHelper uploadHelper(getDevice(), GetRequiredIntermediateSize(m_texture.Get(), 0, 1));
  uploadHelper.uploadTexture(image.get(), m_texture, textureWidth, textureHeight, getCommandQueue());

  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors             = 1;
  desc.NodeMask                   = 0;
  desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  throwIfFailed(getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_shaderResourceView)));

  D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
  shaderResourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  shaderResourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  shaderResourceViewDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM;
  shaderResourceViewDesc.Texture2D.MipLevels             = 1;
  shaderResourceViewDesc.Texture2D.MostDetailedMip       = 0;
  shaderResourceViewDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
  getDevice()->CreateShaderResourceView(m_texture.Get(), &shaderResourceViewDesc,
                                        m_shaderResourceView->GetCPUDescriptorHandleForHeapStart());
}

f32v3 MeshViewer::calculateCentroidOfMeshLoaded()
{
  // Zero-initializing a local variable to keep track of the component wise sum
  f32v3 componentwiseSumOfPositions(0.0f, 0.0f, 0.0f);
  for (const Vertex& vertex : m_vertexBufferOnCPU)
  {
    componentwiseSumOfPositions += vertex.position;
  }

  // Calculating centroid of the mesh loaded
  f32v3 centroid = componentwiseSumOfPositions / static_cast<f32>(m_vertexBufferOnCPU.size());
  return centroid;
}

glm::mat2x3 MeshViewer::calculateBoundingBox()
{
  // Zero-initializing both local variables for finding beginning and end of the bounding box
  f32v3 startOfBoundingBox = m_vertexBufferOnCPU.at(0).position;
  f32v3 endOfBoundingBox = m_vertexBufferOnCPU.at(0).position;

  // Finding beginning and end of the bounding box
  for (auto& vertex : m_vertexBufferOnCPU)
  {
    startOfBoundingBox = glm::min(startOfBoundingBox, vertex.position);
    endOfBoundingBox   = glm::max(endOfBoundingBox, vertex.position);
  }

  // Packing the result into a matrix (only to return them back in a more convenient way)
  glm::mat2x3 result = glm::mat2x3(startOfBoundingBox, endOfBoundingBox);

  return result;
}

void MeshViewer::calculateNormalizationTransformation()
{


  glm::mat2x3 boundingBox = calculateBoundingBox();

  // Calculating axis lengths
  f32v3       axisLengths = boundingBox[1] - boundingBox[0];

  // Finding longest axis
  f32         longestAxis = glm::max(axisLengths.x, glm::max(axisLengths.y, axisLengths.z));

  // Calculating scaling factors based on the longest axes
  f32v3           scalingFactors = axisLengths / (longestAxis);

  f32v3 centroid = (boundingBox[1] + boundingBox[0]) / 2;
  // Assembling a transformation matrix which centers the mesh loaded
  glm::highp_mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -centroid);

  // Assembling a transformation matrix which normalizes the axis lengths
  glm::highp_mat4 scalingMatrix  = glm::scale(glm::mat4(1.0f), scalingFactors);


    // Assembling a transformation matrix which rotates the mesh loaded 180 degress around the x axis so that we can view it from the side
  constexpr glm::f32 rotationAngle  = glm::radians(180.0f);
  glm::vec3          rotationAxisY  = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::mat4          rotationMatrix = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxisY);

  m_meshLoadedNormalizationTransformation = rotationMatrix * scalingMatrix * translationMatrix;
}

void MeshViewer::createConstantBuffers()
{
  ui32                 numberOfFrames = getDX12AppConfig().frameCount;
  m_constantBuffersOnCPU.resize(numberOfFrames);
  for (ui32 i = 0; i < numberOfFrames; i++)
  {
    static const CD3DX12_HEAP_PROPERTIES uploadHeapProperties   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    static const CD3DX12_RESOURCE_DESC constantBufferDescriptor = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));
    getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDescriptor,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(&m_constantBuffersOnCPU[i]));
  }
}

void MeshViewer::updateConstantBuffers()
{
  ConstantBuffer currentConstantBufferOnCPU {};

  // Determining projection matrix
  float     fov = glm::radians(static_cast<float>(m_uiData.fieldOfView));
  glm::mat4 projectionMatrix =
      glm::perspectiveFovLH_ZO<f32>(fov, (f32)getWidth(), (f32)getHeight(), m_uiData.nearPlane, m_uiData.farPlane);

  // Determining the view matrix
  auto V = m_examinerController.getTransformationMatrix();

  // Calculating the model-view-projection matrix
  currentConstantBufferOnCPU.mvp = projectionMatrix * V * m_meshLoadedNormalizationTransformation;
  // Calculating the model-view matrix
  currentConstantBufferOnCPU.mv  = V * m_meshLoadedNormalizationTransformation;

  // Checking whether the parameters must be reseted
  if (m_uiData.parametersResetButtonClicked == true)
  {
    m_uiData.parametersResetButtonClicked = false;
    initializeUIData();
  }

  // Checking whether the camera setting/parameters must be reseted
  if (m_uiData.cameraResetButtonClicked == true)
  {
    m_uiData.cameraResetButtonClicked = false;
    initializeCameraPosition();
  }

  // Updating the UI
  currentConstantBufferOnCPU.wireframeOverlayColor = f32v4(m_uiData.wireframeOverlayColor, 1.0f);
  currentConstantBufferOnCPU.ambientColor          = f32v4(m_uiData.ambient, 1.0f);
  currentConstantBufferOnCPU.diffuseColor          = f32v4(m_uiData.diffuse, 1.0f);
  currentConstantBufferOnCPU.specularColor_and_Exponent =
      f32v4(m_uiData.specular.x, m_uiData.specular.y, m_uiData.specular.z, m_uiData.exponent);
  currentConstantBufferOnCPU.lightDirectionXCoordinate = f32(m_uiData.lightDirectionXCoordinate);
  currentConstantBufferOnCPU.lightDirectionYCoordinate = f32(m_uiData.lightDirectionYCoordinate);
  currentConstantBufferOnCPU.flags =
      ui32v1((m_uiData.flatShadingEnabled << 2) | (m_uiData.useTexture << 1) | int(m_uiData.twoSidedLightingEnabled));

  // Actually updating the current constant buffer
  const auto& currentConstantBuffer = m_constantBuffersOnCPU[this->getFrameIndex()];
  void*       p;
  currentConstantBuffer->Map(0, nullptr, &p);
  ::memcpy(p, &currentConstantBufferOnCPU, sizeof(currentConstantBufferOnCPU));
  currentConstantBuffer->Unmap(0, nullptr);
}

void MeshViewer::onDraw()
{
  updateConstantBuffers();
  if (!ImGui::GetIO().WantCaptureMouse)
  {
    bool pressed  = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right);
    if (pressed || released)
    {
      bool left = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
      m_examinerController.click(pressed, left == true ? 1 : 2,
                                 ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl),
                                 getNormalizedMouseCoordinates());
    }
    else
    {
      m_examinerController.move(getNormalizedMouseCoordinates());
    }
  }

  const auto commandList = getCommandList();
  const auto rtvHandle   = getRTVHandle();
  const auto dsvHandle   = getDSVHandle();

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  float clearColor[4] = {m_uiData.backgroundColor.x, m_uiData.backgroundColor.y, m_uiData.backgroundColor.z, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  commandList->SetPipelineState(m_pipelineStates.at(m_uiData.backFaceCullingEnabled).Get());

  commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  commandList->SetDescriptorHeaps(1, m_shaderResourceView.GetAddressOf());
  commandList->SetGraphicsRootDescriptorTable(1, m_shaderResourceView->GetGPUDescriptorHandleForHeapStart());

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffersOnCPU[getFrameIndex()]->GetGPUVirtualAddress());

  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->DrawIndexedInstanced(static_cast<ui32>(m_indexBufferOnCPU.size()), 1, 0, 0, 0);

  if (m_uiData.wireFrameOverlayEnabled)
  {
    commandList->SetPipelineState(
        m_pipelineStates.at(static_cast<ui8>(m_uiData.backFaceCullingEnabled) | (m_uiData.wireFrameOverlayEnabled << 1))
            .Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffersOnCPU[getFrameIndex()]->GetGPUVirtualAddress());

    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->DrawIndexedInstanced(static_cast<ui32>(m_indexBufferOnCPU.size()), 1, 0, 0, 0);
  }



}

void MeshViewer::onDrawUI()
{
  const auto imGuiFlags    = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;

  // Setting the width, height and also the aspect ratio of the current frame
  m_uiData.frameWorkWidth  = static_cast<f32>(getWidth());
  m_uiData.frameWorkHeight = static_cast<f32>(getHeight());
  m_uiData.frameAspectRatio = m_uiData.frameWorkWidth / m_uiData.frameWorkHeight;

  // Calculating the frame time
  f32 frameTime = 1.0f / ImGui::GetIO().Framerate * 1000.0f;

  // Drawing the information window
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frame time: %f", frameTime);
  ImGui::Text("Frame Width: %.2f", m_uiData.frameWorkWidth);
  ImGui::Text("Frame Height: %.2f", m_uiData.frameWorkHeight);
  ImGui::Text("Frame Aspect Ratio: %.2f", m_uiData.frameAspectRatio);
  ImGui::End();

  // Drawing the configurations window
  ImGui::Begin("Configurations", nullptr, imGuiFlags);
  ImGui::ColorEdit3("Background Color", &m_uiData.backgroundColor[0]);
  ImGui::Checkbox("Back-Face Culling", &m_uiData.backFaceCullingEnabled);
  ImGui::Checkbox("Wireframe Overlay", &m_uiData.wireFrameOverlayEnabled);

  // Setting/adjusting the wireframe overlay color if it is enabled
  f32v3 temporaryWireframeColor = m_uiData.wireframeOverlayColor;
  if (ImGui::ColorEdit3("Wireframe Overlay Color", &m_uiData.wireframeOverlayColor[0]))
  {
    if (!m_uiData.wireFrameOverlayEnabled)
    {
      m_uiData.wireframeOverlayColor = temporaryWireframeColor;
    }

  }
  ImGui::Checkbox("Two-Sided Lighting", &m_uiData.twoSidedLightingEnabled);
  ImGui::Checkbox("Use Texture", &m_uiData.useTexture);
  ImGui::Checkbox("Flat Shading", &m_uiData.flatShadingEnabled);
  ImGui::ColorEdit3("Ambient", &m_uiData.ambient[0]);
  ImGui::ColorEdit3("Diffuse", &m_uiData.diffuse[0]);
  ImGui::ColorEdit3("Specular", &m_uiData.specular[0]);
  ImGui::SliderFloat("Exponent", &m_uiData.exponent, 0.0f, 255.0f);
  ImGui::SliderFloat("Light Direction X Coordinate", &m_uiData.lightDirectionXCoordinate, -1.0f, 1.0f);
  ImGui::SliderFloat("Light Direction Y Coordinate", &m_uiData.lightDirectionYCoordinate, -1.0f, 1.0f);
  ImGui::SliderFloat("Field of View", &m_uiData.fieldOfView, 0.1f, 90.0f);
  ImGui::SliderFloat("Near Plane", &m_uiData.nearPlane, 0.1f, 10.0f);
  ImGui::SliderFloat("Far Plane", &m_uiData.farPlane, 10.1f, 10000.0f);
  if (ImGui::Button("Reset Parameters"))
  {
    m_uiData.parametersResetButtonClicked = true;
  }
  if (ImGui::Button("Reset Camera"))
  {
    m_uiData.cameraResetButtonClicked = true;
  }
  ImGui::End();

}
