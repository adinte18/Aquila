#include "UI/Managers/FontManager.h"
#include "Aquila/Core/Defines.h"
#include "lucide.h"
#include <imgui.h>

#ifdef AQUILA_PLATFORM_WINDOWS
#include <windows.h>
#elif defined(AQUILA_PLATFORM_LINUX) || defined(AQUILA_PLATFORM_MACOS)
#include <cstdlib>
#endif

namespace Editor::UI {

FontManager &FontManager::Get() {
	static FontManager instance;
	return instance;
}

void FontManager::Initialize(const Editor::Config::FontSettings &settings) {
	if (m_Initialized) {
		AQUILA_LOG_WARNING("FontManager already initialized");
		return;
	}

	m_Settings = settings;

	AQUILA_LOG_INFO("FontSettings.mainFontPath = {}", m_Settings.mainFontPath);

	m_DefaultFontSize = settings.defaultFontSize;

	AQUILA_LOG_INFO("Initializing FontManager...");

	if (!LoadDefaultFonts()) {
		AQUILA_LOG_ERROR("Failed to load default fonts");
		return;
	}

	m_Initialized = true;
	AQUILA_LOG_INFO("FontManager initialized successfully");
}

void FontManager::Shutdown() {
	if (!m_Initialized) {
		return;
	}

	m_Fonts.clear();
	m_DefaultFont = nullptr;
	m_Font12 = m_Font14 = m_Font16 = m_Font18 = m_Font20 = nullptr;
	m_Font24 = m_Font28 = m_Font32 = m_Font40 = m_Font48 = nullptr;

	m_Initialized = false;
	AQUILA_LOG_INFO("FontManager shut down");
}

bool FontManager::LoadDefaultFonts() {
	AQUILA_LOG_INFO("Loading default fonts from: {}", m_Settings.mainFontPath);

	m_Font12 = LoadSingleFont(m_Settings.mainFontPath, 12.0f, true);
	m_Font14 = LoadSingleFont(m_Settings.mainFontPath, 14.0f, true);
	m_Font16 = LoadSingleFont(m_Settings.mainFontPath, 16.0f, true);
	m_Font18 = LoadSingleFont(m_Settings.mainFontPath, 18.0f, true);
	m_Font20 = LoadSingleFont(m_Settings.mainFontPath, 20.0f, true);
	m_Font24 = LoadSingleFont(m_Settings.mainFontPath, 24.0f, true);
	m_Font28 = LoadSingleFont(m_Settings.mainFontPath, 28.0f, true);
	m_Font32 = LoadSingleFont(m_Settings.mainFontPath, 32.0f, true);
	m_Font40 = LoadSingleFont(m_Settings.mainFontPath, 40.0f, true);
	m_Font48 = LoadSingleFont(m_Settings.mainFontPath, 48.0f, true);

	m_DefaultFont = m_Font16;

	bool allLoaded = m_Font12 && m_Font14 && m_Font16 && m_Font18 && m_Font20 && m_Font24 && m_Font28 && m_Font32 &&
					 m_Font40 && m_Font48;

	if (!allLoaded) {
		AQUILA_LOG_ERROR("Failed to load all default font sizes");
		return false;
	}

	CachePredefinedFonts();

	AQUILA_LOG_INFO("Default fonts loaded successfully");
	return true;
}

bool FontManager::LoadFont(const std::string &name, const std::string &path, f32 size) {
	ImFont *font = LoadSingleFont(path, size, false);

	if (!font) {
		AQUILA_LOG_ERROR("Failed to load font: {} ({}pt)", name, size);
		return false;
	}

	FontDescriptor desc;
	desc.name = name;
	desc.path = path;
	desc.size = size;
	desc.font = font;

	m_Fonts[name] = desc;

	AQUILA_LOG_INFO("Font loaded: {} ({}pt)", name, size);
	return true;
}

bool FontManager::LoadFontWithIcons(const std::string &name, const std::string &fontPath, const std::string &iconPath,
									f32 size) {
	ImGuiIO &io = ImGui::GetIO();

	ImFontConfig config;
	config.OversampleH = m_Settings.oversampleH;
	config.OversampleV = m_Settings.oversampleV;
	config.PixelSnapH = true;

	ImFont *font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), size, &config);

	if (!font) {
		AQUILA_LOG_ERROR("Failed to load base font: {}", fontPath);
		return false;
	}

	// Merge icon font
	static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };
	ImFontConfig icon_config;
	icon_config.MergeMode = true;
	icon_config.PixelSnapH = true;
	icon_config.GlyphMinAdvanceX = size;
	icon_config.GlyphOffset.y = size * 0.25f;

	io.Fonts->AddFontFromFileTTF(iconPath.c_str(), size, &icon_config, icon_ranges);

	FontDescriptor desc;
	desc.name = name;
	desc.path = fontPath;
	desc.size = size;
	desc.font = font;

	m_Fonts[name] = desc;

	AQUILA_LOG_INFO("Font with icons loaded: {} ({}pt)", name, size);
	return true;
}

