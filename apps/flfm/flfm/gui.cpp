#include "gui.h"
#include "callbacks.h"
#include "BigIcon.h"
#include <libintl.h>

#define _(String) gettext((String))

int GUI::dnd_action = 0;
extern Fl_Widget *fl_selection_requestor;
extern Atom fl_XdndSelection;
extern Window fl_dnd_target_window;
extern int (*fl_local_grab)(int);

GUI::GUI(int X, int Y, int W, int H) : Fl_Window(W, H, "FLFM")
{
	mnu_bar = NULL;
	mnu = NULL;
	loc_inp = NULL;
	icon_can = NULL;
	stat_bar = NULL;
	size = 16;
	font = (Fl_Font) 0;
	
	dnd_menu = NULL;
	dnd_items = NULL;
	
	dnd_target = NULL;

	StatesValues.sort_size = 0;
	StatesValues.sort_type = 0;
	StatesValues.sort_name = 1;
	StatesValues.view_icon = 1;
	StatesValues.view_detail = 0;
	StatesValues.show_hide = 0;
	StatesValues.url = 0;
	StatesValues.newurl = 0;
	StatesValues.status = 0;
	StatesValues.history[0] = 0;
	StatesValues.history[1] = 0;
	StatesValues.history[2] = 0;
	StatesValues.history[3] = 0;
	StatesValues.history[4] = 0;
	StatesValues.history[5] = 0;
	StatesValues.history[6] = 0;
	StatesValues.history[7] = 0;
	StatesValues.history[8] = 0;
	StatesValues.history[9] = 0;
}

GUI::~GUI()
{
	Fl::remove_timeout(cb_directory, 0);
	delete(mnu_bar);
	delete(mnu);
	delete(loc_inp);
	delete(icon_can);
	delete(stat_bar);
}

void GUI::create_layout()
{
	int dw, dh;
	
	create_menu();

	begin();

	dw = w() - 240;
	dh = h() - 190;
	mnu_bar = new Fl_Menu_Bar(0, 0, 240 + dw, 25);;
	mnu_bar->menu(mnu);
	mnu_bar->textfont(font);
	mnu_bar->textsize(size);
	mnu_bar->selection_color(137);
	refresh_volumes_menu();

	loc_inp = new Location(0, 25, 240 + dw, 27);

	icon_can = new IconCanvas(0, 52, 241 + dw, 119 + dh);
	icon_can->end();
	resizable(icon_can);

	stat_bar = new Fl_Output(0, 171 + dh, 240 + dw, 20);
	stat_bar->color(FL_GRAY);

	end();

	size_range(300, 200);
	Fl_X::make_xid(this);
}

void GUI::create_menu()
{
	int i, l;
	mnu = new Fl_Menu_Item[50];
	mnu->text = NULL;

	i = mnu->add(_("File"), FL_ALT+'f', 0, 0, 0);
	mnu->add(_("Apps"), 0, (Fl_Callback*)cb_apps, 0, FL_MENU_DIVIDER);
	mnu->add(_("Rescan Directory"), 0, (Fl_Callback*)cb_rescan, 0, 0);
	mnu->add(_("Find ..."), FL_CTRL+'f', (Fl_Callback*)cb_find, 0, FL_MENU_DIVIDER);
	mnu->add(_("New Directory ..."), 0, (Fl_Callback*)cb_newdir, 0, 0);
	mnu->add(_("New File ..."), 0, (Fl_Callback*)cb_newfile, 0, FL_MENU_DIVIDER);
	mnu->add(_("Volumes"), 0, 0, 0, FL_SUBMENU_POINTER|FL_MENU_DIVIDER);
	mnu->add(_("Trash ..."), 0, (Fl_Callback*)cb_trash, 0, FL_MENU_DIVIDER);
	mnu->add(_("Preferences ..."), 0, (Fl_Callback*)cb_pref, 0, FL_MENU_DIVIDER);
	l = mnu->add(_("Exit"), FL_ALT+'x', (Fl_Callback*)cb_exit, 0, 0);
	mnu[i].flags = FL_SUBMENU;
	mnu[l + 2].text = NULL;

	i = mnu->add(_("View"), FL_ALT+'v', 0, 0, 0);
	l = mnu->add(_("Sort by Name"), 0,
		(Fl_Callback*)cb_sort, (void *)1, FL_MENU_RADIO);
	if (StatesValues.sort_name) mnu[l].set();
	l = mnu->add(_("Sort by File Type"), 0,
		(Fl_Callback*)cb_sort, (void *)2, FL_MENU_RADIO);
	if (StatesValues.sort_type) mnu[l].set();
	l = mnu->add(_("Sort by Size"), 0,
		(Fl_Callback*)cb_sort, (void *)3, FL_MENU_RADIO|FL_MENU_DIVIDER);
	if (StatesValues.sort_size) mnu[l].set();
	l = mnu->add(_("Icon View"), 0,
		(Fl_Callback*)cb_sort, (void *)4, FL_MENU_RADIO);
	if (StatesValues.view_icon) mnu[l].set();
	l = mnu->add(_("Detailed View"), 0,
		(Fl_Callback*)cb_sort, (void *)6, FL_MENU_RADIO|FL_MENU_DIVIDER);
	if (StatesValues.view_detail) mnu[l].set();
	l = mnu->add(_("Show Hidden Files"), 0,
		(Fl_Callback*)cb_sort, (void *)7, FL_MENU_TOGGLE);
	if (StatesValues.show_hide) mnu[l].set();
	mnu[i].flags = FL_SUBMENU;
	mnu[l + 2].text = NULL;

	i = mnu->add(_("Help"), FL_ALT+'h', 0, 0, 0);
	mnu->add(_("About"), FL_ALT+'a', (Fl_Callback*)cb_about, 0, 0);
	l = mnu->add(_("Online Help"), FL_ALT+'o', (Fl_Callback*)cb_help, 0, 0);
	mnu[i].flags = FL_SUBMENU;

	mnu[l + 2].text = NULL;

	dnd_menu = new Fl_Menu_Button(0, 0, 0, 0);
	dnd_menu->textfont(font);
        dnd_menu->textsize(size);
        dnd_menu->selection_color(137);
	
	dnd_items = new Fl_Menu_Item[6];
	dnd_items[0].text = NULL;
	dnd_items->add(_("Move"), 0, dnd_cb, (void*)DND_MOVE, 0);
	dnd_items->add(_("Link"), 0, dnd_cb, (void*)DND_LINK, 0);
	dnd_items->add(_("Copy"), 0, dnd_cb, (void*)DND_COPY, 
		FL_MENU_DIVIDER);
	dnd_items->add(_("Abort drag..."), 0, dnd_cb, 0, 0);
	dnd_menu->menu(dnd_items);
}

