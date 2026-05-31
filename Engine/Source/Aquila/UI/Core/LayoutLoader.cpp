#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/Slider.h"
#include "Aquila/UI/Widgets/Popup.h"
#include "Aquila/UI/Widgets/ContextMenu.h"
#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/UI/Widgets/NumberInput.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/DragInt.h"
#include "Aquila/UI/Widgets/Toggle.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/TabView.h"
#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Widgets/VecField.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/TreeView.h"
#include "Aquila/UI/Widgets/AssetPicker.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Style/StyleParserHelper.h"

namespace Aquila::UI::Core {

namespace {

struct Parser {
	std::string_view src;
	size_t pos = 0;
	const LayoutLoader &loader;

	[[nodiscard]] bool AtEnd() const { return pos >= src.size(); }
	[[nodiscard]] char Peek() const { return AtEnd() ? '\0' : src[pos]; }
	[[nodiscard]] char Peek2() const { return (pos + 1 < src.size()) ? src[pos + 1] : '\0'; }

	char Advance() {
		AQUILA_ASSERT(!AtEnd(), "LayoutLoader: unexpected end of input");
		return src[pos++];
	}

	void SkipWS() {
		while (!AtEnd() && std::isspace(static_cast<unsigned char>(Peek()))) {
			Advance();
		}
	}

	bool Match(char c) {
		if (Peek() == c) {
			Advance();
			return true;
		}
		return false;
	}

	void Expect(char c) {
		if (!Match(c)) {
			AQUILA_LOG_ERROR("LayoutLoader: expected '{}' at position {}", c, pos);
		}
	}

	std::string ReadName() {
		size_t start = pos;
		while (!AtEnd() && ((std::isalnum(static_cast<unsigned char>(Peek())) != 0) || Peek() == '-' || Peek() == '_' ||
							Peek() == ':')) {
			Advance();
		}
		return std::string(src.substr(start, pos - start));
	}

	std::string ReadQuoted() {
		char q = Advance();
		size_t start = pos;
		while (!AtEnd() && Peek() != q) {
			Advance();
		}
		std::string val(src.substr(start, pos - start));
		if (!AtEnd()) {
			Advance();
		}
		return val;
	}

	std::string ReadUnquoted() {
		size_t start = pos;
		while (!AtEnd() && Peek() != '>' && Peek() != '/' && (std::isspace(static_cast<unsigned char>(Peek())) == 0)) {
			Advance();
		}
		return std::string(src.substr(start, pos - start));
	}

	std::string ReadTextContent() {
		size_t start = pos;
		while (!AtEnd() && Peek() != '<') {
			Advance();
		}
		std::string_view raw = src.substr(start, pos - start);
		while (!raw.empty() && (std::isspace(static_cast<unsigned char>(raw.front())) != 0)) {
			raw.remove_prefix(1);
		}
		while (!raw.empty() && (std::isspace(static_cast<unsigned char>(raw.back())) != 0)) {
			raw.remove_suffix(1);
		}
		return std::string(raw);
	}

	bool TrySkipComment() {
		if (pos + 3 < src.size() && src[pos] == '<' && src[pos + 1] == '!' && src[pos + 2] == '-' &&
			src[pos + 3] == '-') {
			pos += 4;
			while (pos + 2 < src.size()) {
				if (src[pos] == '-' && src[pos + 1] == '-' && src[pos + 2] == '>') {
					pos += 3;
					return true;
				}
				++pos;
			}
		}
		return false;
	}

