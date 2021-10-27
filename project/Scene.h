#pragma once

#include <functional>

struct Cmd;
struct Renderer;
struct SwapChain;
struct RenderTarget;

struct Scene {
    std::function<bool(Renderer *pRenderer, SwapChain *pSwapChain, RenderTarget *pDepthBuffer)> Load;
    std::function<void(float)> Update;
    std::function<void(Cmd *cmd, int imageIndex)> Draw;
    std::function<void()> DrawUI;
    std::function<void(Renderer *pRenderer)> Unload;
    std::function<void(Renderer *pRenderer)> Init;
    std::function<void(Renderer *pRenderer)> Exit;
};
