#include "SceneGraphViewerApp.hpp"
#include "SceneFactory.hpp"
#include <d3dx12/d3dx12.h>
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

SceneGraphViewerApp::SceneGraphViewerApp(const DX12AppConfig config, const std::filesystem::path pathToScene)
    : DX12App(config)
    , m_examinerController(true)
{

    // Setting an initial camera position so that the whole scene is visible
  m_examinerController.setTranslationVector(f32v3(0, -0.25f, 2.0f));

  createRootSignatureForComputePipeline();
  createRootSignature();
  createSceneConstantBuffer();
  createComputePipeline();


  const auto& cmds             = getCommandList();
  const auto& commandAllocator = getCommandAllocator();

  throwIfFailed(commandAllocator->Reset());
  throwIfFailed(cmds->Reset(commandAllocator.Get(), nullptr));

  cmds->SetPipelineState(m_pipelineState.Get());
  cmds->SetComputeRootSignature(m_rootSignatureForComputePipeline.Get());

  ComPtr<ID3D12Resource> calculatedAABBPointsReadBack;
  ComPtr<ID3D12Resource> inputAABB;
  ComPtr<ID3D12Resource> calculatedAABBPoints;
 

  m_scene = SceneGraphFactory::createFromAssImpScene(pathToScene, cmds, getDevice(), getCommandQueue(), calculatedAABBPointsReadBack, inputAABB, calculatedAABBPoints);
  waitForGPU();
  SceneGraphFactory::createSceneAABBs(m_scene, calculatedAABBPointsReadBack);

  createMeshShaderPipeline();
  createPipeline();

}

void SceneGraphViewerApp::onDraw()
{
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

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  drawScene(commandList);

  
}

void SceneGraphViewerApp::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  ImGui::Begin("Scene Information", nullptr, imGuiFlags);
  ImGui::Text("Frame time: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::Text("Frame Width: %d", getWidth());
  ImGui::Text("Frame Height: %d", getHeight());
  ImGui::Text("Frame Aspect Ratio: %.2f", getWidth() / ((f32)(getHeight())));
  ImGui::Text("Number of Meshes Available: %d", m_scene.getNumberOfMeshesAvailable());
  ImGui::Text("Number of Nodes Available: %d", m_scene.getNumberOfNodes());
  ImGui::Text("Number of Materials Available: %d", m_scene.getNumberOfMaterialsAvailable());
  ImGui::Text("Number of Textures Available: %d", m_scene.getNumberOfTexturesAvailable());
  ImGui::End();
  ImGui::Begin("Scene Configuration", nullptr, imGuiFlags);
  ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor[0]);
  ImGui::Checkbox("Wrap Objects With Bounding Boxes", &m_uiData.m_wrapObjectsWithBoundingBoxes);
  ImGui::ColorEdit3("Bounding Box Color", &m_uiData.m_boundingBoxColor[0]);
  ImGui::SliderFloat("Light Direction X Coordinate", &m_uiData.m_lightDirectionXCoordinate, -1.0f, 1.0f);
  ImGui::SliderFloat("Light Direction Y Coordinate", &m_uiData.m_lightDirectionYCoordinate, -1.0f, 1.0f);
  ImGui::SliderFloat("Light Intensity", &m_uiData.m_lightIntensity, 1.0f, 5.0f);
  ImGui::SliderFloat("Field of View", &m_uiData.m_fieldOfView, 0.1f, 90.0f);
  ImGui::SliderFloat("Near Plane", &m_uiData.m_nearPlane, 0.1f, 10.0f);
  ImGui::SliderFloat("Far Plane", &m_uiData.m_farPlane, 10.1f, 10000.0f);
  ImGui::End();
}


void SceneGraphViewerApp::createRootSignatureForComputePipeline()
{
  const uint8_t          NUMBER_OF_ROOT_PARAMETERS = 2;
  const uint8_t          NUMBER_OF_STATIC_SAMPLERS             = 0;
  CD3DX12_ROOT_PARAMETER parameters[NUMBER_OF_ROOT_PARAMETERS] = {};
  parameters[0].InitAsShaderResourceView(0);
  parameters[1].InitAsUnorderedAccessView(0);

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
  rootSignatureDescription.Init(NUMBER_OF_ROOT_PARAMETERS, parameters, NUMBER_OF_STATIC_SAMPLERS, nullptr,
                                D3D12_ROOT_SIGNATURE_FLAG_NONE);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignatureForComputePipeline));
}

