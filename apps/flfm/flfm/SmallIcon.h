#ifndef SmallIcon_h
#define SmallIcon_h

#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Pixmap.H>
#include "BigIcon.h"


class SmallIcon : public BigIcon {
public:
	char info[256];
	int offset;
	int info_len;

	SmallIcon(int X, int Y, int W, int H);
	~SmallIcon();
	void set_data(struct file_info *fi);
	virtual void draw(void);
	virtual int is_inside(void);
};

#endif
