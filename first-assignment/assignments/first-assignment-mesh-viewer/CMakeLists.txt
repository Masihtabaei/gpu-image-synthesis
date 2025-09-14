include("../../CreateApp.cmake")
set(SOURCES "./src/main.cpp" "./src/mesh-viewer.cpp" "./include/mesh-viewer.h")
set(SHADERS "./shaders/mesh-viewer.hlsl")
create_app(first-assignment-mesh-viewer "${SOURCES}" "${SHADERS}")

