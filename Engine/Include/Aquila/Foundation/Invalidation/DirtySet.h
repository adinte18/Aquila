#pragma once
#include <functional>
#include <unordered_set>
#include <vector>

namespace Aquila::Foundation {

template <typename Key, typename Hash = std::hash<Key>> class DirtySet {
  public:
	void MarkDirty(const Key &key) {
		if (m_Set.insert(key).second) {
			m_Ordered.push_back(key);
		}
	}

	[[nodiscard]] bool IsDirty(const Key &key) const { return m_Set.count(key) > 0; }
	[[nodiscard]] bool IsEmpty() const { return m_Ordered.empty(); }
	[[nodiscard]] const std::vector<Key> &GetOrdered() const { return m_Ordered; }

	void Clear() {
		m_Set.clear();
		m_Ordered.clear();
	}

  private:
	std::unordered_set<Key, Hash> m_Set;
	std::vector<Key> m_Ordered;
};

} // namespace Aquila::Foundation
