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

#include "Xd6Std.h"
#include "Xd6FindDialog.h"
#include <FL/Fl.h>
#include <FL/Fl_Button.h>

#define _(String) gettext((String))

Xd6FindDialog::Xd6FindDialog(Xd6HtmlFrame *f) : Fl_Window(260, 160)
{
	frame  = f;
	status = 0;

	label(_("Find / Replace"));

	i_find = new Fl_Input(5, 20, 250, 22, _("Find:"));
	i_find->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

	i_replace = new Fl_Input(5, 60, 250, 22, _("Replace by:"));
	i_replace->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

	b_next         = new Fl_Button(5,   96, 80,  25, _("Find next"));
	b_replace_next = new Fl_Button(90,  96, 110, 25, _("Replace+Find"));
	b_cancel       = new Fl_Button(205, 96, 50,  25, _("Close"));
	b_replace_all  = new Fl_Button(5,  128, 250, 25, _("Replace all"));

	b_next->callback(cb_n);
	b_replace_next->callback(cb_rn);
	b_cancel->callback(cb_c);
	b_replace_all->callback(cb_ra);

	end();
	set_modal();
	show();
}

Xd6FindDialog::~Xd6FindDialog() {}

void Xd6FindDialog::cb_n(Fl_Widget *w, void *)
{
	((Xd6FindDialog*)w->parent())->status = 2;
}

void Xd6FindDialog::cb_rn(Fl_Widget *w, void *)
{
	((Xd6FindDialog*)w->parent())->status = 1;
}

void Xd6FindDialog::cb_ra(Fl_Widget *w, void *)
{
	((Xd6FindDialog*)w->parent())->status = 3;
}

void Xd6FindDialog::cb_c(Fl_Widget *w, void *)
{
	Xd6FindDialog *d = (Xd6FindDialog*)w->parent();
	d->status = -1;
	d->hide();
}
