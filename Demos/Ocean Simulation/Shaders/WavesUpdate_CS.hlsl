#include "Waves.hlsli"

float3 SampleOffset(float2 sample_pos)
{
    float x1 = uint(sample_pos.x);
    float x2 = uint(sample_pos.x + 1.0f);
    float y1 = uint(sample_pos.y);
    float y2 = uint(sample_pos.y + 1.0f);
        
    float3 s11 = g_OriginalOffset[uint2(x1, y1)].xyz;
    float3 s21 = g_OriginalOffset[uint2(x2, y1)].xyz;
    float3 s22 = g_OriginalOffset[uint2(x2, y2)].xyz;
    float3 s12 = g_OriginalOffset[uint2(x1, y2)].xyz;
        
    float d1 = x2 - x1;
    float d2 = y2 - y1;
    float wx1 = (x2 - sample_pos.x)/d1;
    float wx2 = (sample_pos.x - x1)/d1;
    float wy1 = (y2 - sample_pos.y)/d2;
    float wy2 = (sample_pos.y - y1)/d2;
        
    float3 sample_value =
        wx1 * wy1 * s11 + wx2 * wy1 * s21 +
        wx1 * wy2 * s12 + wx2 * wy2 * s22;
    return sample_value;
}

[numthreads(16, 16, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    // 我们不需要进行边界检验，因为:
    // --读取超出边界的区域结果为0，和我们对边界处理的需求一致
    // --对超出边界的区域写入并不会执行
    uint x = DTid.x;
    uint y = DTid.y;
    
    //波动方程部分
    float y_offset = g_WaveConstant0 * g_PrevSolInput[uint2(x, y)].x +
        g_WaveConstant1 * g_CurrSolInput[uint2(x, y)].x +
        g_WaveConstant2 * (
            g_CurrSolInput[uint2(x, y + 1)].x +
            g_CurrSolInput[uint2(x, y - 1)].x +
            g_CurrSolInput[uint2(x + 1, y)].x +
            g_CurrSolInput[uint2(x - 1, y)].x);
    
    //g_Output[uint2(x, y)] = y_offset;
    
    //Gerstner Waves部分
    float t = g_WaveTime; //?
    //方向
    float4 Dx = float4(0.9, 1, 0.5, 0.7);
    float4 Dy = float4(0.1, 0, 0.2, -0.3);
    //空间角速度
    float4 L = float4(10000, 10000, 1000, 1000);
    //时间角速度
    float4 phi = float4(1, 2, 1, 2);
    //振幅
    float4 A = float4(3, 4, 2, 2);
    //压缩因子
    //float4 Q = float4(0.05f, 0.5f, 0.1f, 0.8f);
    float4 Q = float4(0.1f, 0.1f, 0.1f, 0.1f);

    float4 Phase;
    
    int wave_num = 4;
    float PI = 3.1415926f;
    float g = 9.8f;
    
    float4 w;
    for (int i = 0; i < wave_num; i++)
    {
        w[i] = sqrt(g * 2 * PI / L[i]);
        Q[i] /= (w[i] * A[i]);
        Phase[i] = w[i] * (Dx[i] * x + Dy[i] * y) + phi[i] * t;
    }
    
    float4 sinp;
    float4 cosp;
    
    for (int i = 0; i < wave_num; i++)
    {
        sinp[i] = sin(Phase[i]);
        cosp[i] = cos(Phase[i]);
    }
    //两平面方向为xy //z为高度
    float3 offset = float3(x, y, 0.0f);
    
    for (int i = 0; i < wave_num; i++)
    {
        offset.xy += Q[i] * A[i] * float2(Dx[i], Dy[i]) * cosp[i];
        offset.z += A[i] * sinp[i];
    }
    
    g_OriginalOffset[uint2(x, y)] = float4(offset, 0);
    
    AllMemoryBarrierWithGroupSync();
    
    //偏移逼近
    float2 sample_pos = float2(x, y);
    for (int i = 0; i < 4; i++)
    {
        float2 dr = sample_pos - SampleOffset(sample_pos).xy;
        sample_pos += dr * 0.618;
    }
    g_GerstnerOffset[uint2(x, y)] = SampleOffset(sample_pos).z;
    g_Output[uint2(x, y)] = g_GerstnerOffset[uint2(x, y)];
}
