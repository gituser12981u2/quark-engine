#pragma once

/**
 * @def QUARK_NO_COPY(Type)
 * @brief Deletes copy constructor and copy assignment for @p Type.
 *
 * Use for RAII/handle-owning types to prevent accidental copying.
 */
#define QUARK_NO_COPY(Type)                                                    \
  Type(const Type &) = delete;                                                 \
  Type &operator=(const Type &) = delete

/**
 * @def QUARK_NO_MOVE(Type)
 * @brief Deletes move constructor and move assignment for @p Type.
 *
 * Use when a type must not be moved (e.g. self-referential objects).
 */
#define QUARK_NO_MOVE(Type)                                                    \
  Type(Type &&) = delete;                                                      \
  Type &operator=(Type &&) = delete

/**
 * @def QUARK_NO_COPY_NO_MOVE(Type)
 * @brief Deletes both copy and move operations for @p Type.
 */
#define QUARK_NO_COPY_NO_MOVE(Type)                                            \
  QUARK_NO_COPY(Type);                                                         \
  QUARK_NO_MOVE(Type)

/**
 * @def QUARK_DEFAULT_MOVE(Type)
 * @brief Defaults move constructor and move assignment (both noexcept).
 *
 * Only use when the type's members are safely movable and leaving the
 * moved-from object in a destructible state is correct.
 */
#define QUARK_DEFAULT_MOVE(Type)                                               \
  Type(Type &&) noexcept = default;                                            \
  Type &operator=(Type &&) noexcept = default

/**
 * @def QUARK_MOVE_ONLY(Type)
 * @brief Makes @p Type move-only: deletes copy operations and defaults noexcept
 * move.
 *
 * Equivalent to:
 * - QUARK_NO_COPY(Type)
 * - QUARK_DEFAULT_MOVE(Type)
 */
#define QUARK_MOVE_ONLY(Type)                                                  \
  QUARK_NO_COPY(Type);                                                         \
  QUARK_DEFAULT_MOVE(Type)
