#include "TriangleApp.h"
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/types.hpp>
#include <iostream>

using namespace gims;

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.useVSync = false;
  config.debug    = true;
  try
  {
    MeshViewer meshViewer(config);
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
