/*
 * shiori_harness - calls load / request / unload of a SHIORI DLL directly.
 *
 * Usage:
 *   shiori_harness <dll> <ghost dir> <out file> <request file 1> [<request file 2> ...]
 *
 * load() is called once with <ghost dir> (a path separator is appended when
 * it is missing), then every request file is passed to request() as it is
 * (raw bytes, so the file must use CRLF and end with an empty line), and the
 * raw response bytes are appended to <out file>. Responses are separated by
 * "\n=====\n". unload() is called at the end.
 *
 * No character code conversion is done. Build with the same bitness as the
 * DLL (the DLL is 32bit), for example:
 *   gcc -O1 -Wall -o shiori_harness.exe shiori_harness.c
 *
 * This file is ASCII only.
 *
 * Exit code: 0 = done, 1 = bad usage, 2 = LoadLibrary failed,
 *            3 = export not found, 4 = load() returned FALSE,
 *            5 = cannot write <out file>
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef BOOL (__cdecl *LOADF)(HGLOBAL, long);
typedef HGLOBAL (__cdecl *REQF)(HGLOBAL, long *);
typedef BOOL (__cdecl *UNLOADF)(void);

static char *readfile(const char *fn, long *len)
{
	FILE *f = fopen(fn, "rb");
	char *buf;
	size_t n;
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = (char *)malloc(*len + 1);
	if (!buf) { fclose(f); return NULL; }
	n = fread(buf, 1, *len, f);
	fclose(f);
	*len = (long)n;
	return buf;
}

int main(int argc, char **argv)
{
	HMODULE h;
	LOADF load;
	REQF request;
	UNLOADF unload;
	HGLOBAL g;
	long len;
	int i;
	FILE *out;
	size_t dirlen;

	if (argc < 5) {
		fprintf(stderr, "usage: shiori_harness <dll> <ghost dir> <out file> <request file> [<request file> ...]\n");
		return 1;
	}

	h = LoadLibrary(argv[1]);
	if (!h) {
		fprintf(stderr, "LoadLibrary failed: %s (error %lu; is the DLL the same bitness as this program?)\n",
			argv[1], GetLastError());
		return 2;
	}
	load = (LOADF)GetProcAddress(h, "load");
	request = (REQF)GetProcAddress(h, "request");
	unload = (UNLOADF)GetProcAddress(h, "unload");
	if (!load || !request || !unload) {
		fprintf(stderr, "load / request / unload is not exported by %s\n", argv[1]);
		return 3;
	}

	out = fopen(argv[3], "wb");
	if (!out) {
		fprintf(stderr, "cannot open %s\n", argv[3]);
		return 5;
	}

	/* load() takes ownership of the buffer and frees it with GlobalFree. */
	dirlen = strlen(argv[2]);
	len = (long)dirlen;
	if (dirlen == 0 || (argv[2][dirlen - 1] != '\\' && argv[2][dirlen - 1] != '/')) len++;
	g = GlobalAlloc(GMEM_FIXED, len);
	memcpy(g, argv[2], dirlen);
	if ((size_t)len > dirlen) ((char *)g)[dirlen] = '\\';
	if (!load(g, len)) {
		fprintf(stderr, "load() returned FALSE for %s\n", argv[2]);
		fclose(out);
		return 4;
	}

	for (i = 4; i < argc; i++) {
		char *req = readfile(argv[i], &len);
		HGLOBAL r;
		if (!req) {
			fprintf(stderr, "cannot read %s\n", argv[i]);
			fwrite("\n=====\n", 1, 7, out);
			continue;
		}
		/* request() takes ownership of the request buffer and frees it with GlobalFree. */
		g = GlobalAlloc(GMEM_FIXED, len);
		memcpy(g, req, len);
		free(req);
		r = request(g, &len);
		if (r) {
			fwrite(r, 1, len, out);
			GlobalFree(r);
		}
		fwrite("\n=====\n", 1, 7, out);
	}

	fclose(out);
	unload();
	FreeLibrary(h);
	return 0;
}
