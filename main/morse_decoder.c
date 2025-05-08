#include "morse_decoder.h"

#include "esp_log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "MORSE_DECODER";

// Global pointer to the root of the Morse code tree
static MorseNode_t *morse_tree_root = NULL;
// Global pointer to the current state in the Morse code tree
static MorseNode_t *current_node = NULL;


// Helper function to create a new Morse node
static MorseNode_t* create_morse_node() {
    MorseNode_t *node = (MorseNode_t *)calloc(1, sizeof(MorseNode_t));
    if (!node) {
        ESP_LOGE(TAG, "Failed to allocate memory for Morse node");
        printf("Error: Failed to allocate memory for Morse node\n");
    }
    return node;
}

// Function to add a Morse code sequence to the tree
static void add_morse_code(MorseNode_t *root, const char *code, char character) {
    if (!root) return;
    MorseNode_t *curr = root;
    for (int i = 0; code[i] != '\0'; i++) {
        if (code[i] == '*') { // Dit
            if (curr->dit == NULL) {
                curr->dit = create_morse_node();
                if (!curr->dit) return; // Allocation failed, stop
            }
            curr = curr->dit;
        } else if (code[i] == '-') { // Dah
            if (curr->dah == NULL) {
                curr->dah = create_morse_node();
                 if (!curr->dah) return; // Allocation failed, stop
            }
            curr = curr->dah;
        } else {
            // Invalid character in Morse code string
            ESP_LOGW(TAG, "Invalid char in Morse code definition: %c", code[i]);
            return;
        }
    }
    curr->character = character; // Mark the character at the end of the sequence
}

/**
 * @brief Initializes the Morse code decoder system.
 *
 * This function builds the Morse code tree. It should be called once before
 * using decode_morse_signal().
 */
void morse_decoder_init(void) {
    if (morse_tree_root != NULL) {
        ESP_LOGI(TAG, "Morse decoder already initialized.");
        // To re-initialize, call morse_decoder_deinit() first.
        return;
    }

    ESP_LOGI(TAG, "Initializing Morse decoder...");
    morse_tree_root = create_morse_node();
    if (!morse_tree_root) {
        ESP_LOGE(TAG, "Failed to create Morse tree root. Decoder unusable.");
        return;
    }

    // Letters
    add_morse_code(morse_tree_root, "*-", 'A');
    add_morse_code(morse_tree_root, "-***", 'B');
    add_morse_code(morse_tree_root, "-*-*", 'C');
    add_morse_code(morse_tree_root, "-**", 'D');
    add_morse_code(morse_tree_root, "*", 'E');
    add_morse_code(morse_tree_root, "**-*", 'F');
    add_morse_code(morse_tree_root, "--*", 'G');
    add_morse_code(morse_tree_root, "****", 'H');
    add_morse_code(morse_tree_root, "**", 'I');
    add_morse_code(morse_tree_root, "*---", 'J');
    add_morse_code(morse_tree_root, "-*-", 'K');
    add_morse_code(morse_tree_root, "*-**", 'L');
    add_morse_code(morse_tree_root, "--", 'M');
    add_morse_code(morse_tree_root, "-*", 'N');
    add_morse_code(morse_tree_root, "---", 'O');
    add_morse_code(morse_tree_root, "*--*", 'P');
    add_morse_code(morse_tree_root, "--*-", 'Q');
    add_morse_code(morse_tree_root, "*-*", 'R');
    add_morse_code(morse_tree_root, "***", 'S');
    add_morse_code(morse_tree_root, "-", 'T');
    add_morse_code(morse_tree_root, "**-", 'U');
    add_morse_code(morse_tree_root, "***-", 'V');
    add_morse_code(morse_tree_root, "*--", 'W');
    add_morse_code(morse_tree_root, "-**-", 'X');
    add_morse_code(morse_tree_root, "-*--", 'Y');
    add_morse_code(morse_tree_root, "--**", 'Z');

    // Numbers
    add_morse_code(morse_tree_root, "-----", '0');
    add_morse_code(morse_tree_root, "*----", '1');
    add_morse_code(morse_tree_root, "**---", '2');
    add_morse_code(morse_tree_root, "***--", '3');
    add_morse_code(morse_tree_root, "****-", '4');
    add_morse_code(morse_tree_root, "*****", '5');
    add_morse_code(morse_tree_root, "-****", '6');
    add_morse_code(morse_tree_root, "--***", '7');
    add_morse_code(morse_tree_root, "---**", '8');
    add_morse_code(morse_tree_root, "----*", '9');

    // Common Punctuation
    add_morse_code(morse_tree_root, "*-*-*-", '.');
    add_morse_code(morse_tree_root, "--**--", ',');
    add_morse_code(morse_tree_root, "**--**", '?');
    // add_morse_code(morse_tree_root, "-...-", '='); // Or use for prosign BT

    current_node = morse_tree_root; // Initialize current state to the root
    ESP_LOGI(TAG, "Morse decoder initialized successfully.");
}

