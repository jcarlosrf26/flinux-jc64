#ifndef Location_h
#define Location_h

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pixmap.H>

class Location : public Fl_Group {
public:
	char *url;
	Fl_Input *loc;
	Fl_Button *back;
	Fl_Button *up;
	Fl_Button *nfm;
	Fl_Button *home;
	Fl_Pixmap *p1;
	Fl_Pixmap *p2;
	Fl_Pixmap *p3;
	Fl_Pixmap *p4;

	Location(int X, int Y, int W, int H);
	~Location();
	int handle(int e);
	void new_url(const char *);
	void value(const char *val);
};


#endif
