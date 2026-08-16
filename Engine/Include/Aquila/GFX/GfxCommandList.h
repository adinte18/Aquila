#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::GFX {

class GfxContext;

class GfxCommandList {
  public:
	~GfxCommandList() = default;
	AQUILA_NONCOPYABLE(GfxCommandList);

	void begin();
	void end();
	void reset();

	[[nodiscard]] bool is_recording() const;
	[[nodiscard]] RHI::CommandListType get_type() const;
	[[nodiscard]] const std::string &get_name() const;

	void transition_texture(GfxTexture &texture, RHI::ResourceState old_state, RHI::ResourceState new_state);
	void transition_buffer(GfxBuffer &buffer, RHI::ResourceState old_state, RHI::ResourceState new_state);

	void bind_pipeline(GfxPipeline &pipeline);
	void set_viewport(float x, float y, float width, float height, float min_depth = 0.0F, float max_depth = 1.0F);
	void set_scissor(Int32 x, Int32 y, Uint32 width, Uint32 height);

	void bind_descriptor_set(Uint32 set, GfxDescriptorSet &descriptor_set);

	template <typename T>
	void push_constants(const T &data, RHI::ShaderStageFlags stages = RHI::ShaderStageFlags::Vertex, Uint32 offset = 0) {
		m_cmd->push_constants(&data, sizeof(T), stages, offset);
	}

	void bind_vertex_buffer(GfxBuffer &buf, Uint32 binding = 0, Uint64 offset = 0);
	void bind_index_buffer(GfxBuffer &buf, RHI::IndexFormat fmt = RHI::IndexFormat::UInt32, Uint64 offset = 0);

	void draw(Uint32 vertex_count, Uint32 instance_count = 1, Uint32 first_vertex = 0, Uint32 first_instance = 0);
	void draw_indexed(Uint32 index_count, Uint32 instance_count = 1, Uint32 first_index = 0, Int32 vertex_offset = 0,
					 Uint32 first_instance = 0);
	void draw_indirect(GfxBuffer &buffer, Uint64 offset, Uint32 draw_count, Uint32 stride);
	void draw_indexed_indirect(GfxBuffer &buffer, Uint64 offset, Uint32 draw_count, Uint32 stride);

	void copy_buffer_to_texture(GfxBuffer &src, GfxTexture &dst, Uint32 width, Uint32 height, Uint32 dst_array_layer = 0,
							 Uint32 dst_mip_level = 0);

	static constexpr Uint64 WHOLE_SIZE = ~0ULL;
	void fill_buffer(GfxBuffer &buffer, Uint32 value = 0u, Uint64 offset = 0, Uint64 size = WHOLE_SIZE);

	void dispatch(Uint32 x, Uint32 y, Uint32 z);

	void push_debug_group(const char *name);
	void pop_debug_group();

	[[nodiscard]] RHI::IRHICommandList &get_rhi() { return *m_cmd; }

  private:
	friend class GfxContext;
	explicit GfxCommandList(Unique<RHI::IRHICommandList> cmd);

	Unique<RHI::IRHICommandList> m_cmd;
};

} // namespace Aquila::GFX
