#include "Location.h"
#include "callbacks.h"
#include "xpm/new.xpm"
#include "xpm/back.xpm"
#include "xpm/up.xpm"
#include "xpm/home.xpm"

Location::Location(int X, int Y, int W, int H) : Fl_Group(X, Y, W, H)
{
	url = NULL;
	nfm = new Fl_Button(X + 1, Y + 1, 26, H - 2);
	up = new Fl_Button(X + 28, Y + 1, 26, H - 2);
	back = new Fl_Button(X + 55, Y + 1, 26, H - 2);
	home = new Fl_Button(X + 82, Y + 1, 26, H - 2);
	loc = new Fl_Input(X + 109, Y + 1, W - 109, H - 2);
	p1 = new Fl_Pixmap(new_xpm);
	p2 = new Fl_Pixmap(back_xpm);
	p3 = new Fl_Pixmap(up_xpm);
	p4 = new Fl_Pixmap(home_xpm);
	p1->label(nfm);
	p2->label(back);
	p3->label(up);
	p4->label(home);
	up->callback(cb_up);
	back->callback(cb_back);
	nfm->callback(cb_new_fm);
	home->callback(cb_home);
	loc->callback(cb_loc_input, this);
	loc->when(FL_WHEN_ENTER_KEY_ALWAYS);
	end();
	resizable(loc);
	box(FL_THIN_UP_BOX);
}

Location::~Location()
{
	delete(p1);
	delete(p2);
	delete(p3);
	delete(p4);
	free(url);
}

void Location::value(const char *val)
{
	free(url);
	url = strdup(val);
	loc->value(url);
}

int Location::handle(int e)
{
	int r;

	r = Fl_Group::handle(e);
	
	switch(e) {
	case FL_ENTER:
		return 1;
	default:
		break;
	}
	
	return r;
}

void Location::new_url(const char *u)
{
	int i = 0;
	if (!u) return;

	while (i < 10) {
		if (!StatesValues.history[i]) break;
		i++;
	}
	if (i >= 10) {
		free(StatesValues.history[0]);
		for (i = 0; i < 9; i++) {
			StatesValues.history[i] = StatesValues.history[i + 1];
		}
	}
	StatesValues.history[i] = strdup(u);
}

