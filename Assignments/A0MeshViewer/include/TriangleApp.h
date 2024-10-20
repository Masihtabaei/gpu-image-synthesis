#pragma once
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>

using namespace gims;

struct Vertex
{
  f32v3 position;
};
    
class MeshViewer : public gims::DX12App
{
public:
  MeshViewer(const DX12AppConfig config);

  ~MeshViewer();
  
  void         printInformationOfMeshLoaded();
  void         loadVertices();
  void         loadIndices();
  virtual void onDraw();
  virtual void onDrawUI();

private:
  
  // TODO Implement me!
  CograBinaryMeshFile      m_meshLoaded;
  std::vector<Vertex>      m_vertexBufferOnCPU;
  std::vector<ui32>        m_indexBufferOnCPU;
  size_t                   m_vertexBufferOnCPUSizeInBytes;
  size_t                   m_indexBufferOnCPUSizeInBytes;

  gims::ExaminerController m_examinerController;
  f32m4                    m_normalizationTransformation;

  struct UiData
  {
    f32v3 m_backgroundColor  = f32v3(0.25f, 0.25f, 0.25f);
  // TODO Implement me!
  };

  UiData m_uiData;

};
