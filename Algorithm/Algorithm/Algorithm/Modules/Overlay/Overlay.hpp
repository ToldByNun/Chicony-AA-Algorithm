#ifndef OVERLAY_HPP
#define OVERLAY_HPP

#include "../../Manager/Classmanager/Classmanager.hpp"

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

struct ImDrawList;

class Overlay : public IManagedClass {
public:
    bool init() override;
    void deinit() override;
    void update() override;

    [[nodiscard]] bool isRunning() const noexcept { return running_; }

protected:
    virtual void onDraw(ImDrawList* drawList);

private:
    bool createWindow();
    bool createDevice();
    void cleanupDevice();
    void createRenderTarget();
    void cleanupRenderTarget();
    bool processMessages();
    void renderFrame();

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* context_ = nullptr;
    IDXGISwapChain* swapChain_ = nullptr;
    ID3D11RenderTargetView* renderTarget_ = nullptr;
    bool running_ = false;
};

#endif // OVERLAY_HPP