ImFont *FontManager::GetFont(const std::string &name) const {
	auto it = m_Fonts.find(name);
	if (it != m_Fonts.end()) {
		return it->second.font;
	}

	AQUILA_LOG_WARNING("Font not found: {}", name);
	return m_DefaultFont;
}

ImFont *FontManager::GetFontAtSize(f32 size) const {
	// Find closest matching size
	if (size <= 13.0f) {
		return m_Font12;
	}
	if (size <= 15.0f) {
		return m_Font14;
	}
	if (size <= 17.0f) {
		return m_Font16;
	}
	if (size <= 19.0f) {
		return m_Font18;
	}
	if (size <= 22.0f) {
		return m_Font20;
	}
	if (size <= 26.0f) {
		return m_Font24;
	}
	if (size <= 30.0f) {
		return m_Font28;
	}
	if (size <= 36.0f) {
		return m_Font32;
	}
	if (size <= 44.0f) {
		return m_Font40;
	}
	return m_Font48;
}

void FontManager::ReloadFonts() {
	AQUILA_LOG_INFO("Reloading fonts...");

	Shutdown();
	Initialize(m_Settings);

	AQUILA_LOG_INFO("Fonts reloaded");
}

void FontManager::SetDefaultFontSize(f32 size) {
	m_DefaultFontSize = size;
	m_DefaultFont = GetFontAtSize(size);
}

ImFont *FontManager::LoadSingleFont(const std::string &path, f32 size, bool mergeIcons) {
	ImGuiIO &io = ImGui::GetIO();

	ImFontConfig config;
	config.OversampleH = m_Settings.oversampleH;
	config.OversampleV = m_Settings.oversampleV;
	config.PixelSnapH = true;

	ImFont *font = io.Fonts->AddFontFromFileTTF(path.c_str(), size, &config);

	if (!font) {
		AQUILA_LOG_ERROR("Failed to load font: {} at {}pt", path, size);
		return nullptr;
	}

	if (mergeIcons) {
		static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };
		ImFontConfig icon_config;
		icon_config.MergeMode = true;
		icon_config.PixelSnapH = true;
		icon_config.GlyphMinAdvanceX = size;
		icon_config.GlyphOffset.y = size * 0.25f;

		io.Fonts->AddFontFromFileTTF(m_Settings.iconsFontPath.c_str(), size, &icon_config, icon_ranges);
	}

	return font;
}

