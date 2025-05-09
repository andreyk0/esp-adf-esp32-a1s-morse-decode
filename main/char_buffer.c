#include "char_buffer.h"

#include "char_buffer.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy
#include "esp_log.h"       // For logging

static const char *TAG = "CB";

// Definition of the character buffer structure
struct char_buffer_t {
    char* buffer;           // Pointer to the actual buffer holding characters (ring buffer)
    size_t head;            // Index of the next free spot to write
    size_t tail;            // Index of the first character to read
    size_t count;           // Number of characters currently in the buffer
    size_t max_length;      // Maximum number of characters the buffer can hold
    char* output_string;    // Buffer for get_string (max_length + 1 for null terminator)
};

char_buffer_t* char_buffer_init(size_t max_len) {
    if (max_len == 0) {
        ESP_LOGE(TAG, "Max length for char_buffer cannot be 0.");
        return NULL;
    }

    char_buffer_t* cb = (char_buffer_t*)malloc(sizeof(char_buffer_t));
    if (!cb) {
        ESP_LOGE(TAG, "Failed to allocate memory for char_buffer_t structure.");
        return NULL;
    }

    cb->buffer = (char*)malloc(max_len * sizeof(char));
    if (!cb->buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for char_buffer data.");
        free(cb);
        return NULL;
    }

    cb->output_string = (char*)malloc((max_len + 1) * sizeof(char));
    if (!cb->output_string) {
        ESP_LOGE(TAG, "Failed to allocate memory for char_buffer output string.");
        free(cb->buffer);
        free(cb);
        return NULL;
    }

    cb->max_length = max_len;
    char_buffer_reset(cb); // Initialize head, tail, count, and clear output_string

    ESP_LOGI(TAG, "Character buffer initialized with max_len: %u", (unsigned int)max_len);
    return cb;
}

bool char_buffer_append_char(char_buffer_t* cb, char ch) {
    if (!cb || !cb->buffer) { // Added cb->buffer check for robustness
        ESP_LOGW(TAG, "Character buffer not properly initialized for append.");
        return false;
    }

    if (cb->count < cb->max_length) {
        cb->buffer[cb->head] = ch;
        cb->head = (cb->head + 1) % cb->max_length;
        cb->count++;
        return true;
    } else {
        // Buffer is full, ignore the character
        ESP_LOGD(TAG, "Character buffer full. Character '%c' (0x%02X) ignored.", ch, ch);
        return false;
    }
}

const char* char_buffer_get_string(char_buffer_t* cb) {
    if (!cb || !cb->output_string || !cb->buffer) { // Added cb->buffer check
        ESP_LOGW(TAG, "Character buffer not properly initialized for get_string.");
        if (cb && cb->output_string) { // Still try to return an empty string if output_string exists
             cb->output_string[0] = '\0';
             return cb->output_string;
        }
        return ""; // Or NULL, depending on desired error handling for totally uninitialized state.
                   // Returning "" is safer for direct printf.
    }

    if (cb->count == 0) {
        cb->output_string[0] = '\0'; // Empty string
        return cb->output_string;
    }

    size_t current_tail = cb->tail;
    for (size_t i = 0; i < cb->count; ++i) {
        cb->output_string[i] = cb->buffer[current_tail];
        current_tail = (current_tail + 1) % cb->max_length;
    }
    cb->output_string[cb->count] = '\0'; // Null-terminate

    return cb->output_string;
}

void char_buffer_reset(char_buffer_t* cb) {
    if (!cb) {
        ESP_LOGW(TAG, "Character buffer not initialized for reset.");
        return;
    }
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    if (cb->output_string) { // Clear the output string as well
        cb->output_string[0] = '\0';
    }
    ESP_LOGD(TAG, "Character buffer reset.");
}

size_t char_buffer_get_count(const char_buffer_t* cb) {
    if (!cb) return 0;
    return cb->count;
}

size_t char_buffer_get_capacity(const char_buffer_t* cb) {
    if (!cb) return 0;
    return cb->max_length;
}

void char_buffer_deinit(char_buffer_t* cb) {
    if (cb) {
        free(cb->buffer);          // Free the character data buffer
        cb->buffer = NULL;         // Defensive
        free(cb->output_string);   // Free the output string buffer
        cb->output_string = NULL;  // Defensive
        free(cb);                  // Free the structure itself
        ESP_LOGI(TAG, "Character buffer deinitialized.");
    }
}
