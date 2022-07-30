//h0 and conjugate h0
RWTexture2D<float4> g_h0Data : register(u0);

static const float PI = 3.14159265358979323846;

cbuffer CBPrecompute : register(b0)
{
    uint N; // sections
    float A;
    float G;
    float2 wind;
    float3 pad0;
}

cbuffer CBUpdate : register(b1)
{
    float Time;
    float3 pad1;
}

//Helper

float2 complexMultiply(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

//随机种子
uint wangHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}
static uint rngState;
//计算均匀分布随机数[0,1)
float rand()
{
    // Xorshift算法
    rngState ^= (rngState << 13);
    rngState ^= (rngState >> 17);
    rngState ^= (rngState << 5);
    return rngState / 4294967296.0f;;
}
//计算高斯随机数
float2 gaussian(float2 id)
{
    //均匀分布随机数
    rngState = wangHash(id.y * N + id.x);
    float x1 = rand();
    float x2 = rand();

    x1 = max(1e-6f, x1);
    x2 = max(1e-6f, x2);
    //计算两个相互独立的高斯随机数
    float g1 = sqrt(-2.0f * log(x1)) * cos(2.0f * PI * x2);
    float g2 = sqrt(-2.0f * log(x1)) * sin(2.0f * PI * x2);

    return float2(g1, g2);
}

float2 vec_k(uint2 id)
{
    return float2(2.0f * PI * id.x / N - PI, 2.0f * PI * id.y / N - PI);
}

float dispersion(float2 k)
{
    return sqrt(G * length(k));
}