void FontManager::CachePredefinedFonts() {
	// Cache predefined fonts with descriptive names
	if (m_Font12) {
		FontDescriptor desc{ "Default12", m_Settings.mainFontPath, 12.0f, m_Font12 };
		m_Fonts["Default12"] = desc;
	}
	if (m_Font14) {
		FontDescriptor desc{ "Default14", m_Settings.mainFontPath, 14.0f, m_Font14 };
		m_Fonts["Default14"] = desc;
	}
	if (m_Font16) {
		FontDescriptor desc{ "Default16", m_Settings.mainFontPath, 16.0f, m_Font16 };
		m_Fonts["Default16"] = desc;
	}
	if (m_Font18) {
		FontDescriptor desc{ "Default18", m_Settings.mainFontPath, 18.0f, m_Font18 };
		m_Fonts["Default18"] = desc;
	}
	if (m_Font20) {
		FontDescriptor desc{ "Default20", m_Settings.mainFontPath, 20.0f, m_Font20 };
		m_Fonts["Default20"] = desc;
	}
	if (m_Font24) {
		FontDescriptor desc{ "Default24", m_Settings.mainFontPath, 24.0f, m_Font24 };
		m_Fonts["Default24"] = desc;
	}
	if (m_Font28) {
		FontDescriptor desc{ "Default28", m_Settings.mainFontPath, 28.0f, m_Font28 };
		m_Fonts["Default28"] = desc;
	}
	if (m_Font32) {
		FontDescriptor desc{ "Default32", m_Settings.mainFontPath, 32.0f, m_Font32 };
		m_Fonts["Default32"] = desc;
	}
	if (m_Font40) {
		FontDescriptor desc{ "Default40", m_Settings.mainFontPath, 40.0f, m_Font40 };
		m_Fonts["Default40"] = desc;
	}
	if (m_Font48) {
		FontDescriptor desc{ "Default48", m_Settings.mainFontPath, 48.0f, m_Font48 };
		m_Fonts["Default48"] = desc;
	}
}

void FontManager::ScanSystemFonts() {
	if (m_SystemFontsScanned) {
		AQUILA_LOG_INFO("System fonts already scanned");
		return;
	}

	AQUILA_LOG_INFO("Scanning system fonts...");
	m_SystemFonts.clear();

	// Platform-specific font directories
	std::vector<std::string> fontDirectories;

#ifdef AQUILA_PLATFORM_WINDOWS
	// Windows font directories
	char windir[MAX_PATH];
	GetEnvironmentVariableA("WINDIR", windir, MAX_PATH);
	fontDirectories.push_back(std::string(windir) + "\\Fonts");

	// User fonts
	char localAppData[MAX_PATH];
	GetEnvironmentVariableA("LOCALAPPDATA", localAppData, MAX_PATH);
	fontDirectories.push_back(std::string(localAppData) + "\\Microsoft\\Windows\\Fonts");

#elif defined(AQUILA_PLATFORM_LINUX)
	// Linux font directories
	fontDirectories.push_back("/usr/share/fonts");
	fontDirectories.push_back("/usr/local/share/fonts");
	fontDirectories.push_back(std::string(getenv("HOME")) + "/.fonts");
	fontDirectories.push_back(std::string(getenv("HOME")) + "/.local/share/fonts");
	fontDirectories.push_back("/usr/share/fonts/truetype");
	fontDirectories.push_back("/usr/share/fonts/opentype");

#elif defined(AQUILA_PLATFORM_MACOS)
	// macOS font directories
	fontDirectories.push_back("/System/Library/Fonts");
	fontDirectories.push_back("/Library/Fonts");
	fontDirectories.push_back(std::string(getenv("HOME")) + "/Library/Fonts");
#endif

	// Scan all directories
	for (const auto &dir : fontDirectories) {
		ScanFontDirectory(dir);
	}

	// Sort by family name for easier browsing
	std::sort(m_SystemFonts.begin(), m_SystemFonts.end(), [](const SystemFont &a, const SystemFont &b) {
		if (a.family == b.family) {
			return a.style < b.style;
		}
		return a.family < b.family;
	});

	m_SystemFontsScanned = true;
	AQUILA_LOG_INFO("Found {} system fonts", m_SystemFonts.size());
}

void FontManager::ScanFontDirectory(const std::string &directory) {
	namespace fs = std::filesystem;

	if (!fs::exists(directory) || !fs::is_directory(directory)) {
		return;
	}

	try {
		for (const auto &entry : fs::recursive_directory_iterator(directory)) {
			if (!entry.is_regular_file()) {
				continue;
			}

			std::string extension = entry.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

			// Only TTF and OTF fonts
			if (extension != ".ttf" && extension != ".otf") {
				continue;
			}

			SystemFont font;
			font.path = entry.path().string();
			font.isTrueType = (extension == ".ttf");

			std::string filename = entry.path().stem().string();
			font.family = ExtractFontFamily(filename);
			font.style = ExtractFontStyle(filename);
			font.name = font.family + " " + font.style;

			m_SystemFonts.push_back(font);
		}
	} catch (const std::exception &e) {
		AQUILA_LOG_WARNING("Error scanning font directory {}: {}", directory, e.what());
	}
}

