#include "Scene01Colors.h"
#include <array>

#include <Common_3/OS/Interfaces/ICameraController.h>
#include <Common_3/OS/Interfaces/IInput.h>
#include <Common_3/OS/Interfaces/IUI.h>
#include <Common_3/Renderer/IResourceLoader.h>

// Based on https://learnopengl.com/Lighting/Colors

using namespace Ch2Lighings::Scene01Colors;

namespace {
#pragma pack(push, 1)
struct UniformBlock {
    mat4 model;
    mat4 view;
    mat4 projection;

    float3 objectColor;
    float3 lightColor;
};
#pragma pack(pop)

Shader *lightingShader = nullptr;
Shader *lightCubeShader = nullptr;

RootSignature *pRootSignature = nullptr;
Pipeline *pCubePipeline = nullptr;
Pipeline *pLightPipeline = nullptr;

std::array<Buffer *, ImageCount> pCubeUniformBuffers = {nullptr};
DescriptorSet *pCubeUniformsDS = {nullptr};

std::array<Buffer *, ImageCount> pLightUniformBuffers = {nullptr};
DescriptorSet *pLightUniformsDS = {nullptr};

ICameraController *pCameraController = nullptr;
float3 lightPos{1.2f, 1.0f, 2.0f};
float3 objectColor{1.0f, 0.5f, 0.31f};
float3 lightColor{1.0f, 1.0f, 1.0f};

Buffer *pVerticesBuffer;
} // namespace


Scene Ch2Lighings::Scene01Colors::Create() {
    Scene out;

    out.Draw = Draw;
    out.Load = Load;
    out.DrawUI = DrawUI;
    out.Unload = Unload;
    out.Update = Update;
    out.Init = Init;
    out.Exit = Exit;

    return out;
}

static bool onCameraInput(InputActionContext *ctx, InputBindings::Binding binding) {
    if (uiIsFocused() || !(*ctx->pCaptured)) {
        return true;
    }

    switch (binding) {
    case InputBindings::FLOAT_LEFTSTICK:
        pCameraController->onMove(ctx->mFloat2);
        break;

    case InputBindings::FLOAT_RIGHTSTICK:
        pCameraController->onRotate(ctx->mFloat2);
        break;

    case InputBindings::BUTTON_NORTH:
        pCameraController->resetView();
    }

    return true;
};

void Ch2Lighings::Scene01Colors::Init(Renderer *pRenderer) {

    {
        ShaderLoadDesc desc{};
        desc.mStages[0] = {"2.1.colors.vert", nullptr, 0};
        desc.mStages[1] = {"2.1.colors.frag", nullptr, 0};

        addShader(pRenderer, &desc, &lightingShader);
    }

    {
        ShaderLoadDesc desc{};
        desc.mStages[0] = {"2.1.light_cube.vert", nullptr, 0};
        desc.mStages[1] = {"2.1.light_cube.frag", nullptr, 0};

        addShader(pRenderer, &desc, &lightCubeShader);
    }

    {
        float *pVertices;
        int vertexCount;

        generateCuboidPoints(&pVertices, &vertexCount);
        uint64_t cubeDataSize = vertexCount * sizeof(float);

        BufferLoadDesc desc = {};
        desc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        desc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
        desc.mDesc.mSize = cubeDataSize;
        desc.pData = pVertices;
        desc.ppBuffer = &pVerticesBuffer;

        addResource(&desc, NULL);
        tf_free(pVertices);
    }

    {
        RootSignatureDesc rootDesc = {};
        rootDesc.mStaticSamplerCount = 0;
        rootDesc.mShaderCount = 1;
        rootDesc.ppShaders = &lightingShader;
        addRootSignature(pRenderer, &rootDesc, &pRootSignature);
    }

    CameraMotionParameters cmp{16.0f, 10.0f, 20.0f};
    vec3 camPos{0.0f, 0.0f, 3.0f};
    vec3 lookAt{vec3(0)};

    pCameraController = initFpsCameraController(camPos, lookAt);
    pCameraController->setMotionParameters(cmp);

    {
        InputActionDesc actionDesc = {
            InputBindings::FLOAT_RIGHTSTICK,
            [](InputActionContext *ctx) { return onCameraInput(ctx, InputBindings::FLOAT_RIGHTSTICK); },
            NULL,
            2.0f,
            20.0f,
            0.05f};
        addInputAction(&actionDesc);
    }
    {
        InputActionDesc actionDesc = {
            InputBindings::FLOAT_LEFTSTICK,
            [](InputActionContext *ctx) { return onCameraInput(ctx, InputBindings::FLOAT_LEFTSTICK); },
            NULL,
            2.0f,
            20.0f,
            0.1f};
        addInputAction(&actionDesc);
    }
    {
        InputActionDesc actionDesc = {
            InputBindings::BUTTON_NORTH,
            [](InputActionContext *ctx) { return onCameraInput(ctx, InputBindings::BUTTON_NORTH); },
        };
        addInputAction(&actionDesc);
    }
    {
        DescriptorSetDesc desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, ImageCount};
        addDescriptorSet(pRenderer, &desc, &pCubeUniformsDS);
    }

    {
        DescriptorSetDesc desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME, ImageCount};
        addDescriptorSet(pRenderer, &desc, &pLightUniformsDS);
    }
}

