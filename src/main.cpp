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

// which client did the say command
gentity_t* g_sayentity = nullptr;

// rotating buffers to return values for engines where the arg is returned from G_ARGV instead of in a passed buffer
#define NUM_BUFFERS 8
static char buffers[NUM_BUFFERS][MAX_INFO_STRING];
static int bufindex = 0;


// "safe" strncpy that always null-terminates
char* strncpyz(char* dest, const char* src, std::size_t count) {
	char* ret = strncpy(dest, src, count);
	dest[count - 1] = '\0';
	return ret;
}


// scan buf for var, replace with int value
int replace_var_int(char* buf, intptr_t buflen, const char* var, intptr_t value) {
	int total = 0;
	int varlen = strlen(var);

	// check for var token
	char* replace = strstr(buf, var);
	// this argument contains at least one token
	while (replace) {
		// stop 'buf' from being read past the token
		*replace = '\0';

		// form new string with the token replaced
		strncpyz(buf, QMM_VARARGS(PLID, "%s%d%s", buf, value, replace + varlen), buflen);
		total++;

		// look for another token
		replace = strstr(buf, var);
	}
	
	return total;
}


// scan buf for var, replace with string value
int replace_var_str(char* buf, intptr_t buflen, const char* var, const char* value) {
	int total = 0;
	int varlen = strlen(var);

	// check for var token
	char* replace = strstr(buf, var);
	// this argument contains at least one token
	while (replace) {
		// stop 'buf' from being read past the token
		*replace = '\0';

		// form new string with the token replaced
		strncpyz(buf, QMM_VARARGS(PLID, "%s%s%s", buf, value, replace + varlen), buflen);
		total++;

		// look for another token
		replace = strstr(buf, var);
	}
	
	return total;
}


int get_player_health() {
#ifdef GAME_HAS_STAT_HEALTH
	gclient_t* client = g_sayentity->client;
	// if QVM, we need to get a real client pointer
	client = GETPTR(client, gclient_t*);
	return client ? client->ps.stats[STAT_HEALTH] : 0;
#else
	return g_sayentity->health;
#endif
}


#ifndef GAME_NO_ARMOR
int get_player_armor() {
	gclient_t* client = g_sayentity->client;
	// if QVM, we need to get a real client pointer
	client = GETPTR(client, gclient_t*);
	return client ? client->ps.stats[STAT_ARMOR] : 0;
}
#endif


// single point to do all variable replacements
int replace_vars(char* buf, intptr_t buflen) {
	int replaced = 0;

	// check for "$h" tokens, replace with health
	replaced += replace_var_int(buf, buflen, "$h", get_player_health());

#ifndef GAME_NO_ARMOR
	// check for "$a" tokens, replace with armor
	replaced += replace_var_int(buf, buflen, "$a", get_player_armor());
#endif

	return replaced;
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
	// cancel arg checking if a new command is entered
	if ((cmd == GAME_CONSOLE_COMMAND || cmd == GAME_CLIENT_COMMAND)) {
		g_sayentity = nullptr;
	}

	// hook client console commands
	if (cmd == GAME_CLIENT_COMMAND) {
		// check what command it is
		char buf[16];
		QMM_ARGV(PLID, 0, buf, sizeof(buf));

		// player is using a say command, set global flag ("dmmessage" is MOHAA)
		if (!strncmp(buf, "say", 3) || !strncmp(buf, "dmmessage", 9)) {
			intptr_t clientNum = args[0];
			// some engines (quake 2-based games mostly) pass entity pointer to GAME_CLIENT_ messages instead of a number
			if (clientNum > MAX_CLIENTS)
				g_sayentity = (gentity_t*)clientNum;
			else
				g_sayentity = ENT_FROM_NUM(clientNum);
		}
	}

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_syscall(intptr_t cmd, intptr_t* args) {
	// store player/entity information
	if (cmd == G_LOCATE_GAME_DATA) {
		g_gents = (gentity_t*)args[0];
		g_gentsize = args[2];
	}

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_vmMain_Post(intptr_t cmd, intptr_t* args) {
	// cancel arg checking once the mod is done with command-handling
	if ((cmd == GAME_CONSOLE_COMMAND || cmd == GAME_CLIENT_COMMAND)) {
		g_sayentity = nullptr;
	}

	QMM_RET_IGNORED(0);
}


C_DLLEXPORT intptr_t QMM_syscall_Post(intptr_t cmd, intptr_t* args) {
#ifdef GAME_ARGV_RETURN
	// this is the main workhorse for games where G_ARGV returns a buffer
	// whenever the mod calls G_ARGV for a say command, this checks each argument for the
	// existence of a replacable token (like "$h") and then replaces all instances of them
	if (cmd == G_ARGV && g_sayentity) {
		intptr_t argnum = args[0];

		// this engine returns a buffer to the mod, so we have to use our own
		// cycle rotating buffer and clear string
		bufindex = (bufindex + 1) % NUM_BUFFERS;
		char* buf = buffers[bufindex];
		intptr_t buflen = sizeof(buffers[bufindex]);
		memset(buf, 0, buflen);

		// read arg into buf regardless of engine
		QMM_ARGV(PLID, argnum, buf, buflen);

		// if we didn't change anything, ignore and let the engine return its own buffer
		if (!replace_vars(buf, buflen))
			QMM_RET_IGNORED(0);

		// we did change something, so we need to return our buffer to the mod
		QMM_RET_OVERRIDE((intptr_t)buf);
	}
#else // GAME_ARGV_RETURN
	// this is the main workhorse for games where G_ARGV fills in a mod-given buffer
	// whenever the mod calls G_ARGV for a say command, this checks each argument for the
	// existence of a replacable token (like "$h") and then replaces all instances of them
	if (cmd == G_ARGV && g_sayentity) {
		intptr_t argnum = args[0];
		char* buf = (char*)args[1];
		intptr_t buflen = args[2];

		replace_vars(buf, buflen);
	}
#endif  // !GAME_ARGV_RETURN

	// whenever the mod calls Args() for a say command, this checks each argument for the
	// existence of a replacable token (like "$h") and then replaces all instances of them
	// Args() which returns all command arguments after arg 0
	else if (cmd == G_ARGS && g_sayentity) {
		// G_ARGS returns a buffer to the mod, so we have to use our own
		// cycle rotating buffer and clear string
		bufindex = (bufindex + 1) % 8;
		char* buf = buffers[bufindex];
		intptr_t buflen = sizeof(buffers[bufindex]);
		memset(buf, 0, buflen);

		// read args into buf
		strncpyz(buf, (char*)g_syscall(G_ARGS), buflen);

		// if we didn't change anything, ignore and let the engine return a buffer
		if (!replace_vars(buf, buflen))
			QMM_RET_IGNORED(0);

		// return our modified string
		QMM_RET_OVERRIDE((intptr_t)buf);
	}

	QMM_RET_IGNORED(0);
}
