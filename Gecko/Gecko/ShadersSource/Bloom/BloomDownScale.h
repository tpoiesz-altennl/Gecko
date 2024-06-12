#include "ShaderDefines.h"

Texture2D<float4> InputTexture : register(t0);
RW_Texture2D_Arr<float4> OutputTexture : register(u0);
SamplerState LinearSampler : register(s0);

struct BloomData
{
	uint Width;
	uint Height;
	float Threshold;
};

DYNAMIC_CALL_DATA(BloomData, bloomData, b0);

#if defined (COMPUTE)
[N_THREADS(8, 8, 1)]
void main(uint3 DTid : S_INPUT_DISPATCH_ID)
{
	float2 texelSize = 1./float2(bloomData.Width, bloomData.Height);
	float2 uv = float2(DTid.xy+.5f) / float2(bloomData.Width, bloomData.Height);

	float4 color = DownsampleBox13Tap(InputTexture, LinearSampler, uv, texelSize);

	OutputTexture[DTid] = color;
}
#endif