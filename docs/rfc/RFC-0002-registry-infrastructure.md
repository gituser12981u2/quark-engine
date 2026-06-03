# RFC: Registry Infrastructure

**Status:** Draft
**Author:** gituser12981u2
**Target:** Internal Engine Architecture
**Date:** May, 2026
**Scope:** Registry concepts, slot management, and generic registry base

## 1. Abstract

Define the common registry infrastructure used for registry-managed resources.

This RFC proposes:

- Registry slot requirements
- Registry type requirements
- A shared generic registry base
- A typed facade pattern for resource-specific registries

## 2. Motivation

Many resource families require a registry and most registries have the same mechanics:

- Slot storage
- Free-list reuse
- Generation checking
- Stale handle rejection
- Alive queries
- Clear behavior
- Deferred retirement transfer

The resource payloads differ, but the lifecycle mechanics do not.

## 3. Design Principles

### 3.1 Typed Ownership Domains

Each resource family keeps its own registry type and handle type.

Examples:

- `FrameRegistry`
- `ImageRegistry`
- `PipelineRegistry`

A `FrameHandle` must not be usable with an `ImageRegistry`.

### 3.2 Shared Mechanics

All registries use the same core machinery for:

- slot allocation
- slot reuse
- generation validation
- liveness checks
- retirement transfer
- clearing

### 3.3 Resource-specific policies

Resource-specific behavior remains in the typed registry or policy layer:

- creation
- destruction
- payload accessors
- validation
- debug names
- backend-specific cleanup

### 3.4 Global retirement, typed registries

The retirement queue may be shared globally, by payload storage remains typed by registry.

## 4. Concepts

### 4.1 `MoveOnlyNoexcept`

```cpp
template <class T>
concept MoveOnlyNoexcept =
    std::movable<T> &&
    !std::copy_constructible<T> &&
    !std::is_copy_assignable_v<T> &&
    std::is_nothrow_move_constructible_v<T> &&
    std::is_nothrow_move_assignable_v<T>;
```

### 4.2 RegistrySlot

```cpp
template <class S>
concept RegistrySlot = 
    requires(S slot) {
        { slot.live } -> std::convertible_to<bool>;
        { slot.generation } -> std::convertible_to<uint32_t>;
    };
```

### 4.3 RegistryPolicy

```cpp
template <class P, class Slot, class RetiredPayload>
concept RegistryPolicy =
    requires(
        Slot& slot,
        RetiredPayload& payload,
        void* ctx) {
        
        { P::destroy_slot_immediate(slot) }
            noexcept -> std::same_as<void>;

        {
        P::move_slot_to_retired_payload(
                slot,
                payload)
        } noexcept -> std::same_as<void>;

        { P::destroy_retired_payload(ctx) }
            noexcept -> std::same_as<void>;

        { P::cleanup_retired_payload(ctx) }
            noexcept -> std::same_as<void>;
    };
```

## 5. Registry Base

The engine shall provide a generic registry base responsible for common registry mechanics.

The base owns:

- slots_
- free_
- retire_queue_

The base provides:

- handle validation helpers
- slot allocation
- generation validation
- liveness checks
- slot lookup helpers
- transactional creation support
- retirement transfer
- clear semantics

The base does not own resource-specific creation logic.

### 5.1 Transactional Creation

The base provides a `PendingSlot` helper used during creation.

`PendingSlot` guarantees that a partially created slot is automatically returned to the registry if creation fails.

This enables registry implementations to use normal error propagation without manual rollback logic.

Example:

```cpp
auto pending = begin_create_();

QUARK_TRY_STATUS(slot.cmd.create(...));
QUARK_TRY_STATUS(slot.sync.create(...));

return pending.commit();
```

If `commit()` is never called, the slot is automatically released.

## 6. Required Registry Semantics

A registry shall guarantee:

- opaque-handle-only resource access
- generation-checked stale handle protection
- move-only ownership
- noexcept move construction and assignment
- copy construction and copy assignment deleted
- transactional creation
- destroy retires or destroys live payloads at most once
- failed creation automatically releases unpublished slots
- clear leaves the registry empty and consistent
- accessors reject or return null for non-live handles

## 7. Typed Facade Pattern

Concrete registries remain resource-specific facade types.

Example:

```cpp
struct FrameRegistryPolicy;

class FrameRegistry final : public RegistryBase<FrameHandle, FrameSlot, RetiredFramePayload, FrameRegistryPolicy> {
    public:
        struct CreateInfo;

        util::Result<FrameHandle> create(const CreateInfo& ci);
        void destroy(FrameHandle handle, uint64_t retire_at) noexcept;

        FrameCmd* cmd(FrameHandle handle) noexcept;
        FrameSync* sync(FrameHandle handle) noexcept;
};
```
