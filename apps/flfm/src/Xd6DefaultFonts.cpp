/******************************************************************************
 *   "$Id:  $"
 *
 *                 Copyright (c) 2000-2001  O'ksi'D
 *
 *                      All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *      Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *      Neither the name of O'ksi'D nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER 
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *   Author : Jean-Marc Lienher ( http://oksid.ch )
 *
 ******************************************************************************/


#define HAVE_XUTF8 1

#include "Xd6DefaultFonts.h"
#include "Xd6Std.h"

/* fltk-utf8 → FLTK 1.3 compat */
#include "FL/fl_utf8.h"
static inline int fl_ucs2utf(unsigned ucs, char *buf) {
    return fl_utf8encode(ucs, buf);
}
#include <FL/x.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/fl_utf8.h>
#include <string.h>


#define _(String) gettext((String))

Fl_Font Xd6DefaultFonts::free_font = FL_FREE_FONT;
Fl_Font Xd6DefaultFonts::serif = FL_TIMES;
Fl_Font Xd6DefaultFonts::sans_serif = FL_HELVETICA;
Fl_Font Xd6DefaultFonts::monospace = FL_COURIER;

static void set_font(Xd6ConfigFileGroup *fnt, Xd6ConfigFile *cfg, int font)
{
	const char *val;
	Xd6ConfigFileItem *itm;

	
	itm = fnt->get_item("normal", cfg->locale);
	val = NULL; if (itm) val = itm->get_value();
	if (val) Fl::set_font((Fl_Font)font, val);
	font++;
	itm = fnt->get_item("bold", cfg->locale);
	val = NULL; if (itm) val = itm->get_value();
	if (val) Fl::set_font((Fl_Font)font, val);
	font++;
	itm = fnt->get_item("italic", cfg->locale);
	val = NULL; if (itm) val = itm->get_value();
	if (val) Fl::set_font((Fl_Font)font, val);
	font++;
	itm = fnt->get_item("bold italic", cfg->locale);
	val = NULL; if (itm) val = itm->get_value();
	if (val) Fl::set_font((Fl_Font)font, val);
}

extern int fl_font_;
	
void Xd6DefaultFonts::load(Xd6ConfigFile *cfg)
{
	Xd6ConfigFileSection *sec;
	Xd6ConfigFileGroup *grp;
	Xd6ConfigFileGroup *fnt;
	int font;

	fl_open_display();
	fl_font(0, 14);

	if (!cfg) return;

	sec = cfg->get_xd640_section();
	if (!sec) return;

	grp = sec->get_group("fonts", cfg->locale);
	if (!grp) return;

	font = (int) 0;

	fnt = grp->get_group("sans-serif", cfg->locale);
	if (fnt) {
		sans_serif = (Fl_Font)font;
		set_font(fnt, cfg, font);
	}
	font += 4;
 
	fnt = grp->get_group("monospace", cfg->locale);
	if (fnt) {
		monospace = (Fl_Font)font;
		set_font(fnt, cfg, font);
	}
	font += 4;
	
	fnt = grp->get_group("serif", cfg->locale);
	if (fnt) {
		serif = (Fl_Font)font;
		set_font(fnt, cfg, font);
	}

	fl_font_ = -1;
	fl_font(0, 14);
}

