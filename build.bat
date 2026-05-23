@REM clang ^
@REM     win32/sys_win.c win32/conproc.c win32/vid_dll.c win32/q_shwin.c win32/in_win.c win32/cd_win.c win32/snd_win.c win32/vid_menu.c win32/net_wins.c ^
@REM     client/cl_main.c client/keys.c client/cl_scrn.c client/cl_ents.c client/snd_dma.c client/console.c client/cl_input.c client/qmenu.c client/menu.c client/cl_cin.c client/cl_fx.c client/cl_tent.c client/cl_parse.c client/cl_view.c client/cl_pred.c client/cl_inv.c client/cl_newfx.c client/snd_mix.c client/snd_mem.c ^
@REM     server/sv_main.c server/sv_game.c server/sv_init.c server/sv_send.c server/sv_user.c server/sv_ents.c server/sv_ccmds.c server/sv_world.c ^
@REM     qcommon/common.c qcommon/files.c qcommon/cvar.c qcommon/cmd.c qcommon/net_chan.c qcommon/cmodel.c qcommon/pmove.c qcommon/crc.c qcommon/md4.c ^
@REM     game/q_shared.c game/m_flash.c ^
@REM     -DC_ONLY -DWIN32 ^
@REM     -m32 -ferror-limit=1 ^
@REM     -Wl,/errorlimit:1 -luser32 -lwinmm -lws2_32 ^
@REM     -Wno-deprecated-declarations -Wno-incompatible-pointer-types -Wno-pointer-sign -Wno-parentheses -Wno-return-type -Wno-absolute-value -Wno-switch ^
@REM     -g -o build/quake2.exe

clang code/platform_windows.c -std=c2x -Wall -Wextra -Werror -ferror-limit=1 -Wno-unused-function -Wno-unused-parameter -g -o build/platform_windows.exe