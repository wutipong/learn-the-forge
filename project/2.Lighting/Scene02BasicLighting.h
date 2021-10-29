#pragma once

#include "Scene.h"

#include "MainApp.h"

namespace Ch2Lighings {
namespace Scene02BasicLighting {
Scene Create();

void Update(float deltaTime);
void Draw(Cmd *cmd, int imageIndex);
auto Load(Renderer *pRenderer, SwapChain *pSwapChain, RenderTarget *pDepthBuffer) -> bool;
void Unload(Renderer *pRenderer);
void DrawUI();
void Init(Renderer *pRenderer);
void Exit(Renderer *pRenderer);
}; // namespace Scene02BasicLighting
} // namespace Ch2Lighings
