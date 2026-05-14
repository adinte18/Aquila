#pragma once
#include <unordered_map>
#include <vector>

namespace Aquila::Foundation {

template <typename Key, typename Value, typename Hash = std::hash<Key>> class ComputedCache {
  public:
	template <typename ComputeFn> const Value &Get(const Key &key, ComputeFn &&computeFn) {
		if (auto it = m_Entries.find(key); it != m_Entries.end() && it->second.valid) {
			return it->second.value;
		}
		Value computed = computeFn();
		auto &entry = m_Entries[key]; // fresh lookup after potential rehash
		entry.value = std::move(computed);
		entry.valid = true;
		return entry.value;
	}

	[[nodiscard]] const Value *Peek(const Key &key) const {
		auto it = m_Entries.find(key);
		if (it != m_Entries.end() && it->second.valid) {
			return &it->second.value;
		}
		return nullptr;
	}

	// Mark key invalid and cascade to all keys that declared a dependency on it.
	void Invalidate(const Key &key) {
		auto it = m_Entries.find(key);
		if (it == m_Entries.end() || !it->second.valid) {
			return;
		}
		it->second.valid = false;

		auto depsIt = m_Dependents.find(key);
		if (depsIt != m_Dependents.end()) {
			for (const Key &dep : depsIt->second) {
				Invalidate(dep);
			}
		}
	}

	void InvalidateAll() {
		for (auto &[key, entry] : m_Entries) {
			entry.valid = false;
		}
	}

	void RegisterDependency(const Key &dependent, const Key &dependency) {
		auto &list = m_Dependents[dependency];
		for (const Key &k : list) {
			if (k == dependent) {
				return;
			}
		}
		list.push_back(dependent);
	}

	void Remove(const Key &key) {
		m_Entries.erase(key);
		m_Dependents.erase(key); // remove key as a dependency source
	}

	[[nodiscard]] bool IsValid(const Key &key) const {
		auto it = m_Entries.find(key);
		return it != m_Entries.end() && it->second.valid;
	}

  private:
	struct Entry {
		Value value{};
		bool valid = false;
	};
	std::unordered_map<Key, Entry, Hash> m_Entries;
	std::unordered_map<Key, std::vector<Key>, Hash> m_Dependents;
};

} // namespace Aquila::Foundation
