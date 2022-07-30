RWTexture2D<float4> g_HeightSpectrumRT : register(u0);
RWTexture2D<float4> g_DisplaceXSpectrumRT : register(u1);
RWTexture2D<float4> g_DisplaceZSpectrumRT : register(u2);
RWTexture2D<float4> g_DisplaceRT : register(u3);

cbuffer CBGerstner : register(b0)
{
    uint N;
    float HeightScale;
    float Lambda;
    float pad;
}

[numthreads(16, 16, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    float y = (g_HeightSpectrumRT[id.xy].x) / (N * N) * HeightScale; //高度
    float x = (g_DisplaceXSpectrumRT[id.xy].x) / (N * N) * Lambda; //x轴偏移
    float z = (g_DisplaceZSpectrumRT[id.xy].x) / (N * N) * Lambda; //z轴偏移
    
    //g_HeightSpectrumRT[id.xy] = float4(y, y, y, 0);
    //g_DisplaceXSpectrumRT[id.xy] = float4(x, x, x, 0);
    //g_DisplaceZSpectrumRT[id.xy] = float4(z, z, z, 0);
    g_DisplaceRT[id.xy] = float4(x, y, z, 0);
}