// 用于更新模拟
cbuffer CBUpdateSettings : register(b0)
{
    float g_WaveConstant0;
    float g_WaveConstant1;
    float g_WaveConstant2;
    float g_DisturbMagnitude;
    
    uint2 g_DisturbIndex;
    float2 g_Pad;
}

cbuffer CBTime : register(b0)
{
    float g_WaveTime;
}
RWTexture2D<float> g_PrevSolInput : register(u0);
RWTexture2D<float> g_CurrSolInput : register(u1);
RWTexture2D<float> g_Output : register(u2);

RWTexture2D<float> g_GerstnerOffset : register(u3);
RWTexture2D<float4> g_OriginalOffset : register(u4);