int GUI::handle(int e)
{
	static int beenhere = 0;
	int a = DND_COPY;
	int (*og)(int);
	int ret;
	if (!beenhere) {
		beenhere = 1;
		Fl::selection(*this, " ", 1);
	}
	switch(e) {
	case FL_SHORTCUT:
		if (Fl::event_key() == FL_Escape ||
			((Fl::event_state() & FL_ALT) && Fl::event_key() == 'x'))
		{
			do_callback();
			return 1;
		}
		break;
	case FL_DND_ENTER:
		dnd_action = 0;
		dnd_target = NULL;
		Fl::focus_ = this;
		return 1;
	case FL_DND_DRAG:
		if (Fl::event_state() & FL_BUTTON1) {
			fl_dnd_action = XdndActionCopy;
		} else {
			fl_dnd_action = XdndActionAsk;
		}
		if (BigIcon::drag_window) {
			BigIcon::drag_window->position(Fl::event_x_root() + 2,
                                Fl::event_y_root() + 2);
		}

		Fl::focus_ = this;
		ret = dnd_move();
		og = fl_local_grab;
		fl_local_grab = 0;
		Fl::belowmouse(this);
		fl_local_grab = og;
			if (Fl::belowmouse_ == BigIcon::drag_widget ||
				(Fl::belowmouse_ && BigIcon::drag_widget ==
				Fl::belowmouse_->parent())) 
			{
				 Fl::first_window()->
					cursor((Fl_Cursor)21);
			} else if (fl_XdndActionCopy == XdndActionAsk) {
				Fl::first_window()->
					cursor((Fl_Cursor)47);
			} else {
				Fl::first_window()->
					cursor((Fl_Cursor)18);
			}
		Fl::focus_ = this;
		Fl::belowmouse_ = this;
		return ret;
	case FL_DND_LEAVE:
		dnd_target = NULL;
		dnd_action = 0;
		return 1;
	case FL_DND_RELEASE:
		ret = dnd_move();
		if (!ret) return 0;
		dnd_target = Fl::belowmouse();
		if (fl_dnd_action == XdndActionAsk) {
			a = DND_ASK;
		} else if (fl_dnd_action == XdndActionMove) {
			a = DND_MOVE;
		} else if (fl_dnd_action == XdndActionLink) {
			a = DND_LINK;
		}
		dnd_release(a);
		Fl::focus_ = this;
		Fl::belowmouse_ = this;
		fl_selection_requestor = this;
		return 1;
	case FL_DROP:
		if (dnd_target && dnd_action > 0) {
			Fl::belowmouse_ = dnd_target;
			cb_move_link_copy(NULL, (void*)dnd_action);
			dnd_target = NULL;
			return 1;
		}
		break;
	default:
		break;
	}
	return Fl_Window::handle(e);	
}

int GUI::dnd_move()
{
	int mx, my;

	Fl::get_mouse(mx, my);
	mx -= x();
	my -= y();
	Fl::e_x = mx;
	Fl::e_y = my;
	Fl_Window::handle(FL_ENTER);
	if (icon_can->contains(Fl::belowmouse()) ||
		loc_inp == Fl::belowmouse() ||
		loc_inp->contains(Fl::belowmouse()))
	{
		return 1;
	}
	return 0;
}

void GUI::dnd_cb(Fl_Widget *w, void *d)
{
	dnd_action = *((int*)&d);
}

int GUI::dnd_release(int a)
{
	dnd_action = a;
	if (a == DND_ASK) {
		if (icon_can->contains(Fl::belowmouse())) {
			dnd_action = 0;
			dnd_menu->resize(Fl::event_x(), Fl::event_y(), 0, 0);
			Fl::check();
			Fl::flush();
			dnd_menu->popup();
		} else {
			dnd_action = DND_COPY;
		}		
	}
	return 1;
}
