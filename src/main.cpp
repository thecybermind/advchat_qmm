/*
ADVCHAT_QMM - Advanced Chat Plugin
Copyright 2004-2025
https://github.com/thecybermind/advchat_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#define _CRT_SECURE_NO_WARNINGS 1

#include <qmmapi.h>

#include "version.h"
#include "game.h"

#include <string>
#include <string.h>

pluginres_t* g_result = nullptr;
plugininfo_t g_plugininfo = {
	QMM_PIFV_MAJOR,									// plugin interface version major
	QMM_PIFV_MINOR,									// plugin interface version minor
	"AdvChat",										// name of plugin
	ADVCHAT_QMM_VERSION,							// version of plugin
	"Advanced Chat Features",						// description of plugin
	ADVCHAT_QMM_BUILDER,							// author of plugin
	"https://github.com/thecybermind/qadmin_qmm",	// website of plugin
	"ADVCHAT",										// logtag of plugin
};
eng_syscall_t g_syscall = nullptr;
mod_vmMain_t g_vmMain = nullptr;
pluginfuncs_t* g_pluginfuncs = nullptr;
pluginvars_t* g_pluginvars = nullptr;

gentity_t* g_gents = nullptr;
intptr_t g_gentsize = 0;

intptr_t g_sayflag = 0;
intptr_t g_sayclient = 0;


// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, std::size_t count) {
	char* ret = strncpy(dest, src, count);
	dest[count - 1] = '\0';
	return ret;
}


C_DLLEXPORT void QMM_Query(plugininfo_t** pinfo) {
	QMM_GIVE_PINFO();
}


C_DLLEXPORT int QMM_Attach(eng_syscall_t engfunc, mod_vmMain_t modfunc, pluginres_t* presult, pluginfuncs_t* pluginfuncs, pluginvars_t* pluginvars) {
	QMM_SAVE_VARS();

	return 1;
}


C_DLLEXPORT void QMM_Detach() {
}


C_DLLEXPORT intptr_t QMM_vmMain(intptr_t cmd, intptr_t* args) {
	// cancel arg checking if a new command is entered before this plugin thinks it
	// has all the "say" args (sanity checking)
	if ((cmd == GAME_CONSOLE_COMMAND || cmd == GAME_CLIENT_COMMAND)) {
		g_sayflag = 0;
	}

	// hook client console commands
	if (cmd == GAME_CLIENT_COMMAND) {
		// check what command it is
		char buf[16];
		QMM_ARGV(PLID, 0, buf, sizeof(buf));

		// player is using a say command, set global flags
		if (!strncmp(buf, "say", 3)) {
			g_sayflag = g_syscall(G_ARGC);
			g_sayclient = args[0];
		}
	}

	QMM_RET_IGNORED(1);
}


C_DLLEXPORT intptr_t QMM_syscall(intptr_t cmd, intptr_t* args) {
	// store player/entity information
	if (cmd == G_LOCATE_GAME_DATA) {
		g_gents = (gentity_t*)args[0];
		g_gentsize = args[2];
	}

	// this is the main workhorse
	// whenever the mod calls trap_Argv for a say command, this checks each argument for the
	// existence of a replacable token (like "$h") and then replaces all instances of them
	if (cmd == G_ARGV) {
		// if this is an argument for a say command
		if (g_sayflag) {
			intptr_t argnum = args[0];
#ifdef GAME_ARGV_RETURN
			// this engine returns a buffer to the mod, so we have to use our own
			static char buffers[MAX_INFO_STRING][8];
			static int index = 0;
			// cycle rotating buffer and clear string
			index = (index + 1) % 8;
			char* buf = buffers[index];
			intptr_t buflen = MAX_INFO_STRING;
			memset(buf, 0, buflen);
#else
			// this engine uses a mod-given buffer/size
			char* buf = (char*)args[1];
			intptr_t buflen = args[2];
#endif
			QMM_ARGV(PLID, argnum, buf, buflen);

#ifdef GAME_HAS_STAT_HEALTH
			int health = ENT_FROM_NUM(g_sayclient)->client->ps.stats[STAT_HEALTH];
#else
			int health = ENT_FROM_NUM(g_sayclient)->health;
#endif
			// check for "$h" tokens
			char* replace = strstr(buf, "$h");
			// this argument contains at least one "$h" token
			while (replace) {
				// stop 'buf' from being read past the token
				*replace = '\0';

				// form new string with the token replaced
				strncpyz(buf, QMM_VARARGS(PLID, "%s%d%s", buf, health, replace + 2), buflen);

				// look for another token
				replace = strstr(buf, "$h");
			}

// skip for games that don't have armor
#ifndef GAME_NO_ARMOR
			int armor = ENT_FROM_NUM(g_sayclient)->client->ps.stats[STAT_ARMOR];
			replace = strstr(buf, "$a");
			while (replace) {
				// stop 'buf' from being read past the token
				*replace = '\0';

				// form new string with the token replaced
				strncpyz(buf, QMM_VARARGS(PLID, "%s%d%s", buf, armor, replace + 2), buflen);

				// look for another token
				replace = strstr(buf, "$a");
			}
#endif

			// stop checking future args if this is the last arg
			if (argnum == g_sayflag - 1)
				g_sayflag = 0;

			// NEVER let this go through for say command args
			// if this game engine returns a buffer, we need to return ours
			// otherwise, return value doesn't matter
			QMM_RET_SUPERCEDE((intptr_t)buf);
		}
	}

	QMM_RET_IGNORED(1);
}


C_DLLEXPORT intptr_t QMM_vmMain_Post(intptr_t cmd, intptr_t* args) {
	QMM_RET_IGNORED(1);
}


C_DLLEXPORT intptr_t QMM_syscall_Post(intptr_t cmd, intptr_t* args) {
	QMM_RET_IGNORED(1);
}
