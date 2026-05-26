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

Most bundles require a registry and most registries have the same mechanics:

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
    std::moveable<T> &&
    !std::copy_constructible<T> &&
    !std::copy-assignable_v<T> &&
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

### 4.3 RegistryLike

```cpp
template <class R>
concept RegistryLike =
    MoveOnlyNoexcept<R> &&
    requires(
        R reg 
        const R creg
        typename R::Handle handle,
        typename R::CreateInfo ci,
        uint64_t retire_at) {
        
        typename R::Handle;
        typename R::CreateInfo;

        { reg.create(ci) };
        { reg.destroy(handle, retire_at) } noexcept -> std::same_as<void>;
        { reg.clear() } noexcept -> std::same_as<void>;
        { creg.alive(handle) } noexcept -> std::convertible_to<bool>;
    };
```

## 5. Registry Base

The engine shall provide a generic registry base responsible for common registry mechanics.

The base owns:

- slots_
- free_
- retire_queue_
- handle validation helpers
- slot allocation
- liveness checks
- common clear behavior
- common retirement transfer behavior

The base does not own resource-specific creation logic.

## 6. Required Registry Semantics

A registry shall guarantee:

- opaque-handle-only-ownership API
- generation-checked stale handle protection
- move-only ownership
- noexcept move construction and assignment
- copy construction and copy assignment deleted
- transactional creation
- destroy transfers live payloads to retirement at most once
- clear leaves and registry empty and consistent
- accessors reject or return null for non-live handles

## 7. Typed Facade Pattern

Concrete registries remain resource-specific facade types.

Example:

```cpp
class FrameRegistry final : public RegistryBase<FrameRegistry, FrameHandle, FrameSlot, RetiredFramePayload> {
    public:
        using Handle = FrameHandle;

        struct CreateInfo;

        util::Result<FrameHandle> create(const CreateInfo& ci);
        void destroy(FrameHandle handle, uint64_t retire_at) noexcept;

        FrameCmd* cmd(FrameHandle handle) noexcept;
        FrameSync* sync(FrameHandle handle) noexcept;
};
```
