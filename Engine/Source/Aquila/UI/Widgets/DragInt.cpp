#include "Aquila/UI/Widgets/DragInt.h"

namespace Aquila::UI::Core {

DragInt::DragInt() {
	SetStep(1.f);
	SetPrecision(0);
}

std::string DragInt::FormatValue() const {
	return m_Prefix + std::to_string(GetIntValue());
}

void DragInt::OnValueCommitted() {
	// Round to nearest integer after every drag tick.
	m_Value = std::round(m_Value);
}

} // namespace Aquila::UI::Core
