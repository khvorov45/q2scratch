#include "game.c"

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

static ReadResult readEntireFile(Arena* arena, Str filepath) {
	ReadResult result = {.status = Status_Error};

    HANDLE hfile = INVALID_HANDLE_VALUE;
    tempMemoryBlock(arena) {
        Str path0 = strfmt(arena, "%.*s", LIT(filepath));
        hfile = CreateFileA(
            path0.ptr,
            GENERIC_READ,
            FILE_SHARE_READ,
            0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            0
        );
    }

	if (hfile != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER fileSize = {};
		BOOL GetFileSizeExResult = GetFileSizeEx(hfile, &fileSize);
		if (GetFileSizeExResult) {
			TempMemory temp = beginTempMemory(arena);
			void* fileContent = arenaAllocArray(arena, u8, fileSize.QuadPart);
			DWORD bytesRead = 0;
			BOOL ReadFileResult = ReadFile(hfile, fileContent, fileSize.QuadPart, &bytesRead, 0);
			if (ReadFileResult && bytesRead == fileSize.QuadPart) {
				result.status = Status_Ok;
				result.file = (u8slice) {fileContent, fileSize.QuadPart};
				keepTempMemory(&temp);
			} else {
				endTempMemory(&temp);
			}
		}
	}

	CloseHandle(hfile);
	return result;
}

static Status writeEntireFile(Arena* arena, Str path, void* ptr, i64 len) {
	Status status = Status_Error;

    HANDLE hfile = INVALID_HANDLE_VALUE;
    tempMemoryBlock(arena) {
        Str path0 = strfmt(arena, "%.*s", LIT(path));
        hfile = CreateFileA(
            path0.ptr,
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            0,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            0
        );
    }

	if (hfile != INVALID_HANDLE_VALUE) {
		DWORD bytesWritten = 0;
		BOOL WriteFileResult = WriteFile(hfile, ptr, len, &bytesWritten, 0);
		if (WriteFileResult && bytesWritten == len) {
			status = Status_Ok;
		}
	}

    CloseHandle(hfile);
	return status;
}

static Status deleteFile(Arena* arena, Str path) {
	Status status = Status_Error;
    tempMemoryBlock(arena) {
        Str path0 = strfmt(arena, "%.*s", LIT(path));
		BOOL deleteFileResult = DeleteFileA(path0.ptr);
		if (deleteFileResult != 0) {
			status = Status_Ok;
		}
    }
	return status;
}

