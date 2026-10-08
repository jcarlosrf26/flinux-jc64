#ifndef AppsWindow_h
#define AppsWindow_h

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Input.H>

class AppIcon : public Fl_Menu_Button {
public:
	char *title;
	char *cmd;
	char *icon_path;
	char *desktop_path;
	char info[64];
	int info_len;
	int offset;
	int selected;
	int detail_mode;
	int icon_size;
	int width;
	int height;
	Fl_Image *pix;

	AppIcon(int X, int Y, int W, int H, const char *title,
		const char *cmd, const char *icon_path,
		const char *desktop_path, Fl_Image *pix, int detail_mode);
	~AppIcon();
	int handle(int e);
	void draw();
	void open_app();
	void show_properties();
};

class AppsIconGroup : public Fl_Group {
public:
	AppsIconGroup(int X, int Y, int W, int H);
	void relayout(int neww);
};

class AppsScroll : public Fl_Scroll {
public:
	AppsIconGroup *group;
	AppsScroll(int X, int Y, int W, int H);
	void resize(int X, int Y, int W, int H);
	int handle(int e);
	void ensure_visible(AppIcon *a);
};

class AppsSearchInput : public Fl_Input {
public:
	AppsSearchInput(int X, int Y, int W, int H);
	int handle(int e);
};

class AppsWindow : public Fl_Double_Window {
public:
	AppsScroll *scroll;
	AppsSearchInput *search;
	AppsWindow();
	void load_apps();
};

#endif
