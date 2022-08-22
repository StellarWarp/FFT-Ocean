#include "OceanCompute.hlsli"

//Donelan-Banner方向拓展
float D(float2 k)
{
    float betaS;
    float omegap = 0.855f * G / length(wind);
    float ratio = dispersion(k) / omegap;

    if (ratio < 0.95f)
    {
        betaS = 2.61f * pow(ratio, 1.3f);
    }
    if (ratio >= 0.95f && ratio < 1.6f)
    {
        betaS = 2.28f * pow(ratio, -1.3f);
    }
    if (ratio >= 1.6f)
    {
        float epsilon = -0.4f + 0.8393f * exp(-0.567f * log(ratio * ratio));
        betaS = pow(10, epsilon);
    }
    float theta = atan2(k.y, k.x) - atan2(wind.y, wind.x);

    return betaS / max(1e-7f, 2.0f * tanh(betaS * PI) * pow(cosh(betaS * theta), 2));
    //float k_len = length(k);
    //float h = 10;
    //sqrt(G * k_len * tanh(k_len * h));
    //return abs(dot(k, wind));

}
float Ph(float2 k)
{
    float k_len = length(k);
    //截断
    k_len = max(0.001f, k_len);
    float k_len2 = k_len * k_len;
    float k_len4 = k_len2 * k_len2;
    
    float windSpeed = length(wind);
    float l = windSpeed * windSpeed / G;
    float l2 = l * l;
    
    float damping = 1.0f;
    float L2 = l2 * damping * damping;
    
    //return A * exp(-1.0f / (k_len2 * l2)) / k_len4*  exp(-k_len2 * L2);
    return A * exp(-1.0f / (k_len2 * l2)) / k_len4 / 100;
}
//complex
float4 H0(float2 k, int2 id)
{
    float2 rand = gaussian(id);
    //float2 rand = float2(1.0f, 1.0f);
    float2 h0 = rand * sqrt(abs(Ph(k) * D(k)) / 2.0f);
    float2 h0Conj = rand * sqrt(abs(Ph(-k) * D(-k)) / 2.0f);
    h0Conj.y *= -1.0f;
    return float4(h0, h0Conj);
}



[numthreads(16, 16, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    float2 k = vec_k(DTid.xy);
    g_h0Data[DTid.xy] = H0(k, DTid.xy);
}