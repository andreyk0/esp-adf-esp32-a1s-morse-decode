#ifndef CHAR_BUFFER_H
#define CHAR_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Opaque character buffer structure.
 * Internally, this uses a ring/circular buffer mechanism.
 */
typedef struct char_buffer_t char_buffer_t;

/**
 * @brief Initializes the character buffer module.
 *
 * This function must be called before any other character buffer functions.
 * It sets up the internal structures for the character buffer.
 *
 * @param max_len The maximum number of characters the buffer can hold (excluding the null terminator for the output
 * string).
 * @return A pointer to the initialized character buffer object, or NULL if initialization failed (e.g., invalid max_len
 * or memory allocation failure).
 */
char_buffer_t *char_buffer_init(size_t max_len);

/**
 * @brief Appends a character to the character buffer.
 *
 * If the buffer is full (i.e., count == max_len set during init), the character is ignored,
 * and the buffer remains unchanged.
 *
 * @param cb Pointer to the character buffer object.
 * @param ch The character to append.
 * @return true if the character was successfully appended, false if the buffer was full or cb is NULL.
 */
bool char_buffer_append_char(char_buffer_t *cb, char ch);

/**
 * @brief Retrieves the accumulated characters as a null-terminated string.
 *
 * This function copies the current content of the character buffer into a
 * statically managed internal buffer (associated with the char_buffer_t instance),
 * null-terminates it, and returns a pointer to this internal buffer. The content
 * of this buffer is valid until the next call to `char_buffer_get_string` for the
 * same character buffer instance or until the character buffer is reset.
 *
 * The order of characters in the string will be the order in which they were appended.
 *
 * @param cb Pointer to the character buffer object.
 * @return A pointer to a null-terminated string containing the characters
 * from the buffer, or NULL if cb is NULL. If the buffer is empty, it returns an empty string.
 * The returned pointer is to an internal static buffer associated with cb.
 */
const char *char_buffer_get_string(char_buffer_t *cb);

/**
 * @brief Resets the character buffer.
 *
 * Clears all characters from the buffer and resets its internal state (head, tail, count).
 * The maximum length remains the same.
 *
 * @param cb Pointer to the character buffer object.
 */
void char_buffer_reset(char_buffer_t *cb);

/**
 * @brief Gets the current number of characters in the buffer.
 *
 * @param cb Pointer to the character buffer object.
 * @return The number of characters currently stored in the buffer, or 0 if cb is NULL.
 */
size_t char_buffer_get_count(const char_buffer_t *cb);

/**
 * @brief Gets the maximum capacity of the buffer.
 *
 * @param cb Pointer to the character buffer object.
 * @return The maximum number of characters the buffer can hold, or 0 if cb is NULL.
 */
size_t char_buffer_get_capacity(const char_buffer_t *cb);

/**
 * @brief Deinitializes the character buffer and frees associated memory.
 *
 * @param cb Pointer to the character buffer object to be deinitialized.
 */
void char_buffer_deinit(char_buffer_t *cb);

#endif // CHAR_BUFFER_H
