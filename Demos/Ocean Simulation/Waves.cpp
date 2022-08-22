#include "Waves.h"
#include <ModelManager.h>
#include <TextureManager.h>
#include <DXTrace.h>
#include "d3dApp.h"

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace Microsoft::WRL;

uint32_t Waves::RowCount() const
{
    return m_NumRows;
}

uint32_t Waves::ColumnCount() const
{
    return m_NumCols;
}

void Waves::InitResource(ID3D11Device* device, uint32_t rows, uint32_t cols,
    float texU, float texV, float timeStep, float spatialStep,
    float waveSpeed, float damping, float flowSpeedX, float flowSpeedY, bool cpuWrite)
{
    m_NumRows = rows;
    m_NumCols = cols;

    m_TexU = texU;
    m_TexV = texV;

    //m_TimeStep = timeStep;
    //m_SpatialStep = spatialStep;
    m_WindSpeedX = flowSpeedX;
    m_WindSpeedY = flowSpeedY;
    //m_AccumulateTime = 0.0f;

    //float d = damping * timeStep + 2.0f;
    //float e = (waveSpeed * waveSpeed) * (timeStep * timeStep) / (spatialStep * spatialStep);
    //m_K1 = (damping * timeStep - 2.0f) / d;
    //m_K2 = (4.0f - 8.0f * e) / d;
    //m_K3 = (2.0f * e) / d;

    m_MeshData = Geometry::CreateGrid(XMFLOAT2((cols - 1) * spatialStep, (rows - 1) * spatialStep), XMUINT2(cols - 1, rows - 1), XMFLOAT2(1.0f, 1.0f));
    Model::CreateFromGeometry(m_Model, device, m_MeshData, cpuWrite);
    m_pModel = &m_Model;

    //if (TextureManager::Get().GetTexture("..\\Texture\\water2.dds") == nullptr)
    //    TextureManager::Get().CreateFromFile("..\\Texture\\water2.dds", false, true);
    ////m_Model.materials[0].Set<std::string>("$Diffuse", "..\\Texture\\water2.dds");
    m_Model.materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.0f, 0.6f, 1.0f, 1.0f));
    m_Model.materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.0f, 0.6f, 1.0f, 1.0f));
    m_Model.materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.0f, 0.6f, 1.0f, 1.0f));
    m_Model.materials[0].Set<float>("$Opacity", 0.5f);
    m_Model.materials[0].Set<float>("$SpecularPower", 32.0f);
    m_Model.materials[0].Set<XMFLOAT2>("$TexOffset", XMFLOAT2());
    m_Model.materials[0].Set<XMFLOAT2>("$TexScale", XMFLOAT2(m_TexU, m_TexV));
}

std::unique_ptr<EffectHelper> Ocean::m_pEffectHelper = nullptr;

void Ocean::InitResource(ID3D11Device* device,
    uint32_t rows, uint32_t cols, float texU, float texV,
    float timeStep, float spatialStep, float waveSpeed, float damping, float flowSpeedX, float flowSpeedY)
{
    // 初始化特效
    if (!m_pEffectHelper)
    {
        m_pEffectHelper = std::make_unique<EffectHelper>();
        std::vector<LPCWSTR> fileName
        {
            //L"Shaders/WavesDisturb_CS.cso",
            //L"Shaders/WavesUpdate_CS.cso",
            L"Shaders/OceanPrecompute_CS.cso",
            L"Shaders/OceanUpdate_CS.cso",
            L"Shaders/FFT_CS.cso",
            L"Shaders/AfterProcess_CS.cso"
        };
        std::vector<std::string> shaderName
        {
            //"WavesDisturb",
            //"WavesUpdate",
            "OceanPrecompute",
            "OceanUpdate",
            "FFT",
            "AfterProcess"
        };

        ComPtr<ID3DBlob> blob;

        auto pfileName = fileName.begin();
        auto pshaderName = shaderName.begin();
        for (; pfileName != fileName.end() && pshaderName != shaderName.end(); ++pfileName, ++pshaderName)
        {
            HR(D3DReadFileToBlob(*pfileName, blob.ReleaseAndGetAddressOf()));
            HR(m_pEffectHelper->AddShader(*pshaderName, device, blob.Get()));
        }

        EffectPassDesc passDesc;

        for (auto& shader : shaderName)
        {
            passDesc.nameCS = shader;
            HR(m_pEffectHelper->AddEffectPass(shader, device, &passDesc));
        }

    }

    Waves::InitResource(device, rows, cols, texU, texV, timeStep,
        spatialStep, waveSpeed, damping, flowSpeedX, flowSpeedY, false);

    //Gerstner Waves
    m_pGerstnerOffset = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    
    m_pOriginalOffsetTexture = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pNormalTexture = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);

    //Ocean
    m_pHTide0Buffer = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pHeight = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pDisplaceXZ = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pGrad = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32G32B32A32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);

}

