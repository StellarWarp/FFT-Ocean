
#define FFT_SIZE_256

#if defined(FFT_SIZE_512)
#define SIZE 512
#define LOG_SIZE 9
#elif defined(FFT_SIZE_256)
#define SIZE 256
#define LOG_SIZE 8
#elif defined(FFT_SIZE_128)
#define SIZE 128
#define LOG_SIZE 7
#else
#define SIZE 64
#define LOG_SIZE 6
#endif

static uint Size = SIZE;

#ifdef FFT_ARRAY_TARGET
RWTexture2DArray<float4> Target;
#else
RWTexture2D<float4> g_Target;
#endif

cbuffer Params
{
    uint TargetsCount;
    uint Direction;//bool
    uint Inverse; //bool
    uint Scale; //bool
    uint Permute; //bool
};