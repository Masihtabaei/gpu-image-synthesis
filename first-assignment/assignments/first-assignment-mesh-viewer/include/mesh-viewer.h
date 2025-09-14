#pragma once
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>

using namespace gims;

class MeshViewer : public gims::DX12App
{
public:
  // Constructor and destructor
  MeshViewer(const DX12AppConfig config);
  ~MeshViewer();

   /**
   * Prints information about the mesh loaded
   *
   * @param [in] CograBinaryMeshFile* Address (pointer) of the mesh loaded
   * @return void
   */
  // Method for printing some information about the mesh loaded
  void         printInformationOfMeshToLoad(const CograBinaryMeshFile* meshToLoad);
  
   /**
   * Specifies which primitives should get rendered
   *
   * @param void
   * @return void
   */
  virtual void onDraw();
  
   /**
   * Specifies which UI elements should get rendered 
   *
   * @param void
   * @return void
   */
  virtual void onDrawUI();

private:

   // Stores data related to the UI
  struct UIData
  {
    f32v3 backgroundColor;
    f32v3 wireframeOverlayColor;
    f32   frameWorkWidth;
    f32   frameWorkHeight;
    f32   frameAspectRatio;
    bool  backFaceCullingEnabled;
    bool  wireFrameOverlayEnabled;
    bool  twoSidedLightingEnabled;
    bool  useTexture;
    bool  flatShadingEnabled;
    f32v3 ambient;
    f32v3 diffuse;
    f32v3 specular;
    f32   lightDirectionXCoordinate;
    f32   lightDirectionYCoordinate;
    f32   exponent;
    f32   fieldOfView;
    f32   nearPlane;
    f32   farPlane;
    bool  cameraResetButtonClicked;
    bool  parametersResetButtonClicked;
  };

  // Stores attributes of a vertex
  struct Vertex
  {
    f32v3 position;
    f32v3 normal;
    f32v2 UV;
  };

  // Stores values of a constant buffer
  struct ConstantBuffer
  {
    f32m4  mvp;
    f32m4  mv;
    f32v4  wireframeOverlayColor;
    f32v4  specularColor_and_Exponent;
    f32v4  ambientColor;
    f32v4  diffuseColor;
    f32    lightDirectionXCoordinate;
    f32    lightDirectionYCoordinate;
    ui32v1 flags;
  };

  // Stores data related to the UI
   UIData m_uiData;

  // Stores controller of the camera
  gims::ExaminerController m_examinerController;

  // Stores normalization transformation of the mesh loaded
  f32m4 m_meshLoadedNormalizationTransformation;

  // Stores vertex buffer of the mesh loaded on the system memory (RAM)
  std::vector<Vertex> m_vertexBufferOnCPU;

  // Stores index buffer of the mesh loaded on the system memory (RAM)
  std::vector<ui32>   m_indexBufferOnCPU;

  // Stores size of the previously created vertex buffer residing in the system memory (RAM)
  size_t              m_vertexBufferOnCPUSizeInBytes;

  // Stores size of the previously created index buffer residing in the system memory (RAM)
  size_t              m_indexBufferOnCPUSizeInBytes;

  // Stores the root signature
  ComPtr<ID3D12RootSignature>              m_rootSignature;

  // Stores state of the pipeline (a. k. a. PSO)
  std::vector<ComPtr<ID3D12PipelineState>> m_pipelineStates;

  // Stores COM pointer for the vertex buffer residing in the GPU memory (VRAM)
  ComPtr<ID3D12Resource>       m_vertexBuffer;

  // Stores view of the vertex buffer residing in the GPU memory (VRAM)
  D3D12_VERTEX_BUFFER_VIEW     m_vertexBufferView;

  // Stores COM pointer for the index buffer residing in the GPU memory (VRAM)
  ComPtr<ID3D12Resource>       m_indexBuffer;

  // Stores view of the index buffer residing in the GPU memory (VRAM)
  D3D12_INDEX_BUFFER_VIEW      m_indexBufferView;

  // Stores COM pointer for the texture residing in the GPU memory (VRAM)
  ComPtr<ID3D12Resource>       m_texture;

  // Stores COM pointer for view of the shader resource
  ComPtr<ID3D12DescriptorHeap> m_shaderResourceView;

  // Stores COM pointers for the constant buffers
  std::vector<ComPtr<ID3D12Resource>> m_constantBuffersOnCPU;


   /**
   * Initializes data related to the UI
   *
   * @param void
   * @return void
   */
  void initializeUIData();

   /**
   * Initializes parameters of the camera
   *
   * @param void
   * @return void
   */
  void initializeCameraPosition();

   /**
   * Loads and initializes the mesh 
   *
   * @param [in] CograBinaryMeshFile* meshToLoad a Cogra-Binary-Mesh object
   * @return void
   */
  void loadMesh(const CograBinaryMeshFile* meshToLoad);

   /**
   * Loads positions of the mesh loaded
   *
   * @param [in] CograBinaryMeshFile* meshToLoad a Cogra-Binary-Mesh object
   * @return void
   */
  void loadVertices(const CograBinaryMeshFile* meshToLoad);

   /**
   * Loads indices of the mesh loaded
   *
   * @param [in] CograBinaryMeshFile* meshToLoad a Cogra-Binary-Mesh object
   * @return void
   */
  void loadIndices(const CograBinaryMeshFile* meshToLoad);

   /**
   * Loads normal vectors of the mesh loaded
   *
   * @param [in] CograBinaryMeshFile* meshToLoad a Cogra-Binary-Mesh object
   * @return void
   */
  void loadNormals(const CograBinaryMeshFile* meshToLoad);

   /**
   * Loads UV coordinates of the mesh loaded
   *
   * @param [in] CograBinaryMeshFile* meshToLoad a Cogra-Binary-Mesh object
   * @return void
   */
  void loadUVs(const CograBinaryMeshFile* meshToLoad);

   /**
   * Loads UV coordinates of the mesh loaded
   *
   * @return void
   */
  void createTexture();

     /**
   * Calculates coordinates of centroid of the mesh loaded
   *
   * @return f32v3 A vector containing coordinates of the centroid
   */
  f32v3 calculateCentroidOfMeshLoaded();

   /**
   * Calculates bounding box of the mesh loaded
   *
   * @return mat2x3 A matrix with two columns containing coordinates of the beginning and end of the bounding box
   */
  glm::mat2x3 calculateBoundingBox();

   /**
   * Calculates normalization transformation of the mesh loaded
   *
   * @return mat2x3 A matrix with two columns containing coordinates of the beginning and end of the bounding box
   */
  void       calculateNormalizationTransformation();
 
   /**
   * Creates the root signature
   *
   * @return void
   */
  void createRootSignature();

   /**
   * Creates a pipeline and assigns it to the member variable responsible for storing the current/active PSO
   *
   * @param [in] backfaceCullingEnabled Specifying whether back-face culling is desired
   * @param [in] wireFrameOverlayEnabled Specifying whether wireframe overlay is desired
   * @return void
   */
  void createPipelineForRenderingMeshes(bool backfaceCullingEnabled = false, bool wireFrameOverlayEnabled = false);

   /**
   * Creates a pipeline and assigns it to the member variable responsible for storing the current/active PSO
   *
   * @return void
   */
  void createTriangleMesh();

   /**
   * Creates constant buffers required for the three frames available
   *
   * @return void
   */
  void createConstantBuffers();

   /**
   * Updates content of the current constant buffer and uploads it on to the GPU memory (VRAM)
   *
   * @return void
   */
  void updateConstantBuffers();
};
