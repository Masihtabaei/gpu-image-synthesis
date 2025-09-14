#pragma once
#include "Scene.hpp"
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
using namespace gims;

/// <summary>
/// An app for viewing an Asset Importer Scene Graph.
/// </summary>
class SceneGraphViewerApp : public gims::DX12App
{
public:

  /// <summary>
  /// Creates the SceneGraphViewerApp and loads a scene.
  /// </summary>
  /// <param name="config">Configuration.</param>
  SceneGraphViewerApp(const DX12AppConfig config, const std::filesystem::path pathToScene);

  ~SceneGraphViewerApp() = default;

  /// <summary>
  /// Called whenever a new frame is drawn.
  /// </summary>
  virtual void onDraw();

  /// <summary>
  /// Draw UI onto of rendered result.
  /// </summary>
  virtual void onDrawUI();


private:

  /// <summary>
  /// Root signature connecting shader and GPU resources for our compute pipeline.
  /// </summary>

  void   createRootSignatureForComputePipeline();

  /// <summary>
  /// Root signature connecting shader and GPU resources.
  /// </summary>
  void createRootSignature();

  /// <summary>
  /// Creates the pipeline
  /// </summary>
  void createPipeline();


  /// <summary>
  /// Creates the pipeline for AABB rendering with mesh shaders
  /// </summary>
  void createMeshShaderPipeline();

    /// <summary>
  /// Creates the pipeline for AABB rendering with mesh shaders
  /// </summary>
  void createComputePipeline();

  /// <summary>
  /// Draws the scene.
  /// </summary>
  /// <param name="commandList">Command list to which we upload the buffer</param>
  void drawScene(const ComPtr<ID3D12GraphicsCommandList6>& commandList);

  /// <summary>
  /// Creates the scene's constant buffer
  /// </summary>
  void createSceneConstantBuffer();

  /// <summary>
  /// Updates the scene's constant buffer
  /// </summary>
  void updateSceneConstantBuffer();

  /// <summary>
  /// Strructs containing data related to the UI
  /// </summary>
  struct UiData
  {
    f32v3 m_backgroundColor = f32v3(0.25f, 0.25f, 0.25f);
    bool  m_wrapObjectsWithBoundingBoxes = false;
    f32v3 m_boundingBoxColor              = f32v3(0.5f, 0.5f, 0.5f);
    f32   m_lightDirectionXCoordinate = 0.0f;
    f32   m_lightDirectionYCoordinate = 0.0f;
    f32   m_lightIntensity = 3.0f;
    f32   m_fieldOfView = 45.0f;
    f32   m_nearPlane   = 1.0f / 256.0f;
    f32   m_farPlane    = 256.0f;
  };

  ComPtr<ID3D12PipelineState>      m_pipelineState;
  ComPtr<ID3D12PipelineState>      m_meshShaderPipelineState;
  ComPtr<ID3D12RootSignature>      m_rootSignature;
  ComPtr<ID3D12RootSignature>      m_rootSignatureForComputePipeline;
  std::vector<ConstantBufferD3D12> m_constantBuffers;
  gims::ExaminerController         m_examinerController;  
  Scene                            m_scene;
  UiData                           m_uiData;
};
