#include <stdint.h>

/**
 * This header file contains C structures describing the format of `hamlet.bin`.
 *
 * The top level container is "Play" - start there.
 */

typedef struct play Play;
typedef struct act Act;
typedef struct scene Scene;
typedef struct line Line;
typedef struct string_table_entry StringTableEntry;

#define PLAY_MAX_NAME_LEN 32
#define PLAY_MAX_DESC_LEN 64

typedef char
    Character[PLAY_MAX_NAME_LEN]; // Represents a Character in the play, a null terminated string

// Container for an Act in the play
struct act
{
    uint32_t number;             // The number of this Act
    uint32_t num_scenes;         // The number of Scenes in this Act
    uint32_t first_scene_offset; // Offset from start of the scene table
                                 //    to the first Scene in this Act  (in bytes)
};

// Rrepresentation of a play
struct play
{
    char title[PLAY_MAX_DESC_LEN];      // null terminated string
    char playwright[PLAY_MAX_NAME_LEN]; // null terminated string
    uint32_t character_table_offset;    // Offset from the beginning of this struct to the Character
                                        // table (in bytes)
    uint32_t scene_table_offset;  // Offset from the beginning of this struct to the Scene table (in
                                  // bytes)
    uint32_t string_table_offset; // Offset from the beginning of this struct to the string table
                                  // (in bytes)
    uint32_t num_characters;      // The number of entries in the Character table
    uint32_t num_acts;            // The number of Acts in 'acts'
    Act acts[0];                  // The Acts in this Play

    // Character table is here (array of Character types)
    // Scene table is here (array of Scene types)
    // String table is here (array of StringTableEntry types)
};

// Container for a line in a Scene
struct line
{
    int32_t character;    // Index to entry in character table (-1 for stage directions)  (0 for the
                          // first character, 1 for the second, etc)
    uint32_t number;      // The number of this line in the Scene (0 for stage directions)
    uint32_t text_offset; // Offset to this line in the string table  (in bytes)
};

// Container for a Scene in an act
struct scene
{
    uint32_t size;                   // The size of this Scene struct, in bytes
    uint32_t number;                 // The number of this Scene
    char setting[PLAY_MAX_DESC_LEN]; // null terminated string for the Scene setting
    uint32_t num_lines;              // The number of lines in this Scene
    Line lines[0];                   // The lines in this Scene
};

// String table entry
struct string_table_entry
{
    uint32_t size; // The size of 'text' (including a null terminator)
    char text[0];  // The null terminated string
};