static void allTests_(Arena* arena, Platform* platform) {tempMemoryBlock(arena) {
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

	{
		Log log = createLog(arena, 2, 2, 8 * Kilobyte);

		addLogEntry(&log, LogEntryCategory_Ok, "test");
		assert(streq(log.circle.ptr[0].entries.ptr[0].str, STR("test")));

		addLogEntry(&log, LogEntryCategory_Ok, "test %i", 123);
		assert(streq(log.circle.ptr[0].entries.ptr[0].str, STR("test")));
		assert(streq(log.circle.ptr[0].entries.ptr[1].str, STR("test 123")));

		assert(log.circle.ptr[0].entries.ptr[0].time < log.circle.ptr[0].entries.ptr[1].time);
		assert(log.circle.ptr[0].entries.len == 2);

		addLogEntry(&log, LogEntryCategory_Ok, "second circle");
		assert(streq(log.circle.ptr[0].entries.ptr[0].str, STR("test")));
		assert(streq(log.circle.ptr[0].entries.ptr[1].str, STR("test 123")));
		assert(streq(log.circle.ptr[1].entries.ptr[0].str, STR("second circle")));

		addLogEntry(&log, LogEntryCategory_Ok, "second circle 2");
		assert(streq(log.circle.ptr[0].entries.ptr[0].str, STR("test")));
		assert(streq(log.circle.ptr[0].entries.ptr[1].str, STR("test 123")));
		assert(streq(log.circle.ptr[1].entries.ptr[0].str, STR("second circle")));
		assert(streq(log.circle.ptr[1].entries.ptr[1].str, STR("second circle 2")));

		addLogEntry(&log, LogEntryCategory_Ok, "back to first circle");
		assert(streq(log.circle.ptr[0].entries.ptr[0].str, STR("back to first circle")));
		assert(streq(log.circle.ptr[1].entries.ptr[0].str, STR("second circle")));
		assert(streq(log.circle.ptr[1].entries.ptr[1].str, STR("second circle 2")));

		assert(log.circle.ptr[0].entries.len == 1);
		assert(log.circle.ptr[1].entries.len == 2);
	}

	{
		Log log = createLog(arena, 3, 3, 8 * Kilobyte);

		addLogEntry(&log, LogEntryCategory_Ok, "entry1");
		addLogEntry(&log, LogEntryCategory_Ok, "entry2");
		addLogEntry(&log, LogEntryCategory_Ok, "entry3");
		addLogEntry(&log, LogEntryCategory_Ok, "entry4");
		addLogEntry(&log, LogEntryCategory_Ok, "entry5");
		addLogEntry(&log, LogEntryCategory_Ok, "entry6");
		addLogEntry(&log, LogEntryCategory_Ok, "entry7");
		addLogEntry(&log, LogEntryCategory_Ok, "entry8");

		LogChoronologicalIter iter = chronologicalIter(&log);
		assert(streq(currentEntry(&iter)->str, STR("entry1")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry2")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry3")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry4")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry5")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry6")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry7")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry8")));
		assert(iter.ended);

		// NOTE: Should not crash
		chronologicalIterNext(&iter);
		chronologicalIterNext(&iter);

		addLogEntry(&log, LogEntryCategory_Ok, "entry9");
		addLogEntry(&log, LogEntryCategory_Ok, "entry10");
		iter = chronologicalIter(&log);
		assert(streq(currentEntry(&iter)->str, STR("entry4")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry5")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry6")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry7")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry8")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry9")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("entry10")));
		assert(iter.ended);
	}

	{
		Log log = createLog(arena, 1, 1024, 8 * Kilobyte);
		LogEntries* logents = &log.circle.ptr[0].entries;

		CommandData cmdData = createCommandData(arena, 2, 1024, &log, platform);

		assert(logents->len == 0);
		cmdlist(&cmdData);
		assert(logents->len == 1);

		LogChoronologicalIter iter = chronologicalIter(&log);
		assert(streq(currentEntry(&iter)->str, STR("0 commands")));

		addCommand(&cmdData, cmdlist);
		assert(cmdData.cmds.len == 1);
		assert(streq(cmdData.cmds.ptr[0].name, STR("cmdlist")));
		cmdlist(&cmdData);

		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("cmdlist")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("1 commands")));

		iter.ended = false;
		addCommand(&cmdData, cmdlist);
		assert(streq(currentEntry(&iter)->str, STR("1 commands")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("addCommand: cmdlist already defined")));

		iter.ended = false;
		addCommand(&cmdData, cmdlist);
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("addCommand: cmdlist already defined")));

		CommandVar cmdVar = {.name = STR("cmdexec")};
		dynarrpush(&cmdData.vars, cmdVar);

		iter.ended = false;
		addCommand(&cmdData, cmdexec);
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("addCommand: cmdexec already defined as a var")));
		cmdData.vars.len = 0;

		i64 logentLenBefore = logents->len;
		addCommand(&cmdData, cmdexec);
		assert(logentLenBefore == logents->len);

		assert(cmdData.cmds.len == 2);
		assert(streq(cmdData.cmds.ptr[1].name, STR("cmdexec")));

		iter.ended = false;
		addCommand(&cmdData, cmdexec);
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("addCommand: cmdexec could not be added, buffer full")));

		cmdlist(&cmdData);
		iter.ended = false;
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("cmdlist")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("cmdexec")));
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("2 commands")));

		cmdexec(&cmdData);
		iter.ended = false;
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("exec <filename> : execute a script file")));

		cmdData.args.len = 2;
		cmdData.args.ptr = arenaAllocAndZeroArray(arena, Str, cmdData.args.len);
		cmdData.args.ptr[0] = STR("cmdexec");

		Str tempfile = STR("temp_file_for_testing.txt");
		assert(writeEntireFile(arena, tempfile, "temp", 4) == Status_Ok);

		cmdData.args.ptr[1] = tempfile;
		cmdexec(&cmdData);
		iter.ended = false;
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, strfmt(arena, "execing %*s", LIT(tempfile))));

		assert(deleteFile(arena, tempfile) == Status_Ok);

		cmdexec(&cmdData);
		iter.ended = false;
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, strfmt(arena, "couldn't exec %*s", LIT(tempfile))));

		cmdData.args.len = 4;
		cmdData.args.ptr = arenaAllocAndZeroArray(arena, Str, cmdData.args.len);
		cmdData.args.ptr[0] = STR("echo");
		cmdData.args.ptr[1] = STR("arg1");
		cmdData.args.ptr[2] = STR("arg2");
		cmdData.args.ptr[3] = STR("arg3");

		cmdecho(&cmdData);
		iter.ended = false;
		chronologicalIterNext(&iter);
		assert(streq(currentEntry(&iter)->str, STR("arg1 arg2 arg3 ")));
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

	Platform platform = {.readEntireFile = readEntireFile};

	allTests_(arena, &platform);

	Strslice cmdLineArguments = parseCommandLine(arena, (Str) {lpCmdLine, strlen(lpCmdLine)});
	unused(cmdLineArguments);

	gameInit(arena, &platform);
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
