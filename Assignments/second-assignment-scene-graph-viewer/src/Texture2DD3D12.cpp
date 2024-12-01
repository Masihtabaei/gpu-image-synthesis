#include "Texture2DD3D12.hpp"
#include <d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>

using namespace gims;
namespace
{

ComPtr<ID3D12Resource> createTexture(void const* const data, ui32 textureWidth, ui32 textureHeight,
                                     const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  ComPtr<ID3D12Resource> textureResource;
  
  D3D12_RESOURCE_DESC textureDescription = {};
  textureDescription.MipLevels           = 1;
  textureDescription.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDescription.Width               = textureWidth;
  textureDescription.Height              = textureHeight;
  textureDescription.Flags               = D3D12_RESOURCE_FLAG_NONE;
  textureDescription.DepthOrArraySize    = 1;
  textureDescription.SampleDesc.Count    = 1;
  textureDescription.SampleDesc.Quality  = 0;
  textureDescription.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  const CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  throwIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDescription,
                                                D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&textureResource)));

  UploadHelper uploadHelper(device, GetRequiredIntermediateSize(textureResource.Get(), 0, 1));
  uploadHelper.uploadTexture(data, textureResource, textureWidth, textureHeight, commandQueue);

  return textureResource;
}
} // namespace

namespace gims
{
Texture2DD3D12::Texture2DD3D12(std::filesystem::path path, const ComPtr<ID3D12Device>& device,
                               const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  const auto fileName     = path.generic_string();
  const auto fileNameCStr = fileName.c_str();
  i32        textureWidth, textureHeight, textureComp;

  std::unique_ptr<ui8, void (*)(void*)> image(stbi_load(fileNameCStr, &textureWidth, &textureHeight, &textureComp, 4),
                                              &stbi_image_free);
  if (image.get() == nullptr)
  {
    throw std::exception("Error loading texture.");
  }

  m_textureResource = createTexture(image.get(), textureWidth, textureHeight, device, commandQueue);
}
Texture2DD3D12::Texture2DD3D12(ui8v4 const* const data, ui32 width, ui32 height, const ComPtr<ID3D12Device>& device,
                               const ComPtr<ID3D12CommandQueue>& commandQueue)

{
  m_textureResource = createTexture(data, width, height, device, commandQueue);
}

void Texture2DD3D12::addToDescriptorHeap(const ComPtr<ID3D12Device>&         device,
                                         const ComPtr<ID3D12DescriptorHeap>& descriptorHeap, i32 descriptorIndex) const
{

  D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDescription = {};
  shaderResourceViewDescription.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  shaderResourceViewDescription.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  shaderResourceViewDescription.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  shaderResourceViewDescription.Texture2D.MipLevels             = 1;
  shaderResourceViewDescription.Texture2D.MostDetailedMip       = 0;
  shaderResourceViewDescription.Texture2D.ResourceMinLODClamp = 0.0f;

  CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
  descriptorHandle = descriptorHandle.Offset(
      descriptorIndex, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
  device->CreateShaderResourceView(m_textureResource.Get(), &shaderResourceViewDescription, descriptorHandle);
  // Assignment 8
}
} // namespace gims