void SceneGraphViewerApp::createRootSignature()
{
  const uint8_t NUMBER_OF_ROOT_PARAMETERS = 5;
  const uint8_t  NUMBER_OF_STATIC_SAMPLERS   = 1;


  // Assignment 1

  CD3DX12_DESCRIPTOR_RANGE range[1] = {
      {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0},
  };
  CD3DX12_ROOT_PARAMETER rootParameters[NUMBER_OF_ROOT_PARAMETERS] = {};
  rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
  rootParameters[1].InitAsConstants(16, 1, 0, D3D12_SHADER_VISIBILITY_ALL);
  rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  rootParameters[3].InitAsDescriptorTable(1, &range[0]);
  rootParameters[4].InitAsConstants(32, 3, 0, D3D12_SHADER_VISIBILITY_MESH);


  D3D12_STATIC_SAMPLER_DESC staticSamplerDescription = {};
  staticSamplerDescription.Filter                    = D3D12_FILTER_MIN_MAG_MIP_POINT;
  staticSamplerDescription.AddressU                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplerDescription.AddressV                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplerDescription.AddressW                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplerDescription.MipLODBias                = 0;
  staticSamplerDescription.MaxAnisotropy             = 0;
  staticSamplerDescription.ComparisonFunc            = D3D12_COMPARISON_FUNC_NEVER;
  staticSamplerDescription.BorderColor               = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  staticSamplerDescription.MinLOD                    = 0.0f;
  staticSamplerDescription.MaxLOD                    = D3D12_FLOAT32_MAX;
  staticSamplerDescription.ShaderRegister            = 0;
  staticSamplerDescription.RegisterSpace             = 0;
  staticSamplerDescription.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

  CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
  rootSignatureDescription.Init(NUMBER_OF_ROOT_PARAMETERS, rootParameters, NUMBER_OF_STATIC_SAMPLERS,
                                &staticSamplerDescription,
                                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
  
  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&rootSignatureDescription, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);
  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
}

void SceneGraphViewerApp::createComputePipeline()
{
  const auto computeShader =
        compileShader("../../../Assignments/second-assignment-scene-graph-viewer/Shaders/BoundingBoxComputeShader.hlsl", L"main", L"cs_6_0");
  D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};

  psoDesc.pRootSignature = m_rootSignatureForComputePipeline.Get();
  psoDesc.CS             = HLSLCompiler::convert(computeShader);
  psoDesc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;
  throwIfFailed(getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}


