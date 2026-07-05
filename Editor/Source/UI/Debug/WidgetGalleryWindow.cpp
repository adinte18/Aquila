#include "UI/Debug/WidgetGalleryWindow.h"

#include "Aquila/Application/Events/Event.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleParser.h"

#include "Aquila/UI/Widgets/AssetCard.h"
#include "Aquila/UI/Widgets/AssetSlot.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/DragInt.h"
#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/ListBox.h"
#include "Aquila/UI/Widgets/NumberInput.h"
#include "Aquila/UI/Widgets/ProgressBar.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/Separator.h"
#include "Aquila/UI/Widgets/Slider.h"
#include "Aquila/UI/Widgets/TabView.h"
#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/UI/Widgets/Toggle.h"
#include "Aquila/UI/Widgets/Tooltip.h"
#include "Aquila/UI/Widgets/TreeView.h"
#include "Aquila/UI/Widgets/VecField.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

WidgetGalleryWindow::WidgetGalleryWindow() = default;
WidgetGalleryWindow::~WidgetGalleryWindow() = default;

View *WidgetGalleryWindow::AddGroup(View *host, const std::string &heading) {
	auto *group = host->AddChild<View>();
	group->AddClass("gallery-item");
	auto *label = group->AddChild<Label>(heading);
	label->AddClass("gallery-heading");
	return group;
}

void WidgetGalleryWindow::Build(GFX::GfxContext &ctx, TextureCache *textureCache, uint32 width, uint32 height,
								const std::string &stylePath) {
	m_Canvas = CreateUnique<Canvas>(width, height);
	UI::StyleParser::LoadFile(stylePath, m_Canvas->GetStyleSheet());

	auto *root = m_Canvas->GetRoot();
	root->SetId("gallery-root");
	root->AddClass("gallery-root");

	auto *header = root->AddChild<View>();
	header->AddClass("gallery-header");
	auto *title = header->AddChild<Label>(std::string("Widget Gallery"));
	title->AddClass("gallery-title");

	auto *scroll = root->AddChild<ScrollView>();
	scroll->AddClass("gallery-scroll");
	auto *content = scroll->AddContent<View>();
	content->AddClass("gallery-content");

	m_Tooltip = root->AddChild<Tooltip>();

	AddGroup(content, "Button")->AddChild<Button>(std::string("Click me"));

	AddGroup(content, "Label")->AddChild<Label>(std::string("The quick brown fox"));

	AddGroup(content, "Checkbox")->AddChild<Checkbox>(true);

	AddGroup(content, "Toggle")->AddChild<Toggle>(true);

	auto *slider = AddGroup(content, "Slider")->AddChild<Slider>();
	slider->SetRange(0.f, 100.f);
	slider->SetValue(40.f);

	auto *progress = AddGroup(content, "ProgressBar")->AddChild<ProgressBar>();
	progress->SetValue(0.65f);

	AddGroup(content, "TextInput")->AddChild<TextInput>(std::string("Type here…"));

	AddGroup(content, "NumberInput")->AddChild<NumberInput>();

	AddGroup(content, "DragFloat")->AddChild<DragFloat>();

	AddGroup(content, "DragInt")->AddChild<DragInt>();

	auto *dropdown = AddGroup(content, "Dropdown")->AddChild<Dropdown>();
	dropdown->AddOption("first", "First");
	dropdown->AddOption("second", "Second");
	dropdown->AddOption("third", "Third");

	AddGroup(content, "ColorPicker")->AddChild<ColorPicker>(ctx, vec4(0.8f, 0.4f, 0.2f, 1.f));

	AddGroup(content, "Vec3Field")->AddChild<Vec3Field>();

	AddGroup(content, "Separator")->AddChild<Separator>();

	auto *collapsible = AddGroup(content, "Collapsible")->AddChild<Collapsible>(std::string("Expandable section"));
	collapsible->AddContent<Label>(std::string("Hidden content revealed on expand"));

	auto *tabs = AddGroup(content, "TabView")->AddChild<TabView>();
	tabs->AddClass("gallery-tabview");
	tabs->AddTab("Tab One")->AddChild<Label>(std::string("Contents of tab one"));
	tabs->AddTab("Tab Two")->AddChild<Label>(std::string("Contents of tab two"));
	tabs->SetActiveTab(0);

	auto *listBox = AddGroup(content, "ListBox")->AddChild<ListBox>();
	listBox->AddClass("gallery-listbox");
	listBox->AddItem("a", "Alpha");
	listBox->AddItem("b", "Bravo");
	listBox->AddItem("c", "Charlie");

	auto *tree = AddGroup(content, "TreeView")->AddChild<TreeView>();
	auto *treeRoot = tree->AddNode("Root");
	treeRoot->AddChildNode("Child A");
	treeRoot->AddChildNode("Child B");

	auto *grid = AddGroup(content, "PropertyGrid")->AddChild<PropertyGrid>();
	grid->AddRow<DragFloat>("Scale");
	grid->AddRow<Checkbox>("Visible");

	if (textureCache) {
		auto *image =
			AddGroup(content, "Image")->AddChild<Image>(textureCache->Load("Engine/UI/Icons/info.png"), vec4(1.f));
		image->AddClass("gallery-image");
	}

	AddGroup(content, "AssetSlot")->AddChild<AssetSlot>(std::string("texture"));

	auto *card = AddGroup(content, "AssetCard")->AddChild<AssetCard>();
	card->SetAsset(AssetPayload{ "textures/wood.png", "texture", "wood.png" });
	if (textureCache) {
		card->SetThumbnail(textureCache->Load("Engine/UI/Icons/info.png"));
	}

	auto *tooltipBtn = AddGroup(content, "Tooltip")->AddChild<Button>(std::string("Show tooltip"));
	Tooltip *tip = m_Tooltip;
	tooltipBtn->onClick.Connect([tip, tooltipBtn] {
		const Rect rect = tooltipBtn->GetAbsoluteRect();
		tip->ShowAt({ rect.position.x, rect.position.y + rect.size.y + 4.f }, "Hello from a Tooltip!");
	});

	m_Canvas->ReloadStyles();
}

void WidgetGalleryWindow::Update(f32 deltaTime) {
	m_Canvas->Update(deltaTime);
	m_Canvas->Compute();
}

void WidgetGalleryWindow::Render(Graphics::QuadBatcher &batcher, GFX::GfxCommandList &cmd) {
	m_Canvas->SubmitToQuadBatcher(batcher, cmd);
}

void WidgetGalleryWindow::OnEvent(Application::Events::Event &event) {
	Application::Events::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<Application::Events::WindowResizeEvent>([this](Application::Events::WindowResizeEvent &e) {
		if (e.GetWidth() > 0 && e.GetHeight() > 0) {
			m_Canvas->Resize(e.GetWidth(), e.GetHeight());
		}
		return false;
	});

	m_Canvas->OnEvent(event);
}

} // namespace Editor
