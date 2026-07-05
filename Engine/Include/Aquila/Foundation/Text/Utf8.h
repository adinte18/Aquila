#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

#include <string_view>

namespace Aquila::Foundation::Utf8 {

// thank you
// https://www.youtube.com/watch?v=vpSkBV5vydg

constexpr uint32 REPLACEMENT = 0xFFFDu; // U+FFFD REPLACEMENT CHARACTER

constexpr uint8 CONTINUATION_MARKER = 0x80u; // 10xxxxxx : a trailing byte
constexpr uint8 TWO_BYTE_MARKER = 0xC0u; // 110xxxxx : lead of a 2-byte sequence
constexpr uint8 THREE_BYTE_MARKER = 0xE0u; // 1110xxxx : lead of a 3-byte sequence
constexpr uint8 FOUR_BYTE_MARKER = 0xF0u; // 11110xxx : lead of a 4-byte sequence

constexpr uint8 CONTINUATION_MASK = 0xC0u; // keep top 2 bits
constexpr uint8 TWO_BYTE_MASK = 0xE0u; // keep top 3 bits
constexpr uint8 THREE_BYTE_MASK = 0xF0u; // keep top 4 bits
constexpr uint8 FOUR_BYTE_MASK = 0xF8u; // keep top 5 bits

// Data bits each byte carries once the marker bits are stripped off.
constexpr uint8 TWO_BYTE_DATA = 0x1Fu; // low 5 bits
constexpr uint8 THREE_BYTE_DATA = 0x0Fu; // low 4 bits
constexpr uint8 FOUR_BYTE_DATA = 0x07u; // low 3 bits
constexpr uint8 CONTINUATION_DATA = 0x3Fu; // low 6 bits
constexpr uint32 CONTINUATION_BITS = 6u; // data bits per continuation byte

// Inclusive upper bound of the codepoints each sequence length can hold.
constexpr uint32 MAX_ONE_BYTE = 0x7Fu; // U+007F
constexpr uint32 MAX_TWO_BYTE = 0x7FFu; // U+07FF
constexpr uint32 MAX_THREE_BYTE = 0xFFFFu; // U+FFFF
constexpr uint32 MAX_CODEPOINT = 0x10FFFFu; // highest valid Unicode scalar

constexpr uint32 SURROGATE_MIN = 0xD800u; // U+D800..U+DFFF are illegal in UTF-8
constexpr uint32 SURROGATE_MAX = 0xDFFFu;

struct Decoded {
	uint32 codepoint;
	uint32 size; // bytes consumed from the source (always >= 1)
};

inline Decoded Decode(std::string_view text, usize pos) {
	if (pos >= text.size()) {
		return { .codepoint = 0u, .size = 0u };
	}

	const auto lead = static_cast<uint8>(text[pos]);

	if (lead <= MAX_ONE_BYTE) {
		return { .codepoint = lead, .size = 1u };
	}

	uint32 codepoint = 0;
	uint32 extra = 0;
	if ((lead & TWO_BYTE_MASK) == TWO_BYTE_MARKER) {
		codepoint = lead & TWO_BYTE_DATA;
		extra = 1;
	} else if ((lead & THREE_BYTE_MASK) == THREE_BYTE_MARKER) {
		codepoint = lead & THREE_BYTE_DATA;
		extra = 2;
	} else if ((lead & FOUR_BYTE_MASK) == FOUR_BYTE_MARKER) {
		codepoint = lead & FOUR_BYTE_DATA;
		extra = 3;
	} else {
		return { .codepoint = REPLACEMENT, .size = 1u };
	}

	for (uint32 i = 1; i <= extra; ++i) {
		if (pos + i >= text.size()) {
			return { .codepoint = REPLACEMENT, .size = 1u };
		}
		const auto cont = static_cast<uint8>(text[pos + i]);
		if ((cont & CONTINUATION_MASK) != CONTINUATION_MARKER) {
			return { .codepoint = REPLACEMENT, .size = 1u };
		}
		codepoint = (codepoint << CONTINUATION_BITS) | (cont & CONTINUATION_DATA);
	}

	// Reject overlong encodings, the UTF-16 surrogate range, and out-of-range values.
	// The sequence is structurally valid, so consume all of it and emit a replacement.
	constexpr uint32 MIN_FOR_EXTRA[4] = { 0u, MAX_ONE_BYTE + 1u, MAX_TWO_BYTE + 1u, MAX_THREE_BYTE + 1u };
	if (codepoint < MIN_FOR_EXTRA[extra] || codepoint > MAX_CODEPOINT ||
		(codepoint >= SURROGATE_MIN && codepoint <= SURROGATE_MAX)) {
		return { REPLACEMENT, extra + 1u };
	}

	return { codepoint, extra + 1u };
}

inline uint32 Encode(uint32 codepoint, char out[4]) {
	if (codepoint <= MAX_ONE_BYTE) {
		out[0] = static_cast<char>(codepoint);
		return 1u;
	}
	if (codepoint <= MAX_TWO_BYTE) {
		out[0] = static_cast<char>(TWO_BYTE_MARKER | (codepoint >> CONTINUATION_BITS));
		out[1] = static_cast<char>(CONTINUATION_MARKER | (codepoint & CONTINUATION_DATA));
		return 2u;
	}
	if (codepoint <= MAX_THREE_BYTE) {
		out[0] = static_cast<char>(THREE_BYTE_MARKER | (codepoint >> (2u * CONTINUATION_BITS)));
		out[1] = static_cast<char>(CONTINUATION_MARKER | ((codepoint >> CONTINUATION_BITS) & CONTINUATION_DATA));
		out[2] = static_cast<char>(CONTINUATION_MARKER | (codepoint & CONTINUATION_DATA));
		return 3u;
	}
	out[0] = static_cast<char>(FOUR_BYTE_MARKER | (codepoint >> (3u * CONTINUATION_BITS)));
	out[1] = static_cast<char>(CONTINUATION_MARKER | ((codepoint >> (2u * CONTINUATION_BITS)) & CONTINUATION_DATA));
	out[2] = static_cast<char>(CONTINUATION_MARKER | ((codepoint >> CONTINUATION_BITS) & CONTINUATION_DATA));
	out[3] = static_cast<char>(CONTINUATION_MARKER | (codepoint & CONTINUATION_DATA));
	return 4u;
}

inline usize NextIndex(std::string_view text, usize pos) {
	if (pos >= text.size()) {
		return text.size();
	}
	const Decoded d = Decode(text, pos);
	return pos + (d.size > 0 ? d.size : 1u);
}

inline usize PrevIndex(std::string_view text, usize pos) {
	if (pos == 0) {
		return 0;
	}
	usize i = pos - 1;
	while (i > 0 && (static_cast<uint8>(text[i]) & CONTINUATION_MASK) == CONTINUATION_MARKER) {
		--i;
	}
	return i;
}

} // namespace Aquila::Foundation::Utf8