void SceneGraphViewerApp::createMeshShaderPipeline()
{
  D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureData = {};
  getDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureData, sizeof(featureData));
  if (featureData.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
  {
    std::cout << "OOPS! Mesh shader is not supported :(" << std::endl;
  }
  waitForGPU();

  const auto meshShader =
      compileShader(L"../../../Assignments/second-assignment-scene-graph-viewer/Shaders/BoundingBoxMeshShader.hlsl",
                    L"MS_main", L"ms_6_5");
  const auto pixelShader =
      compileShader(L"../../../Assignments/second-assignment-scene-graph-viewer/Shaders/BoundingBoxMeshShader.hlsl",
                    L"PS_main", L"ps_6_5");

  D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.pRootSignature                         = m_rootSignature.Get();
  psoDesc.MS                                     = HLSLCompiler::convert(meshShader);
  psoDesc.PS                                     = HLSLCompiler::convert(pixelShader);
  psoDesc.RasterizerState                        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode               = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode               = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                             = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DSVFormat                              = getDX12AppConfig().depthBufferFormat;
  psoDesc.DepthStencilState.DepthEnable          = TRUE;
  psoDesc.DepthStencilState.DepthFunc            = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask       = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable        = FALSE;
  psoDesc.SampleMask                             = UINT_MAX;
  psoDesc.NumRenderTargets                       = 1;
  psoDesc.RTVFormats[0]                          = getDX12AppConfig().renderTargetFormat;
  psoDesc.SampleDesc.Count                       = 1;

 auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

  D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
  streamDesc.pPipelineStateSubobjectStream = &psoStream;
  streamDesc.SizeInBytes                   = sizeof(psoStream);

  throwIfFailed(getDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_meshShaderPipelineState)));
}
void SceneGraphViewerApp::createPipeline()
{
  waitForGPU();
  const auto inputElementDescs = TriangleMeshD3D12::getInputElementDescriptors();

  const auto vertexShader = compileShader(L"../../../Assignments/second-assignment-scene-graph-viewer/Shaders/TriangleMesh.hlsl", L"VS_main", L"vs_6_0");
  const auto pixelShader=
      compileShader(L"../../../Assignments/second-assignment-scene-graph-viewer/Shaders/TriangleMesh.hlsl", L"PS_main", L"ps_6_0");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {inputElementDescs.data(), (ui32)inputElementDescs.size()};
  psoDesc.pRootSignature                     = m_rootSignature.Get();
  psoDesc.VS                                 = HLSLCompiler::convert(vertexShader);
  psoDesc.PS                                 = HLSLCompiler::convert(pixelShader);
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode           = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
  psoDesc.DepthStencilState.DepthEnable      = TRUE;
  psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable    = FALSE;
  psoDesc.SampleMask                         = UINT_MAX;
  psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                   = 1;
  psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
  psoDesc.SampleDesc.Count                   = 1;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void SceneGraphViewerApp::drawScene(const ComPtr<ID3D12GraphicsCommandList6>& cmdLst)
{
  updateSceneConstantBuffer();
  // Assignment 2
  // Assignment 6


  const auto currentConstantBuffer                   = m_constantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  const auto viewTransformation = m_examinerController.getTransformationMatrix();

  auto sceneAABB = m_scene.getAABB();
  auto sceneNormalizationTransformation = sceneAABB.getNormalizationTransformation();

  cmdLst->SetGraphicsRootSignature(m_rootSignature.Get());
  cmdLst->SetGraphicsRootConstantBufferView(0, currentConstantBuffer);

  // Rendering AABBs of meshes availale in the scene if needed
  if (m_uiData.m_wrapObjectsWithBoundingBoxes == true)
  {
    cmdLst->SetPipelineState(m_meshShaderPipelineState.Get());
    m_scene.addToCommandList(cmdLst, viewTransformation * sceneNormalizationTransformation, 1, 2, 3, 1);
  }

  // Rendering the meshes
  cmdLst->SetPipelineState(m_pipelineState.Get());
  m_scene.addToCommandList(cmdLst, viewTransformation * sceneNormalizationTransformation, 1, 2, 3, 0);



}

namespace
{
struct ConstantBuffer
{
  f32m4 projectionMatrix;
  f32v3 boundingBoxColor;
  bool  useNormalMapping;
  f32   m_lightDirectionXCoordinate;
  f32   m_lightDirectionYCoordinate;
  f32   m_lightIntensity;
};

} // namespace

void SceneGraphViewerApp::createSceneConstantBuffer()
{
  const ConstantBuffer cb {};
  const auto           frameCount = getDX12AppConfig().frameCount;
  m_constantBuffers.resize(frameCount);
  for (ui32 i = 0; i < frameCount; i++)
  {
    m_constantBuffers[i] = ConstantBufferD3D12(cb, getDevice());
  }
}

void SceneGraphViewerApp::updateSceneConstantBuffer()
{
  ConstantBuffer cb = {};
  cb.projectionMatrix =
      glm::perspectiveFovLH_ZO<f32>(glm::radians(m_uiData.m_fieldOfView), (f32)getWidth(), (f32)getHeight(), m_uiData.m_nearPlane, m_uiData.m_farPlane);

  cb.boundingBoxColor = m_uiData.m_boundingBoxColor;
  cb.m_lightDirectionXCoordinate = f32(m_uiData.m_lightDirectionXCoordinate);
  cb.m_lightDirectionYCoordinate = f32(m_uiData.m_lightDirectionYCoordinate);
  cb.m_lightIntensity            = f32(m_uiData.m_lightIntensity);

  m_constantBuffers[getFrameIndex()].upload(&cb);
}
