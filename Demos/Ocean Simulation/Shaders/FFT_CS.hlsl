#include "FFT.hlsli"

groupshared float4 buffer[2][SIZE];

float2 ComplexMult(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

void ButterflyValues(uint step, uint index, out uint2 indices, out float2 twiddle)
{
    const float twoPi = 6.28318530718;
    uint b = Size >> (step + 1);
    uint w = b * (index / b);
    uint i = (w + index) % Size;
    sincos(-twoPi / Size * w, twiddle.y, twiddle.x);
    if (Inverse)
        twiddle.y = -twiddle.y;
    indices = uint2(i, i + b);
}

float4 DoFft(uint threadIndex, float4 input)
{
    buffer[0][threadIndex] = input;
    GroupMemoryBarrierWithGroupSync();
    bool flag = false;
    
    [unroll(LOG_SIZE)]
    for (uint step = 0; step < LOG_SIZE; step++)
    {
        uint2 inputsIndices;
        float2 twiddle;
        ButterflyValues(step, threadIndex, inputsIndices, twiddle);
        float4 v = buffer[flag][inputsIndices.y];
        //一次执行两个计算
        //ak = a0 + w*a1  
        buffer[!flag][threadIndex] = buffer[flag][inputsIndices.x]
		    + float4(ComplexMult(twiddle, v.xy), ComplexMult(twiddle, v.zw));
        flag = !flag;
        GroupMemoryBarrierWithGroupSync();
    }
    
    return buffer[flag][threadIndex];
}

[numthreads(SIZE, 1, 1)]
void CS(uint3 id : SV_DispatchThreadID)
{
    uint threadIndex = id.x;
    uint2 targetIndex;
    if (Direction)
        targetIndex = id.yx;
    else
        targetIndex = id.xy;
    
#ifdef FFT_ARRAY_TARGET
    for (uint k = 0; k < TargetsCount; k++)
    {
        Target[uint3(targetIndex, k)] = DoFft(threadIndex, Target[uint3(targetIndex, k)]);
    }
#else
    g_Target[targetIndex] = DoFft(threadIndex, g_Target[targetIndex]);
#endif
}