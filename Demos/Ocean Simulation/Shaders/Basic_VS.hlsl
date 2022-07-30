#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTex VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
    
    // 绘制水波时用到
    if (g_WavesEnabled)
    {
        // 使用映射到[0,1]x[0,1]区间的纹理坐标进行采样
        //vIn.posL.y += g_DisplacementMap.SampleLevel(g_SamLinearWrap, vIn.tex, 0.0f).r;
        vIn.posL.xyz += g_OriginalDisplacementMap.SampleLevel(g_SamLinearWrap, vIn.tex, 0.0f).rgb;
        //// 使用有限差分法估算法向量
        //float left = g_OriginalDisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(-1, 0)).g;
        //float right = g_OriginalDisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(1, 0)).g;
        //float top = g_OriginalDisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(0, -1)).g;
        //float bottom = g_OriginalDisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(0, 1)).g;
        vIn.normalL = g_NormalMap.SampleLevel(g_SamLinearWrap, vIn.tex, 0.0f).rgb;
    }
    else
    {
        //debug
        vIn.posL.y = (g_OriginalDisplacementMap.SampleLevel(g_SamLinearWrap, vIn.tex, 0.0f).xy) * 1;
    }
    
    vector posW = mul(float4(vIn.posL, 1.0f), g_World);

    vOut.posW = posW.xyz;
    vOut.posH = mul(posW, g_ViewProj);
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    vOut.tex = mul(float4(vIn.tex, 0.0f, 1.0f), g_TexTransform).xy;
    return vOut;
}