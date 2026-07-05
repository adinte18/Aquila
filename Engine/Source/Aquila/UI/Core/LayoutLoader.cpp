#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/UI/Widgets/IconLabel.h"
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
#include "Aquila/UI/Widgets/Separator.h"
#include "Aquila/UI/Widgets/ProgressBar.h"
#include "Aquila/UI/Widgets/Tooltip.h"
#include "Aquila/UI/Widgets/Menubar.h"
#include "Aquila/UI/Widgets/ListBox.h"
#include "Aquila/UI/Widgets/AssetSlot.h"
#include "Aquila/UI/Widgets/AssetCard.h"
#include "Aquila/UI/Widgets/DockSpace.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Style/StyleParserHelper.h"
#include <algorithm>
#include <utility>

namespace Aquila::UI::Core {

namespace {

struct Parser {
	std::string_view src;
	size_t pos = 0;
	LayoutLoader &loader;

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
		while (!AtEnd() &&
			   ((std::isalnum(static_cast<unsigned char>(Peek())) != 0) || Peek() == '-' || Peek() == '_' ||
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
		std::string imageTint;
		std::vector<std::pair<std::string, std::string>> genericAttrs;

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
				imageTint = attrValue;
			} else {
				genericAttrs.emplace_back(std::move(attrName), std::move(attrValue));
			}
		}

		if (tagName == "Include") {
			if (!AtEnd() && Peek() == '/') {
				Advance();
			}
			Expect('>');
			if (imageSrc.empty()) {
				AQUILA_LOG_ERROR("LayoutLoader: <Include> is missing a 'src' attribute");
				return nullptr;
			}
			return loader.LoadFile(imageSrc);
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

		LayoutLoader *loaderCtx = const_cast<LayoutLoader *>(&loader);
		if (!imageTint.empty()) {
			view->ApplyXmlAttribute("tint", imageTint, loaderCtx);
		}
		if (!imageSrc.empty()) {
			view->ApplyXmlAttribute("src", imageSrc, loaderCtx);
		}
		if (!imageBank.empty()) {
			view->ApplyXmlAttribute("bank", imageBank, loaderCtx);
		}
		if (!imageIcon.empty()) {
			view->ApplyXmlAttribute("icon", imageIcon, loaderCtx);
		}
		if (!imageUV.empty()) {
			view->ApplyXmlAttribute("uv", imageUV, loaderCtx);
		}

		for (const auto &attr : genericAttrs) {
			view->ApplyXmlAttribute(attr.first, attr.second, loaderCtx);
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

		view->OnXmlLoaded();
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

void LayoutLoader::RegisterCommand(const std::string &name, Delegate<void()> command) {
	m_Commands[name] = std::move(command);
}

Delegate<void()> LayoutLoader::ResolveCommand(const std::string &name) const {
	auto it = m_Commands.find(name);
	return (it != m_Commands.end()) ? it->second : Delegate<void()>{};
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
	std::string resolved = path;
	if (!path.empty() && path.front() != '/' && !m_CurrentDir.empty()) {
		resolved = m_CurrentDir + "/" + path;
	}

	if (std::find(m_IncludeStack.begin(), m_IncludeStack.end(), resolved) != m_IncludeStack.end()) {
		AQUILA_LOG_ERROR("LayoutLoader: include cycle detected at '{}'", resolved);
		return nullptr;
	}

	const std::string src = Platform::Filesystem::VirtualFileSystem::Get()->ReadTextFile(resolved);
	if (src.empty()) {
		AQUILA_LOG_ERROR("LayoutLoader: cannot open '{}'", resolved);
		return nullptr;
	}

	const std::string prevDir = m_CurrentDir;
	const auto slash = resolved.find_last_of('/');
	m_CurrentDir = (slash != std::string::npos) ? resolved.substr(0, slash) : std::string();
	m_IncludeStack.push_back(resolved);

	Unique<View> result = LoadString(src);

	m_IncludeStack.pop_back();
	m_CurrentDir = prevDir;
	return result;
}

Unique<View> LayoutLoader::LoadString(std::string_view xml) {
	Parser p{ .src = xml, .pos = 0, .loader = *this };
	return p.Parse();
}

void LayoutLoader::RegisterBuiltins() {
	m_Factories["Label"] = [](std::string_view text, Text::FontAtlas *font) -> Unique<View> {
		return CreateUnique<Label>(std::string(text), font);
	};
	m_Factories["Collapsible"] = [](std::string_view text, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Collapsible>(std::string(text));
	};

	Register<View>("View");
	Register<Button>("Button");
	Register<Image>("Image");
	Register<IconLabel>("IconLabel");
	Register<Checkbox>("Checkbox");
	Register<Slider>("Slider");
	Register<TextInput>("TextInput");
	Register<Popup>("Popup");
	Register<ContextMenu>("ContextMenu");
	Register<NumberInput>("NumberInput");
	Register<DragFloat>("DragFloat");
	Register<DragInt>("DragInt");
	Register<Toggle>("Toggle");
	Register<ScrollView>("ScrollView");
	Register<TabView>("TabView");
	Register<Dropdown>("Dropdown");
	Register<Vec2Field>("Vec2Field");
	Register<Vec3Field>("Vec3Field");
	Register<Vec4Field>("Vec4Field");
	Register<PropertyGrid>("PropertyGrid");
	Register<TreeView>("TreeView");
	Register<Separator>("Separator");
	Register<ProgressBar>("ProgressBar");
	Register<Tooltip>("Tooltip");
	Register<MenuBar>("MenuBar");
	Register<ListBox>("ListBox");
	Register<AssetSlot>("AssetSlot");
	Register<AssetCard>("AssetCard");
	Register<DockSpace>("DockSpace");
	Register<DockNode>("DockNode");
	Register<DockPanel>("DockPanel");
}

} // namespace Aquila::UI::Core