void Ocean::Precompute(ID3D11DeviceContext* deviceContext, float L,float A, float G, float wind[2])
{
    m_L = L;
    m_A = A;
    m_G = G;
    m_pEffectHelper->GetConstantBufferVariable("N")->SetUInt(m_NumCols);
    m_pEffectHelper->GetConstantBufferVariable("kL")->SetFloat(L);
    m_pEffectHelper->GetConstantBufferVariable("A")->SetFloat(A);
    m_pEffectHelper->GetConstantBufferVariable("G")->SetFloat(G);
    m_pEffectHelper->GetConstantBufferVariable("wind")->SetFloatVector(2, wind);

    m_pEffectHelper->SetUnorderedAccessByName("g_h0Data", m_pHTide0Buffer->GetUnorderedAccess(), 0);

    auto pPass = m_pEffectHelper->GetEffectPass("OceanPrecompute");
    pPass->Apply(deviceContext);
    pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);

    // 清除绑定
    ID3D11UnorderedAccessView* nullUAVs[1]{};
    deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
}

void Ocean::OceanUpdate(ID3D11DeviceContext* deviceContext, float dt)
{
        static float t = 0;
        t += dt;
        m_pEffectHelper->GetConstantBufferVariable("Time")->SetFloat(t/10);
        m_pEffectHelper->GetConstantBufferVariable("N")->SetUInt(m_NumCols);
        m_pEffectHelper->GetConstantBufferVariable("kL")->SetFloat(m_L);
        m_pEffectHelper->GetConstantBufferVariable("A")->SetFloat(m_A);
        m_pEffectHelper->GetConstantBufferVariable("G")->SetFloat(m_G);

        m_pEffectHelper->SetUnorderedAccessBySlot(0, m_pHTide0Buffer->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessBySlot(1, m_pHeight->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessBySlot(2, m_pDisplaceXZ->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessBySlot(3, m_pGrad->GetUnorderedAccess(), 0);

        auto pPass = m_pEffectHelper->GetEffectPass("OceanUpdate");
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);

        // 清除绑定
        ID3D11UnorderedAccessView* nullUAVs[5]{};
        deviceContext->CSSetUnorderedAccessViews(0, 4, nullUAVs, nullptr);


        //IFFT
        //参数设置
        m_pEffectHelper->GetConstantBufferVariable("TargetsCount")->SetUInt(1);
        m_pEffectHelper->GetConstantBufferVariable("Direction")->SetUInt(false);
        m_pEffectHelper->GetConstantBufferVariable("Inverse")->SetUInt(true);
        m_pEffectHelper->GetConstantBufferVariable("Scale")->SetUInt(false);
        m_pEffectHelper->GetConstantBufferVariable("Permute")->SetUInt(false);
        
        //设置变换目标
        //高度偏移
        m_pEffectHelper->SetUnorderedAccessBySlot(0, m_pHeight->GetUnorderedAccess(), 0);
        //m_pEffectHelper->SetUnorderedAccessByName("g_Target", m_pHeightSpectrumRT->GetUnorderedAccess(), 0);
        pPass = m_pEffectHelper->GetEffectPass("FFT");
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
        //deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
        //另一方向
        m_pEffectHelper->GetConstantBufferVariable("Direction")->SetUInt(true);
        //m_pEffectHelper->SetUnorderedAccessByName("g_Target", m_pHeightSpectrumRT->GetUnorderedAccess(), 0);
        //m_pEffectHelper->SetUnorderedAccessBySlot(0, m_pHeightSpectrumRT->GetUnorderedAccess(), 0);
        //pPass = m_pEffectHelper->GetEffectPass("FFT");
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
        deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);

        //水平偏移X
        m_pEffectHelper->SetUnorderedAccessBySlot(0, m_pDisplaceXZ->GetUnorderedAccess(), 0);
        m_pEffectHelper->GetConstantBufferVariable("Direction")->SetUInt(false);
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
        //另一方向
        m_pEffectHelper->GetConstantBufferVariable("Direction")->SetUInt(true);
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
        deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);

        //水平偏移Z
        m_pEffectHelper->SetUnorderedAccessBySlot(0, m_pGrad->GetUnorderedAccess(), 0);
        m_pEffectHelper->GetConstantBufferVariable("Direction")->SetUInt(false);
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
        //另一方向
        m_pEffectHelper->GetConstantBufferVariable("Direction")->SetUInt(true);
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
        deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);

        //储存到统一贴图
        m_pEffectHelper->GetConstantBufferVariable("N")->SetUInt(1);
        m_pEffectHelper->GetConstantBufferVariable("HeightScale")->SetFloat(m_HightScale);
        m_pEffectHelper->GetConstantBufferVariable("Lambda")->SetFloat(m_Lambda);
        
        m_pEffectHelper->SetUnorderedAccessBySlot(0, m_pHeight->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessBySlot(1, m_pDisplaceXZ->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessBySlot(2, m_pGrad->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessBySlot(3, m_pOriginalOffsetTexture->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessBySlot(4, m_pNormalTexture->GetUnorderedAccess(), 0);
        
        pPass = m_pEffectHelper->GetEffectPass("AfterProcess");
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
        deviceContext->CSSetUnorderedAccessViews(0, 5, nullUAVs, nullptr);
        
}


//void Ocean::Update(ID3D11DeviceContext* deviceContext, float dt)
//{
//    //m_AccumulateTime += dt;
//    //auto& texOffset = m_Model.materials[0].Get<XMFLOAT2>("$TexOffset");
//    //texOffset.x += m_FlowSpeedX * dt;
//    //texOffset.y += m_FlowSpeedY * dt;
//    //// 仅仅在累积时间大于时间步长时才更新
//    //if (m_AccumulateTime > m_TimeStep)
//    //{
//    //    m_pEffectHelper->GetConstantBufferVariable("g_WaveConstant0")->SetFloat(m_K1);
//    //    m_pEffectHelper->GetConstantBufferVariable("g_WaveConstant1")->SetFloat(m_K2);
//    //    m_pEffectHelper->GetConstantBufferVariable("g_WaveConstant2")->SetFloat(m_K3);
//
//    //    m_pEffectHelper->SetUnorderedAccessByName("g_PrevSolInput", m_pPrevSolutionTexture->GetUnorderedAccess(), 0);
//    //    m_pEffectHelper->SetUnorderedAccessByName("g_CurrSolInput", m_pCurrSolutionTexture->GetUnorderedAccess(), 0);
//    //    m_pEffectHelper->SetUnorderedAccessByName("g_Output", m_pNextSolutionTexture->GetUnorderedAccess(), 0);
//
//    //    static float t = 0;
//    //    t += m_AccumulateTime;
//    //    m_pEffectHelper->GetConstantBufferVariable("g_WaveTime")->SetFloat(t);
//    //    m_pEffectHelper->SetUnorderedAccessByName("g_GerstnerOffset", m_pGerstnerOffset->GetUnorderedAccess(), 0);
//    //    m_pEffectHelper->SetUnorderedAccessByName("g_OriginalOffset", m_pOriginalOffsetTexture->GetUnorderedAccess(), 0);
//
//    //    auto pPass = m_pEffectHelper->GetEffectPass("WavesUpdate");
//    //    pPass->Apply(deviceContext);
//    //    pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);
//
//    //    // 清除绑定
//    //    ID3D11UnorderedAccessView* nullUAVs[5]{};
//    //    deviceContext->CSSetUnorderedAccessViews(0, 5, nullUAVs, nullptr);
//
//    //    //
//    //    // 对缓冲区进行Ping-pong交换以准备下一次更新
//    //    // 上一次模拟的缓冲区不再需要，用作下一次模拟的输出缓冲
//    //    // 当前模拟的缓冲区变成上一次模拟的缓冲区
//    //    // 下一次模拟的缓冲区变换当前模拟的缓冲区
//    //    //
//    //    m_pCurrSolutionTexture.swap(m_pNextSolutionTexture);
//    //    m_pPrevSolutionTexture.swap(m_pNextSolutionTexture);
//
//    //    m_AccumulateTime = 0.0f;        // 重置时间
//    //}
//}
//
//void Ocean::Disturb(ID3D11DeviceContext* deviceContext, uint32_t i, uint32_t j, float magnitude)
//{
//    //// 更新常量缓冲区
//    //m_pEffectHelper->GetConstantBufferVariable("g_DisturbMagnitude")->SetFloat(magnitude);
//    //uint32_t idx[2] = { j, i };
//    //m_pEffectHelper->GetConstantBufferVariable("g_DisturbIndex")->SetUIntVector(2, idx);
//
//    //m_pEffectHelper->SetUnorderedAccessByName("g_Output", m_pCurrSolutionTexture->GetUnorderedAccess(), 0);
//    //m_pEffectHelper->GetEffectPass("WavesDisturb")->Apply(deviceContext);
//
//    //deviceContext->Dispatch(1, 1, 1);
//
//    //// 清除绑定
//    //ID3D11UnorderedAccessView* nullUAVs[]{ nullptr };
//    //deviceContext->CSSetUnorderedAccessViews(m_pEffectHelper->MapUnorderedAccessSlot("g_Output"), 1, nullUAVs, nullptr);
//}

void Ocean::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
    // 设置位移贴图
    effect.SetTextureOriginalDisplacement(m_pOriginalOffsetTexture->GetShaderResource());
    effect.SetTextureNormal(m_pNormalTexture->GetShaderResource());
    effect.SetTextureDebug(m_pHeight->GetShaderResource());

    //effect.SetWavesStates(true, m_SpatialStep);

    GameObject::Draw(deviceContext, effect);

    // 立即撤下位移贴图的绑定跟关闭水波绘制状态
    effect.SetTextureDisplacement(nullptr);
    effect.SetTextureDebug(nullptr);
    //effect.SetWavesStates(false);
    effect.Apply(deviceContext);
}


