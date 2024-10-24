#include "TriangleApp.h"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/sys/Event.hpp>
#include <imgui.h>
#include <iostream>
#include <format>
#include <vector>

using namespace gims;

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
{
  m_uiData.backgroundColor = f32v3(0.25f, 0.25f, 0.25f);
  m_uiData.wireframeOverlayColor   = f32v3(0.0f, 0.0f, 0.0f);
  m_uiData.frameWorkWidth          = static_cast<f32>(config.width);
  m_uiData.frameWorkHeight         = static_cast<f32>(config.height);
  m_uiData.frameWorkCenter         = f32v2(m_uiData.frameWorkWidth / 2, m_uiData.frameWorkHeight / 2);
  m_uiData.backFaceCullingEnabled = false;
  m_uiData.wireFrameOverlayEnabled = false;

  m_appConfig = config;
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));
  CograBinaryMeshFile cbm("../../../data/bunny.cbm");
  loadMesh(cbm);
  //doVerticesMakeSense();
  createRootSignature();
  createPipelineForRenderingMeshes();
  createPipelineForRenderingWireframeOverlay();
  createConstantBuffers();
  createTriangleMesh();
}

MeshViewer::~MeshViewer()
{
}

void MeshViewer::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER parameter = {};
  parameter.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);


  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription;
  rootSignatureDescription.Init(1, &parameter, 0, nullptr,
                                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));

  std::cout << "The Root signature was created successfully!" << std::endl;
}

void MeshViewer::printInformationOfMeshToLoad()
{
  ui32 numberOfAttributes = m_meshLoaded.getNumAttributes();
  std::cout << std::format("The mesh to load consists of {} vertices, resulting in {} position coordinates.\n"
                           "It also contains {} triangles, with a total of {} indices.\n"
                           "Additionally, there are {} constants and {}",
                           m_meshLoaded.getNumVertices(), m_meshLoaded.getNumVertices() * 3,
                           m_meshLoaded.getNumTriangles(), m_meshLoaded.getNumTriangles() * 3,
                           m_meshLoaded.getNumConstants(),
                           numberOfAttributes == 0 
      ? "no attribute available!" 
      : std::format("following {} attributes available:", numberOfAttributes))
      << std::endl;
  for (ui32 i = 0; i < numberOfAttributes; i++)
  {
    std::cout << std::format("Attribute Nr. {}: ", i + 1) << m_meshLoaded.getAttributeName(i) << std::endl;
  }
}

void MeshViewer::createPipelineForRenderingMeshes()
{

  const auto vertexShader =
      compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"VS_main", L"vs_6_0");

  const auto pixelShader =
      compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"PS_main", L"ps_6_0");

  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDescription = {};
  pipelineStateDescription.InputLayout                      = {inputElementDescs, _countof(inputElementDescs)};
  pipelineStateDescription.pRootSignature                   = m_rootSignature.Get();
  pipelineStateDescription.VS                               = HLSLCompiler::convert(vertexShader);
  pipelineStateDescription.PS                               = HLSLCompiler::convert(pixelShader);
  pipelineStateDescription.RasterizerState                  = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  //pipelineStateDescription.RasterizerState.FrontCounterClockwise = FALSE; 
  pipelineStateDescription.RasterizerState.CullMode              = D3D12_CULL_MODE_NONE;
  pipelineStateDescription.BlendState                       = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  pipelineStateDescription.DepthStencilState                = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  pipelineStateDescription.SampleMask                       = UINT_MAX;
  pipelineStateDescription.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pipelineStateDescription.NumRenderTargets                 = 1;
  pipelineStateDescription.SampleDesc.Count                 = 1;
  pipelineStateDescription.RTVFormats[0]                    = getRenderTarget()->GetDesc().Format;
  pipelineStateDescription.DSVFormat                        = getDepthStencil()->GetDesc().Format;
  pipelineStateDescription.DepthStencilState.DepthEnable    = FALSE;
  pipelineStateDescription.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_ALWAYS;
  pipelineStateDescription.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  pipelineStateDescription.DepthStencilState.StencilEnable  = FALSE;

  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&pipelineStateDescription, IID_PPV_ARGS(&m_pipelineStateForRenderingMeshes)));
  std::cout << "The pipeline for rendering meshes was created successfully!" << std::endl;
}

