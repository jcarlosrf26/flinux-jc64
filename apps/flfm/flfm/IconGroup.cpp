#include "IconCanvas.h"
#include "IconGroup.h"
#include "BigIcon.h"
#include "SmallIcon.h"
#include "callbacks.h"
#include <FL/fl_draw.H>

#define _(String) gettext((String))


IconGroup::IconGroup(int X, int Y, int W, int H, IconCanvas *C) : 
	Fl_Group(X, Y, W, H)
{
	canvas = C;
	draw_sel = 0;
	sw = sh = sx = sy = 0;
	lx = X;
	ly = Y;
	px = py = ph = pw = 0;
	box(FL_FLAT_BOX);
}

IconGroup::~IconGroup()
{
}

void IconGroup::show_selector(int x, int y, int w, int h)
{
	sx = x;
	sy = y;
	sw = w;
	sh = h;
	draw_sel = 1;
}

void IconGroup::hide_selector()
{
	draw_sel = 0;
	fl_clip(parent()->x(), parent()->y(), parent()->w() - 16, 
		parent()->h() - 16);
	if (pw) overlay_rect(); 
	pw = 0;
	fl_pop_clip();
}

void IconGroup::overlay_rect()
{
  XSetFunction(fl_display, fl_gc, GXxor);
  XSetForeground(fl_display, fl_gc, 0xffffffff);
  XDrawRectangle(fl_display, fl_window, fl_gc, px, py, pw, ph);
  XSetFunction(fl_display, fl_gc, GXcopy);
}

void IconGroup::draw()
{
	if (draw_sel && pw) {
		overlay_rect(); 
		pw = 0;
	}
	lx = x();
	ly = y();
	Fl_Group::draw();
	if (draw_sel) {
		px = sx; py = sy; pw = sw; ph = sh;
		if (pw) overlay_rect();
	}
}

int IconGroup::handle(int e)
{
	return Fl_Group::handle(e);
}

NormalGroup::NormalGroup(int X, int Y, int W, int H, IconCanvas *C) : 
	IconGroup(X, Y, W, H, C)
{
	int i;
	BigIcon *b;
	int xx = x() + 15;
	int yy = y() + 15;
	int mx;
	int ww, hh;
	int lineh = 0;
	
	mx = w() + x() - 30;

	i = 0;
	begin();	
	while (i < canvas->nbf) {
		b = new BigIcon(xx, yy, 32, 32);
		b->labelfont(gui->font);
		b->labelsize(gui->size);
		b->set_data(canvas->info[i]);
		if (b->height > lineh) lineh = b->height;
		if (xx + b->width > mx) {
			yy += lineh + 15;
			lineh = 0;
			xx = x() + 15;
			b->resize(xx, yy, b->w(), b->h());
			xx += b->width + 15;
		} else {
			xx += b->width + 15;
		}
		i++;
	}
	yy += lineh + 15;
	end();	
	resizable(NULL);
	ww = w();
	hh = yy - y() + 80;
	if (ww < parent()->w() - 16) {
		ww = parent()->w() - 16;
	} 
	if (hh < parent()->h() - 16) {
		hh = parent()->h() - 16;
	}
	resize(x(), y(), ww, hh);
	redraw();
}

NormalGroup::~NormalGroup()
{
}

void NormalGroup::relayout(int neww)
{
	int n = children();
	if (n == 0) return;

	BigIcon *b;
	int xx = x() + 15;
	int yy = y() + 15;
	int mx = neww + x() - 30;
	int lineh = 0;
	int ww, hh;

	for (int i = 0; i < n; i++) {
		b = (BigIcon*) child(i);
		if (b->height > lineh) lineh = b->height;
		if (xx + b->width > mx) {
			yy += lineh + 15;
			lineh = 0;
			xx = x() + 15;
		}
		b->resize(xx, yy, b->w(), b->h());
		xx += b->width + 15;
	}
	yy += lineh + 15;

	ww = neww;
	hh = yy - y() + 80;
	if (ww < parent()->w() - 16) ww = parent()->w() - 16;
	if (hh < parent()->h() - 16) hh = parent()->h() - 16;
	resize(x(), y(), ww, hh);
	redraw();
}

DetailGroup::DetailGroup(int X, int Y, int W, int H, IconCanvas *C) : 
	IconGroup(X, Y, W, H, C)
{
	int i;
	SmallIcon *b;
	int xx = x() + 5;
	int yy = y() + 30;
	int lineh = gui->size + 4;
	int mw;

	fl_font(gui->font, gui->size);
	mw  = (int) fl_width("W") * 150;

	if (lineh < 18) lineh = 18;

	i = 0;
	begin();	
	while (i < canvas->nbf) {
		b = new SmallIcon(xx, yy, mw, lineh);
		b->labelfont(gui->font);
		b->labelsize(gui->size);
		b->set_data(canvas->info[i]);
		yy += lineh + 4;
		i++;
	}
	end();	
	resizable(NULL);
	resize(x(), y(), mw + 50, yy + 20);
	redraw();
}

DetailGroup::~DetailGroup()
{
}