std::string FontManager::ExtractFontFamily(const std::string &filename) const {
	std::vector<std::string> styleSuffixes = { "Regular", "Bold",  "Italic", "BoldItalic", "Light",	   "Medium",
											   "Thin",	  "Black", "Heavy",	 "Oblique",	   "SemiBold", "ExtraBold" };

	std::string family = filename;

	for (const auto &style : styleSuffixes) {
		size_t pos = family.find("-" + style);
		if (pos != std::string::npos) {
			family = family.substr(0, pos);
			break;
		}
		pos = family.find("_" + style);
		if (pos != std::string::npos) {
			family = family.substr(0, pos);
			break;
		}
		pos = family.find(style);
		if (pos != std::string::npos && pos > 0) {
			family = family.substr(0, pos);
			break;
		}
	}

	while (!family.empty() && (family.back() == '-' || family.back() == '_')) {
		family.pop_back();
	}

	return family.empty() ? filename : family;
}

std::string FontManager::ExtractFontStyle(const std::string &filename) const {
	std::vector<std::string> styles = { "BoldItalic", "Bold",  "Italic",  "Light",	  "Medium",	   "Thin",
										"Black",	  "Heavy", "Oblique", "SemiBold", "ExtraBold", "Regular" };

	for (const auto &style : styles) {
		if (filename.find(style) != std::string::npos) {
			return style;
		}
	}

	return "Regular";
}

const FontManager::SystemFont *FontManager::FindSystemFont(const std::string &familyName,
														   const std::string &style) const {
	for (const auto &font : m_SystemFonts) {
		if (font.family == familyName && font.style == style) {
			return &font;
		}
	}

	// Try to find just the family with Regular style
	if (style != "Regular") {
		for (const auto &font : m_SystemFonts) {
			if (font.family == familyName && font.style == "Regular") {
				return &font;
			}
		}
	}

	return nullptr;
}

bool FontManager::LoadSystemFont(const std::string &familyName, const std::string &style, f32 size) {
	if (!m_SystemFontsScanned) {
		ScanSystemFonts();
	}

	const SystemFont *sysFont = FindSystemFont(familyName, style);
	if (!sysFont) {
		AQUILA_LOG_ERROR("System font not found: {} {}", familyName, style);
		return false;
	}

	std::string fontName = familyName + "_" + style + "_" + std::to_string(static_cast<int>(size));
	return LoadFont(fontName, sysFont->path, size);
}

void FontManager::ShowFontSelector(const char *label, std::string &selectedFont, std::string &selectedStyle) {
	if (!m_SystemFontsScanned) {
		if (ImGui::Button("Scan System Fonts")) {
			ScanSystemFonts();
		}
		return;
	}

	ImGui::Text("%s", label);

	// Get unique font families
	std::set<std::string> families;
	for (const auto &font : m_SystemFonts) {
		families.insert(font.family);
	}

	// Font family selector
	if (ImGui::BeginCombo("Font Family", selectedFont.c_str())) {
		for (const auto &family : families) {
			bool isSelected = (selectedFont == family);
			if (ImGui::Selectable(family.c_str(), isSelected)) {
				selectedFont = family;
				// Auto-select Regular style if available
				selectedStyle = "Regular";
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	// Style selector (only show styles available for selected family)
	if (!selectedFont.empty()) {
		std::vector<std::string> availableStyles;
		for (const auto &font : m_SystemFonts) {
			if (font.family == selectedFont) {
				availableStyles.push_back(font.style);
			}
		}

		if (!availableStyles.empty()) {
			if (ImGui::BeginCombo("Font Style", selectedStyle.c_str())) {
				for (const auto &style : availableStyles) {
					bool isSelected = (selectedStyle == style);
					if (ImGui::Selectable(style.c_str(), isSelected)) {
						selectedStyle = style;
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
	}

	// Preview
	if (!selectedFont.empty()) {
		const SystemFont *font = FindSystemFont(selectedFont, selectedStyle);
		if (font) {
			ImGui::TextDisabled("Path: %s", font->path.c_str());
		}
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Total fonts available: %zu", m_SystemFonts.size());
}

} // namespace Editor::UI
