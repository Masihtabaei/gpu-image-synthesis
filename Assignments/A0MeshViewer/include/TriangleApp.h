#pragma once
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>

using namespace gims;


    
class MeshViewer : public gims::DX12App
{
public:
  MeshViewer(const DX12AppConfig config);

  ~MeshViewer();
  
  void         printInformationOfMeshToLoad();
  virtual void onDraw();
  virtual void onDrawUI();

private:

  struct UIData
  {
    f32v3 backgroundColor;
    f32v3 wireframeOverlayColor;
    f32 frameWorkWidth;
    f32   frameWorkHeight;
    f32v2 frameWorkCenter;
    bool  backFaceCullingEnabled;
    bool  wireFrameOverlayEnabled;
  };

  struct Vertex
  {
    f32v3 position;
    f32v3 normal;
  };


  struct ConstantBuffer
  {
    f32m4 mvp;
    //f32m4 modelViewMatrix;
    f32v4 wireframeOverlayColor;
  };

  void loadMesh(const CograBinaryMeshFile& meshToLoad);
  void loadVertices();
  void loadIndices();
  void loadNormals();

  CograBinaryMeshFile      m_meshLoaded;
  std::vector<Vertex>      m_vertexBufferOnCPU;
  std::vector<ui32>        m_indexBufferOnCPU;
  size_t                   m_vertexBufferOnCPUSizeInBytes;
  size_t                   m_indexBufferOnCPUSizeInBytes;

  gims::ExaminerController m_examinerController;
  f32m4                    m_normalizationTransformation;

  DX12AppConfig m_appConfig;

  ComPtr<ID3D12RootSignature> m_rootSignature;
  ComPtr<ID3D12PipelineState> m_pipelineStateForRenderingMeshes;
  ComPtr<ID3D12PipelineState> m_pipelineStateForRenderingWireframeOverlay;
  ComPtr<ID3D12PipelineState> m_pipelineStateForRenderingWithBackfaceCulling;
  ComPtr<ID3D12PipelineState> m_pipelineStateForRenderingWireframeOverlayWithBackfaceCulling;

  ComPtr<ID3D12Resource>      m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW    m_vertexBufferView;
  ComPtr<ID3D12Resource>      m_indexBuffer;
  D3D12_INDEX_BUFFER_VIEW     m_indexBufferView;

  std::vector<ComPtr<ID3D12Resource>> m_constantBuffersOnCPU;

  glm::mat2x3 calculateBoundingBox();
  f32v3 calculateCentroidOfMeshLoaded();
  f32m4 getNormalizationTransformation();

  void   createRootSignature();
  void   createPipelineForRenderingMeshes();
  void   createPipelineForRenderingWireframeOverlay();
  void   createPipelineforRenderingWithBackfaceCulling();
  void   createPipelineForRenderingWireframeOverlayWithBackfaceCulling();
  void   createTriangleMesh();
  void   doVerticesMakeSense();
  void   createConstantBuffers();
  void   updateConstantBuffers();

  UIData m_uiData;
};
