#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Foundation {

// Standardized frame lifecycle for any engine system.
//
// Stages execute in this order each frame:
//   OnUpdate      — advance simulation, process events, tick animations
//   OnCompute     — derive computed state (styles, transforms, layout) — no GPU work
//   OnPrepareRender — write GPU-visible data (upload buffers, update descriptors)
//   OnPostFrame   — per-frame cleanup, reclaim transient allocations
//
// All methods are optional. Systems implement only the stages they participate in.
// This interface is subsystem-agnostic: Canvas, Scene, AnimationSystem, and any
// future system can implement it and be driven by a shared frame orchestrator.
class IFrameParticipant {
  public:
    virtual ~IFrameParticipant() = default;

    virtual void OnUpdate(f32 deltaTime) {}
    virtual void OnCompute() {}
    virtual void OnPrepareRender() {}
    virtual void OnPostFrame() {}

  protected:
    IFrameParticipant() = default;
    AQUILA_NONCOPYABLE(IFrameParticipant);
};

} // namespace Aquila::Foundation