void MeshViewer::createPipelineForRenderingWireframeOverlay()
{
  const auto vertexShader =
      compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"VS_WireFrame_main", L"vs_6_0");

  const auto pixelShader =
      compileShader(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", L"PS_WireFrame_main", L"ps_6_0");

  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDescription = {};
  pipelineStateDescription.InputLayout                        = {inputElementDescs, _countof(inputElementDescs)};
  pipelineStateDescription.pRootSignature                     = m_rootSignature.Get();
  pipelineStateDescription.VS                                 = HLSLCompiler::convert(vertexShader);
  pipelineStateDescription.PS                                 = HLSLCompiler::convert(pixelShader);
  pipelineStateDescription.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  pipelineStateDescription.RasterizerState.FillMode           = D3D12_FILL_MODE_WIREFRAME,   
  pipelineStateDescription.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  pipelineStateDescription.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  pipelineStateDescription.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  pipelineStateDescription.SampleMask                         = UINT_MAX;
  pipelineStateDescription.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pipelineStateDescription.NumRenderTargets                   = 1;
  pipelineStateDescription.SampleDesc.Count                   = 1;
  pipelineStateDescription.RTVFormats[0]                      = getRenderTarget()->GetDesc().Format;
  pipelineStateDescription.DSVFormat                          = getDepthStencil()->GetDesc().Format;
  pipelineStateDescription.DepthStencilState.DepthEnable      = FALSE;
  pipelineStateDescription.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
  pipelineStateDescription.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
  pipelineStateDescription.DepthStencilState.StencilEnable    = FALSE;

  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&pipelineStateDescription,
                                                         IID_PPV_ARGS(&m_pipelineStateForRenderingWireframeOverlay)));
  std::cout << "The pipeline for rendering wireframe overlay was created successfully!" << std::endl;
}

void MeshViewer::doVerticesMakeSense()
{
  int count = 0;
  for (auto& vertex : m_vertexBufferOnCPU)
  {
      if (vertex.position.x > 1 || vertex.position.x < -1 || vertex.position.y > 1 || vertex.position.x < -1 ||
          vertex.position.z > 1 || vertex.position.z < 0)
      {
        count++;
          std::cout << glm::to_string(vertex.position) << std::endl;
      }
  }

  std::cout << count << std::endl;
}

void MeshViewer::createTriangleMesh()
{
  UploadHelper uploadBuffer(getDevice(), std::max(m_vertexBufferOnCPUSizeInBytes, m_indexBufferOnCPUSizeInBytes));
  
  
  //std::vector<Vertex>   temp = m_vertexBufferOnCPU;



  //for (ui32 i = 0; i < m_meshLoaded.getNumVertices(); i++)
  //{
  //  m_vertexBufferOnCPU.at(i).position = V * projectionMatrix * f32v4(m_vertexBufferOnCPU.at(i).position, 1.0f);
  //  m_vertexBufferOnCPU.at(i).position.x = (m_vertexBufferOnCPU.at(i).position.x) / 1.5f;
  //  m_vertexBufferOnCPU.at(i).position.y = (m_vertexBufferOnCPU.at(i).position.y) / 1.5f;
  //  m_vertexBufferOnCPU.at(i).position.z = (m_vertexBufferOnCPU.at(i).position.z + 2) / 4.0f;
  //}

  //doVerticesMakeSense();
  const CD3DX12_RESOURCE_DESC vertexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferOnCPUSizeInBytes);
  const CD3DX12_HEAP_PROPERTIES defaultHeapProperties   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = static_cast<ui32>(m_vertexBufferOnCPUSizeInBytes);
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);
  uploadBuffer.uploadBuffer(m_vertexBufferOnCPU.data(), m_vertexBuffer, m_vertexBufferOnCPUSizeInBytes, getCommandQueue());

  const CD3DX12_RESOURCE_DESC indexBufferDescription = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferOnCPUSizeInBytes);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDescription,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));
  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = static_cast<ui32>(m_indexBufferOnCPUSizeInBytes);
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  uploadBuffer.uploadBuffer(m_indexBufferOnCPU.data(), m_indexBuffer, m_indexBufferOnCPUSizeInBytes, getCommandQueue());

  // std::cout << "The triangle mesh was created successfully!" << std::endl;
  //std::cout << "V Matrix:" <<  glm::to_string(V) << std::endl;
  // m_vertexBufferOnCPU = temp;

}