	Unique<View> ParseElement() {
		Expect('<');
		SkipWS();

		std::string tagName = ReadName();
		if (tagName.empty()) {
			AQUILA_LOG_ERROR("LayoutLoader: empty tag name at position {}", pos);
			return nullptr;
		}

		std::string id;
		std::vector<std::string> classes;
		UI::StyleProperties props;
		Text::FontAtlas *font = loader.ResolveFont("default");

		std::string imageSrc;
		std::string imageIcon;
		std::string imageBank;
		std::string imageUV;
		vec4 imageTint = vec4(1.f);

		SkipWS();
		while (!AtEnd() && Peek() != '>' && Peek() != '/') {
			std::string attrName = ReadName();
			if (attrName.empty()) {
				break;
			}

			SkipWS();
			std::string attrValue;
			if (Match('=')) {
				SkipWS();
				attrValue = (Peek() == '"' || Peek() == '\'') ? ReadQuoted() : ReadUnquoted();
			}
			SkipWS();

			if (attrName == "id") {
				id = attrValue;
			} else if (attrName == "class") {
				std::istringstream iss(attrValue);
				std::string cls;
				while (iss >> cls) {
					classes.push_back(cls);
				}
			} else if (attrName == "font") {
				font = loader.ResolveFont(attrValue);
			} else if (attrName == "style") {
				std::string_view decls = attrValue;
				while (!decls.empty()) {
					const auto semi = decls.find(';');
					std::string_view decl = (semi != std::string_view::npos) ? decls.substr(0, semi) : decls;
					decls = (semi != std::string_view::npos) ? decls.substr(semi + 1) : std::string_view{};

					const auto colon = decl.find(':');
					if (colon == std::string_view::npos) {
						continue;
					}
					auto trimSV = [](std::string_view s) {
						while (!s.empty() && static_cast<unsigned char>(s.front()) <= ' ') {
							s.remove_prefix(1);
						}
						while (!s.empty() && static_cast<unsigned char>(s.back()) <= ' ') {
							s.remove_suffix(1);
						}
						return s;
					};
					const std::string_view prop = trimSV(decl.substr(0, colon));
					const std::string_view val = trimSV(decl.substr(colon + 1));
					if (!prop.empty() && !val.empty()) {
						UI::StyleParser::ApplyProperty(props, prop, val);
					}
				}
			} else if (attrName == "src") {
				imageSrc = attrValue;
			} else if (attrName == "icon") {
				imageIcon = attrValue;
			} else if (attrName == "bank") {
				imageBank = attrValue;
			} else if (attrName == "uv") {
				imageUV = attrValue;
			} else if (attrName == "tint") {
				if (auto parsed = UI::ParserHelper::ParseColor(attrValue)) {
					imageTint = *parsed;
				}
			} else {
				UI::StyleParser::ApplyProperty(props, attrName, attrValue);
			}
		}

		Unique<View> view = loader.CreateWidget(tagName, "", font);
		if (!view) {
			AQUILA_LOG_ERROR("LayoutLoader: unknown widget type '{}'", tagName);

			while (!AtEnd() && Peek() != '>') {
				Advance();
			}
			if (!AtEnd()) {
				Advance();
			}
			return nullptr;
		}

		if (!id.empty()) {
			view->SetId(id);
		}
		for (auto &cls : classes) {
			view->AddClass(cls);
		}
		view->MergeStyle(props);

		if (font != nullptr) {
			view->SetFont(font);
		}

		if (imageTint != vec4(1.f)) {
			if (auto *img = dynamic_cast<Image *>(view.get())) {
				img->SetTint(imageTint);
			}
		}
		if (!imageSrc.empty()) {
			view->ApplyXmlAttribute("src", imageSrc, const_cast<LayoutLoader *>(&loader));
		}
		if (auto *img = dynamic_cast<Image *>(view.get())) {
			if (!imageIcon.empty()) {
				if (TextureIconBank *bank = loader.ResolveTextureIconBank(imageBank)) {
					if (const IconEntry *entry = bank->GetIcon(imageIcon)) {
						img->SetTexture(entry->texture);
						img->SetUVRegion(entry->uvMin, entry->uvMax);
					} else {
						AQUILA_LOG_WARNING("LayoutLoader: icon '{}' not found in bank '{}'", imageIcon,
										   imageBank.empty() ? "default" : imageBank);
					}
				} else {
					AQUILA_LOG_WARNING("LayoutLoader: no TextureIconBank registered as '{}'",
									   imageBank.empty() ? "default" : imageBank);
				}
			}
			if (!imageUV.empty()) {
				float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
				std::istringstream ss(imageUV);
				if (ss >> u0 >> v0 >> u1 >> v1) {
					img->SetUVRegion({ u0, v0 }, { u1, v1 });
				}
			}
		}

		if (!AtEnd() && Peek() == '/') {
			Advance();
			Expect('>');
			return view;
		}

		Expect('>');

		while (!AtEnd()) {
			SkipWS();
			if (AtEnd()) {
				break;
			}

			if (Peek() == '<' && Peek2() == '/') {
				pos += 2;
				SkipWS();
				ReadName();
				SkipWS();
				Expect('>');
				break;
			}

			if (Peek() == '<' && pos + 1 < src.size() && src[pos + 1] == '!') {
				TrySkipComment();
				continue;
			}

			if (Peek() == '<') {
				auto child = ParseElement();
				if (child) {
					view->AddChild(std::move(child));
				}
				continue;
			}

			std::string text = ReadTextContent();
			if (!text.empty()) {
				view->ApplyXmlTextContent(text);

				if (font != nullptr) {
					view->SetFont(font);
				}
			}
		}

		return view;
	}

