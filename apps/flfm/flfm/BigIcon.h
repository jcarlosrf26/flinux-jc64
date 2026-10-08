#ifndef BigIcon_h
#define BigIcon_h

#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Pixmap.H>
#include "xd640/Xd6IconWindow.h"

struct file_info;
void reset_selection(void);
Fl_Pixmap *resolve_icon_for_path(const char *path);

class BigIcon : public Fl_Menu_Button {
public:
	static Xd6IconWindow *drag_window;
	static Fl_Menu_Item *m_folder;
	static Fl_Menu_Item *m_exec;
	static Fl_Menu_Item *m_unknown;
	static Fl_Menu_Item *m_special;
	static Fl_Menu_Item *m_multi;
	static Fl_Menu_Item *m_canvas;
	static Fl_Widget *drag_widget;
	Fl_Pixmap *pix;
	const char *real_name;
	int selected;
	int width;
	int height;

	BigIcon(int X, int Y, int W, int H);
	~BigIcon();
	void set_data(struct file_info *fi);
	virtual int handle(int e);
	virtual void draw(void);
	virtual int is_inside(void);
	static void create_menus(void);
	static void destroy_menus(void);
	static void create_drag_window(void);
};

#endif
