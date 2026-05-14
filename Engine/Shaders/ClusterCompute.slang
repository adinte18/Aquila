import FrameData;

struct AABB {
	float3 min;
	float3 max;
};

struct GridData {
	int3 grid;
};

[vk::binding(0, 1)] RWStructuredBuffer<AABB> outBuffer;
[vk::binding(1, 1)] ConstantBuffer<GridData> gridData;

[numthreads(4, 4, 4)] void main(uint3 id : SV_DispatchThreadID) {
	if (id.x >= gridData.grid.x || id.y >= gridData.grid.y || id.z >= gridData.grid.z) {
		return;
	}

	// we need AABB into outBuffer
	// to get the AABB we need 8 corners in view space
	// then we take min/max of those 8 corners

	// NDC XY positions
	float2 gridSize = float2(gridData.grid.xy);
	float2 ndcMin = float2((float(id.x) / gridSize.x) * 2.0 - 1.0, (float(id.y) / gridSize.y) * 2.0 - 1.0);
	float2 ndcMax = float2((float(id.x + 1) / gridSize.x) * 2.0 - 1.0, (float(id.y + 1) / gridSize.y) * 2.0 - 1.0);

	// Compute depths
	float nearPlane = AQ_GetNearPlane();
	float farPlane = AQ_GetFarPlane();

	float ratio = farPlane / nearPlane;

	//the negation is Vulkan convention
	float zNearSlice = (nearPlane * pow(ratio, id.z / float(gridData.grid.z)));
	float zFarSlice = (nearPlane * pow(ratio, (id.z + 1) / float(gridData.grid.z)));

	// 8 reconstructed corners
	float3 corners[8];

	float4x4 invProj = aqFrameData.mainCamera.inverseProjection;

	corners[0] = AQ_NDCToView(invProj, float2(ndcMin.x, ndcMin.y), zNearSlice);
	corners[1] = AQ_NDCToView(invProj, float2(ndcMax.x, ndcMin.y), zNearSlice);
	corners[2] = AQ_NDCToView(invProj, float2(ndcMin.x, ndcMax.y), zNearSlice);
	corners[3] = AQ_NDCToView(invProj, float2(ndcMax.x, ndcMax.y), zNearSlice);

	corners[4] = AQ_NDCToView(invProj, float2(ndcMin.x, ndcMin.y), zFarSlice);
	corners[5] = AQ_NDCToView(invProj, float2(ndcMax.x, ndcMin.y), zFarSlice);
	corners[6] = AQ_NDCToView(invProj, float2(ndcMin.x, ndcMax.y), zFarSlice);
	corners[7] = AQ_NDCToView(invProj, float2(ndcMax.x, ndcMax.y), zFarSlice);

	float3 bmin = corners[0];
	float3 bmax = corners[0];

	[unroll] for (int i = 1; i < 8; i++) {
		bmin = min(bmin, corners[i]);
		bmax = max(bmax, corners[i]);
	}

	//https://stackoverflow.com/questions/7367770/how-to-flatten-or-index-3d-array-in-1d-array
	uint index = id.z * (gridData.grid.x * gridData.grid.y) + id.y * gridData.grid.x + id.x;

	outBuffer[index] = (AABB){ bmin, bmax };
}
