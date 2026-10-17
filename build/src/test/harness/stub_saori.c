/*
 * stub_saori - a harmless SAORI/1.0 module for tests.
 *
 * Copy it over the SAORI of a real ghost (exec.dll, process.dll, bln.dll ...) so that the
 * ghost can be driven without opening windows or starting programs. Every request is
 * answered with "SAORI/1.0 200 OK" and "Result: 0" (so, for example, "process exist <exe>"
 * means "not running"). By default no Charset header is sent back, like the SAORI of the
 * old days (the caller then keeps using Shift_JIS). If the environment variable
 * KAWARI_STUB_CHARSET is "echo", the Charset of the request is echoed back, like a SAORI
 * that understands UTF-8.
 *
 * Every request is appended to "<this dll>.log" as raw bytes followed by "\n-----\n", so a
 * test can compare what KAWARI sends to a SAORI (headers, charset, argument encoding)
 * between versions. The log is truncated by load().
 *
 * Build (same bitness as the SHIORI DLL, which is 32bit):
 *   gcc -O1 -Wall -shared -o stub_saori.dll stub_saori.c
 *
 * This file is ASCII only.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char logpath[MAX_PATH + 8];

static void logbytes(const char *p, long n)
{
	FILE *f = fopen(logpath, "ab");
	if (!f) return;
	fwrite(p, 1, n, f);
	fwrite("\n-----\n", 1, 7, f);
	fclose(f);
}

static HMODULE self(void)
{
	HMODULE h = NULL;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&self, &h);
	return h;
}

__declspec(dllexport) BOOL __cdecl load(HGLOBAL h, long len)
{
	FILE *f;
	(void)len;
	GlobalFree(h);
	GetModuleFileNameA(self(), logpath, MAX_PATH);
	strcat(logpath, ".log");
	f = fopen(logpath, "wb");
	if (f) fclose(f);
	return TRUE;
}

__declspec(dllexport) BOOL __cdecl unload(void)
{
	return TRUE;
}

__declspec(dllexport) HGLOBAL __cdecl request(HGLOBAL h, long *len)
{
	char *req = (char *)h;
	char *resp;
	char charset[32] = "Shift_JIS";
	HGLOBAL r;
	long n;
	char *p;

	logbytes(req, *len);
	/* echo the request's Charset (the value is a short token) */
	{
		char *copy = (char *)malloc(*len + 1);
		memcpy(copy, req, *len);
		copy[*len] = '\0';
		p = strstr(copy, "Charset: ");
		if (p) {
			size_t i = 0;
			p += 9;
			while (p[i] && p[i] != '\r' && p[i] != '\n' && i < sizeof(charset) - 1) { charset[i] = p[i]; i++; }
			charset[i] = '\0';
		}
		free(copy);
	}
	resp = (char *)malloc(128);
	{
		const char *mode = getenv("KAWARI_STUB_CHARSET");
		if (mode && strcmp(mode, "echo") == 0)
			n = sprintf(resp, "SAORI/1.0 200 OK\r\nCharset: %s\r\nResult: 0\r\n\r\n", charset);
		else
			n = sprintf(resp, "SAORI/1.0 200 OK\r\nResult: 0\r\n\r\n");
	}
	GlobalFree(h);
	r = GlobalAlloc(GMEM_FIXED, n);
	memcpy(r, resp, n);
	free(resp);
	*len = n;
	return r;
}
