#include "interpreter.h"

void handle_jmp(unsigned char* stream, size_t* ip) {
    short offset;
    memcpy(&offset, stream + *ip + 1, sizeof(short));
    *ip += offset;
}

void handle_swp(unsigned char* stream, size_t* ip, size_t length) {
    unsigned char i1 = stream[*ip + 1];
    unsigned char i2 = stream[*ip + 2];

    if (i1 >= length || i2 >= length) return;

    unsigned char tmp = stream[i1];
    stream[i1] = stream[i2];
    stream[i2] = tmp;

    *ip += SWP_SIZE;
}

void handle_add(unsigned char* stream, size_t* ip, size_t length) {
    unsigned char index = stream[*ip + 1];
    if (index + 4 > length) return;

    unsigned int value;
    memcpy(&value, stream + *ip + 2, sizeof(unsigned int));

    unsigned int* target = (unsigned int*)(stream + index);
    *target += value;

    *ip += ADD_SIZE;
}

void handle_xor(unsigned char* stream, size_t* ip, size_t length) {
    unsigned char index = stream[*ip + 1];
    if (index + 8 > length) return;

    long long value;
    memcpy(&value, stream + *ip + 2, sizeof(long long));

    long long* target = (long long*)(stream + index);
    *target ^= value;

    *ip += XOR_SIZE;
}

void handle_invert(unsigned char* stream, size_t* ip, size_t length) {
    unsigned char index = stream[*ip + 1];
    if (index >= length) return;

    unsigned char val = stream[index];
    unsigned char rev = 0;
    for (int i = 0; i < 8; i++) {
        rev |= ((val >> i) & 1) << (7 - i);
    }
    stream[index] = rev;

    *ip += INVERT_SIZE;
}

void handle_print(unsigned char* stream, size_t* ip) {
    putchar(stream[*ip + 1]);
    fflush(stdout);
    *ip += PRINT_SIZE;
}

void interpret(unsigned char* stream, size_t length) {
    size_t ip = 0;

    while (ip < length) {
        unsigned char opcode = stream[ip];

        switch (opcode) {
            case END:
                return;

            case JMP:
                handle_jmp(stream, &ip);
                break;

            case SWP:
                handle_swp(stream, &ip, length);
                break;

            case ADD:
                handle_add(stream, &ip, length);
                break;

            case XOR:
                handle_xor(stream, &ip, length);
                break;

            case INVERT:
                handle_invert(stream, &ip, length);
                break;

            case PRINT:
                handle_print(stream, &ip);
                break;

            default:
                fprintf(stderr, "Unknown opcode %02X at position %zu\n", opcode, ip);
                return;
        }
    }
}

int main() {
    FILE* f = fopen(FILE_NAME, "rb");
    if (!f) {
        perror("Error opening file");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    unsigned char* buffer = (unsigned char*)malloc(fsize);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(f);
        return 1;
    }

    fread(buffer, 1, fsize, f);
    fclose(f);

    interpret(buffer, fsize);

    free(buffer);
    return 0;
}