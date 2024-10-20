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

namespace
{
// TODO Implement me!

f32m4 getNormalizationTransformation(f32v3 const* const positions, ui32 nPositions)
{
  // TODO Implement me!
  // The cast to void is just to avoid warnings!
  (void)nPositions;
  (void)positions;
  return f32m4(1);
}
} // namespace

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
{
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));
  CograBinaryMeshFile cbm("../../../data/bunny.cbm");
  m_meshLoaded = cbm;
   printInformationOfMeshLoaded();
  loadVertices();
  // TODO Implement me!

}

MeshViewer::~MeshViewer()
{
}


void MeshViewer::printInformationOfMeshLoaded()
{
  ui32 numberOfAttributes = m_meshLoaded.getNumAttributes();
  std::cout << std::format("The loaded mesh consists of {} vertices, resulting in {} position coordinates.\n"
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

void MeshViewer::loadVertices()
{
  CograBinaryMeshFile::FloatType* positionsPointer = m_meshLoaded.getPositionsPtr();
  for (ui32 i = 0; i < m_meshLoaded.getNumVertices() * 3; i += 3)
  {
    f32v3  currentPosition(
        positionsPointer[i],
        positionsPointer[i + 1],
        positionsPointer[i + 2]);
    Vertex currentVertex(currentPosition);
    m_vertexBufferCPU.push_back(currentVertex);
  }

   std::cout << std::format("Total number of {} vertices loaded successfully!", m_vertexBufferCPU.size()) << std::endl;
}

void MeshViewer::onDraw()
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

  // TODO Implement me!
  // Use this to get the transformation Matrix.
  // Of course, skip the (void). That is just to prevent warning, sinces I am not using it here (but you will have to!)
  (void)m_examinerController.getTransformationMatrix();

  const auto commandList = getCommandList();
  const auto rtvHandle   = getRTVHandle();
  const auto dsvHandle   = getDSVHandle();
  // TODO Implement me!

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
  // TODO Implement me!

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  // TODO Implement me!
}

void MeshViewer::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  // TODO Implement me!
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();
  // TODO Implement me!
}


// TODO Implement me!
// That is a hell lot of code :-)
// Enjoy!
