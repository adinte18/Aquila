import FrameData;

[vk::binding(0, 1)] RWStructuredBuffer<uint> outBuffer;

[numthreads(64, 1, 1)] void main(uint3 id : SV_DispatchThreadID) { outBuffer[id.x] = id.x + AQ_GetFrameIndex(); }