void Ch2Lighings::Scene01Colors::Update(float deltaTime) { pCameraController->update(deltaTime); }

void Ch2Lighings::Scene01Colors::Draw(Cmd *cmd, int imageIndex) {
    mat4 viewMat = pCameraController->getViewMatrix();
    const float aspectInverse = (float)AppInstance()->mSettings.mHeight / (float)AppInstance()->mSettings.mWidth;
    const float horizontal_fov = PI / 2.0f;
    mat4 projMat = mat4::perspective(horizontal_fov, aspectInverse, 1000.0f, 0.1f);

    {
        UniformBlock uniform;
        uniform.model = mat4::identity();
        uniform.projection = projMat;
        uniform.view = viewMat;
        uniform.lightColor = lightColor;
        uniform.objectColor = objectColor;

        BufferUpdateDesc cubeUniformUpdate = {pCubeUniformBuffers[imageIndex]};
        beginUpdateResource(&cubeUniformUpdate);
        *(UniformBlock *)cubeUniformUpdate.pMappedData = uniform;
        endUpdateResource(&cubeUniformUpdate, NULL);
    }
    {
        UniformBlock uniform;

        uniform.model = mat4::translation(f3Tov3(lightPos)) * mat4::scale(vec3{0.2f});
        uniform.projection = projMat;
        uniform.view = viewMat;
        uniform.lightColor = lightColor;
        uniform.objectColor = objectColor;

        BufferUpdateDesc lightUniformUpdate = {pLightUniformBuffers[imageIndex]};
        beginUpdateResource(&lightUniformUpdate);
        *(UniformBlock *)lightUniformUpdate.pMappedData = uniform;
        endUpdateResource(&lightUniformUpdate, NULL);
    }

    // cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Skybox");
    const uint32_t stride = sizeof(float) * 6;
    cmdBindPipeline(cmd, pCubePipeline);
    cmdBindDescriptorSet(cmd, imageIndex, pCubeUniformsDS);
    cmdBindVertexBuffer(cmd, 1, &pVerticesBuffer, &stride, NULL);
    cmdDraw(cmd, 36, 0);
    // cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

    // cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Skybox");
    cmdBindPipeline(cmd, pLightPipeline);
    cmdBindDescriptorSet(cmd, imageIndex, pLightUniformsDS);
    cmdBindVertexBuffer(cmd, 1, &pVerticesBuffer, &stride, NULL);
    cmdDraw(cmd, 36, 0);
    // cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
}

void Ch2Lighings::Scene01Colors::DrawUI() {}

