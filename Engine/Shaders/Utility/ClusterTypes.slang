#ifndef AQUILA_CLUSTER_TYPES_SLANG
#define AQUILA_CLUSTER_TYPES_SLANG

namespace Aquila::Shading {

static const uint kClusterGridX = 16;
static const uint kClusterGridY = 9;
static const uint kClusterGridZ = 24;
static const uint kMaxLightsPerCluster = 256;

struct AABB {
	float3 min;
	float3 max;
};

struct ClusterLightInfo {
	uint offset;
	uint count;
};

} // namespace Aquila::Shading
#endif