void MeshViewer::loadMesh(const CograBinaryMeshFile& meshToLoad)
{
  printInformationOfMeshToLoad();
  m_meshLoaded = meshToLoad;

  for (ui32 i = 0; i < m_meshLoaded.getNumVertices(); i++)
  {
    Vertex emptyVertex {};
    m_vertexBufferOnCPU.push_back(emptyVertex);
  }

  for (ui32 i = 0; i < m_meshLoaded.getNumTriangles() * 3; i++)
  {
    m_indexBufferOnCPU.push_back(0);
  }

  m_vertexBufferOnCPUSizeInBytes = m_vertexBufferOnCPU.size() * sizeof(Vertex);
  std::cout << "Size of vertex buffer:" << m_vertexBufferOnCPUSizeInBytes << std::endl;
  loadVertices();
  loadIndices();
  loadNormals();
}

void MeshViewer::loadVertices()
{
  CograBinaryMeshFile::FloatType* positionsPointer = m_meshLoaded.getPositionsPtr();
  for (ui32 i = 0; i < m_meshLoaded.getNumVertices(); i++)
  {
    f32v3  currentPosition(
        positionsPointer[i * 3],
        positionsPointer[i * 3 + 1],
        positionsPointer[i * 3 + 2]);

    m_vertexBufferOnCPU.at(i).position = currentPosition;
  }
  m_vertexBufferOnCPUSizeInBytes = m_vertexBufferOnCPU.size() * sizeof(Vertex);
  std::cout << std::format(
                   "A total of {} vertices were successfully loaded into the vertex buffer on CPU, with a size of {} bytes.",
                   m_vertexBufferOnCPU.size(), m_vertexBufferOnCPUSizeInBytes)
            << std::endl;
}

void MeshViewer::loadIndices()
{
  CograBinaryMeshFile::IndexType* indicesPointer = m_meshLoaded.getTriangleIndices();
  for (ui32 i = 0; i < m_meshLoaded.getNumTriangles() * 3; i++)
  {
    m_indexBufferOnCPU.at(i) = indicesPointer[i];
  }
  m_indexBufferOnCPUSizeInBytes = m_indexBufferOnCPU.size() * sizeof(ui32);
  std::cout << std::format(
                   "A total of {} indices were successfully loaded into the index buffer on CPU, with a size of {} bytes.",
                    m_indexBufferOnCPU.size(), m_indexBufferOnCPUSizeInBytes)
            << std::endl;
}

void MeshViewer::loadNormals()
{
  void* normalsVoidPointer = m_meshLoaded.getAttributePtr(0);
  CograBinaryMeshFile::FloatType* normalsPointer =
  static_cast<CograBinaryMeshFile::FloatType*>(normalsVoidPointer);
  for (ui32 i = 0; i < m_meshLoaded.getNumVertices(); i++)
  {
    f32v3 currentNormal(
        normalsPointer[i * 3],
        normalsPointer[i * 3 + 1],
        normalsPointer[i * 3 + 2]);

    m_vertexBufferOnCPU.at(i).normal = currentNormal;
  }

  m_vertexBufferOnCPUSizeInBytes = m_vertexBufferOnCPU.size() * sizeof(Vertex);
  std::cout
      << std::format(
             "A total of {} normals were successfully loaded into the vertex buffer on CPU, with a size of {} bytes.",
             m_vertexBufferOnCPU.size(), m_vertexBufferOnCPUSizeInBytes)
      << std::endl;
}

