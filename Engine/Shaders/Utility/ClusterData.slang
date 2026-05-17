#ifndef AQUILA_CLUSTER_DATA_SLANG
#define AQUILA_CLUSTER_DATA_SLANG

import Utility.FrameData;
import Utility.ClusterTypes;

namespace Aquila::Shading {

[vk::binding(4, 0)] StructuredBuffer<uint> lightIndexList;
[vk::binding(5, 0)] StructuredBuffer<ClusterLightInfo> clusterLightInfoBuffer;

// Compute the flat cluster index for a fragment given its screen-space position
// (SV_Position.xy, top-left origin) and its positive view-space depth.
uint GetClusterIndex(float2 fragCoord, float viewZ) {
	float2 res = GetScreenResolution();

	uint tileX = uint(fragCoord.x / (res.x / float(kClusterGridX)));
	uint tileY = uint(fragCoord.y / (res.y / float(kClusterGridY)));
	tileX = min(tileX, kClusterGridX - 1u);
	tileY = min(tileY, kClusterGridY - 1u);

	// Logarithmic z-slice matching ClusterCompute.slang
	float near = GetNearPlane();
	float far = GetFarPlane();
	float ratio = log(far / near);
	uint sliceZ = uint(max(log(viewZ / near) / ratio * float(kClusterGridZ), 0.0));
	sliceZ = min(sliceZ, kClusterGridZ - 1u);

	return sliceZ * (kClusterGridX * kClusterGridY) + tileY * kClusterGridX + tileX;
}

ClusterLightInfo GetClusterInfo(uint clusterIdx) {
	return clusterLightInfoBuffer[clusterIdx];
}
uint GetLightListIndex(uint offset) {
	return lightIndexList[offset];
}

} // namespace Aquila::Shading
#endif // AQUILA_CLUSTER_DATA_SLANG
