#ifndef MORSE_DECODER_H_
#define MORSE_DECODER_H_

// Define the structure for a Morse code tree node
typedef struct MorseNode {
  char character;        // The character this node represents (0 if not a terminal node)
  struct MorseNode *dit; // Pointer to the node for a '*' (dit)
  struct MorseNode *dah; // Pointer to the node for a '-' (dah)
} MorseNode_t;

void morse_decoder_init(void);

char decode_morse_signal(char signal_input);

void morse_decoder_reset(void);

void morse_decoder_deinit(void);

#endif // MORSE_DECODER_H_