f32v3 MeshViewer::calculateCentroidOfMeshLoaded()
{
  f32v3 componentwiseSumOfPositions(0.0f, 0.0f, 0.0f);
  for (const Vertex& vertex : m_vertexBufferOnCPU)
  {
    componentwiseSumOfPositions += vertex.position;
  }
  f32v3 centroid = componentwiseSumOfPositions / static_cast<f32>(m_vertexBufferOnCPU.size());
  // std::cout << std::format("The centroid of the model has the following coordinates: {}", to_string(centroid))
   //         << std::endl;

  return centroid;
}

glm::mat2x3 MeshViewer::calculateBoundingBox()
{
  f32v3 startOfBoundingBox;
  f32v3 endOfBoundingBox;
  ;

  for (auto& vertex : m_vertexBufferOnCPU)
  {
    startOfBoundingBox = glm::min(startOfBoundingBox, vertex.position);
    endOfBoundingBox   = glm::max(endOfBoundingBox, vertex.position);
  }
  
  glm::mat2x3 result = glm::mat2x3(startOfBoundingBox, endOfBoundingBox);

  return result;
}

f32m4 MeshViewer::getNormalizationTransformation()
{
  f32v3 centroid       = calculateCentroidOfMeshLoaded();
  glm::highp_mat4 translationMatrix = glm::translate(glm::mat4(1.0f), -centroid);


  glm::mat2x3 boundingBox = calculateBoundingBox();
  f32v3       axisLengths = boundingBox[1] - boundingBox[0];
  f32         longestAxis = glm::max(axisLengths.x, glm::max(axisLengths.y, axisLengths.z));
  f32v3           scalingFactors = axisLengths / (longestAxis);
  glm::highp_mat4 scalingMatrix  = glm::scale(glm::mat4(1.0f), scalingFactors);

  glm::highp_mat4 normalizationMatrix = scalingMatrix * translationMatrix;
  return normalizationMatrix;
}

void MeshViewer::createConstantBuffers()
{
  const ConstantBuffer cb = {};
  ui32                 numberOfFrames = getDX12AppConfig().frameCount;
  m_constantBuffersOnCPU.resize(numberOfFrames);
  for (ui32 i = 0; i < numberOfFrames; i++)
  {
    static const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    static const CD3DX12_RESOURCE_DESC   constantBufferDescriptor   = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));
    getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDescriptor,
                                         D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                         IID_PPV_ARGS(&m_constantBuffersOnCPU[i]));
  }
}


