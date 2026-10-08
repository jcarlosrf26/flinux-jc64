#ifndef TrashWindow_h
#define TrashWindow_h

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Image.H>
#include "IconGroup.h"

class TrashRow : public Fl_Menu_Button {
public:
	char *id;
	char *original_path;
	char info[96];
	int info_len;
	int offset;
	int selected;
	int detail_mode;
	int icon_size;
	int width;
	int height;
	Fl_Image *pix;

	TrashRow(int X, int Y, int W, int H, const char *id,
		const char *original_path, long deleted_time,
		Fl_Image *pix, int detail_mode);
	~TrashRow();
	int handle(int e);
	void draw();
	void restore();
	void show_properties();
};

class TrashRowGroup : public IconGroup {
public:
	TrashRowGroup(int X, int Y, int W, int H);
	void relayout(int neww);
};

class TrashScroll : public Fl_Scroll {
public:
	TrashRowGroup *group;
	int dragging;
	int drag_x;
	int drag_y;
	TrashScroll(int X, int Y, int W, int H);
	void resize(int X, int Y, int W, int H);
	int handle(int e);
	void ensure_visible(TrashRow *r);
	void draw_selector(int ev_x, int ev_y);
};

class TrashWindow : public Fl_Double_Window {
public:
	TrashScroll *scroll;
	TrashWindow();
	void load_trash();
};

#endif
