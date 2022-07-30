#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

    m_TextureManager.Init(m_pd3dDevice.Get());
    m_ModelManager.Init(m_pd3dDevice.Get());

    // 务必先初始化所有渲染状态，以供下面的特效使用
    RenderStates::InitAll(m_pd3dDevice.Get());

    if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();

    m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
    m_pLitTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pDepthTexture->SetDebugObjectName("DepthTexture");
    m_pLitTexture->SetDebugObjectName("LitTexture");

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
    }
}

void GameApp::UpdateScene(float dt)
{

    // 获取子类
    auto cam3rd = std::dynamic_pointer_cast<ThirdPersonCamera>(m_pCamera);

    // ******************
    // 第三人称摄像机的操作
    //

    ImGuiIO& io = ImGui::GetIO();
    // 绕物体旋转
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        cam3rd->RotateX(io.MouseDelta.y * 0.01f);
        cam3rd->RotateY(io.MouseDelta.x * 0.01f);
    }
    cam3rd->Approach(-io.MouseWheel * 1.0f);

    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

    if (ImGui::Begin("Waves"))
    {
        static const char* wavemode_strs[] = {
           "DeBug",
            "Wave"
        };
        if (ImGui::Combo("Mode", &m_WavesMode, wavemode_strs, ARRAYSIZE(wavemode_strs)))
        {
            /*           if (m_WavesMode)
                           m_Ocean.InitResource(m_pd3dDevice.Get(), 1024, 1024, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.05f, 0.1f);*/
            if (m_WavesMode)
            {
                m_BasicEffect.SetWavesStates(true);
            }
            else
            {
                m_BasicEffect.SetWavesStates(false);
            }

        }
        static int rendermode = 1;
        static const char* rendermode_strs[] = {
            "Default",
            "Transparent",
            "Wireframe"
        };
        if (ImGui::Combo("RenderMode", &rendermode, rendermode_strs, ARRAYSIZE(rendermode_strs)))
        {
            if (rendermode == 0)
            {
                m_BasicEffect.SetRenderDefault();
            }
            else if (rendermode == 1)
            {
                m_BasicEffect.SetRenderTransparent();
            }
            else
            {
                m_BasicEffect.SetRenderWireframe();
            }
        }
        if (ImGui::Checkbox("Enable Fog", &m_EnabledFog))
        {
            m_BasicEffect.SetFogState(m_EnabledFog);
        }
        static float A = 1.0f;
        static float G = 9.8f;
        static float wind[] = { 1,0 };
        if (ImGui::SliderFloat("A", &A, 0, 10, "%.1f"))
        {
            m_Ocean.Precompute(m_pd3dImmediateContext.Get(), A, G, wind);
        }
        if (ImGui::SliderFloat("G", &G, 0, 100, "%.1f"))
        {
            m_Ocean.Precompute(m_pd3dImmediateContext.Get(), A, G, wind);
        }
        if (ImGui::SliderFloat2("Wind", wind, -2, 2, "%.1f"))
        {
            m_Ocean.Precompute(m_pd3dImmediateContext.Get(), A, G, wind);
        }
    }
    ImGui::End();

    ImGui::Render();

    // 更新波浪
    m_Ocean.OceanUpdate(m_pd3dImmediateContext.Get(), dt);

}

void GameApp::DrawScene()
{
    // 创建后备缓冲区的渲染目标视图
    if (m_FrameCount < m_BackBufferCount)
    {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_pRenderTargetViews[m_FrameCount].ReleaseAndGetAddressOf());
    }


    float clearColor[4] = { 0.25f, 0.74f, 0.64f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(GetBackBufferRTV(), clearColor);
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11RenderTargetView* pRTVs[1] = { GetBackBufferRTV() };
    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    D3D11_VIEWPORT viewport = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &viewport);

    // ******************
    // 1. 绘制不透明对象
    //
    //m_BasicEffect.SetRenderDefault();
    //m_Land.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);
    // ******************
    // 2. 绘制半透明/透明对象
    //
    static bool begin = true;
    if (begin)
    {
        m_BasicEffect.SetWavesStates(true);
        m_BasicEffect.SetFogState(m_EnabledFog);
        m_BasicEffect.SetRenderTransparent();
        begin = false;
    }
    m_Ocean.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}



bool GameApp::InitResource()
{
    // ******************
    // 初始化游戏对象
    //

    // ******************
    // 初始化水面波浪
    //
    m_Ocean.InitResource(m_pd3dDevice.Get(), 256, 256, 5.0f, 5.0f, 0.03f, 0.625f, 2.0f, 0.2f, 0.0f, 0.0f);
    float wind[] = { 1, 0 };
    m_Ocean.Precompute(m_pd3dImmediateContext.Get(),1,9.9, wind);

    // ******************
    // 初始化摄像机
    //
    auto camera = std::make_shared<ThirdPersonCamera>();
    m_pCamera = camera;

    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetTarget(XMFLOAT3(0.0f, 1.0f, 0.0f));
    camera->SetDistance(20.0f);
    camera->SetDistanceMinMax(10.0f, 200.0f);
    camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    camera->SetRotationX(XM_PIDIV4);

    m_BasicEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_BasicEffect.SetProjMatrix(camera->GetProjMatrixXM());

    // ******************
    // 初始化不会变化的值
    //

    // 方向光
    DirectionalLight dirLight[3]{};
    dirLight[0].ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    dirLight[0].diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
    dirLight[0].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);

    dirLight[1].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    dirLight[1].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    dirLight[1].specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
    dirLight[1].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);

    dirLight[2].ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    dirLight[2].diffuse = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
    dirLight[2].specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    dirLight[2].direction = XMFLOAT3(0.0f, -0.707f, -0.707f);
    for (int i = 0; i < 3; ++i)
        m_BasicEffect.SetDirLight(i, dirLight[i]);

    // ******************
    // 初始化雾效和绘制状态
    //

    m_BasicEffect.SetFogState(false);
    m_BasicEffect.SetFogColor(XMFLOAT4(0.3f, 0.75f, 0.75f, 1.0f));
    m_BasicEffect.SetFogStart(15.0f);
    m_BasicEffect.SetFogRange(135.0f);

    return true;
}

