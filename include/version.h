/*
ADVCHAT_QMM - Advanced Chat Plugin
Copyright 2004-2025
https://github.com/thecybermind/advchat_qmm/
3-clause BSD license: https://opensource.org/license/bsd-3-clause

Created By:
    Kevin Masterson < k.m.masterson@gmail.com >

*/

#ifndef __ADVCHAT_QMM_VERSION_H__
#define __ADVCHAT_QMM_VERSION_H__

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

#define ADVCHAT_QMM_VERSION_MAJOR	2
#define ADVCHAT_QMM_VERSION_MINOR	4
#define ADVCHAT_QMM_VERSION_REV	    2

#define ADVCHAT_QMM_VERSION		STRINGIFY(ADVCHAT_QMM_VERSION_MAJOR) "." STRINGIFY(ADVCHAT_QMM_VERSION_MINOR) "." STRINGIFY(ADVCHAT_QMM_VERSION_REV)

#if defined(_WIN32)
#define ADVCHAT_QMM_OS             "Windows"
#ifdef _WIN64
#define ADVCHAT_QMM_ARCH           "x86_64"
#else
#define ADVCHAT_QMM_ARCH           "x86"
#endif
#elif defined(__linux__)
#define ADVCHAT_QMM_OS             "Linux"
#ifdef __LP64__
#define ADVCHAT_QMM_ARCH           "x86_64"
#else
#define ADVCHAT_QMM_ARCH           "x86"
#endif
#endif

#define ADVCHAT_QMM_VERSION_DWORD	ADVCHAT_QMM_VERSION_MAJOR , ADVCHAT_QMM_VERSION_MINOR , ADVCHAT_QMM_VERSION_REV , 0
#define ADVCHAT_QMM_COMPILE		__TIME__ " " __DATE__
#define ADVCHAT_QMM_BUILDER		"Kevin Masterson"

#endif // __ADVCHAT_QMM_VERSION_H__
