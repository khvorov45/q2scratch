#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#define Byte 1
#define Kilobyte 1024 * Byte
#define Megabyte 1024 * Kilobyte
#define Gigabyte 1024 * Megabyte

#ifndef assert
#define assert(cond) do { if (cond) {} else __debugbreak(); } while (0)
#endif

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define STR(x) ((Str){x, sizeof(x) - 1})
#define LIT(x) (int)x.len, x.ptr
#define absval(x) ((x) < 0 ? -(x) : x)
#define unused(x) ((x) = (x))
#define arrayCount(x) (sizeof(x) / sizeof((x)[0]))
#define arrpush(arr, val) assert((arr).len < (arr).cap); (arr).ptr[(arr).len++] = (val)
#define arrpusharr(arr, val) assert((arr).len + (val).len <= (arr).cap); memcpy((arr).ptr + (arr).len, (val).ptr, (val).len * sizeof(*(val).ptr)); (arr).len += (val).len
#define arenaAllocArray(arena, type, count) ((type*)arenaAlloc((arena), sizeof(type) * (count)))
#define arenaAllocAndZeroArray(arena, type, count) ((type*)arenaAllocAndZero((arena), sizeof(type) * (count)))
#define tempMemoryBlock(arena_) for (TempMemory _temp_ = beginTempMemory(arena_); _temp_.arena; endTempMemory(&_temp_))

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

typedef struct i8arr {i8* ptr; i64 len;} i8arr;
typedef struct i16arr {i16* ptr; i64 len;} i16arr;
typedef struct i32arr {i32* ptr; i64 len;} i32arr;
typedef struct i64arr {i64* ptr; i64 len;} i64arr;
typedef struct u8arr {u8* ptr; i64 len;} u8arr;
typedef struct u16arr {u16* ptr; i64 len;} u16arr;
typedef struct u32arr {u32* ptr; i64 len;} u32arr;
typedef struct u64arr {u64* ptr; i64 len;} u64arr;
typedef struct f32arr {f32* ptr; i64 len;} f32arr;
typedef struct f64arr {f64* ptr; i64 len;} f64arr;

//
// SECTION Memory
//

typedef struct Arena {
    void* base;
    i64 size;
    i64 used;
    i64 tempCount;
} Arena;
static i64 arenaFreesize(Arena* arena) { return arena->size - arena->used;}
static void* arenaFreeptr(Arena* arena) { return arena->base + arena->used;}

static void* arenaAlloc(Arena* arena, i64 size) {
    assert(arenaFreesize(arena) >= size);
    void* result = arenaFreeptr(arena);
    arena->used += size;
    return result;
}

static void* arenaAllocAndZero(Arena* arena, i64 size) {
    void* ptr = arenaAlloc(arena, size);
    memset(ptr, 0, size);
    return ptr;
}

typedef struct TempMemory {
    i64 usedBefore;
    i64 tempBefore;
    Arena* arena;
} TempMemory;

static TempMemory beginTempMemory(Arena* arena) {
    TempMemory result = {.usedBefore = arena->used, .tempBefore = arena->tempCount, .arena = arena};
    arena->tempCount += 1;
    return result;
}

static void endTempMemory(TempMemory* temp) {
    assert(temp->usedBefore <= temp->arena->used);
    assert(temp->tempBefore == temp->arena->tempCount - 1);
    temp->arena->used = temp->usedBefore;
    temp->arena->tempCount -= 1;
    *temp = (TempMemory) {};
}

static bool memeq(void* ptr1, void* ptr2, i64 len) {
    int memcmpResult = memcmp(ptr1, ptr2, len);
    bool result = memcmpResult == 0;
    return result;
}

//
// SECTION String
//

typedef struct Str {
    char* ptr;
    i64 len;
} Str;

typedef struct Strarr {
    Str* ptr;
    i64 len;
} Strarr;

static bool streq(Str str1, Str str2) {
    bool result = false;
    if (str1.len == str2.len) {
        result = memeq(str1.ptr, str2.ptr, str1.len);
    }
    return result;
}

static bool strarreq(Strarr arr1, Strarr arr2) {
    bool result = false;
    if (arr1.len == arr2.len) {
        result = true;
        for (i64 index = 0; index < arr1.len && result; index++) {
            Str str1 = arr1.ptr[index];
            Str str2 = arr2.ptr[index];
            result = streq(str1, str2);
        }
    }
    return result;
}
