#pragma once

#include "Scene.h"

#include "MainApp.h"

namespace Scene01Colors {
Scene Create();

void Update(float deltaTime);
void Draw(Cmd *cmd, int imageIndex);
auto Load(Renderer *pRenderer, SwapChain *pSwapChain) -> bool;
void Unload(Renderer *pRenderer);
void DrawUI();
void Init(Renderer *pRenderer);
void Exit(Renderer *pRenderer);
}; // namespace TestScene
