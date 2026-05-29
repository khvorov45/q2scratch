#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>

#define Byte 1
#define Kilobyte 1024 * Byte
#define Megabyte 1024 * Kilobyte
#define Gigabyte 1024 * Megabyte

#ifndef assert
#define assert(cond) do { if (cond) {} else __debugbreak(); } while (0)
#endif

#define unimplemented() __debugbreak()
#define assertStrInArena(str, arena) assert((u64)(str)->ptr >= (u64)(arena)->base && (u64)(str)->ptr + (u64)(str)->len <= (u64)(arena)->base + (u64)(arena)->size);
#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)
#define STR(x) ((Str){x, sizeof(x) - 1})
#define LIT(x) (int)x.len, x.ptr
#define absval(x) ((x) < 0 ? -(x) : x)
#define unused(x) ((x) = (x))
#define carrayCount(x) (sizeof(x) / sizeof((x)[0]))
#define dynarrpush(arr, val) assert((arr)->len < (arr)->cap); (arr)->ptr[(arr)->len++] = (val)
#define dynarrpusharr(arr, val) assert((arr).len + (val).len <= (arr).cap); memcpy((arr).ptr + (arr).len, (val).ptr, (val).len * sizeof(*(val).ptr)); (arr).len += (val).len
#define slicefromcarray(carr) {.ptr = carr, .len = carrayCount(carr)}
#define arenaAllocArray(arena, type, count) ((type*)arenaAlloc((arena), sizeof(type) * (count)))
#define arenaAllocAndZeroArray(arena, type, count) ((type*)arenaAllocAndZero((arena), sizeof(type) * (count)))
#define arenaAllocDynarr(arena, type, capacity) {.ptr = arenaAllocArray(arena, type, capacity), .len = 0, .cap = capacity}
#define arenaAllocAndZeroDynarr(arena, type, capacity) {.ptr = arenaAllocAndZeroArray(arena, type, capacity), .len = 0, .cap = capacity}
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

