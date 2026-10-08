#include <FL/x.H>
#include <FL/fl_utf8.h>
#include <FL/fl_draw.H>
#include <stdint.h>

Atom fl_XdndAware = None;

/* FLTK 1.1 internal globals — used by Xd6HtmlPrint / Xd6DefaultFonts */
int fl_font_ = 0;
int fl_size_ = 14;

extern "C" {

/* FLTK 1.1 → 1.3: fl_ucs2utf → fl_utf8encode */
int fl_ucs2utf(unsigned ucs, char *buf)
{
	return fl_utf8encode(ucs, buf);
}

/* FLTK 1.1 → 1.3: fl_utf2ucs → fl_utf8decode */
int fl_utf2ucs(const unsigned char *buf, int len, unsigned int *ucs)
{
	int n;
	unsigned u = fl_utf8decode((const char*)buf, (const char*)buf + len, &n);
	if (ucs) *ucs = u;
	return n;
}

} /* extern "C" */
