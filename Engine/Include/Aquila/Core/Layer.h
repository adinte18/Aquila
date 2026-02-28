#ifndef AQUILA_LAYER_H
#define AQUILA_LAYER_H

#include "AquilaCore.h"
#include "Aquila/Events/Event.h"

namespace Aquila::Core {

class Layer {
  public:
	Layer(const std::string &name = "Layer") : m_DebugName(name) {}
	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate(f32 deltaTime) {}
	virtual void OnRender() {}
	virtual void OnImGuiRender() {}
	virtual void OnEvent(Events::Event &event) {}

	const std::string &GetName() const { return m_DebugName; }

  protected:
	std::string m_DebugName;
};

class LayerStack {
  public:
	LayerStack() = default;
	void Clear();
	~LayerStack();

	void PushLayer(Unique<Layer> layer);
	void PushOverlay(Unique<Layer> overlay);
	void PopLayer(Layer *layer);
	void PopOverlay(Layer *overlay);

	auto begin() { return m_Layers.begin(); }
	auto end() { return m_Layers.end(); }
	auto rbegin() { return m_Layers.rbegin(); }
	auto rend() { return m_Layers.rend(); }

	auto begin() const { return m_Layers.begin(); }
	auto end() const { return m_Layers.end(); }
	auto rbegin() const { return m_Layers.rbegin(); }
	auto rend() const { return m_Layers.rend(); }

  private:
	std::vector<Unique<Layer>> m_Layers;
	uint32 m_LayerInsertIndex = 0;
};

inline LayerStack::~LayerStack() {
	// Only detach if Clear() wasn't called
	if (!m_Layers.empty()) {
		for (auto &layer : m_Layers) {
			if (layer) {
				layer->OnDetach();
			}
		}
	}
}

inline void LayerStack::Clear() {
	for (auto &layer : m_Layers) {
		if (layer) {
			layer->OnDetach();
		}
	}
	m_Layers.clear();
	m_LayerInsertIndex = 0;
}

inline void LayerStack::PushLayer(Unique<Layer> layer) {
	m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
	m_LayerInsertIndex++;
	m_Layers[m_LayerInsertIndex - 1]->OnAttach();
}

inline void LayerStack::PushOverlay(Unique<Layer> overlay) {
	m_Layers.emplace_back(std::move(overlay));
	m_Layers.back()->OnAttach();
}

inline void LayerStack::PopLayer(Layer *layer) {
	const auto it = std::find_if(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex,
								 [layer](const Unique<Layer> &l) { return l.get() == layer; });
	if (it != m_Layers.begin() + m_LayerInsertIndex) {
		(*it)->OnDetach();
		m_Layers.erase(it);
		m_LayerInsertIndex--;
	}
}

inline void LayerStack::PopOverlay(Layer *overlay) {
	const auto it = std::find_if(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(),
								 [overlay](const Unique<Layer> &l) { return l.get() == overlay; });
	if (it != m_Layers.end()) {
		(*it)->OnDetach();
		m_Layers.erase(it);
	}
}

} // namespace Aquila::Core

#endif
