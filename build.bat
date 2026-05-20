clang ^
    win32/sys_win.c win32/conproc.c win32/vid_dll.c win32/q_shwin.c win32/in_win.c ^
    client/cl_main.c client/keys.c ^
    qcommon/common.c qcommon/files.c ^
    game/q_shared.c ^
    -DC_ONLY -DWIN32 ^
    -m32 -ferror-limit=1 ^
    -Wl,/errorlimit:1 -luser32 -lwinmm ^
    -Wno-deprecated-declarations -Wno-incompatible-pointer-types -Wno-pointer-sign -Wno-parentheses -Wno-return-type ^
    -o build/quake2.exe