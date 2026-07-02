#include "Overlay.hpp"

#include "../../Manager/Globals/Globals.hpp"
#include "../Visuals/Visuals.hpp"
#include "../../Engine/ImGui/imgui.h"
#include "../../Engine/ImGui/imgui_impl_dx11.h"
#include "../../Engine/ImGui/imgui_impl_win32.h"

#include <d3d11.h>
#include <dxgi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

constexpr wchar_t kWindowClassName[] = L"AlgorithmOverlay";

}

bool Overlay::init() {
    if (!createWindow())
        return false;

    if (!createDevice()) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(device_, context_);

    running_ = true;
    return true;
}

void Overlay::deinit() {
    running_ = false;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanupDevice();

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));
}

void Overlay::update() {
    if (!running_)
        return;

    if (!processMessages()) {
        running_ = false;
        return;
    }

    renderFrame();
}

void Overlay::onDraw(ImDrawList* drawList) {
    if (globals.visuals)
        globals.visuals->draw(drawList);
}

bool Overlay::createWindow() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = wndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc))
        return false;

    const int width = GetSystemMetrics(SM_CXSCREEN);
    const int height = GetSystemMetrics(SM_CYSCREEN);

    hwnd_ = CreateWindowExW(
        WS_EX_TOPMOST,
        kWindowClassName,
        L"Algorithm Overlay",
        WS_POPUP,
        0, 0, width, height,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd_) {
        UnregisterClassW(kWindowClassName, wc.hInstance);
        return false;
    }

    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hwnd_, SW_SHOWDEFAULT);
    UpdateWindow(hwnd_);
    return true;
}

bool Overlay::createDevice() {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd_;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    D3D_FEATURE_LEVEL featureLevel{};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        &sd,
        &swapChain_,
        &device_,
        &featureLevel,
        &context_);

    if (hr == DXGI_ERROR_UNSUPPORTED) {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            featureLevels,
            static_cast<UINT>(std::size(featureLevels)),
            D3D11_SDK_VERSION,
            &sd,
            &swapChain_,
            &device_,
            &featureLevel,
            &context_);
    }

    if (FAILED(hr))
        return false;

    createRenderTarget();
    return true;
}

void Overlay::cleanupDevice() {
    cleanupRenderTarget();

    if (swapChain_) {
        swapChain_->Release();
        swapChain_ = nullptr;
    }
    if (context_) {
        context_->Release();
        context_ = nullptr;
    }
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
}

void Overlay::createRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain_->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer != 0)
        device_->CreateRenderTargetView(backBuffer, nullptr, &renderTarget_);

    backBuffer->Release();
}

void Overlay::cleanupRenderTarget() {
    if (renderTarget_) {
        renderTarget_->Release();
        renderTarget_ = nullptr;
    }
}

bool Overlay::processMessages() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_QUIT)
            return false;
    }
    return true;
}

void Overlay::renderFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    drawList->AddRectFilled(ImVec2(0.f, 0.f), displaySize, IM_COL32(0, 0, 0, 255));

    onDraw(drawList);

    ImGui::Render();

    constexpr float clearColor[4] = { 0.f, 0.f, 0.f, 1.f };
    context_->OMSetRenderTargets(1, &renderTarget_, nullptr);
    context_->ClearRenderTargetView(renderTarget_, clearColor);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    swapChain_->Present(1, 0);
}

LRESULT CALLBACK Overlay::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    auto* overlay = reinterpret_cast<Overlay*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_SIZE:
        if (overlay && overlay->device_ && wParam != SIZE_MINIMIZED) {
            overlay->cleanupRenderTarget();
            overlay->swapChain_->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            overlay->createRenderTarget();
        }
        return 0;

    case WM_DESTROY:
        if (overlay)
            overlay->running_ = false;
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
