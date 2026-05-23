#include "common.c"

#include <windows.h>

#pragma comment(lib, "user32")

//
// SECTION Misc
//

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

static void ShowErrorMsgBoxAndExit(Str errorMsg) {
	i64 textBufSize = 1024;
	char text[textBufSize];
	assert(errorMsg.len - 1 < textBufSize);
	memcpy(text, errorMsg.ptr, errorMsg.len);
	text[errorMsg.len] = '\0';
	MessageBox(NULL, text, "Error", MB_OK);
	ExitProcess(1);
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
		Str cmdLine2 = STR("test cmd   line");
		Str arg0 = STR("exe");	
		Str arg1 = STR("test");	
		Str arg2 = STR("cmd");	
		Str arg3 = STR("line");	
		Str rawexpected[4] = {arg0, arg1, arg2, arg3};
		Strslice expected = slicefromcarray(rawexpected);
		Strslice result = parseCommandLine(arena, cmdLine);
		assert(strsliceeq(result, expected));
		Strslice result2 = parseCommandLine(arena, cmdLine2);
		assert(strsliceeq(result2, expected));
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
	unused(cmdLineArguments);

	commonInit(arena);
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
