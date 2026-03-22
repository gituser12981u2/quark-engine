# RFC: Opaque Generational Handles

**Status:** Draft
**Author:** gituser12981u2
**Target:** Internal Engine Architecture
**Date:** March, 2026
**Scope**: Opaque handle types used for resource addressing
and lifetime

## 1. Abstract

This document defines the structure and requirements for opaque
generational handle types passed out by registry systems. These handles
provide a stable, type safe mechanism for referencing registry managed
resources.

## 2. Motivation

Engine systems require a mechanism to reference resources without:

- Exposing raw backend handles
- Allowing accidental destruction or mutation
- Introducing aliasing or lifetime ambiguity

Opaque generational handles address these concerns by:

- Decoupling identity from storage
- Enabling validation via generation counters
- Supporting efficient slot reuse without dangling references

A generic handle type:

- Eliminates repetitive boilerplate across handle definitions
- Ensures uniform behavior across all handle types
- Prevents accidental mixing of handles from different domains
via type tagging

## 3. Design Principles

### 3.1 Opaque ownership boundary

Handles do not grant ownership or destruction rights over the
underlying resource.

### 3.2 Value Semantics

Handle behave as lightweight, copyable value types.

### 3.3 Stale reference detection

- Handles must support detection of invalid or outdated references via
generation tracking.

### 3.4 Minimal structure

The handle representation is intentionally minimal to reduce overhead
and maximize performance.

## 4. Specification

### 4.1 OpaqueHandle

#### 4.1.1 Definition

A type satisfying OpaqueHandle:

- Encodes an index into a storage domain (e.g., registry slot array)
- Encodes a generation counter associated with that index
- Supports value semantics
- Is equality comparable

#### 4.1.2 Requirements

An opaque handle type shall:

- Expose an index member convertible to uint32_t
- Expose a generation member convertible to uint32_t
- Satisfy std::equality_comparable

#### 4.1.3 Specification

```cpp
template <class H>
concept OpaqueHandle = 
    std::equality_comparable<H> &&
    requires(H handle) {
        { handle.index } -> std::convertible_to<uint32_t>;
        { handle.generation } -> std::convertible_to<uint32_t>;
    };
```

### 4.2 Generic Handle Type

#### 4.2.1 Definition

The engine shall provide a generic handle template parameterized by a tag type:

```cpp
template <class Tag>
struct GenericHandle final {
    uint32_t index = 0;
    uint32_t generation = 0;

    friend constexpr bool operator==(GenericHandle, GenericHandle) noexcept = default;
}
```

#### 4.2.2 Requirements

A GenericHandle<Tag>:

- Satisfies OpaqueHandle
- Is a distinct type for each unique Tag
- Has no implicit conversions between different tag instantiations

#### 4.2.3 Tag Types

Tag types are empty types used solely to distinguish handle domains.

Example:

```cpp
struct FrameHandleTag;
struct InstanceHandleTag;

using FrameHandle = GenericHandle<FrameHandleTag>;
using InstanceHandle = GenericHandle<InstanceHandleTag>;
```

#### 4.2.3 Rationale

The generic handle pattern ensures:

- Compile-time separation between resource domains
- Elimination of repetitive handle definitions
- Consistent structure and behavior across all handles

## 5. Semantics

### 5.1 Identity

A handle uniquely identifies a resource within a specific storage
domain by the pair:

```cpp
(index, generation)
```

Two handles are equal if and only if both components are equal.

### 5.2 Validity

A handle is considered valid with respect to a registry if:

- index refers to an in-bounds slot, and
- generation matches the current generation of that slot, and
- the slot is marked live

Validity is encoded in the handle itself and must be checked
by the owning system.

### 5.3 Staleness

A handle becomes stale when:

- The referenced slot is destroyed, and
- The slot's generation is incremented

Stale handles must not alias newly created objects occupying
the same index.

### 5.4 Lifetime

Handles:

- Do not own resources
- Do not extend resource lifetime
- May outlive the underlying resource

Using a handle after destruction is defined behavior only
insofar as the owning system detects and rejects it.

## 6. Example

```cpp
struct FrameHandleTag;

using FrameHandle = GenericHandle<FrameHandleTag>;
```
