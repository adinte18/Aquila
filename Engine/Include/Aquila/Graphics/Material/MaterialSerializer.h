#ifndef MATERIAL_SERIALIZER_H
#define MATERIAL_SERIALIZER_H

#include "json.hpp"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Material/MaterialLibrary.h"

namespace Aquila::Graphics::Material {

class MaterialSerializer {
  public:
	struct TextureReference {
		std::string propertyName;
		std::string texturePath;
		int bindingIndex;
	};

	struct PropertyData {
		std::string name;
		ParameterType type;
		ParameterValue value;
	};

	struct DeserializedMaterialData {
		std::string name;
		std::string templateName;
		std::string shaderPath;
		std::string version;
		std::vector<PropertyData> properties;
		std::vector<TextureReference> textureReferences;
		RenderState renderState;
	};

	static nlohmann::json SerializeToJson(const Ref<Material> &material) {
		nlohmann::json j;

		j["name"] = material->name;
		j["template"] = material->GetTemplate() ? material->GetTemplate()->name : "PBR";
		j["shaderPath"] = material->GetShaderAssetPath();
		j["version"] = "1.0";

		nlohmann::json properties = nlohmann::json::object();
		auto allProps = material->GetAllProperties();

		for (const auto &[propName, prop] : allProps) {
			if (!prop.m_IsEditable)
				continue;

			nlohmann::json propJson;
			propJson["type"] = static_cast<int>(prop.m_Type);
			propJson["displayName"] = prop.m_DisplayName;

			switch (prop.m_Type) {
			case ParameterType::Float:
				propJson["value"] = std::get<f32>(prop.m_Value);
				break;

			case ParameterType::Vec2: {
				auto v = std::get<vec2>(prop.m_Value);
				propJson["value"] = { v.x, v.y };
				break;
			}

			case ParameterType::Vec3: {
				auto v = std::get<vec3>(prop.m_Value);
				propJson["value"] = { v.x, v.y, v.z };
				break;
			}

			case ParameterType::Vec4:
			case ParameterType::Color: {
				auto v = std::get<vec4>(prop.m_Value);
				propJson["value"] = { v.x, v.y, v.z, v.w };
				break;
			}

			case ParameterType::Bool:
				propJson["value"] = std::get<bool>(prop.m_Value);
				break;

			case ParameterType::Int:
				propJson["value"] = std::get<int>(prop.m_Value);
				break;

			case ParameterType::Texture2D: {
				auto tex = material->GetTexture(propName);
				if (tex && !material->IsFallbackTexture(propName, tex)) {
					propJson["value"] = tex->GetPath();
				} else {
					propJson["value"] = nullptr;
				}
				propJson["bindingIndex"] = prop.m_BindingIndex;
				break;
			}

			default:
				break;
			}

			properties[propName] = propJson;
		}

		j["properties"] = properties;

		auto renderState = material->GetRenderState();
		nlohmann::json renderStateJson;
		renderStateJson["blendMode"] = static_cast<int>(renderState.m_BlendMode);
		renderStateJson["cullMode"] = static_cast<int>(renderState.m_CullMode);
		renderStateJson["depthTest"] = renderState.m_DepthTest;
		renderStateJson["depthWrite"] = renderState.m_DepthWrite;
		renderStateJson["wireframe"] = renderState.m_Wireframe;
		renderStateJson["lineWidth"] = renderState.m_LineWidth;

		j["renderState"] = renderStateJson;

		return j;
	}