/**
 * @brief Decodes a single Morse signal input ('*', '-', or ' ').
 *
 * Feed signals one by one to this function.
 * - If '*' (dit) or '-' (dah) is provided, the internal state is updated.
 * The function returns -1 to indicate that the character is not yet complete.
 * If an invalid signal for the current sequence is received (e.g., a dit
 * where no dit path exists), the current sequence becomes invalid.
 * - If ' ' (space) is provided, it marks the end of the current letter:
 * - Returns the decoded ASCII character if the sequence was valid and complete.
 * - Returns 0 if the sequence was invalid, incomplete (not a defined char),
 * or if no signals were input before the space.
 * - The decoder state is then reset for the next letter.
 * - For any other input character, it's treated as an error for the current
 * letter, the state is reset, and 0 is returned.
 *
 * @param signal_input The Morse signal: '*' for dit, '-' for dah, ' ' for end of letter.
 * @return The decoded character if ' ' finalized a valid character,
 * 0 if ' ' finalized an invalid/incomplete character or an unknown signal was input,
 * -1 if '*' or '-' was processed (character incomplete).
 */
char decode_morse_signal(char signal_input) {
    if (morse_tree_root == NULL) {
        // ESP_LOGW(TAG, "Morse tree not initialized. Call morse_decoder_init() first or it will be lazy-initialized now.");
        morse_decoder_init();
        if (morse_tree_root == NULL) {
            // ESP_LOGE(TAG, "Critical: Morse tree initialization failed. Decoder cannot operate.");
            return 0; // Cannot proceed
        }
    }

    // Ensure current_node is valid; it might be NULL if a previous signal invalidated the sequence.
    // Or if morse_decoder_init was called, current_node should be morse_tree_root.
    if (current_node == NULL && signal_input != ' ') {
        // This state means the previous signal sequence was invalid.
        // Any new dit/dah continues an invalid sequence until a space resets.
        return -1; // Still in an invalid sequence, awaiting space to reset and report 0.
    } else if (current_node == NULL && signal_input == ' ') {
        // Space after an invalid sequence. Reset and return 0.
        current_node = morse_tree_root;
        return 0;
    }


    if (signal_input == '*') {
        if (current_node->dit != NULL) {
            current_node = current_node->dit;
        } else {
            // Invalid sequence: no path for a dit from current node.
            // Mark sequence as invalid by setting current_node to NULL.
            // The next ' ' will return 0 and reset.
            current_node = NULL;
        }
        return -1; // Signal processed, letter not yet complete
    } else if (signal_input == '-') {
        if (current_node->dah != NULL) {
            current_node = current_node->dah;
        } else {
            // Invalid sequence: no path for a dah.
            current_node = NULL;
        }
        return -1; // Signal processed, letter not yet complete
    } else if (signal_input == ' ') { // End of letter mark
        char decoded_char = 0;
        if (current_node != NULL && current_node != morse_tree_root) {
            // Valid sequence if current_node is not NULL and it's not the empty root.
            // current_node->character will be 0 if the path is valid but doesn't end on a defined character.
            decoded_char = current_node->character;
        }
        // If current_node is NULL (invalid sequence), morse_tree_root (empty sequence "" before space),
        // or current_node->character is 0 (e.g. sequence like "*-*-" which is not defined for a char),
        // decoded_char will be 0, indicating a failed decode for this letter.

        current_node = morse_tree_root; // Reset for the next character
        return decoded_char; // Returns 0 if decoding failed, or the ASCII char.
    } else {
        // ESP_LOGW(TAG, "Unknown signal input: %c. Resetting current letter.", signal_input);
        // Treat unknown signal as an error for the current letter, reset, and return 0.
        current_node = morse_tree_root;
        return 0;
    }
}

// Recursive helper to free tree nodes
static void morse_decoder_deinit_recursive(MorseNode_t *node) {
    if (node == NULL) {
        return;
    }
    morse_decoder_deinit_recursive(node->dit);
    morse_decoder_deinit_recursive(node->dah);
    free(node);
}

/**
 * @brief Deinitializes the Morse code decoder.
 *
 * Frees all memory allocated for the Morse code tree. Call this when the
 * decoder is no longer needed to prevent memory leaks.
 */
void morse_decoder_deinit(void) {
    if (morse_tree_root != NULL) {
        morse_decoder_deinit_recursive(morse_tree_root);
        morse_tree_root = NULL;
        current_node = NULL; // Also reset current node pointer
        ESP_LOGI(TAG, "Morse decoder deinitialized.");
    } else {
       ESP_LOGI(TAG, "Morse decoder was not initialized or already deinitialized.");
    }
}
