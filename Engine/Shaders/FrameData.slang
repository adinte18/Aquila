
static const uint AQ_MAX_CAMERAS = 8;

struct AqCameraData {
	float4x4 view;
	float4x4 projection;
	float4x4 viewProjection;
	float4x4 inverseView;
	float4x4 inverseProjection;
	float4x4 inverseViewProjection;
	float4x4 prevViewProjection;

	float4 position; // .xyz = world-space position
	float4 forward;	 // .xyz = world-space forward
	float4 up;		 // .xyz = world-space up
	float4 right;	 // .xyz = world-space right

	float nearPlane;
	float farPlane;
	float fov; // vertical FOV in degrees
	float aspectRatio;

	float2 resolution; // viewport size in pixels
	float2 jitter;	   // TAA sub-pixel jitter

	uint isOrthographic;
	uint cameraIndex;
	uint2 _pad;
};

struct AqFrameData {
	AqCameraData mainCamera;
	AqCameraData cameras[AQ_MAX_CAMERAS];

	uint cameraCount;
	float time;
	float deltaTime;
	uint frameIndex;

	float2 screenResolution;
	float2 _pad;
};

[vk::binding(0, 0)] ConstantBuffer<AqFrameData> aqFrameData;

AqCameraData AQ_GetMainCamera() {
	return aqFrameData.mainCamera;
}
AqCameraData AQ_GetCamera(uint index) {
	return aqFrameData.cameras[index];
}
uint AQ_GetCameraCount() {
	return aqFrameData.cameraCount;
}

float3 AQ_GetCameraPosition() {
	return aqFrameData.mainCamera.position.xyz;
}
float3 AQ_GetCameraForward() {
	return aqFrameData.mainCamera.forward.xyz;
}
float3 AQ_GetCameraUp() {
	return aqFrameData.mainCamera.up.xyz;
}
float3 AQ_GetCameraRight() {
	return aqFrameData.mainCamera.right.xyz;
}
float AQ_GetNearPlane() {
	return aqFrameData.mainCamera.nearPlane;
}
float AQ_GetFarPlane() {
	return aqFrameData.mainCamera.farPlane;
}
float AQ_GetFOV() {
	return aqFrameData.mainCamera.fov;
}
float AQ_GetAspectRatio() {
	return aqFrameData.mainCamera.aspectRatio;
}
bool AQ_IsOrthographic() {
	return aqFrameData.mainCamera.isOrthographic != 0;
}

// World-space position to clip space (NDC + w)
float4 AQ_WorldToClip(float3 worldPos) {
	return mul(aqFrameData.mainCamera.viewProjection, float4(worldPos, 1.0));
}

float3 AQ_NDCToView(float4x4 matrix, float2 ndc, float linearDepth) {
	float4 clip = float4(ndc, 1.0, 1.0);
	float4 view = mul(matrix, clip);
	view /= view.w;
	return view.xyz * (linearDepth / view.z);
}

// World-space position to view space
float4 AQ_WorldToView(float3 worldPos) {
	return mul(aqFrameData.mainCamera.view, float4(worldPos, 1.0));
}

// Clip space to world-space position (performs perspective divide)
float3 AQ_ClipToWorld(float4 clipPos) {
	float4 worldPos = mul(aqFrameData.mainCamera.inverseViewProjection, clipPos);
	return worldPos.xyz / worldPos.w;
}

// View space → world space
float3 AQ_ViewToWorld(float3 viewPos) {
	return mul(aqFrameData.mainCamera.inverseView, float4(viewPos, 1.0)).xyz;
}

// Reconstruct world-space position from a screen UV (0..1) and a raw depth value.
// depth is the value sampled from the depth buffer (0..1 in Vulkan).
float3 AQ_ReconstructWorldPos(float2 uv, float depth) {
	// Convert uv to NDC: x in [-1,1], y in [-1,1], z = depth (Vulkan: 0..1)
	float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 worldPos = mul(aqFrameData.mainCamera.inverseViewProjection, clipPos);
	return worldPos.xyz / worldPos.w;
}

// Reconstruct view-space position from screen UV and depth.
float3 AQ_ReconstructViewPos(float2 uv, float depth) {
	float4 clipPos = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 viewPos = mul(aqFrameData.mainCamera.inverseProjection, clipPos);
	return viewPos.xyz / viewPos.w;
}

// Linearize a raw depth buffer value to a view-space depth (positive, in world units).
float AQ_LinearizeDepth(float rawDepth) {
	float near = aqFrameData.mainCamera.nearPlane;
	float far = aqFrameData.mainCamera.farPlane;
	return near * far / (far - rawDepth * (far - near));
}

// Convert a linear view-space depth back to a raw [0..1] depth buffer value.
float AQ_LinearDepthToRaw(float linearDepth) {
	float near = aqFrameData.mainCamera.nearPlane;
	float far = aqFrameData.mainCamera.farPlane;
	return (far * (linearDepth - near)) / (linearDepth * (far - near));
}

float2 AQ_GetScreenResolution() {
	return aqFrameData.screenResolution;
}
float2 AQ_GetTexelSize() {
	return 1.0 / aqFrameData.screenResolution;
}

// Convert screen-space pixel coords to UV (0..1).
float2 AQ_PixelToUV(float2 pixelCoord) {
	return pixelCoord / aqFrameData.screenResolution;
}

float AQ_GetTime() {
	return aqFrameData.time;
}
float AQ_GetDeltaTime() {
	return aqFrameData.deltaTime;
}
uint AQ_GetFrameIndex() {
	return aqFrameData.frameIndex;
}