void MeshViewer::updateConstantBuffers()
{
  ConstantBuffer cb;

  constexpr float fov       = glm::radians(45.0f); // More standard FOV
  float width     = m_uiData.frameWorkWidth;
  float height    = m_uiData.frameWorkHeight;
  float nearPlane = 0.0f;  // Slightly further near plane to avoid distortion
  float farPlane  = 1.01f; // Far enough to capture distant objects

  glm::mat4 projectionMatrix = glm::perspectiveFovLH_ZO<f32>(fov, width, height, nearPlane, farPlane);

  auto T = getNormalizationTransformation();

  auto           V = m_examinerController.getTransformationMatrix();

  cb.mvp = projectionMatrix * V * T;

  const auto& currentConstantBuffer = m_constantBuffersOnCPU[this->getFrameIndex()];
  void*       p;
  currentConstantBuffer->Map(0, nullptr, &p);
  ::memcpy(p, &cb, sizeof(cb));
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

  // TODO Implement me!
  // Use this to get the transformation Matrix.
  // Of course, skip the (void). That is just to prevent warning, sinces I am not using it here (but you will have to!)
  //(void)m_examinerController.getTransformationMatrix();
  

  const auto commandList = getCommandList();
  const auto rtvHandle   = getRTVHandle();
  const auto dsvHandle   = getDSVHandle();
  // TODO Implement me!

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
  // TODO Implement me!

  float clearColor[4] = {m_uiData.backgroundColor.x, m_uiData.backgroundColor.y, m_uiData.backgroundColor.z, 1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());


  commandList->SetPipelineState(m_pipelineStateForRenderingMeshes.Get());
  commandList->SetGraphicsRootSignature(m_rootSignature.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffersOnCPU[getFrameIndex()]->GetGPUVirtualAddress());


  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);


  commandList->DrawIndexedInstanced(m_meshLoaded.getNumTriangles() * 3, 1, 0, 0, 0);

  if (m_uiData.wireFrameOverlayEnabled)
  {
    commandList->SetPipelineState(m_pipelineStateForRenderingWireframeOverlay.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffersOnCPU[getFrameIndex()]->GetGPUVirtualAddress());

    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->DrawIndexedInstanced(m_meshLoaded.getNumTriangles() * 3, 1, 0, 0, 0);

  }
  // TODO Implement me!
}

void MeshViewer::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  m_uiData.frameWorkWidth = f32(ImGui::GetMainViewport()->WorkSize.x);
  m_uiData.frameWorkHeight = f32(ImGui::GetMainViewport()->WorkSize.y);
  m_uiData.frameWorkCenter =
      f32v2(ImGui::GetMainViewport()->GetWorkCenter().x, ImGui::GetMainViewport()->GetWorkCenter().y);
  // TODO Implement me!
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::Text("Frame Width: %.2f", m_uiData.frameWorkWidth);
  ImGui::Text("Frame Height: %.2f", m_uiData.frameWorkHeight);
  ImGui::Text("Frame Aspect Ratio: %.2f", (m_uiData.frameWorkWidth) / (m_uiData.frameWorkHeight));
  ImGui::Text("Frame Center: (%.2f, %.2f)", ImGui::GetMainViewport()->GetWorkCenter().x,
              ImGui::GetMainViewport()->GetWorkCenter().y);
  ImGui::End();

  ImGui::Begin("Configurations", nullptr, imGuiFlags);
  if (ImGui::ColorEdit3("Background Color", &m_uiData.backgroundColor[0]))
  {
    std::cout << "Background color has changed!" << std::endl;
  }
  if (ImGui::Checkbox("Back-Face Culling", &m_uiData.backFaceCullingEnabled))
  {
    std::cout << std::format("Back-face culling mode has changed and is now {}!",
                             static_cast<bool>(m_uiData.backFaceCullingEnabled) ? "enabled" : "disabled")
              << std::endl;
  }
  if (ImGui::Checkbox("Wireframe Overlay", &m_uiData.wireFrameOverlayEnabled))
  {
    std::cout << std::format("Wire frame overlay mode has changed and is now {}!",
                             static_cast<bool>(m_uiData.wireFrameOverlayEnabled) ? "enabled" : "disabled")
              << std::endl;
  }
  f32v3 temporaryWireframeColor = m_uiData.wireframeOverlayColor;
  if (ImGui::ColorEdit3("Wireframe Overlay Color", &m_uiData.wireframeOverlayColor[0]))
  {
    if (m_uiData.wireFrameOverlayEnabled)
    {
      std::cout << "Wireframe overlay color has changed successfully!" << std::endl;
    }
    else
    {
      std::cout << "Wireframe overlay color can not be changed since it is disabled!" << std::endl;
      m_uiData.wireframeOverlayColor = temporaryWireframeColor;
    }
  }
  ImGui::End();

  // TODO Implement me!
}


// TODO Implement me!
// That is a hell lot of code :-)
// Enjoy!
