#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

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
#define carrayCount(x) (sizeof(x) / sizeof((x)[0]))
#define dynarrpush(arr, val) assert((arr).len < (arr).cap); (arr).ptr[(arr).len++] = (val)
#define dynarrpusharr(arr, val) assert((arr).len + (val).len <= (arr).cap); memcpy((arr).ptr + (arr).len, (val).ptr, (val).len * sizeof(*(val).ptr)); (arr).len += (val).len
#define slicefromcarray(carr) {.ptr = carr, .len = carrayCount(carr)}
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

typedef struct i8slice {i8* ptr; i64 len;} i8slice;
typedef struct i16slice {i16* ptr; i64 len;} i16slice;
typedef struct i32slice {i32* ptr; i64 len;} i32slice;
typedef struct i64slice {i64* ptr; i64 len;} i64slice;
typedef struct u8slice {u8* ptr; i64 len;} u8slice;
typedef struct u16slice {u16* ptr; i64 len;} u16slice;
typedef struct u32slice {u32* ptr; i64 len;} u32slice;
typedef struct u64slice {u64* ptr; i64 len;} u64slice;
typedef struct f32slice {f32* ptr; i64 len;} f32slice;
typedef struct f64slice {f64* ptr; i64 len;} f64slice;

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

typedef struct Strslice {
    Str* ptr;
    i64 len;
} Strslice;

static bool streq(Str str1, Str str2) {
    bool result = false;
    if (str1.len == str2.len) {
        result = memeq(str1.ptr, str2.ptr, str1.len);
    }
    return result;
}

static bool strsliceeq(Strslice slice1, Strslice slice2) {
    bool result = false;
    if (slice1.len == slice2.len) {
        result = true;
        for (i64 index = 0; index < slice1.len && result; index++) {
            Str str1 = slice1.ptr[index];
            Str str2 = slice2.ptr[index];
            result = streq(str1, str2);
        }
    }
    return result;
}

__attribute__((format(printf,2,3)))
static Str strfmt(Arena* arena, char* fmt, ...) {
    char* out = arenaFreeptr(arena);

    va_list va;
    va_start(va, fmt);
    int printResult = stbsp_vsnprintf(out, arenaFreesize(arena), fmt, va);
    va_end(va);

    arena->used += printResult + 1;
    Str result = {out, printResult};
    return result;
}

typedef struct StrIter {
	Str cur;
} StrIter;

static StrIter striter(Str str) {
	if (str.len > 0) assert(str.ptr);
	StrIter iter = {.cur = str};
	return iter;
}

static bool striterEnded(StrIter* iter) {
	bool result = iter->cur.len == 0;
	return result;
}

static void striterAdvanceOne(StrIter* iter) {
	if (!striterEnded(iter)) {
		iter->cur.ptr++;
		iter->cur.len--;
	}
}

static void striterAdvanceUntilWhitespace(StrIter* iter) {
	if (!striterEnded(iter)) {
		while (!isspace(*iter->cur.ptr)) {
			striterAdvanceOne(iter);
		}
	}
}

static void striterAdvancePastWhitespace(StrIter* iter) {
	if (!striterEnded(iter)) {
		while (isspace(*iter->cur.ptr)) {
			striterAdvanceOne(iter);
		}
	}
}
