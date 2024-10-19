#include "TriangleApp.h"
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/types.hpp>
#include <iostream>

using namespace gims;

int main(int /* argc*/, char /* **argv */)
{
  // Instantiating a configuration for our D3D12-app
  gims::DX12AppConfig config;
  // Changing the title of the window
  config.title    = L"Coburg Mesh Viewer";
  // Deactivating the vertical synchronization
  config.useVSync = false;
  // Activating the creation of the debug context
  config.debug    = true;
  try
  {
    // Instantiating the mesh viewer
    MeshViewer meshViewer(config);
    // Running the mesh viewer
    meshViewer.run();
  }
  catch (const std::exception& exceptionThrown)
  {
    std::cerr << "Error: " << exceptionThrown.what() << "\n";
  }
  if (config.debug)
  {
    DX12Util::reportLiveObjects();
  }

  return 0;
}
