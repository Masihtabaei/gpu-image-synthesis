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
    bool  twoSidedLightingEnabled;
    bool  useTexture;
    f32v3 ambient;
    f32v3 diffuse;
    f32v3 specular;
    f32   exponent;
  };

  struct Vertex
  {
    f32v3 position;
    f32v3 normal;
    f32v2 UV;
  };


  struct ConstantBuffer
  {
    f32m4 mvp;
    f32m4 mv;
    f32v4 wireframeOverlayColor;
    f32v4 specularColor_and_Exponent;
    f32v4 ambientColor;
    f32v4 diffuseColor;
    ui32v1  flags;
  };


  void loadMesh(const CograBinaryMeshFile& meshToLoad);
  void loadVertices();
  void loadIndices();
  void loadNormals();
  void loadUVs();

  void createTexture();

  CograBinaryMeshFile      m_meshLoaded;
  std::vector<Vertex>      m_vertexBufferOnCPU;
  std::vector<ui32>        m_indexBufferOnCPU;
  size_t                   m_vertexBufferOnCPUSizeInBytes;
  size_t                   m_indexBufferOnCPUSizeInBytes;

  gims::ExaminerController m_examinerController;
  f32m4                    m_normalizationTransformation;

  DX12AppConfig m_appConfig;

  ComPtr<ID3D12RootSignature> m_rootSignature;
  std::vector<ComPtr<ID3D12PipelineState>> m_pipelineStates;

  ComPtr<ID3D12Resource>      m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW    m_vertexBufferView;
  ComPtr<ID3D12Resource>      m_indexBuffer;
  D3D12_INDEX_BUFFER_VIEW     m_indexBufferView;
  ComPtr<ID3D12Resource>       m_texture;
  ComPtr<ID3D12DescriptorHeap> m_shaderResourceView;

  std::vector<ComPtr<ID3D12Resource>> m_constantBuffersOnCPU;

  glm::mat2x3 calculateBoundingBox();
  f32v3 calculateCentroidOfMeshLoaded();
  f32m4 getNormalizationTransformation();

  void   createRootSignature();
  void   createPipelineForRenderingMeshes(bool backfaceCullingEnabled=false, bool wireFrameOverlayEnabled=false);
  void   createTriangleMesh();
  void   doVerticesMakeSense();
  void   createConstantBuffers();
  void   updateConstantBuffers();

  UIData m_uiData;
};
