#ifndef IconCanvas_h
#define IconCanvas_h

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Scroll.H>
#include <dirent.h>
#include <sys/stat.h>
#include "IconGroup.h"

class BigIcon;

struct file_info {
	const char *real_name;
	struct stat st;
};

class IconCanvas : public Fl_Scroll {
public:
	struct dirent **newlist;
	struct dirent **list;
	struct file_info **newinfo;
	struct file_info **info;
	int newnbf;
	int nbf;
	IconGroup *group;
	IconGroup *oldgroup;
	static int onmover;
        int drag_x;
        int drag_y;
        int dragging;
	IconCanvas(int X, int Y, int W, int H);
	~IconCanvas();
	void rescan(void);
	static void mover(void*);
	void draw_selector(int, int);
	int handle(int);
	void update_status(void);
	void ensure_visible(BigIcon *b);
	void draw(void);
	void resize(int X, int Y, int W, int H);
};


#endif
