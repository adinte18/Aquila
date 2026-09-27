#include "Aquila/UI/Widgets/DragInt.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::UI::Core {

DragInt::DragInt() {
	set_step(1.F);
	set_precision(0);
}

std::string DragInt::format_value() const {
	return m_prefix + std::to_string(get_int_value());
}

void DragInt::on_value_committed() {
	// Round to nearest integer after every drag tick.
	m_value = Math::round(m_value);
}

} // namespace Aquila::UI::Core
