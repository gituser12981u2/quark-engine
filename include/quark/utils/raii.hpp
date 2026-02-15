#pragma once

#define QUARK_NO_COPY(Type)                                                    \
  Type(const Type &) = delete;                                                 \
  Type &operator=(const Type &) = delete

#define QUARK_NO_MOVE(Type)                                                    \
  Type(Type &&) = delete;                                                      \
  Type &operator=(Type &&) = delete

#define QUARK_NO_COPY_NO_MOVE(Type)                                            \
  QUARK_NO_COPY(Type);                                                         \
  QUARK_NO_MOVE(Type)

#define QUARK_DEFAULT_MOVE(Type)                                               \
  Type(Type &&) noexcept = default;                                            \
  Type &operator=(Type &&) noexcept = default

#define QUARK_MOVE_ONLY(Type)                                                  \
  QUARK_NO_COPY(Type);                                                         \
  QUARK_DEFAULT_MOVE(Type)
