# Coburg Mesh And Scene Viewers

> [!NOTE]  
> This project was completed as part of the **GPU Image Synthesis** course during the **Winter Term 2024/25** of our undergraduate studies, offered by [Prof. Dr. Quirin Meyer](https://www.hs-coburg.de/en/personen/prof-dr-quirin-meyer/) at [Coburg University](https://www.hs-coburg.de/en/). It was originally developed on our university’s GitLab instance and later partially migrated here, so some elements like issues may be missing.

> [!IMPORTANT]  
> This project is licensed under the [MIT License](https://masihtabaei.dev/licenses/mit) and [CC-BY 4.0](https://masihtabaei.dev/licenses/cc-by-4-0).

## General Information

Sueprvisors: [Prof. Dr. Quirin Meyer](https://www.hs-coburg.de/en/personen/prof-dr-quirin-meyer/) and [Bastian Kuth](https://bloodwyn.github.io/#CV_Bastian_Kuth.pdf)

Here you will find both Coburg mesh and scene viewer projects.

The Coburg mesh viewer is capabale of:
- Loading a mesh stored in the Cogra binary format
- Applying a wireframe overly
- Applying lighting
- Applying shading (both flat and smooth)
- Applying a texture

In this project, simple vertex and pixel shaders were implemented.

The Coburg scene viewer is capable of:
- Loading a glTF scene
- Applying a wireframe overly
- Drawing axis-aligned bounding boxes
- Applying lighting
- Applying shading (both flat and smooth)
- Applying a texture

In addition to vertex and pixel shaders, we had also a simple compute shader to calculate axis-aligned bounding boxes on the GPU.


## Usage

```bash
git clone <repo-link>
cd first-assignment # or second-assignment
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=<vcpkg.cmake-file-path> ..
```

After generating the solution, you can open it and select and run the project desired. 

## Screenshots
<img width="400" height="400" alt="first-assignment-mesh-viewer_CHX7p73IQP" src="https://github.com/user-attachments/assets/c1f951dd-7edc-499d-b16e-f006d713fdd4" />
<img width="400" height="400" alt="first-assignment-mesh-viewer_RdtgermjTO" src="https://github.com/user-attachments/assets/d63d3176-a552-47ae-b346-7e9c51e162d0" />
<img width="400" height="400" alt="first-assignment-mesh-viewer_7hBpzpi6aR" src="https://github.com/user-attachments/assets/0063e1cd-531b-4528-8abc-ada204ffc312" />
<img width="400" height="400" alt="first-assignment-mesh-viewer_rriTYbaNpE" src="https://github.com/user-attachments/assets/e7a59a7d-8734-4649-b776-a130582fbe24" />
<img width="400" height="400" alt="first-assignment-mesh-viewer_jD7p3chqvG" src="https://github.com/user-attachments/assets/5a24db9b-ea28-4757-83be-641f81ee3580" />
<img width="400" height="400" alt="first-assignment-mesh-viewer_A5Hnlc1Zhj" src="https://github.com/user-attachments/assets/64fdca34-5f5e-47dc-b8a0-2b9c4657382a" />
<img width="400" height="400" alt="second-assignment-scene-graph-viewer_HNE1wtOGoN" src="https://github.com/user-attachments/assets/c386400a-759c-4836-b828-60aa101bc365" />
<img width="400" height="400" alt="second-assignment-scene-graph-viewer_hSi3E4t9sC" src="https://github.com/user-attachments/assets/3e8b26bc-efcb-4e41-b42d-8ebe3ab947fe" />
<img width="400" height="400" alt="second-assignment-scene-graph-viewer_KhSGECKPw5" src="https://github.com/user-attachments/assets/ef277212-0da2-405a-897c-b4fc03c7f89e" />
<img width="400" height="400" alt="second-assignment-scene-graph-viewer_KIhgXqEhq8" src="https://github.com/user-attachments/assets/bf1c995c-d493-4df9-b47e-12fb893fdab1" />
<img width="400" height="400" alt="second-assignment-scene-graph-viewer_tWvSETKSXe" src="https://github.com/user-attachments/assets/b0f5e953-062a-48f3-b7f7-664f1c5b9528" />
<img width="400" height="400" alt="second-assignment-scene-graph-viewer_wGGRbJ5HvA" src="https://github.com/user-attachments/assets/97fe04c6-4a95-4ab7-aa5b-38ac00573f59" />

## Acknowledgement

**Important:** See also license files within the `data`- and `gimslib`-directories for more precise information.
Special thanks go to:
- Prof. Dr. Quirin Meyer for his framework
- nothings for stb_image.c
- G-Truc Creation for GLM
- ocornut for Dear ImGui
- Microsoft for DirectX 12 and DirectX compiler
- The Stanford 3D Scanning Repository for the Stanford Bunny
- olmopotums for The Noble Craftsman
- SilkevdSmissen for WW2 Cityscene


