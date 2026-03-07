#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <quark/engine/retire/retirement_queue.hpp>
#include <quark/utils/diagnostic.hpp>
#include <quark/vk/sync/gpu_timeline.hpp>

namespace quark::vk {

util::Status RetirementQueue::create(const CreateInfo &ci) {
  destroy();

  // TODO: replace with gpu timeline validate()
  QUARK_ENSURE(ci.timeline != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "timeline is null"));

  timeline_ = ci.timeline;

  if (ci.reserve > 0) {
    try {
      entries_.reserve(ci.reserve);
    } catch (...) {
      destroy();
      QUARK_FAIL(QUARK_ERR(util::Errc::OutOfMemory, "reserve failed"));
    }
  }

  head_ = 0;
  last_enqueued_retire_at_ = 0;
  QUARK_OK();
}

void RetirementQueue::destroy() noexcept {
  for (std::size_t i = head_; i < entries_.size(); ++i) {
    run_task_(entries_[i].task);
  }

  entries_.clear();
  head_ = 0;
  last_enqueued_retire_at_ = 0;
  timeline_ = nullptr;
}

void RetirementQueue::run_task_(Task &t) noexcept {
  if (t.fn != nullptr) {
    t.fn(t.ctx);
  }

  if (t.cleanup != nullptr) {
    t.cleanup(t.ctx);
  }

  t.fn = nullptr;
  t.cleanup = nullptr;
  t.ctx = nullptr;
}

util::Status RetirementQueue::enqueue(uint64_t retire_at, Task task) {
  QUARK_ENSURE(
      timeline_ != nullptr,
      QUARK_ERR(util::Errc::InvalidState, "RetirementQueue is not created"));
  QUARK_ENSURE(task.fn != nullptr,
               QUARK_ERR(util::Errc::InvalidArg, "task.fn is null"));

  uint64_t completed = 0;
  QUARK_TRY_ASSIGN(completed, timeline_->completed_value());

  if (retire_at <= completed) {
    run_task_(task);
    QUARK_OK();
  }

  retire_at = std::max(retire_at, last_enqueued_retire_at_);

  try {
    entries_.push_back(Entry{.retire_at = retire_at, .task = task});
  } catch (...) {
    QUARK_FAIL(QUARK_ERR(util::Errc::OutOfMemory, "push_back failed"));
  }

  last_enqueued_retire_at_ = retire_at;
  QUARK_OK();
}

util::Result<std::size_t> RetirementQueue::drain() noexcept {
  if (timeline_ == nullptr) {
    return 0;
  }

  uint64_t completed = 0;
  QUARK_TRY_ASSIGN(completed, timeline_->completed_value());

  std::size_t drained = 0;
  while (head_ < entries_.size()) {
    Entry &e = entries_[head_];
    if (e.retire_at > completed) {
      break;
    }

    run_task_(e.task);
    ++head_;
    ++drained;
  }

  compact_if_needed_();
  return drained;
}

void RetirementQueue::compact_if_needed_() noexcept {
  if (head_ == 0) {
    return;
  }

  const std::size_t n = entries_.size();
  const std::size_t dead = head_;
  const std::size_t live = n - head_;

  constexpr std::size_t kDeadThreshold = 256;

  if (dead < kDeadThreshold && dead < live) {
    return;
  }

  for (std::size_t i = 0; i < live; ++i) {
    entries_[i] = entries_[head_ + i];
  }
  entries_.resize(live);
  head_ = 0;

  if (entries_.empty()) {
    last_enqueued_retire_at_ = 0;
  }
}

} // namespace quark::vk
