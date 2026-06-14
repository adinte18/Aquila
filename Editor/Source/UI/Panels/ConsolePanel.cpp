#include "UI/Panels/ConsolePanel.h"

namespace Editor {

using namespace Aquila;

void ConsolePanel::Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *) {
	// create a scroll view
	auto consoleScrollView = CreateUnique<UI::Core::ScrollView>();
	consoleScrollView->SetId("console-scroll");

	m_ScrollView = static_cast<UI::Core::ScrollView *>(panel->AddChild(std::move(consoleScrollView)));

	auto label1 = CreateUnique<UI::Core::Label>("Hello");
	label1->AddClass("console-text");
	auto label2 = CreateUnique<UI::Core::Label>("Hello2");
	label2->AddClass("console-text");
	auto label3 = CreateUnique<UI::Core::Label>("Hello3");
	label3->AddClass("console-text");

	m_ScrollView->AddContent(std::move(label1));
	m_ScrollView->AddContent(std::move(label2));
	m_ScrollView->AddContent(std::move(label3));
}

} // namespace Editor