	Unique<View> Parse() {
		SkipWS();
		while (!AtEnd()) {
			if (Peek() == '<' && Peek2() == '!') {
				TrySkipComment();
				SkipWS();
				continue;
			}
			if (Peek() == '<' && pos + 1 < src.size() && src[pos + 1] == '?') {
				while (!AtEnd() && Peek() != '>') {
					Advance();
				}
				if (!AtEnd()) {
					Advance();
				}
				SkipWS();
				continue;
			}
			if (Peek() == '<') {
				return ParseElement();
			}
			Advance();
		}
		return nullptr;
	}
};

} // namespace

LayoutLoader::LayoutLoader() {
	RegisterBuiltins();
}

void LayoutLoader::RegisterFont(const std::string &name, Text::FontAtlas *font) {
	m_Fonts[name] = font;
	if (m_DefaultFont == nullptr) {
		m_DefaultFont = font;
	}
}

void LayoutLoader::SetDefaultFont(Text::FontAtlas *font) {
	m_DefaultFont = font;
}

void LayoutLoader::RegisterWidget(const std::string &typeName, WidgetFactory factory) {
	m_Factories[typeName] = std::move(factory);
}

Text::FontAtlas *LayoutLoader::ResolveFont(const std::string &name) const {
	auto it = m_Fonts.find(name);
	return (it != m_Fonts.end()) ? it->second : m_DefaultFont;
}

void LayoutLoader::RegisterTextureCache(TextureCache *cache) {
	m_TextureCache = cache;
}

GFX::GfxTexture *LayoutLoader::ResolveTexture(const std::string &path) const {
	if (m_TextureCache == nullptr) {
		AQUILA_LOG_ERROR("LayoutLoader: no TextureCache registered — call RegisterTextureCache() first");
		return nullptr;
	}
	return m_TextureCache->Load(path);
}

void LayoutLoader::RegisterTextureIconBank(const std::string &name, TextureIconBank *bank) {
	m_IconBanks[name] = bank;
}

TextureIconBank *LayoutLoader::ResolveTextureIconBank(const std::string &name) const {
	const std::string &key = name.empty() ? "default" : name;
	auto it = m_IconBanks.find(key);
	if (it != m_IconBanks.end()) {
		return it->second;
	}
	if (key != "default") {
		auto def = m_IconBanks.find("default");
		return (def != m_IconBanks.end()) ? def->second : nullptr;
	}
	return nullptr;
}

Unique<View> LayoutLoader::CreateWidget(const std::string &type, std::string_view text, Text::FontAtlas *font) const {
	auto it = m_Factories.find(type);
	if (it == m_Factories.end()) {
		return nullptr;
	}
	return it->second(text, font);
}

Unique<View> LayoutLoader::LoadFile(const std::string &path) {
	const std::string src = Platform::Filesystem::VirtualFileSystem::Get()->ReadTextFile(path);
	if (src.empty()) {
		AQUILA_LOG_ERROR("LayoutLoader: cannot open '{}'", path);
		return nullptr;
	}
	return LoadString(src);
}

Unique<View> LayoutLoader::LoadString(std::string_view xml) {
	Parser p{ .src = xml, .pos = 0, .loader = *this };
	return p.Parse();
}

void LayoutLoader::RegisterBuiltins() {
	m_Factories["View"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<View>(); };
	m_Factories["Label"] = [](std::string_view text, Text::FontAtlas *font) -> Unique<View> {
		auto lbl = CreateUnique<Label>(std::string(text), font);
		return lbl;
	};
	m_Factories["Button"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<Button>(); };
	m_Factories["Image"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<Image>(); };
	m_Factories["Checkbox"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Checkbox>();
	};
	m_Factories["Slider"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<Slider>(); };
	m_Factories["TextInput"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<TextInput>();
	};
	m_Factories["Popup"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<Popup>(); };
	m_Factories["ContextMenu"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<ContextMenu>();
	};
	m_Factories["NumberInput"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<NumberInput>();
	};
	m_Factories["DragFloat"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<DragFloat>();
	};
	m_Factories["DragInt"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<DragInt>();
	};
	m_Factories["Toggle"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<Toggle>(); };
	m_Factories["ScrollView"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<ScrollView>();
	};
	m_Factories["Collapsible"] = [](std::string_view text, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Collapsible>(std::string(text));
	};
	m_Factories["TabView"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<TabView>();
	};
	m_Factories["Dropdown"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Dropdown>();
	};
	m_Factories["Vec2Field"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Vec2Field>();
	};
	m_Factories["Vec3Field"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Vec3Field>();
	};
	m_Factories["Vec4Field"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Vec4Field>();
	};
	m_Factories["PropertyGrid"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<PropertyGrid>();
	};
	m_Factories["TreeView"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<TreeView>();
	};
	m_Factories["AssetPicker"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<AssetPicker>();
	};
}

} // namespace Aquila::UI::Core
