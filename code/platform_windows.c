#include "common.c"

#include "windows.h"

#pragma comment(lib, "user32")

static bool charIsPrintable(char ch) {
	char firstPrintable = ' ';
	char lastPrintable = '~';
	bool result = ch >= firstPrintable && ch <= lastPrintable;
	return result;
}

typedef struct CStrIter {
	char* ptr;
	char* curptr;
	bool ended;
} CStrIter;

static CStrIter cstriter(char* str) {
	CStrIter iter = {.ptr = str, .curptr = str, .ended = !(str && *str)};
	return iter;
}

static void cstriterAdvanceUntil(CStrIter* iter, char ch) {
	if (!iter->ended) {
		while (*iter->curptr && *iter->curptr != ch) {
			iter->curptr++;
		}
	}
	iter->ended = *iter->curptr == '\0';
}

static void cstriterAdvancePast(CStrIter* iter, char ch) {
	if (!iter->ended) {
		while (*iter->curptr == ch) {
			iter->curptr++;
		}
	}
	iter->ended = *iter->curptr == '\0';
}

static Strarr parseCommandLine(Arena* arena, char* lpCmdLine) {	
	Strarr cmdLineArguments = {.len = 1}; // NOTE: the first one is the executable

	CStrIter iterCopy = {};
	{
		CStrIter iter = cstriter(lpCmdLine);
		cstriterAdvancePast(&iter, ' '); // NOTE(khvorov) Leading spaces
		iterCopy = iter;
	
		// NOTE(khvorov) Count
		for (;!iter.ended;) {
			cstriterAdvanceUntil(&iter, ' ');
			cstriterAdvancePast(&iter, ' ');
			cmdLineArguments.len++;
		}
	}

	// NOTE(khvorov) Allocate array
	cmdLineArguments.ptr = arenaAllocAndZeroArray(arena, Str, cmdLineArguments.len);
	cmdLineArguments.ptr[0] = STR("exe");
	i64 curArgIndex = 1;
	for (;!iterCopy.ended;) {
		cstriterAdvancePast(&iterCopy, ' ');
		char* argStart = iterCopy.curptr;
		cstriterAdvanceUntil(&iterCopy, ' ');
		cmdLineArguments.ptr[curArgIndex++] = (Str) {.ptr = argStart, .len = iterCopy.curptr - argStart};
		cstriterAdvancePast(&iterCopy, ' ');
	}

	return cmdLineArguments;
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
		Strarr arr1 = {.ptr = rawarr1, .len = arrayCount(rawarr1)};
		Strarr arr2 = {.ptr = rawarr2, .len = arrayCount(rawarr2)};
		Strarr arr3 = {.ptr = rawarr3, .len = arrayCount(rawarr3)};
		Strarr arr4 = {.ptr = rawarr4, .len = arrayCount(rawarr4)};
		assert(strarreq(arr1, arr2));
		assert(!strarreq(arr1, arr3));
		assert(!strarreq(arr1, arr4));
	}

	{
		Str cmdLine = STR("  test cmd   line  ");
		Str arg0 = STR("exe");	
		Str arg1 = STR("test");	
		Str arg2 = STR("cmd");	
		Str arg3 = STR("line");	
		Str rawexpected[4] = {arg0, arg1, arg2, arg3};
		Strarr expected = {.ptr = rawexpected, .len = arrayCount(rawexpected)};
		Strarr result = parseCommandLine(arena, cmdLine.ptr);
		assert(strarreq(result, expected));
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

	Strarr cmdLineArguments = parseCommandLine(arena, lpCmdLine);
	unused(cmdLineArguments);

    // TODO(khvorov)
	// NOTE if we find the CD, add a +set cddir xxx command line
	// char* cddir = Sys_ScanForCD ();
	// if (cddir && argc < MAX_NUM_ARGVS - 3)
	// {
	// 	int		i;

	// 	// don't override a cddir on the command line
	// 	for (i=0 ; i<argc ; i++)
	// 		if (!strcmp(argv[i], "cddir"))
	// 			break;
	// 	if (i == argc)
	// 	{
	// 		argv[argc++] = "+set";
	// 		argv[argc++] = "cddir";
	// 		argv[argc++] = cddir;
	// 	}
	// }

    // TODO(khvorov)
	// Qcommon_Init (argc, argv);
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
