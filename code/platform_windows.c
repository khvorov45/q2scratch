#include "common.c"

#include "windows.h"

#pragma comment(lib, "user32")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    unused(hInstance);
    unused(hPrevInstance);
    unused(lpCmdLine);
    unused(nCmdShow);

    // TODO(khvorov)
	// ParseCommandLine (lpCmdLine);

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
