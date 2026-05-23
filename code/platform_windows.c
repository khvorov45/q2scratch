#include "common.c"

#include <windows.h>

#pragma comment(lib, "user32")

static Strslice parseCommandLine(Arena* arena, Str cmdline) {	
	Strslice cmdLineArguments = {.len = 1}; // NOTE: the first one is the executable

	StrIter iterNoLeadingWhitespace = {};
	{
		StrIter iter = striter(cmdline);
		striterAdvancePastWhitespace(&iter);
		iterNoLeadingWhitespace = iter;
	
		// NOTE(khvorov) Count
		for (;!striterEnded(&iter);) {
			striterAdvanceUntilWhitespace(&iter);
			striterAdvancePastWhitespace(&iter);
			cmdLineArguments.len++;
		}
	}

	// NOTE(khvorov) Allocate array
	cmdLineArguments.ptr = arenaAllocAndZeroArray(arena, Str, cmdLineArguments.len);
	cmdLineArguments.ptr[0] = STR("exe");
	i64 curArgIndex = 1;
	for (;!striterEnded(&iterNoLeadingWhitespace);) {
		striterAdvancePastWhitespace(&iterNoLeadingWhitespace);
		char* argStart = iterNoLeadingWhitespace.cur.ptr;
		striterAdvanceUntilWhitespace(&iterNoLeadingWhitespace);
		cmdLineArguments.ptr[curArgIndex++] = (Str) {.ptr = argStart, .len = iterNoLeadingWhitespace.cur.ptr - argStart};
		striterAdvancePastWhitespace(&iterNoLeadingWhitespace);
	}

	return cmdLineArguments;
}

void ShowErrorMsgBoxAndExit(Str errorMsg) {
	i64 textBufSize = 1024;
	char text[textBufSize];
	assert(errorMsg.len - 1 < textBufSize);
	memcpy(text, errorMsg.ptr, errorMsg.len);
	text[errorMsg.len] = '\0';
	MessageBox(NULL, text, "Error", MB_OK);
	ExitProcess(1);
}

typedef void (*CommandProc)(void*);

typedef struct CommandProcEntry {
	CommandProc proc;
	Str name;
	struct CommandProcEntry* next;	
} CommandProcEntry;

typedef struct CommandProcList {
	CommandProcEntry sentinel;
	CommandProcEntry* freelist;
} CommandProcList;

#define addCommand(name) addCommand_(STR(STRINGIFY(name)), (CommandProc)name);
void addCommand_(Str cmdname, CommandProc function) {
// 	cmd_function_t	*cmd;
	
// // fail if the command is a variable name
// 	if (Cvar_VariableString(cmd_name)[0])
// 	{
// 		Com_Printf ("addCommand: %s already defined as a var\n", cmd_name);
// 		return;
// 	}
	
// // fail if the command already exists
// 	for (cmd=cmd_functions ; cmd ; cmd=cmd->next)
// 	{
// 		if (!strcmp (cmd_name, cmd->name))
// 		{
// 			Com_Printf ("addCommand: %s already defined\n", cmd_name);
// 			return;
// 		}
// 	}

// 	cmd = Z_Malloc (sizeof(cmd_function_t));
// 	cmd->name = cmd_name;
// 	cmd->function = function;
// 	cmd->next = cmd_functions;
// 	cmd_functions = cmd;
}

void cmdlist(CommandProcList* cmds) {
	i64 index = 0;
	for (CommandProcEntry* cmd = cmds->sentinel.next; cmd; cmd=cmd->next, index++) {
		// Com_Printf ("%s\n", cmd->name);
	}
	// Com_Printf ("%i commands\n", i);
}

void commonInit(Arena* arena, Strslice cmdArgs) {
	unused(cmdArgs);
	
	// NOTE: prepare enough of the subsystems to handle cvar and command buffer management
	i64 cmdArenaSize = 8 * Kilobyte;
	Arena cmdArena = {.base = arenaAllocAndZero(arena, cmdArenaSize), .size = cmdArenaSize};
	unused(cmdArena);

	addCommand(cmdlist);
	// addCommand(STR("exec"), Cmd_Exec_f);
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

static void allTests_(Arena* arena) {tempMemoryBlock(arena) {
	{
		Str str1 = STR("test str 1");
		Str str2 = STR("test str 2");
		Str str3 = STR("test str 3");
		Str rawarr1[2] = {str1, str2};
		Str rawarr2[2] = {str1, str2};
		Str rawarr3[2] = {str1, str3};
		Str rawarr4[3] = {str1, str2, str3};
		Strslice arr1 = slicefromcarray(rawarr1);
		Strslice arr2 = slicefromcarray(rawarr2);
		Strslice arr3 = slicefromcarray(rawarr3);
		Strslice arr4 = slicefromcarray(rawarr4);
		assert(strsliceeq(arr1, arr2));
		assert(!strsliceeq(arr1, arr3));
		assert(!strsliceeq(arr1, arr4));
	}

	{
		Str cmdLine = STR("  test cmd   line  ");
		Str arg0 = STR("exe");	
		Str arg1 = STR("test");	
		Str arg2 = STR("cmd");	
		Str arg3 = STR("line");	
		Str rawexpected[4] = {arg0, arg1, arg2, arg3};
		Strslice expected = slicefromcarray(rawexpected);
		Strslice result = parseCommandLine(arena, cmdLine);
		assert(strsliceeq(result, expected));
	}
}}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unused(hInstance);
    unused(hPrevInstance);
    unused(nCmdShow);

    Arena arena_ = {.size = Gigabyte};
    arena_.base = VirtualAlloc(0, arena_.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(arena_.base);
    Arena* arena = &arena_;

	allTests_(arena);

	Strslice cmdLineArguments = parseCommandLine(arena, (Str) {lpCmdLine, strlen(lpCmdLine)});

	commonInit(arena, cmdLineArguments);
	// int oldtime = Sys_Milliseconds ();

    // NOTE main window message loop
	for (;;) {
		// NOTE if at a full screen console, don't update unless needed
        // TODO(khvorov)
		// if (Minimized || (dedicated && dedicated->value)) {
		// 	Sleep (1);
		// }

        for (MSG msg; PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // TODO(khvorov)
        // int time = 0;
		// while (time < 1) {
		// 	int newtime = Sys_Milliseconds();
		// 	time = newtime - oldtime;
		// }

        // TODO(khvorov)
        // Con_Printf ("time:%5.2f - %5.2f = %5.2f\n", newtime, oldtime, time);

        // TODO(khvorov)
		// Qcommon_Frame (time);

		// oldtime = newtime;
	}

	// NOTE never gets here
    return TRUE;
}