bool Ch2Lighings::Scene01Colors::Load(Renderer *pRenderer, SwapChain *pSwapChain, RenderTarget *pDepthBuffer) {
    {
        BufferLoadDesc ubDesc = {};
        ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        ubDesc.mDesc.mSize = sizeof(UniformBlock);
        ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        ubDesc.pData = NULL;

        for (auto &buffer : pCubeUniformBuffers) {
            ubDesc.ppBuffer = &buffer;
            addResource(&ubDesc, NULL);
        }
    }

    {
        BufferLoadDesc ubDesc = {};
        ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
        ubDesc.mDesc.mSize = sizeof(UniformBlock);
        ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
        ubDesc.pData = NULL;

        for (auto &buffer : pLightUniformBuffers) {
            ubDesc.ppBuffer = &buffer;
            addResource(&ubDesc, NULL);
        }
    }

    waitForAllResourceLoads();

    for (uint32_t i = 0; i < ImageCount; ++i) {
        DescriptorData params{};
        auto size = sizeof(UniformBlock);
        params.pName = "uniformBlock";
        params.ppBuffers = &pCubeUniformBuffers[i];
        updateDescriptorSet(pRenderer, i, pCubeUniformsDS, 1, &params);

        params.ppBuffers = &pLightUniformBuffers[i];
        updateDescriptorSet(pRenderer, i, pLightUniformsDS, 1, &params);
    }

    {
        RasterizerStateDesc rasterizerStateDesc = {};
        rasterizerStateDesc.mCullMode = CULL_MODE_FRONT;

        DepthStateDesc depthStateDesc = {};
        depthStateDesc.mDepthTest = true;
        depthStateDesc.mDepthWrite = true;
        depthStateDesc.mDepthFunc = CMP_GEQUAL;

        BlendStateDesc blendStateAlphaDesc = {};
        blendStateAlphaDesc.mSrcFactors[0] = BC_SRC_ALPHA;
        blendStateAlphaDesc.mDstFactors[0] = BC_ONE_MINUS_SRC_ALPHA;
        blendStateAlphaDesc.mBlendModes[0] = BM_ADD;
        blendStateAlphaDesc.mSrcAlphaFactors[0] = BC_ONE;
        blendStateAlphaDesc.mDstAlphaFactors[0] = BC_ZERO;
        blendStateAlphaDesc.mBlendAlphaModes[0] = BM_ADD;
        blendStateAlphaDesc.mMasks[0] = ALL;
        blendStateAlphaDesc.mRenderTargetMask = BLEND_STATE_TARGET_0;
        blendStateAlphaDesc.mIndependentBlend = false;

        PipelineDesc desc = {};
        desc.mType = PIPELINE_TYPE_GRAPHICS;

        // layout and pipeline for sphere draw
        VertexLayout vertexLayout = {};
        vertexLayout.mAttribCount = 2;
        vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[0].mBinding = 0;
        vertexLayout.mAttribs[0].mLocation = 0;
        vertexLayout.mAttribs[0].mOffset = 0;
        vertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
        vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[1].mBinding = 0;
        vertexLayout.mAttribs[1].mLocation = 1;
        vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);

        GraphicsPipelineDesc &pipelineSettings = desc.mGraphicsDesc;
        pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        pipelineSettings.mRenderTargetCount = 1;
        pipelineSettings.pDepthState = &depthStateDesc;
        pipelineSettings.pBlendState = &blendStateAlphaDesc;
        pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        pipelineSettings.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        pipelineSettings.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
        pipelineSettings.pRootSignature = pRootSignature;
        pipelineSettings.pVertexLayout = &vertexLayout;
        pipelineSettings.pRasterizerState = &rasterizerStateDesc;
        pipelineSettings.pShaderProgram = lightCubeShader;

        addPipeline(pRenderer, &desc, &pLightPipeline);

        pipelineSettings.pShaderProgram = lightingShader;

        addPipeline(pRenderer, &desc, &pCubePipeline);
    }
    return true;
}

void Ch2Lighings::Scene01Colors::Unload(Renderer *pRenderer) {

    for (auto &buffer : pCubeUniformBuffers) {
        removeResource(buffer);
    }

    for (auto &buffer : pLightUniformBuffers) {
        removeResource(buffer);
    }

    removePipeline(pRenderer, pCubePipeline);
    removePipeline(pRenderer, pLightPipeline);
}

void Ch2Lighings::Scene01Colors::Exit(Renderer *pRenderer) {
    exitCameraController(pCameraController);
    removeRootSignature(pRenderer, pRootSignature);

    removeResource(pVerticesBuffer);

    removeShader(pRenderer, lightingShader);
    removeShader(pRenderer, lightCubeShader);

    removeDescriptorSet(pRenderer, pCubeUniformsDS);
    removeDescriptorSet(pRenderer, pLightUniformsDS);
}
