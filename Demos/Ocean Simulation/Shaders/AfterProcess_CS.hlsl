RWTexture2D<float4> g_Height : register(u0);
RWTexture2D<float4> g_DisplaceXZ : register(u1);
RWTexture2D<float4> g_Grad : register(u2);
RWTexture2D<float4> g_Displace : register(u3);
RWTexture2D<float4> g_Normal : register(u4);

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
    float y = (g_Height[id.xy].x) / (N * N) * HeightScale; //高度
    float x = (g_DisplaceXZ[id.xy].x) / (N * N) * Lambda; //x轴偏移
    float z = (g_DisplaceXZ[id.xy].z) / (N * N) * Lambda; //z轴偏移
    
    float3 normal = normalize(float3(-g_Grad[id.xy].x, 1, -g_Grad[id.xy].z));
    g_Displace[id.xy] = float4(x, y, z, 0);
    g_Normal[id.xy] = float4(normal, 0);
}