/*
 * Local addition for the WASM build (not part of upstream fastchart): see
 * PROVENANCE.md, "Fonts".
 *
 * Every chart measures and draws its text through FreeType, from a font
 * file, and fastchart's default is the first of a few system paths that
 * exists (fastchart.c's FASTCHART_DEFAULT_FONT_CANDIDATES). The PHP.wasm VM
 * has no system fonts, so this embeds DejaVu Sans 2.37 (wasm-font/, with
 * its license) into the module and writes it to the first of those paths
 * when the module is loaded, before fastchart's MINIT runs. An application
 * that ships its own font there, or calls setFontPath(), is unaffected.
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
static const unsigned char fastchart_wasm_default_font[] = {
#embed "wasm-font/DejaVuSans.ttf"
};
#pragma clang diagnostic pop

#define FASTCHART_WASM_FONT_DIR "/usr/share/fonts/truetype/dejavu"
#define FASTCHART_WASM_FONT_PATH FASTCHART_WASM_FONT_DIR "/DejaVuSans.ttf"

__attribute__((constructor))
static void fastchart_wasm_install_default_font(void)
{
	static const char *const dirs[] = {
		"/usr", "/usr/share", "/usr/share/fonts",
		"/usr/share/fonts/truetype", FASTCHART_WASM_FONT_DIR,
	};
	struct stat st;
	FILE *fp;

	if (stat(FASTCHART_WASM_FONT_PATH, &st) == 0) {
		return;
	}
	for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
		mkdir(dirs[i], 0755); /* EEXIST is fine */
	}
	fp = fopen(FASTCHART_WASM_FONT_PATH, "wb");
	if (fp == NULL) {
		return; /* no default font then: setFontPath() still works */
	}
	fwrite(fastchart_wasm_default_font, 1, sizeof(fastchart_wasm_default_font), fp);
	fclose(fp);
}