static Arena arenaFromArena(Arena* arena, i64 size) {
    Arena result = {
        .base = arenaAlloc(arena, size),
        .size = size,
    };
    return result;
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

static void keepTempMemory(TempMemory* temp) {
    assert(temp->usedBefore <= temp->arena->used);
    assert(temp->tempBefore == temp->arena->tempCount - 1);
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

typedef struct StrBuilder {
    Arena* arena;
    char* start;
} StrBuilder;

static StrBuilder beginStr(Arena* arena) {
    StrBuilder builder = {.arena = arena, .start = arenaFreeptr(arena)};
    return builder;
}

static void addToStr_(StrBuilder* builder, char* fmt, va_list args) {
    char* out = arenaFreeptr(builder->arena);
    int printResult = stbsp_vsnprintf(out, arenaFreesize(builder->arena), fmt, args);
    builder->arena->used += printResult;
}

__attribute__((format(printf,2,3)))
static void addToStr(StrBuilder* builder, char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    addToStr_(builder, fmt, va);
    va_end(va);
}

static Str endStr(StrBuilder* builder) {
    Str str = {.ptr = builder->start, .len = (i64)((u64)arenaFreeptr(builder->arena) - (u64)builder->start)};
    if (arenaFreesize(builder->arena) >= 1) {
        arenaAllocAndZero(builder->arena, 1); // NOTE: null terminator, we don't need it but it's good for debugging
    }
    *builder = (StrBuilder) {};
    return str;
}

static Str strfmt_(Arena* arena, char* fmt, va_list args) {
    StrBuilder builder = beginStr(arena);
    addToStr_(&builder, fmt, args);
    Str result = endStr(&builder);
    return result;
}

__attribute__((format(printf,2,3)))
static Str strfmt(Arena* arena, char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    Str result = strfmt_(arena, fmt, va);
    va_end(va);
    return result;
}

//
// SECTION Logging
//

typedef enum LogEntryCategory {
    LogEntryCategory_Ok,
    LogEntryCategory_Error,
} LogEntryCategory;

typedef struct LogEntry {
    Str str;
    u64 time;
    LogEntryCategory category;
} LogEntry;
typedef struct LogEntries {LogEntry* ptr; i64 len; i64 cap;} LogEntries;

typedef struct LogSubBuffer {
    LogEntries entries;
    Arena arena;
} LogSubBuffer;
typedef struct LogSubBuffers {LogSubBuffer* ptr; i64 len;} LogSubBuffers;

typedef struct Log {
    LogSubBuffers circle;
    i64 currentIndex;
} Log;

static Log createLog(Arena* arena, i64 numSubBuffers, i64 maxEntryCountInEachSubbuffer, i64 stringBufferSizeInEachSubbuffer) {
    Log log = {};
    log.circle.len = numSubBuffers;
    log.circle.ptr = arenaAllocAndZeroArray(arena, LogSubBuffer, log.circle.len);
    for (i64 index = 0; index < log.circle.len; index++) {
        log.circle.ptr[index].entries = (LogEntries) arenaAllocDynarr(arena, LogEntry, maxEntryCountInEachSubbuffer);
        log.circle.ptr[index].arena = arenaFromArena(arena, stringBufferSizeInEachSubbuffer);
    }
    return log;
}

static void addLogEntry_(Log* log, LogEntryCategory category, char* fmt, va_list args) {
    assert(log->currentIndex >= 0 && log->currentIndex < log->circle.len);

    LogEntries* entries = &log->circle.ptr[log->currentIndex].entries;
    Arena* arena = &log->circle.ptr[log->currentIndex].arena;

    i64 maxExpectedSizeForALogEntry = 300;
    if (entries->len >= entries->cap || arenaFreesize(arena) < maxExpectedSizeForALogEntry) {
        log->currentIndex = (log->currentIndex + 1) % log->circle.len;
        entries = &log->circle.ptr[log->currentIndex].entries;
        arena = &log->circle.ptr[log->currentIndex].arena;
        arena->used = 0;
        entries->len = 0;
    }

    assert(entries->len < entries->cap);
    assert(arenaFreesize(arena) >= maxExpectedSizeForALogEntry);

    Str entryStr = strfmt_(arena, fmt, args);

    LogEntry entry = {
        .str = entryStr,
        .time = __rdtsc(),
        .category = category
    };
    dynarrpush(entries, entry);
}

__attribute__((format(printf,3,4)))
static void addLogEntry(Log* log, LogEntryCategory category, char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    addLogEntry_(log, category, fmt, args);
    va_end(args);
}

typedef struct LogChoronologicalIter {
    Log* log;
    i64 currentCircle;
    i64 currentEntryInCurrentCircle;
} LogChoronologicalIter;

static LogChoronologicalIter chronologicalIter(Log* log) {
    LogChoronologicalIter iter = {.log = log, .currentCircle = (log->currentIndex + 1) % log->circle.len, .currentEntryInCurrentCircle = 0};
    return iter;
}

static bool ended(LogChoronologicalIter* iter) {
    bool currentCircleIsMostRecent = iter->currentCircle == iter->log->currentIndex;
    bool currentEntryInCircleIsMostRecent = iter->currentEntryInCurrentCircle >= iter->log->circle.ptr[iter->currentCircle].entries.len - 1;
    bool result = currentCircleIsMostRecent && currentEntryInCircleIsMostRecent;
    return result;
}

static void advance(LogChoronologicalIter* iter) {
    if (!ended(iter)) {
        iter->currentEntryInCurrentCircle += 1;
        bool currentCircleIsDone = iter->currentEntryInCurrentCircle == iter->log->circle.ptr[iter->currentCircle].entries.len;
        if (currentCircleIsDone) {
            iter->currentEntryInCurrentCircle = 0;
            iter->currentCircle = (iter->currentCircle + 1) % iter->log->circle.len;
        }
    }
}

static LogEntry* currentEntry(LogChoronologicalIter* iter) {
    assert(iter->currentCircle >= 0 && iter->currentCircle < iter->log->circle.len);
    LogSubBuffer* currentCircle = iter->log->circle.ptr + iter->currentCircle;
    assert(iter->currentEntryInCurrentCircle >= 0 && iter->currentEntryInCurrentCircle < currentCircle->entries.len);
    LogEntry* result = currentCircle->entries.ptr + iter->currentEntryInCurrentCircle;
    return result;
}

static LogEntry* nextEntry(LogChoronologicalIter* iter) {
    advance(iter);
    LogEntry* entry = currentEntry(iter);
    return entry;
}

//
// SECTION Platform API
//

typedef enum Status {
    Status_Ok,
    Status_Error,
} Status;

typedef struct ReadResult {
    Status status;
    u8slice file;
} ReadResult;

typedef struct Platform {
    ReadResult (*readEntireFile)(Arena* arena, Str filepath);
} Platform;

//
// SECTION Commands
//

struct CommandData;
typedef void (*CommandProc)(struct CommandData*);

typedef struct Command {
    Str name;
    CommandProc proc;
} Command;
typedef struct Commands {Command* ptr; i64 len; i64 cap;} Commands;

typedef struct CommandAlias {
    Str name;
    Str value;
    Arena arena;
} CommandAlias;
typedef struct CommandAliases {struct {CommandAlias* ptr; i64 len; i64 cap;} arr; Arena arena;} CommandAliases;

_STATIC_ASSERT(offsetof(Command, name) == 0 && offsetof(CommandAlias, name) == 0);
#define findByName(slice, name) findByName_(slice.ptr, slice.len, name, sizeof(slice.ptr[0]))
static void* findByName_(void* ptr, i64 len, Str name, i64 sizeOfOneEntry) {
    void* result = 0;
    for (i64 byteIndex = 0; byteIndex < len * sizeOfOneEntry && !result; byteIndex += sizeOfOneEntry) {
        Str* thisName = (Str*)(ptr + byteIndex);
        if (streq(*thisName, name)) {
            result = thisName;
        }
    }
    return result;
}

typedef struct CommandArg {
    Str value;
} CommandArg;
typedef struct CommandArgs {CommandArg* ptr; i64 len; i64 cap;} CommandArgs;

typedef struct CommandExecution {
    Arena arena;
    bool pauseUntilNextFrame;
} CommandExecution;

typedef struct CommandData {
    Commands cmds;
    CommandAliases aliases;
    CommandArgs args;
    CommandExecution execution;
    Arena argsArena;
    Arena scratchArena;
    Log* log;
    Platform* platform;
} CommandData;

typedef struct CommandDataOpts {
    Arena* arena;
    i64 maxCmds, maxAliases, maxArgs, argsArenaSize, executeArenaSize, aliasArenaSize, scratchArenaSize;
    Log* log;
    Platform* platform;
} CommandDataOpts;

#define createCommandData(...) createCommandData_((CommandDataOpts) {.maxCmds = 1024, .maxAliases = 1024, .maxArgs = 64, .argsArenaSize = 1 * Kilobyte, .executeArenaSize = 1 * Kilobyte, .aliasArenaSize = 1 * Kilobyte, .scratchArenaSize = 1 * Kilobyte, __VA_ARGS__})
static CommandData createCommandData_(CommandDataOpts opts) {
    CommandData cmdData = {
        .cmds = (Commands) arenaAllocDynarr(opts.arena, Command, opts.maxCmds),
        .aliases = (CommandAliases) {.arr = arenaAllocDynarr(opts.arena, CommandAlias, opts.maxAliases), .arena = arenaFromArena(opts.arena, opts.aliasArenaSize * (opts.maxAliases))},
        .args = (CommandArgs) arenaAllocDynarr(opts. arena, CommandArg, opts.maxArgs),
        .argsArena = arenaFromArena(opts.arena, opts.argsArenaSize),
        .execution = (CommandExecution) {.arena = arenaFromArena(opts.arena, opts.executeArenaSize), .pauseUntilNextFrame = false},
        .scratchArena = arenaFromArena(opts.arena, opts.scratchArenaSize),
        .log = opts.log,
        .platform = opts.platform
    };
    return cmdData;
}

static void clearArgs(CommandData* data) {
    data->argsArena.used = 0;
    data->args.len = 0;
}

static void addArg(CommandData* data, Str arg) {
    if (data->args.len < data->args.cap) {
        assert(data->args.len >= 0);
        CommandArg newArg = {.value = strfmt(&data->argsArena, "%*s", LIT(arg))};
        dynarrpush(&data->args, newArg);
        CommandArg* addedArg = data->args.ptr + data->args.len - 1;
        assertStrInArena(&addedArg->value, &data->argsArena);
    } else {
        assert(data->args.len == data->args.cap);
        addLogEntry(data->log, LogEntryCategory_Error, "could not add arg, buffer full");
    }
}

#define addCommand(data, name) addCommand_(data, STR(STRINGIFY(name)), name);
static void addCommand_(CommandData* data, Str name, CommandProc function) {
    if (data->cmds.len < data->cmds.cap) {
        if (findByName(data->cmds, name) == 0) {
            Command entry = {.proc = function, .name = name};
            dynarrpush(&data->cmds, entry);
        } else {
            addLogEntry(data->log, LogEntryCategory_Error, "addCommand: %*s already defined", LIT(name));
        }
    } else {
        addLogEntry(data->log, LogEntryCategory_Error, "addCommand: %*s could not be added, buffer full", LIT(name));
    }
}

static void cmdlist(CommandData* data) {
    tempMemoryBlock(&data->scratchArena) {
        StrBuilder builder = beginStr(&data->scratchArena);
        for (i64 index = 0; index < data->cmds.len; index++) {
            Command entry = data->cmds.ptr[index];
            addToStr(&builder, "%*s\n", LIT(entry.name));
        }
        addToStr(&builder, "%lli commands", data->cmds.len);
        Str out = endStr(&builder);
        addLogEntry(data->log, LogEntryCategory_Ok, "%*s", LIT(out));
    }
}

static void cmdexec(CommandData* data) {
    if (data->args.len == 2) {
        assert(data->platform->readEntireFile);
        assert(data->execution.arena.base);
        Str filename = data->args.ptr[1].value;
        ReadResult readResult = data->platform->readEntireFile(&data->execution.arena, filename);
        if (readResult.status == Status_Ok) {
            addLogEntry(data->log, LogEntryCategory_Ok, "execing %*s", LIT(filename));
        } else {
            addLogEntry(data->log, LogEntryCategory_Error, "couldn't exec %*s", LIT(filename));
        }
    } else {
        addLogEntry(data->log, LogEntryCategory_Ok, "exec <filename> : execute a script file");
    }
}

static void cmdecho(CommandData* data) {
    tempMemoryBlock(&data->scratchArena) {
        StrBuilder builder = beginStr(&data->scratchArena);
        for (i64 index = 1; index < data->args.len; index++) {
            Str arg = data->args.ptr[index].value;
            addToStr(&builder, "%*s ", LIT(arg));
        }
        Str str = endStr(&builder);
        addLogEntry(data->log, LogEntryCategory_Ok, "%*s", LIT(str));
    }
}

static void cmdalias(CommandData* data) {
    if (data->args.len == 1) {
        tempMemoryBlock(&data->scratchArena) {
            StrBuilder builder = beginStr(&data->scratchArena);
            addToStr(&builder, "Current alias commands:\n");
            for (i64 index = 0; index < data->aliases.arr.len; index++) {
                CommandAlias alias = data->aliases.arr.ptr[index];
                addToStr(&builder, "%*s : %*.s\n", LIT(alias.name), LIT(alias.value));
            }
            Str out = endStr(&builder);
            addLogEntry(data->log, LogEntryCategory_Ok, "%*s", LIT(out));
        }

    } else if (data->args.len == 2) {
        addLogEntry(data->log, LogEntryCategory_Error, "usage: alias <name> <command(s)>");

    } else if (data->args.len > 2) {
        Str nameInArgs = data->args.ptr[1].value;
        CommandAlias* alias = findByName(data->aliases.arr, nameInArgs);

        if (!alias) {
            if (data->aliases.arr.len < data->aliases.arr.cap) {
                CommandAlias newAlias = {.arena = arenaFromArena(&data->aliases.arena, data->aliases.arena.size / data->aliases.arr.cap)};
                newAlias.name = strfmt(&newAlias.arena, "%*s", LIT(nameInArgs));
                dynarrpush(&data->aliases.arr, newAlias);
                alias = data->aliases.arr.ptr + data->aliases.arr.len - 1;
            } else {
                assert(data->aliases.arr.len == data->aliases.arr.cap);
                addLogEntry(data->log, LogEntryCategory_Error, "could not add alias %*s, buffer full", LIT(nameInArgs));
            }

        } else {
            addLogEntry(data->log, LogEntryCategory_Ok, "overriding previously defined alias %*s", LIT(nameInArgs));
            assert(alias->name.ptr == alias->arena.base);
            alias->arena.used = alias->name.len + 1;
            assert(alias->name.ptr[alias->name.len] == '\0'); // NOTE: we don't need null terminators but it's good for debugging, so preserve them
            alias->value = (Str) {};
        }

        if (alias) {
            assert(alias->arena.base);
            assert(alias->name.len > 0);
            assert(alias->name.ptr);
            assertStrInArena(&alias->name, &alias->arena);

            StrBuilder builder = beginStr(&alias->arena);
            for (i64 index = 2; index < data->args.len; index++) {
                Str arg = data->args.ptr[index].value;
                addToStr(&builder, "%*s", LIT(arg));
                if (index != data->args.len - 1) {
                    addToStr(&builder, " ");
                }
            }
            alias->value = endStr(&builder);

            assertStrInArena(&alias->value, &alias->arena);
        }
    }
}

static void cmdwait(CommandData* data) {
    data->execution.pauseUntilNextFrame = true;
}

static void cmdkeybind(CommandData* data) {
    unused(data);
    unimplemented();
}

static void cmdkeyunbind(CommandData* data) {
    unused(data);
    unimplemented();
}

static void cmdkeyunbindall(CommandData* data) {
    unused(data);
    unimplemented();
}

static void cmdkeybindlist(CommandData* data) {
    unused(data);
    unimplemented();
}

//
// SECTION Init
//

static void gameInit(Arena* arena, Platform* platform) {
    Log* log = arenaAllocAndZeroArray(arena, Log, 1);
    *log = createLog(arena, 2, 1024, 8 * Kilobyte);

    CommandData* cmdData = arenaAllocAndZeroArray(arena, CommandData, 1);
    *cmdData = createCommandData(.arena = arena, .log = log, .platform = platform);

    addCommand(cmdData, cmdlist);
    addCommand(cmdData, cmdexec);
    addCommand(cmdData, cmdecho);
    addCommand(cmdData, cmdalias);
    addCommand(cmdData, cmdwait);
    addCommand(cmdData, cmdkeybind);
    addCommand(cmdData, cmdkeyunbind);
    addCommand(cmdData, cmdkeyunbindall);
    addCommand(cmdData, cmdkeybindlist);

// 	// we need to add the early commands twice, because
// 	// a basedir or cddir needs to be set before execing
// 	// config files, but we want other parms to override
// 	// the settings of the config files
// 	Cbuf_AddEarlyCommands (false);
// 	Cbuf_Execute ();

// 	FS_InitFilesystem ();

// 	Cbuf_AddText ("exec default.cfg\n");
// 	Cbuf_AddText ("exec config.cfg\n");

// 	Cbuf_AddEarlyCommands (true);
// 	Cbuf_Execute ();

// 	//
// 	// init commands and vars
// 	//
//     Cmd_AddCommand ("z_stats", Z_Stats_f);
//     Cmd_AddCommand ("error", Com_Error_f);

// 	host_speeds = Cvar_Get ("host_speeds", "0", 0);
// 	log_stats = Cvar_Get ("log_stats", "0", 0);
// 	developer = Cvar_Get ("developer", "0", 0);
// 	timescale = Cvar_Get ("timescale", "1", 0);
// 	fixedtime = Cvar_Get ("fixedtime", "0", 0);
// 	logfile_active = Cvar_Get ("logfile", "0", 0);
// 	showtrace = Cvar_Get ("showtrace", "0", 0);
// #ifdef DEDICATED_ONLY
// 	dedicated = Cvar_Get ("dedicated", "1", CVAR_NOSET);
// #else
// 	dedicated = Cvar_Get ("dedicated", "0", CVAR_NOSET);
// #endif

// 	char* s = va("%4.2f %s %s %s", VERSION, CPUSTRING, __DATE__, BUILDSTRING);
// 	Cvar_Get ("version", s, CVAR_SERVERINFO|CVAR_NOSET);


// 	if (dedicated->value)
// 		Cmd_AddCommand ("quit", Com_Quit);

// 	Sys_Init ();

// 	NET_Init ();
// 	Netchan_Init ();

// 	SV_Init ();
// 	CL_Init ();

// 	// add + commands from command line
// 	if (!Cbuf_AddLateCommands ())
// 	{	// if the user didn't give any commands, run default action
// 		if (!dedicated->value)
// 			Cbuf_AddText ("d1\n");
// 		else
// 			Cbuf_AddText ("dedicated_start\n");
// 		Cbuf_Execute ();
// 	}
// 	else
// 	{	// the user asked for something explicit
// 		// so drop the loading plaque
// 		SCR_EndLoadingPlaque ();
// 	}

// 	Com_Printf ("====== Quake2 Initialized ======\n\n");
}

