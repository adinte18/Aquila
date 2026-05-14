#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/UI/Widgets/Label.h"
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

	// Tag / attribute name: letters, digits, hyphens, underscores.
	std::string ReadName() {
		size_t start = pos;
		while (!AtEnd() && ((std::isalnum(static_cast<unsigned char>(Peek())) != 0) || Peek() == '-' || Peek() == '_' ||
							Peek() == ':')) {
			Advance();
		}
		return std::string(src.substr(start, pos - start));
	}

	// Quoted string (single or double quotes), returns content without quotes.
	std::string ReadQuoted() {
		char q = Advance();
		size_t start = pos;
		while (!AtEnd() && Peek() != q) {
			Advance();
		}
		std::string val(src.substr(start, pos - start));
		if (!AtEnd()) {
			Advance(); // closing quote
		}
		return val;
	}

	// Unquoted attribute value — reads until whitespace, '>', or '/'.
	std::string ReadUnquoted() {
		size_t start = pos;
		while (!AtEnd() && Peek() != '>' && Peek() != '/' && (std::isspace(static_cast<unsigned char>(Peek())) == 0)) {
			Advance();
		}
		return std::string(src.substr(start, pos - start));
	}

	// Text content between tags — reads until '<', trims whitespace.
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

	// Skip <!-- ... --> comment. Call when pos is at '<'.
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

	// Called when pos is at '<' (and it is NOT a closing tag or comment).
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

		// Image-widget attributes collected during the attribute loop.
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
				// space-separated class list
				std::istringstream iss(attrValue);
				std::string cls;
				while (iss >> cls) {
					classes.push_back(cls);
				}
			} else if (attrName == "font") {
				font = loader.ResolveFont(attrValue);
			} else if (attrName == "style") {
				// Inline style block: "prop: value; prop: value"
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

		// Text content is unknown until we parse the body; create with empty
		// text first, then update below if we find inline text.
		Unique<View> view = loader.CreateWidget(tagName, "", font);
		if (!view) {
			AQUILA_LOG_ERROR("LayoutLoader: unknown widget type '{}'", tagName);
			// skip to closing tag to keep the parser alive
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
		view->SetStyle(props);

		// Apply Image-specific attributes after construction.
		if (auto *img = dynamic_cast<Image *>(view.get())) {
			img->SetTint(imageTint);

			// src="path" — load texture from disk via the registered TextureCache.
			if (!imageSrc.empty()) {
				if (GFX::GfxTexture *tex = loader.ResolveTexture(imageSrc)) {
					img->SetTexture(tex);
				} else {
					AQUILA_LOG_WARNING("LayoutLoader: could not load image '{}'", imageSrc);
				}
			}

			// icon="name" bank="bankName" — look up a named icon in a TextureIconBank.
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

			// uv="u0 v0 u1 v1" — override UV region (e.g. for sprite atlas slices).
			if (!imageUV.empty()) {
				float u0 = 0.f, v0 = 0.f, u1 = 1.f, v1 = 1.f;
				std::istringstream ss(imageUV);
				if (ss >> u0 >> v0 >> u1 >> v1) {
					img->SetUVRegion({ u0, v0 }, { u1, v1 });
				}
			}
		}

		if (!AtEnd() && Peek() == '/') {
			Advance(); // '/'
			Expect('>');
			return view;
		}

		Expect('>');

		// Children / text content
		while (!AtEnd()) {
			SkipWS();
			if (AtEnd()) {
				break;
			}

			// Closing tag?
			if (Peek() == '<' && Peek2() == '/') {
				pos += 2; // consume '</'
				SkipWS();
				ReadName(); // consume tag name (we trust the author lol)
				SkipWS();
				Expect('>');
				break;
			}

			// Comment?
			if (Peek() == '<' && pos + 1 < src.size() && src[pos + 1] == '!') {
				TrySkipComment();
				continue;
			}

			// Child element?
			if (Peek() == '<') {
				auto child = ParseElement();
				if (child) {
					view->AddChild(std::move(child));
				}
				continue;
			}

			// Text content — applies to Label / Button
			std::string text = ReadTextContent();
			if (!text.empty()) {
				if (auto *lbl = dynamic_cast<Label *>(view.get())) {
					lbl->SetText(text);
					if (font != nullptr) {
						lbl->SetFont(font);
					}
					// Re-measure and bake pixel size so Clay knows the label dimensions.
					// vec2 sz = lbl->Measure();
					// if (sz.x > 0.f || sz.y > 0.f) {
					// 	UI::StyleProperties textProps = lbl->GetStyle();
					// 	textProps.width = UI::StyleLength::Pixel(sz.x);
					// 	textProps.height = UI::StyleLength::Pixel(sz.y);
					// 	lbl->SetStyle(textProps);
					// }
				} else if (auto *btn = dynamic_cast<Button *>(view.get())) {
					btn->SetText(text);
					if (font != nullptr) {
						btn->SetFont(font);
					}
				}
			}
		}

		return view;
	}

	Unique<View> Parse() {
		SkipWS();
		while (!AtEnd()) {
			// Skip comments and XML declarations at top level.
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
			Advance(); // ignore stray characters
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
	// Empty name resolves to "default".
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
	std::ifstream f(path);
	if (!f.is_open()) {
		AQUILA_LOG_ERROR("LayoutLoader: cannot open '{}'", path);
		return nullptr;
	}
	std::ostringstream ss;
	ss << f.rdbuf();
	return LoadString(ss.str());
}

Unique<View> LayoutLoader::LoadString(std::string_view xml) {
	Parser p{ .src = xml, .pos = 0, .loader = *this };
	return p.Parse();
}

void LayoutLoader::RegisterBuiltins() {
	// Plain container
	m_Factories["View"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<View>(); };

	// Label, text content is applied by the parser after construction.
	m_Factories["Label"] = [](std::string_view text, Text::FontAtlas *font) -> Unique<View> {
		auto lbl = CreateUnique<Label>(std::string(text), font);
		return lbl;
	};

	// Button — label is created lazily via SetText() so <Button><Image/>...</Button> works.
	m_Factories["Button"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> {
		return CreateUnique<Button>();
	};

	// Image — texture/UV/tint are applied by the parser after construction.
	m_Factories["Image"] = [](std::string_view, Text::FontAtlas *) -> Unique<View> { return CreateUnique<Image>(); };
}

} // namespace Aquila::UI::Core
