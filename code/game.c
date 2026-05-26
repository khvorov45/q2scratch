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
    while (!striterEnded(iter) && !isspace(*iter->cur.ptr)) {
        striterAdvanceOne(iter);
    }
}

static void striterAdvancePastWhitespace(StrIter* iter) {
    while (!striterEnded(iter) && isspace(*iter->cur.ptr)) {
        striterAdvanceOne(iter);
    }
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

__attribute__((format(printf,3,4)))
static void addLogEntry(Log* log, LogEntryCategory category, char* fmt, ...) {
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

    char* out = arenaFreeptr(arena);

    va_list va;
    va_start(va, fmt);
    int printResult = stbsp_vsnprintf(out, arenaFreesize(arena), fmt, va);
    va_end(va);

    arena->used += printResult + 1;
    Str entrystr = {out, printResult};

    LogEntry entry = {
        .str = entrystr,
        .time = __rdtsc(),
        .category = category
    };
    dynarrpush(entries, entry);
}

typedef struct LogChoronologicalIter {
    Log* log;
    i64 currentCircle;
    i64 currentEntryInCurrentCircle;
    bool ended;
} LogChoronologicalIter;

static LogChoronologicalIter chronologicalIter(Log* log) {
    LogChoronologicalIter iter = {.log = log, .currentCircle = (log->currentIndex + 1) % log->circle.len, .currentEntryInCurrentCircle = 0};
    return iter;
}

static void chronologicalIterNext(LogChoronologicalIter* iter) {
    if (!iter->ended) {
        iter->currentEntryInCurrentCircle += 1;
        bool currentCircleIsDone = iter->currentEntryInCurrentCircle == iter->log->circle.ptr[iter->currentCircle].entries.len;
        if (currentCircleIsDone) {
            iter->currentEntryInCurrentCircle = 0;
            iter->currentCircle = (iter->currentCircle + 1) % iter->log->circle.len;
        }
        bool thisCircleIsLastCircle = iter->currentCircle == iter->log->currentIndex;
        bool thisEntryIsLastEntry = iter->currentEntryInCurrentCircle == iter->log->circle.ptr[iter->currentCircle].entries.len - 1;
        iter->ended = thisCircleIsLastCircle && thisEntryIsLastEntry;
    }
}

static LogEntry* currentEntry(LogChoronologicalIter* iter) {
    assert(iter->currentCircle >= 0 && iter->currentCircle < iter->log->circle.len);
    LogSubBuffer* currentCircle = iter->log->circle.ptr + iter->currentCircle;
    assert(iter->currentEntryInCurrentCircle >= 0 && iter->currentEntryInCurrentCircle < currentCircle->entries.len);
    LogEntry* result = currentCircle->entries.ptr + iter->currentEntryInCurrentCircle;
    return result;
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
	CommandProc proc;
	Str name;
} Command;
typedef struct Commands {Command* ptr; i64 len; i64 cap;} Commands;

static Command* commandsFindByName(Commands* cmds, Str cmdname) {
    Command* result = 0;
	for (i64 index = 0; index < cmds->len && !result; index++) {
        Command* var = cmds->ptr + index;
		if (streq(cmdname, var->name)) {
			result = var;
        }
    }
	return result;
}

typedef struct CommandVar {
	Str name;
	Str string;
	Str latched_string; // NOTE: for CVAR_LATCH vars
	i64 flags;
	bool modified; // NOTE: set each time the cvar is changed
	float value;
} CommandVar;
typedef struct CommandVars {CommandVar* ptr; i64 len; i64 cap;} CommandVars;

static CommandVar* commandVarsFindByName(CommandVars* vars, Str varname) {
    CommandVar* result = 0;
	for (i64 index = 0; index < vars->len && !result; index++) {
        CommandVar* var = vars->ptr + index;
		if (streq(varname, var->name)) {
			result = var;
        }
    }
	return result;
}

typedef struct CommandData {
    Commands cmds;
    CommandVars vars;
    Strslice args;
    Arena executeArena;
    Log* log;
    Platform* platform;
} CommandData;

static CommandData createCommandData(Arena* arena, i64 maxCmds, i64 maxVars, Log* log, Platform* platform) {
    CommandData cmdData = {
        .cmds = (Commands) arenaAllocDynarr(arena, Command, maxCmds),
        .vars = (CommandVars) arenaAllocDynarr(arena, CommandVar, maxVars),
        .executeArena = arenaFromArena(arena, 1 * Megabyte),
        .log = log,
        .platform = platform
    };
    return cmdData;
}

#define addCommand(data, name) addCommand_(data, STR(STRINGIFY(name)), name);
static void addCommand_(CommandData* data, Str name, CommandProc function) {
    if (data->cmds.len < data->cmds.cap) {
        if (commandVarsFindByName(&data->vars, name) == 0) {
            if (commandsFindByName(&data->cmds, name) == 0) {
                Command entry = {.proc = function, .name = name};
                dynarrpush(&data->cmds, entry);
            } else {
                addLogEntry(data->log, LogEntryCategory_Error, "addCommand: %*s already defined", LIT(name));
            }
        } else {
            addLogEntry(data->log, LogEntryCategory_Error, "addCommand: %*s already defined as a var", LIT(name));
        }
    } else {
        addLogEntry(data->log, LogEntryCategory_Error, "addCommand: %*s could not be added, buffer full", LIT(name));
    }
}

static void cmdlist(CommandData* data) {
	for (i64 index = 0; index < data->cmds.len; index++) {
        Command entry = data->cmds.ptr[index];
		addLogEntry(data->log, LogEntryCategory_Ok, "%*s", LIT(entry.name));
	}
	addLogEntry(data->log, LogEntryCategory_Ok, "%lli commands", data->cmds.len);
}

static void cmdexec(CommandData* data) {
	if (data->args.len == 2) {
        assert(data->platform->readEntireFile);
        assert(data->executeArena.base);
        Str filename = data->args.ptr[1];
        ReadResult readResult = data->platform->readEntireFile(&data->executeArena, filename);
        if (readResult.status == Status_Ok) {
            addLogEntry(data->log, LogEntryCategory_Ok, "execing %*s", LIT(filename));
        } else {
            addLogEntry(data->log, LogEntryCategory_Error, "couldn't exec %*s", LIT(filename));
        }
    } else {
		addLogEntry(data->log, LogEntryCategory_Ok, "exec <filename> : execute a script file");
    }
}

//
// SECTION Init
//

static void gameInit(Arena* arena, Platform* platform) {
    Log* log = arenaAllocAndZeroArray(arena, Log, 1);
    *log = createLog(arena, 2, 1024, 8 * Kilobyte);

    CommandData* cmdData = arenaAllocAndZeroArray(arena, CommandData, 1);
    *cmdData = createCommandData(arena, 1024, 1024, log, platform);

	addCommand(cmdData, cmdlist);
	addCommand(cmdData, cmdexec);
	// addCommand(STR("echo"), Cmd_Echo_f);
	// addCommand(STR("alias"), Cmd_Alias_f);
	// addCommand(STR("wait"), Cmd_Wait_f);
// 	Cvar_Init ();

// 	Key_Init ();

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

