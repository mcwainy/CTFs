# Argparse Programming Challenge

## Overview

Write an algorithm that interprets a series of command-line instructions in `cmds.bin` and passes them to the binary hosted on port `1445`. The binary will use the instructions to return a character, which can be strung together to build the flag.

Note: Those who wish to succeed on the interview should be able to accomplish this in C.

## Instructions

This challenge represents a scenario where we want to minimize traffic over the wire. Therefore, we encode our traffic using a custom encoding scheme to (1) mask the traffic we send and (2) demand less network traffic.

This challenge provides a series of command-line instructions encoded using our custom encoding scheme. You must read in each instruction, decode it, and send it to the binary to be run. Should you provide the right arguments, it will reveal a correct flag.

### Encoding Scheme

Our scheme accepts five types of arguments, each prefixed with a key. These arguments then have their own encoding scheme:

##### 1. IP Address

- Example: `127.0.0.1`
- Key: `0x01`
- Encoding Scheme: Packed as a 32-bit integer

##### 2. Number

- Example: `4444`
- Key: `0x02`
- Encoding Scheme: Byte Size + Number

##### 3. Short Flag

- Example: `-v`
- Key: `0x03`
- Encoding Scheme: Character Hex

##### 4. Long Flag

- Example: `--config`
- Key: `0x04`
- Encoding Scheme: String Length + String

##### 5. String

- Example: `rev-shell`
- Key: `0x05`
- Encoding Scheme: String Length + String

### Notes

1. Strings do not have null-terminators.
2. Numbers will be no bigger than 64-bits.
3. All IPs are valid.
4. All CLI strings end in newlines.

### Printing the Flag

The flag can be printed by successfully decoding each CLI string and passing it to the binary. Each CLI string will return a character.

Flags will be in the standard `CWE{}` format.

## Scoring

You will be evaluated based on the following software engineering pillars:

- Correctness (printing the flag)
- Safe from bugs (handles bad input without crashing)
- Easy to understand (commented and intuitive program structure)
- Ready for change (modular implementation)
- Adherence to the [Google C Style Guide](https://google.github.io/styleguide/cppguide.html).