	static DeserializedMaterialData ParseFromJson(const nlohmann::json &j) {
		if (!j.contains("name") || !j.contains("template")) {
			throw std::runtime_error("Invalid material JSON: missing name or template");
		}

		DeserializedMaterialData data;
		data.name = j["name"];
		data.templateName = j["template"];
		data.shaderPath = j.value("shaderPath", "");
		data.version = j.value("version", "1.0");

		if (j.contains("properties")) {
			for (const auto &[propName, propJson] : j["properties"].items()) {
				if (!propJson.contains("type") || !propJson.contains("value")) {
					continue;
				}

				ParameterType type = static_cast<ParameterType>(propJson["type"].get<int>());
				PropertyData propData;
				propData.name = propName;
				propData.type = type;

				try {
					switch (type) {
					case ParameterType::Float:
						propData.value = propJson["value"].get<f32>();
						data.properties.push_back(propData);
						break;

					case ParameterType::Vec2: {
						auto arr = propJson["value"].get<std::vector<f32>>();
						if (arr.size() >= 2) {
							propData.value = vec2(arr[0], arr[1]);
							data.properties.push_back(propData);
						}
						break;
					}

					case ParameterType::Vec3: {
						auto arr = propJson["value"].get<std::vector<f32>>();
						if (arr.size() >= 3) {
							propData.value = vec3(arr[0], arr[1], arr[2]);
							data.properties.push_back(propData);
						}
						break;
					}

					case ParameterType::Vec4:
					case ParameterType::Color: {
						auto arr = propJson["value"].get<std::vector<f32>>();
						if (arr.size() >= 4) {
							propData.value = vec4(arr[0], arr[1], arr[2], arr[3]);
							data.properties.push_back(propData);
						}
						break;
					}

					case ParameterType::Bool:
						propData.value = propJson["value"].get<bool>();
						data.properties.push_back(propData);
						break;

					case ParameterType::Int:
						propData.value = propJson["value"].get<int>();
						data.properties.push_back(propData);
						break;

					case ParameterType::Texture2D: {
						if (!propJson["value"].is_null()) {
							TextureReference texRef;
							texRef.propertyName = propName;
							texRef.texturePath = propJson["value"].get<std::string>();
							texRef.bindingIndex = propJson.value("bindingIndex", -1);
							data.textureReferences.push_back(texRef);
						}
						break;
					}

					default:
						break;
					}
				} catch (const std::exception &e) {
					AQUILA_LOG_WARNING("Failed to parse property {}: {}", propName, e.what());
				}
			}
		}

		if (j.contains("renderState")) {
			auto rs = j["renderState"];
			if (rs.contains("blendMode"))
				data.renderState.m_BlendMode = static_cast<BlendMode>(rs["blendMode"].get<int>());
			if (rs.contains("cullMode"))
				data.renderState.m_CullMode = static_cast<CullMode>(rs["cullMode"].get<int>());
			if (rs.contains("depthTest"))
				data.renderState.m_DepthTest = rs["depthTest"].get<bool>();
			if (rs.contains("depthWrite"))
				data.renderState.m_DepthWrite = rs["depthWrite"].get<bool>();
			if (rs.contains("wireframe"))
				data.renderState.m_Wireframe = rs["wireframe"].get<bool>();
			if (rs.contains("lineWidth"))
				data.renderState.m_LineWidth = rs["lineWidth"].get<f32>();
		}

		return data;
	}

	static Ref<Material> CreateFromData(const DeserializedMaterialData &data, Device &device,
										MaterialLibrary &library) {
		auto material = library.CreateMaterial(data.name, data.templateName);
		if (!material || material == library.GetFallbackMaterial()) {
			return nullptr;
		}

		for (const auto &prop : data.properties) {
			material->SetParameter(prop.name, prop.value);
		}

		material->SetRenderState(data.renderState);
		material->MarkDirty();
		return material;
	}

	static bool SaveToFile(const Ref<Material> &material, const std::string &filepath) {
		try {
			auto json = SerializeToJson(material);

			std::ofstream file(filepath);
			if (!file.is_open()) {
				AQUILA_LOG_ERROR("Failed to open file for writing: {}", filepath);
				return false;
			}

			file << json.dump(4);
			file.close();

			AQUILA_LOG_INFO("Material saved to: {}", filepath);
			return true;
		} catch (const std::exception &e) {
			AQUILA_LOG_ERROR("Failed to save material: {}", e.what());
			return false;
		}
	}

	static bool SaveToFileVFS(const Ref<Material> &material, const std::string &virtualPath) {
		try {
			auto json = SerializeToJson(material);
			std::string jsonStr = json.dump(4);

			auto file = Aquila::Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(virtualPath, "w");
			if (!file) {
				AQUILA_LOG_ERROR("Failed to open VFS file for writing: {}", virtualPath);
				return false;
			}

			size_t written = file->Write(jsonStr.data(), jsonStr.size());
			if (written != jsonStr.size()) {
				AQUILA_LOG_ERROR("Failed to write complete material data");
				return false;
			}

			AQUILA_LOG_INFO("Material saved to VFS: {}", virtualPath);
			return true;
		} catch (const std::exception &e) {
			AQUILA_LOG_ERROR("Failed to save material via VFS: {}", e.what());
			return false;
		}
	}
};

} // namespace Aquila::Graphics::Material
#endif
