#pragma once

#include "Base.h"

#if NAUI_PLATFORM_WINDOWS

#include "Renderer.h"
#include "Platform/Window.h"

#include <imgui_impl_dx11.h>

#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")

namespace Naui
{

class Direct3D11Image : public Image
{
public:
    Direct3D11Image(uint32_t width, uint32_t height, ID3D11Resource *resource, ID3D11ShaderResourceView* nativeHandle)
        : m_width(width), m_height(height), m_nativeHandle(nativeHandle), m_texture(resource) {}

    uint32_t GetWidth(void) const override { return m_width; }
    uint32_t GetHeight(void) const override { return m_height; }
    void *GetNativeHandle(void) const override { return m_nativeHandle; }

private:
    uint32_t m_width;
    uint32_t m_height;
    ID3D11ShaderResourceView* m_nativeHandle;
    ID3D11Resource *m_texture;
};

class Direct3D11Renderer : public Renderer
{
public:
    Direct3D11Renderer(const PlatformWindow &window);
    ~Direct3D11Renderer(void);

    void Begin(void) override;
    void End(void) override;

    void Resize(uint32_t width, uint32_t height) override;

    Image *CreateImage(uint32_t width, uint32_t height, const uint8_t *data) override;
    void DestroyImage(Image *image) override;

private:
    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D(void);
    void CreateRenderTarget(void);
    void CleanupRenderTarget(void);

    ID3D11Device *m_device = nullptr;
    ID3D11DeviceContext *m_deviceContext = nullptr;
    IDXGISwapChain *m_swapChain = nullptr;
    ID3D11RenderTargetView *m_renderTargetView = nullptr;
};

Direct3D11Renderer::Direct3D11Renderer(const PlatformWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.GetNativeHandle());
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        return;
    }
    ImGui_ImplDX11_Init(m_device, m_deviceContext);
}

Direct3D11Renderer::~Direct3D11Renderer(void)
{
    ImGui_ImplDX11_Shutdown();
    CleanupDeviceD3D();
}

bool Direct3D11Renderer::CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &featureLevel, &m_deviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &featureLevel, &m_deviceContext);
    if (res != S_OK)
        return false;

    IDXGIFactory* pSwapChainFactory;
    if (SUCCEEDED(m_swapChain->GetParent(IID_PPV_ARGS(&pSwapChainFactory))))
    {
        pSwapChainFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        pSwapChainFactory->Release();
    }

    CreateRenderTarget();
    return true;
}

void Direct3D11Renderer::CleanupDeviceD3D(void)
{
    CleanupRenderTarget();
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_deviceContext) { m_deviceContext->Release(); m_deviceContext = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

void Direct3D11Renderer::CreateRenderTarget(void)
{
    ID3D11Texture2D* pBackBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    m_device->CreateRenderTargetView(pBackBuffer, nullptr, &m_renderTargetView);
    pBackBuffer->Release();
}

void Direct3D11Renderer::CleanupRenderTarget(void)
{
    if (m_renderTargetView) { m_renderTargetView->Release(); m_renderTargetView = nullptr; }
}

void Direct3D11Renderer::Begin(void)
{
    if (!m_deviceContext || !m_renderTargetView)
        return;
    ImGui_ImplDX11_NewFrame();
}

void Direct3D11Renderer::End(void)
{
    if (!m_swapChain)
        return;
    
    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_swapChain->Present(1, 0);
}

void Direct3D11Renderer::Resize(uint32_t width, uint32_t height)
{
    if (!m_deviceContext || !m_swapChain)
        return;

    CleanupRenderTarget();
    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (SUCCEEDED(hr))
    {
        CreateRenderTarget();
    }
}

Image *Direct3D11Renderer::CreateImage(uint32_t width, uint32_t height, const uint8_t *data)
{
    if (!m_device || !data)
        return nullptr;

    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    initData.SysMemPitch = width * 4;
    initData.SysMemSlicePitch = 0;

    ID3D11Texture2D *texture = nullptr;
    HRESULT hr = m_device->CreateTexture2D(&textureDesc, &initData, &texture);
    if (FAILED(hr))
        return nullptr;

    ID3D11ShaderResourceView *shaderResourceView = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = m_device->CreateShaderResourceView(texture, &srvDesc, &shaderResourceView);
    texture->Release();

    if (FAILED(hr))
        return nullptr;

    Direct3D11Image *image = new Direct3D11Image(width, height, texture, shaderResourceView);
    return image;
}

void Direct3D11Renderer::DestroyImage(Image *image)
{
    if (!image)
        return;
    delete image;
}

}

#endif