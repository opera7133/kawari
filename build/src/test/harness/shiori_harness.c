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
 * Interactive mode: if <out file> is "-", no request files are given. Requests are read
 * from stdin and responses are written to stdout, both as frames of a 4 byte little endian
 * length followed by that many raw bytes (no "=====" separator). EOF on stdin ends the
 * session, then unload() is called. This lets a driver decide the next request from the
 * previous response (for example to follow the menu links of a ghost).
 *
 * If the environment variable HARNESS_LOADU is set, loadu() is called with the
 * path converted to UTF-8 instead of load(). In that mode the UTF-16 environment
 * variable HARNESS_DIR_W, if set, is used as the folder instead of <ghost dir>.
 * If HARNESS_RELOAD=<n> is set, the module is loaded and unloaded n more times before the
 * requests (one "GET version" request in each round, like SSP does on a ghost change).
 * HARNESS_DIR_W also makes the harness load the DLL as <HARNESS_DIR_W>\<file name of
 * <dll>> with LoadLibraryW, for a folder whose name is outside the ANSI code page.
 *
 * No character code conversion is done for requests. Build with the same bitness as the
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
#include <fcntl.h>
#include <io.h>
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

	int interactive = (argc >= 4 && strcmp(argv[3], "-") == 0);
	int reload = getenv("HARNESS_RELOAD") ? atoi(getenv("HARNESS_RELOAD")) : 0;
	int pass;

	if (argc < (interactive ? 4 : 5)) {
		fprintf(stderr, "usage: shiori_harness <dll> <ghost dir> <out file> <request file> [<request file> ...]\n"
			"       shiori_harness <dll> <ghost dir> -     (interactive, see the top of the source)\n");
		return 1;
	}

	{
		/* HARNESS_DIR_W (UTF-16 environment variable): the DLL is <HARNESS_DIR_W>\<file name of argv[1]>,
		   loaded with the wide API, so that a folder outside the ANSI code page works */
		WCHAR wdir[MAX_PATH * 2 + 2];
		DWORD ew = GetEnvironmentVariableW(L"HARNESS_DIR_W", wdir, MAX_PATH * 2);
		h = NULL;
		if (ew > 0 && ew < MAX_PATH * 2) {
			WCHAR wpath[MAX_PATH * 2 + 64];
			WCHAR wname[MAX_PATH];
			const char *base = strrchr(argv[1], '\\');
			if (!base) base = strrchr(argv[1], '/');
			base = base ? base + 1 : argv[1];
			MultiByteToWideChar(CP_ACP, 0, base, -1, wname, MAX_PATH);
			wsprintfW(wpath, L"%ls%ls%ls", wdir,
				(wdir[ew - 1] == L'\\' || wdir[ew - 1] == L'/') ? L"" : L"\\", wname);
			h = LoadLibraryW(wpath);
		} else {
			h = LoadLibraryA(argv[1]);
		}
	}
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

	if (interactive) {
		_setmode(_fileno(stdin), _O_BINARY);
		_setmode(_fileno(stdout), _O_BINARY);
		out = stdout;
	} else {
		out = fopen(argv[3], "wb");
	}
	if (!out) {
		fprintf(stderr, "cannot open %s\n", argv[3]);
		return 5;
	}

	/* load() takes ownership of the buffer and frees it with GlobalFree.
	   With HARNESS_RELOAD=<n>, the module is loaded and unloaded n more times first (one request is
	   sent in each round), like SSP does when the ghost is changed. */
	for (pass = 0; pass <= reload; pass++) {
		const char *dir = argv[2];
		if (pass > 0) {
			static const char probe[] = "GET SHIORI/3.0\r\nCharset: Shift_JIS\r\nSender: harness\r\nID: version\r\n\r\n";
			long plen = (long)sizeof(probe) - 1;
			HGLOBAL pg = GlobalAlloc(GMEM_FIXED, plen);
			HGLOBAL pr;
			memcpy(pg, probe, plen);
			pr = request(pg, &plen);
			if (pr) GlobalFree(pr);
			unload();
		}
		int need_sep = 1;
		int useu = (getenv("HARNESS_LOADU") != NULL);
		const char *last;
		dirlen = strlen(dir);
		/* judge the last *character*, not the last byte: a folder name ending with
		   a kanji whose 2nd byte is 0x5C (0x95 0x5C) does not end with a separator */
		if (dirlen > 0) {
			last = CharPrevA(dir, dir + dirlen);
			if (last == dir + dirlen - 1 && (*last == '\\' || *last == '/')) need_sep = 0;
		}
		if (useu) {
			/* ACP -> UTF-16 -> UTF-8, then call loadu() */
			WCHAR wbuf[MAX_PATH * 2 + 2];
			char ubuf[MAX_PATH * 6 + 8];
			int wn = MultiByteToWideChar(CP_ACP, 0, dir, (int)dirlen, wbuf, MAX_PATH * 2);
			int un;
			/* HARNESS_DIR_W (UTF-16 environment variable) overrides the folder, for names outside the ANSI code page */
			{
				DWORD ew = GetEnvironmentVariableW(L"HARNESS_DIR_W", wbuf, MAX_PATH * 2);
				if (ew > 0 && ew < MAX_PATH * 2) {
					wn = (int)ew;
					need_sep = (wbuf[wn - 1] != L'\\' && wbuf[wn - 1] != L'/');
				}
			}
			LOADF loadu = (LOADF)GetProcAddress(h, "loadu");
			if (!loadu) { fprintf(stderr, "loadu is not exported\n"); fclose(out); return 3; }
			if (need_sep) wbuf[wn] = L'\\', wn++;
			un = WideCharToMultiByte(CP_UTF8, 0, wbuf, wn, ubuf, sizeof(ubuf), NULL, NULL);
			len = un;
			g = GlobalAlloc(GMEM_FIXED, len);
			memcpy(g, ubuf, len);
			if (!loadu(g, len)) {
				fprintf(stderr, "loadu() returned FALSE for %s\n", dir);
				fclose(out);
				return 4;
			}
		} else {
			len = (long)dirlen + (need_sep ? 1 : 0);
			g = GlobalAlloc(GMEM_FIXED, len);
			memcpy(g, dir, dirlen);
			if (need_sep) ((char *)g)[dirlen] = '\\';
			if (!load(g, len)) {
				fprintf(stderr, "load() returned FALSE for %s\n", dir);
				fclose(out);
				return 4;
			}
		}
	}

	if (interactive) {
		unsigned char hdr[4];
		while (fread(hdr, 1, 4, stdin) == 4) {
			unsigned long n = hdr[0] | (hdr[1] << 8) | (hdr[2] << 16) | ((unsigned long)hdr[3] << 24);
			char *req = (char *)malloc(n + 1);
			HGLOBAL r;
			unsigned long rn = 0;
			if (!req || fread(req, 1, n, stdin) != n) break;
			g = GlobalAlloc(GMEM_FIXED, n);
			memcpy(g, req, n);
			free(req);
			len = (long)n;
			r = request(g, &len);
			if (r) rn = (unsigned long)len;
			hdr[0] = (unsigned char)(rn & 0xff);
			hdr[1] = (unsigned char)((rn >> 8) & 0xff);
			hdr[2] = (unsigned char)((rn >> 16) & 0xff);
			hdr[3] = (unsigned char)((rn >> 24) & 0xff);
			fwrite(hdr, 1, 4, stdout);
			if (r) {
				fwrite(r, 1, rn, stdout);
				GlobalFree(r);
			}
			fflush(stdout);
		}
		unload();
		FreeLibrary(h);
		return 0;
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
