#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"
#include "Aquila/GFX/GfxContext.h"
#include <cstring>

namespace Aquila::Graphics {

static uint32 Std140Align(ParameterType t) {
	switch (t) {
	case ParameterType::Float:
	case ParameterType::Int:
	case ParameterType::Bool:
		return 4;
	case ParameterType::Vec2:
		return 8;
	case ParameterType::Vec3:
	case ParameterType::Vec4:
	case ParameterType::Color:
		return 16;
	default:
		return 4;
	}
}

// Bytes occupied in the UBO stream (vec3 has a 16-byte stride in std140).
static uint32 Std140Stride(ParameterType t) {
	switch (t) {
	case ParameterType::Float:
	case ParameterType::Int:
	case ParameterType::Bool:
		return 4;
	case ParameterType::Vec2:
		return 8;
	case ParameterType::Vec3:
		return 16; // 12 bytes of data, 4 bytes pad
	case ParameterType::Vec4:
	case ParameterType::Color:
		return 16;
	default:
		return 4;
	}
}

// Bytes actually copied when writing the value (no padding).
static uint32 ValueSize(ParameterType t) {
	switch (t) {
	case ParameterType::Float:
	case ParameterType::Int:
	case ParameterType::Bool:
		return 4;
	case ParameterType::Vec2:
		return 8;
	case ParameterType::Vec3:
		return 12;
	case ParameterType::Vec4:
	case ParameterType::Color:
		return 16;
	default:
		return 4;
	}
}

static ParameterValue DefaultValue(ParameterType t) {
	switch (t) {
	case ParameterType::Float:
		return 0.f;
	case ParameterType::Int:
		return 0;
	case ParameterType::Bool:
		return false;
	case ParameterType::Vec2:
		return vec2{ 0.f, 0.f };
	case ParameterType::Vec3:
		return vec3{ 0.f, 0.f, 0.f };
	case ParameterType::Vec4:
	case ParameterType::Color:
		return vec4{ 0.f, 0.f, 0.f, 1.f };
	default:
		return 0.f;
	}
}

Ref<Material> Material::CreateFromShader(GFX::GfxContext &ctx, Shader::ShaderProgram &shader,
										 Ref<GFX::GfxPipeline> pipeline) {
	auto mat = Ref<Material>(new Material());
	mat->m_Context = &ctx;
	mat->m_Pipeline = std::move(pipeline);
	mat->m_Layout = shader.m_DescriptorSetLayout;

	if (mat->m_Layout) {
		mat->m_Set = ctx.AllocateDescriptorSet(*mat->m_Layout);
	}

	using RBType = Shader::ShaderProgram::ReflectedBindingType;

	uint32 uboSize = 0;

	for (const auto &binding : shader.GetReflectedBindings()) {
		if (binding.type == RBType::UniformBuffer) {
			uint32 offset = 0;
			for (const auto &field : binding.uboFields) {
				uint32 align = Std140Align(field.paramType);
				offset = (offset + align - 1) & ~(align - 1);

				MaterialParameter p;
				p.name = field.name;
				p.type = field.paramType;
				p.uboOffset = offset;
				p.value = DefaultValue(field.paramType);
				p.defaultValue = p.value;
				mat->m_Parameters.push_back(std::move(p));

				offset += Std140Stride(field.paramType);
			}
			uboSize = std::max(uboSize, offset);
		} else if (binding.type == RBType::CombinedImageSampler) {
			MaterialParameter p;
			p.name = binding.name;
			p.type = ParameterType::Texture2D;
			p.textureBinding = binding.bindingIndex;
			mat->m_Parameters.push_back(std::move(p));
		}
	}

	// std140 struct size must be a multiple of 16.
	uboSize = (uboSize + 15u) & ~15u;

	if (uboSize > 0 && mat->m_Set) {
		mat->m_UBOData.assign(uboSize, 0);
		mat->m_UniformBuffer = ctx.CreateBuffer({
			.size = uboSize,
			.usage = RHI::BufferUsage::UniformBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "MaterialUBO",
		});
		mat->m_UniformDirty = true;
	}

	return mat;
}

Ref<Material> Material::Create(GFX::GfxContext &ctx, Ref<GFX::GfxPipeline> pipeline,
							   Ref<GFX::GfxDescriptorSetLayout> layout) {
	auto mat = Ref<Material>(new Material());
	mat->m_Context = &ctx;
	mat->m_Pipeline = std::move(pipeline);
	mat->m_Layout = layout;

	if (mat->m_Layout) {
		mat->m_Set = ctx.AllocateDescriptorSet(*mat->m_Layout);
	}

	return mat;
}

void Material::RegisterParameter(std::string paramName, ParameterType type, uint32 uboOffset, uint32 textureBinding) {
	for (auto &p : m_Parameters) {
		if (p.name == paramName) {
			p.type = type;
			p.uboOffset = uboOffset;
			p.textureBinding = textureBinding;
			return;
		}
	}
	MaterialParameter p;
	p.name = std::move(paramName);
	p.type = type;
	p.uboOffset = uboOffset;
	p.textureBinding = textureBinding;
	p.value = DefaultValue(type);
	p.defaultValue = p.value;
	m_Parameters.push_back(std::move(p));
}

MaterialParameter *Material::FindParameter(const std::string &paramName) {
	for (auto &p : m_Parameters) {
		if (p.name == paramName) {
			return &p;
		}
	}
	return nullptr;
}

const MaterialParameter *Material::GetParameter(const std::string &paramName) const {
	for (const auto &p : m_Parameters) {
		if (p.name == paramName) {
			return &p;
		}
	}
	return nullptr;
}

template <typename T> void Material::WriteUBO(uint32 offset, const T &v) {
	if (m_UBOData.empty()) {
		uint32 needed = offset + static_cast<uint32>(sizeof(T));
		if (needed > m_UBOData.size()) {
			m_UBOData.resize((needed + 15u) & ~15u, 0);
		}
	}
	if (offset + sizeof(T) <= m_UBOData.size()) {
		std::memcpy(m_UBOData.data() + offset, &v, sizeof(T));
	}
}

void Material::EnsureUniformBuffer() {
	if (m_UniformBuffer || m_UBOData.empty() || !m_Context || !m_Set) {
		return;
	}

	m_UniformBuffer = m_Context->CreateBuffer({
		.size = static_cast<uint32>(m_UBOData.size()),
		.usage = RHI::BufferUsage::UniformBuffer,
		.domain = RHI::MemoryDomain::CPU_TO_GPU,
		.debugName = "MaterialUBO",
	});
}

Material &Material::Set(const std::string &paramName, f32 v) {
	if (auto *p = FindParameter(paramName); p && p->uboOffset != UINT32_MAX) {
		p->value = v;
		WriteUBO(p->uboOffset, v);
		m_UniformDirty = true;
	}
	return *this;
}

Material &Material::Set(const std::string &paramName, int v) {
	if (auto *p = FindParameter(paramName); p && p->uboOffset != UINT32_MAX) {
		p->value = v;
		WriteUBO(p->uboOffset, v);
		m_UniformDirty = true;
	}
	return *this;
}

Material &Material::Set(const std::string &paramName, bool v) {
	if (auto *p = FindParameter(paramName); p && p->uboOffset != UINT32_MAX) {
		p->value = v;
		int asInt = v ? 1 : 0;
		WriteUBO(p->uboOffset, asInt);
		m_UniformDirty = true;
	}
	return *this;
}

Material &Material::Set(const std::string &paramName, const vec2 &v) {
	if (auto *p = FindParameter(paramName); p && p->uboOffset != UINT32_MAX) {
		p->value = v;
		WriteUBO(p->uboOffset, v);
		m_UniformDirty = true;
	}
	return *this;
}

Material &Material::Set(const std::string &paramName, const vec3 &v) {
	if (auto *p = FindParameter(paramName); p && p->uboOffset != UINT32_MAX) {
		p->value = v;
		// Only copy 12 bytes; the 4-byte pad is left as zero.
		if (p->uboOffset + ValueSize(ParameterType::Vec3) <= m_UBOData.size()) {
			std::memcpy(m_UBOData.data() + p->uboOffset, &v, ValueSize(ParameterType::Vec3));
		}
		m_UniformDirty = true;
	}
	return *this;
}

Material &Material::Set(const std::string &paramName, const vec4 &v) {
	if (auto *p = FindParameter(paramName); p && p->uboOffset != UINT32_MAX) {
		p->value = v;
		WriteUBO(p->uboOffset, v);
		m_UniformDirty = true;
	}
	return *this;
}

Material &Material::Set(const std::string &paramName, Ref<GFX::GfxTexture> tex) {
	if (auto *p = FindParameter(paramName); p && p->textureBinding != UINT32_MAX && tex) {
		p->value = tex;
		m_PendingTextures.push_back({ p->textureBinding, tex.get() });
	}
	return *this;
}

Material &Material::SetTexture(uint32 binding, GFX::GfxTexture &tex) {
	for (auto &tb : m_PendingTextures) {
		if (tb.binding == binding) {
			tb.tex = &tex;
			return *this;
		}
	}
	m_PendingTextures.push_back({ binding, &tex });
	return *this;
}

void Material::Flush() {
	if (!m_Set) {
		return;
	}

	if (m_UniformDirty && !m_UBOData.empty()) {
		EnsureUniformBuffer();
		if (m_UniformBuffer) {
			m_UniformBuffer->Write(m_UBOData.data(), static_cast<uint32>(m_UBOData.size()));
			m_Set->SetBuffer(0, *m_UniformBuffer);
			m_UniformDirty = false;
		}
	}

	for (auto &tb : m_PendingTextures) {
		m_Set->SetTexture(tb.binding, *tb.tex);
	}
	m_PendingTextures.clear();

	m_Set->Flush();
}

void Material::Bind(GFX::GfxCommandList &cmd, uint32 setIndex) {
	cmd.BindPipeline(*m_Pipeline);
	if (m_Set) {
		cmd.BindDescriptorSet(setIndex, *m_Set);
	}
}

void Material::ReplacePipeline(Ref<GFX::GfxPipeline> newPipeline, Ref<GFX::GfxDescriptorSetLayout> newLayout) {
	m_Pipeline = std::move(newPipeline);

	if (newLayout && m_Context) {
		m_Layout = std::move(newLayout);
		m_Set = m_Context->AllocateDescriptorSet(*m_Layout);
		if (m_UniformBuffer) {
			m_Set->SetBuffer(0, *m_UniformBuffer);
			m_UniformDirty = true;
		}
	}
}

} // namespace Aquila::Graphics
