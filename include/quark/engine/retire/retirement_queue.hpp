#pragma once

// TODO: replace with gpu timeline interface i.e. abstracted from vulkan
#include "quark/vk/sync/gpu_timeline.hpp"

#include <cstddef>
#include <cstdint>
#include <quark/utils/raii.hpp>
#include <quark/utils/result.hpp>
#include <vector>

namespace quark::vk {

/**
 * @brief CPU-side deferred destruction queue keyed by a GPU timeline value.
 *
 * Each entry is scheduled with a retire value (retire_at) on a global timeline.
 * It is safe to run the entry once completed_value() >= retire_at.
 *
 * Performance:
 *   - enqueue: O(1)
 *   - drain: O(k) where k is the number of ready items drained
 *
 * Invariant:
 *   - Internally maintains a monotonic retire_at sequence.
 *     If the caller enqueues an out-of-order retire_at, it is clamped
 *     to the last enqueued retire_at to preserve monotonicity and allow O(k)
 * front-drain.
 *
 * Correctness assumptions:
 *   - All retire_at values refer to the same total order clock as timeline_.
 *   - destroy() is called only when it is safe to execute all pending callbacks
 * (e.g. after vkDeviceWaitIdle()).
 */
class RetirementQueue final {
public:
  /**
   * @brief Type-erased task invoked on retirement.
   *
   * fn(ctx) is executed when the entry is drained.
   * cleanup(ctx) is executed afterwards to free ctx memory.
   */
  struct Task {
    void (*fn)(void *ctx) noexcept = nullptr;
    void (*cleanup)(void *ctx) noexcept = nullptr;
    void *ctx = nullptr;
  };

  struct CreateInfo {
    const GpuTimeline *timeline = nullptr;

    /// Optional; pre-reserve capacity
    std::size_t reserve = 0;
  };

  RetirementQueue() = default;
  ~RetirementQueue() { destroy(); }

  QUARK_MOVE_ONLY(RetirementQueue);

  [[nodiscard]] util::Status create(const CreateInfo &ci);
  void destroy() noexcept;

  [[nodiscard]] bool valid() const noexcept { return timeline_ != nullptr; }

  /**
   * @brief Enqueue a task to run after retire_at is completed.
   *
   * Fast-path:
   *   - If retire_at <= timeline.completed_value(), task executed immediately.
   *
   * Ordering:
   *   - If retire_at < last_enqueued_retire_at_, it is clamped upward to
   *   last_enqueued_retire_at_ to preserver monotonicity.
   *
   * @return Status::OK on Success, or OutOfMemory if internal growth fails.
   */
  [[nodiscard]] util::Status enqueue(uint64_t retire_at, Task task);

  /**
   * @brief Drain all ready tasks (completed_value >= retire_at).
   *
   * Runs tasks in FIFO order for the ready prefix.
   * Does not block; it only checks timeline completed.
   *
   * @return the number of tasks executed.
   */
  [[nodiscard]] util::Result<std::size_t> drain() noexcept;

  /**
   * @brief Number of tasks currently queued (including the ones not yet ready).
   */
  [[nodiscard]] std::size_t pending() const noexcept {
    return (entries_.size() >= head_) ? (entries_.size() - head_) : 0;
  }

private:
  struct Entry {
    uint64_t retire_at = 0;
    Task task{};
  };

  static void run_task_(Task &t) noexcept;

  /**
   * @brief Avoid O(n) erase every frame by compacting when head is large
   * enough.
   *
   * Two Part Heuristic:
   * 1) dead >= live, i.e. at least half the buffer is garbage, OR
   * 2) dead is large enough to matter in absolute terms
   */
  void compact_if_needed_() noexcept;

  const GpuTimeline *timeline_ = nullptr; // non-owning

  std::vector<Entry> entries_;
  std::size_t head_ = 0;
  uint64_t last_enqueued_retire_at_ = 0;
};

} // namespace quark::vk
