#include "SceneGraphViewerApp.hpp"
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/types.hpp>
#include <iostream>

using namespace gims;

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.useVSync = false;
  config.debug    = true;
  config.title    = L"Scene Graph Viewer";
  try
  {
    const std::filesystem::path path = "../../../data/NobleCraftsman/scene.gltf";

    // Be careful! Number of threads and also conditions inside the mesh and compute shaders must be adjusted!
    // const std::filesystem::path path = "../../../data/CityScene/scene.gltf";

    SceneGraphViewerApp app(config, path);
    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
  }
  if (config.debug)
  {
    DX12Util::reportLiveObjects();
  }

  return 0;
}
