#include "Aquila/UI/Text/TextSelection.h"

#include "Aquila/Foundation/Text/Utf8.h"

namespace Aquila::UI::Text {

std::string extract_text(std::span<const std::string> lines, const TextSelection &selection) {
	if (lines.empty() || selection.is_empty()) {
		return {};
	}

	const Int32 last_line = static_cast<Int32>(lines.size()) - 1;
	const TextPosition begin = selection.begin();
	const TextPosition end = selection.end();
	const Int32 first = std::clamp(begin.line, 0, last_line);
	const Int32 last = std::clamp(end.line, 0, last_line);

	auto clamp_column = [&lines](Int32 line, Int32 column) {
		return std::clamp(column, 0, static_cast<Int32>(lines[static_cast<Usize>(line)].size()));
	};

	std::string result;
	for (Int32 line = first; line <= last; ++line) {
		const std::string &text = lines[static_cast<Usize>(line)];
		const Int32 from = line == first ? clamp_column(line, begin.column) : 0;
		const Int32 to = line == last ? clamp_column(line, end.column) : static_cast<Int32>(text.size());
		result.append(text, static_cast<Usize>(from), static_cast<Usize>(std::max(to - from, 0)));
		if (line != last) {
			result.push_back('\n');
		}
	}
	return result;
}

Int32 column_at(std::string_view line, F32 x, const std::function<F32(std::string_view)> &measure) {
	if (x <= 0.F) {
		return 0;
	}

	Usize index = 0;
	F32 previous_width = 0.F;
	while (index < line.size()) {
		const Foundation::Utf8::Decoded decoded = Foundation::Utf8::decode(line, index);
		const Usize next = index + (decoded.size > 0 ? decoded.size : 1U);
		const F32 next_width = measure(line.substr(0, next));
		if (x < (previous_width + next_width) * 0.5F) {
			return static_cast<Int32>(index);
		}
		previous_width = next_width;
		index = next;
	}
	return static_cast<Int32>(line.size());
}

TextSelection select_all(std::span<const std::string> lines) {
	if (lines.empty()) {
		return {};
	}
	return { .anchor = { 0, 0 }, .focus = { static_cast<Int32>(lines.size()) - 1, static_cast<Int32>(lines.back().size()) } };
}

} // namespace Aquila::UI::Text
