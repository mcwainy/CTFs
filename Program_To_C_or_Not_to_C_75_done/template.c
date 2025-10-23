#include "play.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Implement to get a given line in the Play,
 * specified by it's act number, scene number, and finally,
 * it's number in the scene.
 *
 * Arguments:
 *  play: Pointer to the Play struct
 *  act: The act number
 *  scene: The scene number
 *  line_number: The line number
 */

char *get_line(Play *play, int act, int scene, int line_number)
{
    if (!play || act <= 0 || scene <= 0 || line_number < 0)
        return NULL;

    if ((uint32_t)act > play->num_acts)
        return NULL;

    // Select act (acts are 1-based in the inputs)
    Act *correct_act = &play->acts[act - 1];

    // Base of the scene table
    uint8_t *scene_table_base = (uint8_t *)play + play->scene_table_offset;

    // First scene of this act (offset is from start of scene table)
    Scene *correct_scene = (Scene *)(scene_table_base + correct_act->first_scene_offset);

    // Advance to the requested scene (1-based)
    for (int i = 1; i < scene; i++) {
        uint32_t scene_size = correct_scene->size;                // size in bytes
        correct_scene = (Scene *)((uint8_t *)correct_scene + scene_size);
    }

    // Find the line whose 'number' equals line_number
    Line *lines = correct_scene->lines;
    uint32_t nlines = correct_scene->num_lines;
    Line *target = NULL;

    for (uint32_t i = 0; i < nlines; i++) {
        if (lines[i].number == (uint32_t)line_number) {
            target = &lines[i];
            break;
        }
    }
    if (!target)
        return NULL;

    // Resolve the string for this line via the string table
    uint8_t *string_table_base = (uint8_t *)play + play->string_table_offset;
    StringTableEntry *entry = (StringTableEntry *)(string_table_base + target->text_offset);

    return entry->text;  // null-terminated
}



/////////////////////////////////////// YOU DON'T NEED TO EDIT BELOW HERE
//////////////////////////////////////////

void print_flag(Play *play);

int main()
{
    // Open the file
    int fd = open("./hamlet.bin", O_RDONLY);

    if (fd == -1)
    {
        printf("open failed! %s\n", strerror(errno));
        exit(0);
    }

    // Stat it to get its size
    struct stat statbuf = { 0 };
    if (fstat(fd, &statbuf) != 0)
    {
        printf("fstat failed! %s\n", strerror(errno));
        exit(0);
    }

    // Map the file into memory
    Play *play = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (play == MAP_FAILED)
    {
        printf("mmap failed! %s\n", strerror(errno));
        exit(0);
    }

    // Don't need this anymore
    close(fd);

    print_flag(play);
    return 0;
}

void print_flag(Play *play)
{
    struct line_code
    {
        int a;
        int b;
        int c;
        int d;
    };

    char flag[256] = { 0 };
    struct line_code codes[] = {{0x5,0x1,0x137,0x37},{0x4,0x4,0x28,0x1},{0x5,0x2,0x15,0x31},{0x1,0x4,0x4c,0x0},{0x5,0x1,0x70,0x25},{0x2,0x1,0x60,0xf},{0x5,0x1,0x8d,0x36},{0x3,0x2,0x146,0x1d},{0x5,0x1,0x13,0x1a},{0x3,0x4,0x73,0x6},{0x4,0x7,0x96,0x5},{0x3,0x2,0xff,0x3a},{0x5,0x2,0xda,0x1},{0x1,0x2,0x1c,0x9},{0x3,0x2,0xff,0x2d},{0x4,0x1,0x21,0x26},{0x4,0x7,0x82,0x5},{0x3,0x1,0x6c,0x13},{0x3,0x2,0xff,0x30},{0x3,0x1,0x5a,0xd},{0x4,0x3,0x34,0xa},{0x1,0x2,0x11c,0x3},{0x2,0x2,0x16d,0x10},{0x3,0x2,0xff,0x37},{0x5,0x2,0x8f,0x16},{0x4,0x4,0x2b,0x17},{0x3,0x2,0xff,0x3a},{0x3,0x3,0x45,0xf},{0x4,0x6,0xd,0x3e},{0x5,0x2,0x89,0x27},{0x3,0x2,0xff,0x2d},{0x4,0x2,0x22,0x28},{0x5,0x2,0x0,0x9},{0x4,0x5,0xb7,0x7},{0x5,0x2,0xf2,0x0},{0x3,0x1,0x5c,0x12},{0x3,0x2,0x0,0x0},{0x2,0x2,0x1aa,0x1b},{0x4,0x1,0x0,0x1},{0x1,0x1,0x15,0x15}};
    size_t num_codes = sizeof(codes) / sizeof(struct line_code);

    for (int i = 0; i < num_codes; ++i)
    {
        struct line_code code = codes[i];
        char *line = get_line(play, code.a, code.b, code.c);

        if (line != NULL)
            flag[i] = line[code.d];
    }

    printf("%s\n", flag);
}
