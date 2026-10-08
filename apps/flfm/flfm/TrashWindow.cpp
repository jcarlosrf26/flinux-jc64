#include "TrashWindow.h"
#include "callbacks.h"
#include "BigIcon.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Box.H>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define _(String) gettext((String))

#define CELL_W 80
#define CELL_H 76
#define ICON_SIZE 32

struct trash_entry {
	char *id;
	char *original_path;
	long deleted_time;
};

static int time_sort(const void *a, const void *b)
{
	const trash_entry *ea = (const trash_entry*) a;
	const trash_entry *eb = (const trash_entry*) b;
	if (eb->deleted_time > ea->deleted_time) return 1;
	if (eb->deleted_time < ea->deleted_time) return -1;
	return 0;
}

static trash_entry *list_trash_entries(int *count)
{
	char files_dir[900], info_dir[900];
	DIR *dir;
	struct dirent *de;
	trash_entry *entries;
	int cap = 32;
	int n = 0;

	*count = 0;
	trash_dirs(files_dir, info_dir, sizeof(files_dir));
	dir = opendir(info_dir);
	if (!dir) return NULL;

	entries = (trash_entry*) malloc(sizeof(trash_entry) * cap);

	while ((de = readdir(dir))) {
		char path[1200];
		char line[1200];
		FILE *f;
		int len = strlen(de->d_name);

		if (len < 11 || strcmp(de->d_name + len - 10, ".trashinfo") != 0) {
			continue;
		}
		snprintf(path, sizeof(path), "%s/%s", info_dir, de->d_name);
		f = fopen(path, "r");
		if (!f) continue;

		if (!fgets(line, sizeof(line), f)) { fclose(f); continue; }
		line[strcspn(line, "\r\n")] = '\0';

		if (n >= cap) {
			cap *= 2;
			entries = (trash_entry*) realloc(entries, sizeof(trash_entry) * cap);
		}
		entries[n].id = (char*) malloc(len - 10 + 1);
		memcpy(entries[n].id, de->d_name, len - 10);
		entries[n].id[len - 10] = '\0';
		entries[n].original_path = strdup(line);
		entries[n].deleted_time = 0;
		if (fgets(line, sizeof(line), f)) {
			entries[n].deleted_time = atol(line);
		}
		fclose(f);
		n++;
	}
	closedir(dir);

	qsort(entries, n, sizeof(trash_entry), time_sort);
	*count = n;
	return entries;
}

static int move_replacing_collisions(const char *src, const char *dest)
{
	char final_dest[1200];
	struct stat st;
	int n = 1;

	snprintf(final_dest, sizeof(final_dest), "%s", dest);
	while (stat(final_dest, &st) == 0) {
		n++;
		snprintf(final_dest, sizeof(final_dest), "%.1100s.%d", dest, n);
	}
	return rename(src, final_dest) == 0;
}

static void remove_trash_files(const char *files_dir, const char *info_dir, const char *id)
{
	char files_path[1200], info_path[1200], q[2400], buf[2600];

	snprintf(files_path, sizeof(files_path), "%s/%s", files_dir, id);
	snprintf(info_path, sizeof(info_path), "%s/%s.trashinfo", info_dir, id);

	int len = (int)strlen(files_path);
	char *p = q;
	*p++ = '\'';
	for (int i = 0; i < len; i++) {
		if (files_path[i] == '\'') { *p++ = '\''; *p++ = '\\'; *p++ = '\''; *p++ = '\''; }
		else *p++ = files_path[i];
	}
	*p++ = '\'';
	*p = '\0';

	snprintf(buf, sizeof(buf), "rm -rf %s", q);
	system(buf);
	unlink(info_path);
}

static int restore_entry(const char *id, const char *original_path)
{
	char files_dir[900], info_dir[900];
	char src[1200], parent[1200], dest[1200];
	const char *home;
	const char *base;
	char *slash;

	trash_dirs(files_dir, info_dir, sizeof(files_dir));
	snprintf(src, sizeof(src), "%s/%s", files_dir, id);

	snprintf(parent, sizeof(parent), "%s", original_path);
	slash = strrchr(parent, '/');
	if (slash) *slash = '\0';

	base = strrchr(original_path, '/');
	base = base ? base + 1 : original_path;

	if (parent[0] && access(parent, F_OK) == 0) {
		snprintf(dest, sizeof(dest), "%s", original_path);
	} else {
		home = getenv("HOME");
		if (!home) home = "/root";
		snprintf(dest, sizeof(dest), "%s/%s", home, base);
	}

	if (!move_replacing_collisions(src, dest)) return 0;

	char info_path[1200];
	snprintf(info_path, sizeof(info_path), "%s/%s.trashinfo", info_dir, id);
	unlink(info_path);
	return 1;
}

static void format_date(long t, char *out, size_t len)
{
	time_t tt = (time_t) t;
	char *s = ctime(&tt);
	if (s) {
		s[strlen(s) - 1] = '\0';
		snprintf(out, len, "%s", s);
	} else {
		snprintf(out, len, "?");
	}
}

static void original_dir(const char *original_path, char *out, size_t len)
{
	char *slash;
	snprintf(out, len, "%s", original_path);
	slash = strrchr(out, '/');
	if (slash && slash != out) *slash = '\0';
	else if (slash) out[1] = '\0';
}

static Fl_Image *trash_icon(const char *files_dir, const char *id, int size)
{
	char path[1200];
	Fl_Pixmap *pix;

	snprintf(path, sizeof(path), "%s/%s", files_dir, id);
	pix = resolve_icon_for_path(path);
	return pix->w() != size ? pix->copy(size, size) : (Fl_Image*)pix;
}

static Fl_Menu_Item *row_menu = NULL;
static Fl_Menu_Item *canvas_menu = NULL;

static void reload_trash_view();

static void restore_selected(Fl_Group *group)
{
	int n = group->children();
	int any = 0;
	for (int i = n - 1; i >= 0; i--) {
		TrashRow *r = (TrashRow*)group->child(i);
		if (!r->selected) continue;
		if (restore_entry(r->id, r->original_path)) {
			any = 1;
		} else {
			fl_alert("%s", _("Could not restore this item."));
		}
	}
	if (any) reload_trash_view();
}

static void delete_selected_permanently(Fl_Group *group, int ask)
{
	int n = group->children();
	int count = 0;
	char files_dir[900], info_dir[900];
	char msg[128];

	for (int i = 0; i < n; i++) {
		if (((TrashRow*)group->child(i))->selected) count++;
	}
	if (count == 0) return;

	if (ask) {
		if (count == 1) {
			snprintf(msg, sizeof(msg), "%s",
				_("Permanently delete this item from Trash? This cannot be undone."));
		} else {
			snprintf(msg, sizeof(msg),
				_("Permanently delete %d items from Trash? This cannot be undone."),
				count);
		}
		if (!fl_ask("%s", msg)) return;
	}

	trash_dirs(files_dir, info_dir, sizeof(files_dir));
	for (int i = n - 1; i >= 0; i--) {
		TrashRow *r = (TrashRow*)group->child(i);
		if (r->selected) remove_trash_files(files_dir, info_dir, r->id);
	}
	reload_trash_view();
}

static void cb_row_restore(Fl_Widget *w, void*)
{
	restore_selected((Fl_Group*) w->parent());
}

static void cb_row_delete(Fl_Widget *w, void*)
{
	delete_selected_permanently((Fl_Group*) w->parent(), 1);
}

static void cb_row_properties(Fl_Widget *w, void*)
{
	((TrashRow*)w)->show_properties();
}

static void build_row_menu()
{
	if (row_menu) return;
	row_menu = new Fl_Menu_Item[4];
	row_menu->text = NULL;
	row_menu->add(_("Restore"), 0, cb_row_restore, 0, 0);
	row_menu->add(_("Delete Permanently"), 0, cb_row_delete, 0, FL_MENU_DIVIDER);
	row_menu->add(_("Properties"), 0, cb_row_properties, 0, 0);
}

static void cb_empty_trash(Fl_Widget*, void*)
{
	char files_dir[900], info_dir[900];
	char q[2400], buf[2600];

	if (!fl_ask("%s", _("Permanently delete all items in Trash? This cannot be undone."))) {
		return;
	}
	trash_dirs(files_dir, info_dir, sizeof(files_dir));

	int len = (int)strlen(files_dir);
	char *p = q;
	*p++ = '\'';
	for (int i = 0; i < len; i++) {
		if (files_dir[i] == '\'') { *p++ = '\''; *p++ = '\\'; *p++ = '\''; *p++ = '\''; }
		else *p++ = files_dir[i];
	}
	*p++ = '\'';
	*p = '\0';
	snprintf(buf, sizeof(buf), "rm -rf %s/* 2>/dev/null", q);
	system(buf);

	DIR *dir = opendir(info_dir);
	if (dir) {
		struct dirent *de;
		while ((de = readdir(dir))) {
			int l = strlen(de->d_name);
			if (l > 10 && !strcmp(de->d_name + l - 10, ".trashinfo")) {
				char path[1200];
				snprintf(path, sizeof(path), "%s/%s", info_dir, de->d_name);
				unlink(path);
			}
		}
		closedir(dir);
	}
	reload_trash_view();
}

static void build_canvas_menu()
{
	if (canvas_menu) return;
	canvas_menu = new Fl_Menu_Item[2];
	canvas_menu->text = NULL;
	canvas_menu->add(_("Empty Trash"), 0, cb_empty_trash, 0, 0);
}

TrashRow::TrashRow(int X, int Y, int W, int H, const char *i,
	const char *op, long deleted_time, Fl_Image *p, int d)
	: Fl_Menu_Button(X, Y, W, H)
{
	char base[900];

	id = strdup(i);
	original_path = strdup(op);
	pix = p;
	selected = 0;
	detail_mode = d;
	icon_size = pix ? pix->w() : ICON_SIZE;
	align(d ? (FL_ALIGN_LEFT|FL_ALIGN_INSIDE) :
		(FL_ALIGN_BOTTOM|FL_ALIGN_INSIDE));
	selection_color(137);
	type(1);
	box(FL_FLAT_BOX);

	const char *slash = strrchr(original_path, '/');
	const char *base_name = slash ? slash + 1 : original_path;
	label(base_name);

	build_row_menu();
	menu(row_menu);

	info[0] = '\0';
	info_len = 0;
	offset = 200;
	if (detail_mode) {
		char date[64];
		format_date(deleted_time, date, sizeof(date));
		original_dir(original_path, base, sizeof(base));
		snprintf(info, sizeof(info), "%s  (%s)", base, date);
		info_len = strlen(info);
		fl_font(labelfont(), labelsize());
		offset = (int) fl_width(label()) + icon_size + 24;
		if (offset < 200) offset = 200;
		width = W;
		height = H;
	} else {
		fl_font(labelfont(), labelsize());
		width = (int) fl_width(label()) + 8;
		if (width < 48) width = 48;
		height = labelsize() + 8 + icon_size;
	}
}

TrashRow::~TrashRow()
{
	free(id);
	free(original_path);
}

int TrashRow::handle(int e)
{
	switch (e) {
	case FL_PUSH:
		if (!(Fl::event_state() & FL_CTRL) && (Fl::event_state() & FL_BUTTON1)) {
			if (parent()) {
				int n = parent()->children();
				for (int i = 0; i < n; i++) {
					TrashRow *r = (TrashRow*)parent()->child(i);
					if (r != this && r->selected) {
						r->selected = 0;
						r->redraw();
					}
				}
			}
			Fl::focus(parent());
		}
		selected = 1;
		redraw();
		if (Fl::event_state() & FL_BUTTON3) {
			popup();
			return 1;
		}
		if (Fl::event_clicks() > 0) {
			Fl::event_clicks(0);
			restore();
			return 1;
		}
		return 1;
	case FL_RELEASE:
		return 1;
	default:
		break;
	}
	return Fl_Menu_Button::handle(e);
}

void TrashRow::draw()
{
	if (detail_mode) {
		Fl_Color txt = labelcolor();
		if (selected) {
			fl_color(selection_color());
			fl_rectf(x(), y(), w(), h());
			txt = fl_contrast(txt, selection_color());
		} else {
			draw_box(FL_FLAT_BOX, color());
		}
		if (pix) pix->draw(x() + 4, y() + (h() - icon_size) / 2);
		fl_font(labelfont(), labelsize());
		fl_color(txt);
		fl_draw(label(), x() + icon_size + 12,
			y() + h() - fl_descent() - (h() - fl_height()) / 2);
		fl_draw(info, info_len, x() + offset,
			y() + h() - fl_descent() - (h() - fl_height()) / 2);
		return;
	}
	draw_box(FL_FLAT_BOX, color());
	if (selected) {
		fl_color(selection_color());
		fl_rectf(x() + (w() - icon_size) / 2, y(), icon_size, icon_size);
	}
	if (pix) pix->draw(x() + (w() - icon_size) / 2, y());
	draw_label();
}

void TrashRow::restore()
{
	if (!restore_entry(id, original_path)) {
		fl_alert("%s", _("Could not restore this item."));
		return;
	}
	reload_trash_view();
}

void TrashRow::show_properties()
{
	char files_dir[900], info_dir[900];
	char path[1200];
	char info[2048];
	struct stat st;

	trash_dirs(files_dir, info_dir, sizeof(files_dir));
	snprintf(path, sizeof(path), "%s/%s", files_dir, id);

	if (stat(path, &st) == 0) {
		const char *type = S_ISDIR(st.st_mode) ? _("Directory") :
			S_ISLNK(st.st_mode) ? _("Symlink") : _("File");
		snprintf(info, sizeof(info),
			"%s: %s\n%s: %s\n%s: %ld\n%s: %04o",
			_("Original location"), original_path,
			_("Type"), type,
			_("Size"), (long)st.st_size,
			_("Mode"), (unsigned)(st.st_mode & 0777));
		fl_message("%s", info);
	} else {
		fl_alert(_("Cannot access: %s"), original_path);
	}
}

TrashRowGroup::TrashRowGroup(int X, int Y, int W, int H) : IconGroup(X, Y, W, H, NULL)
{
}

void TrashRowGroup::relayout(int neww)
{
	int n = children();
	int ww, hh;

	if (StatesValues.view_detail) {
		int row_h = gui->size + 4;
		if (row_h < 18) row_h = 18;
		int row_w = neww - 16;
		for (int i = 0; i < n; i++) {
			TrashRow *r = (TrashRow*)child(i);
			r->resize(x(), y() + i * row_h, row_w, row_h);
		}
		ww = neww;
		hh = n * row_h + 15;
	} else {
		int xx = x() + 15;
		int yy = y() + 15;
		int mx = neww + x() - 30;
		int lineh = 0;

		for (int i = 0; i < n; i++) {
			TrashRow *r = (TrashRow*)child(i);
			if (r->height > lineh) lineh = r->height;
			if (xx + r->width > mx) {
				yy += lineh + 15;
				lineh = 0;
				xx = x() + 15;
			}
			r->resize(xx, yy, r->width, r->height);
			xx += r->width + 15;
		}
		yy += lineh + 15;
		ww = neww;
		hh = yy - y() + 80;
	}

	if (ww < parent()->w() - 16) ww = parent()->w() - 16;
	if (hh < parent()->h() - 16) hh = parent()->h() - 16;
	resize(x(), y(), ww, hh);
	redraw();
}

TrashScroll::TrashScroll(int X, int Y, int W, int H) : Fl_Scroll(X, Y, W, H)
{
	group = NULL;
	dragging = 0;
	drag_x = 0;
	drag_y = 0;
	type(BOTH_ALWAYS);
	box(FL_FLAT_BOX);
}

void TrashScroll::draw_selector(int ev_x, int ev_y)
{
	int redr = 0;
	const int numchildren = group->children();
	int x = 0, y = 0, dx = 0, dy = 0;
	int drag_dx, drag_dy, mx, my;

	drag_dx = ev_x - drag_x;
	drag_dy = ev_y - drag_y;

	if (drag_x < ev_x) { x = drag_x; dx = drag_dx; }
	else { x = ev_x; dx = -drag_dx; }
	if (drag_y < ev_y) { y = drag_y; dy = drag_dy; }
	else { y = ev_y; dy = -drag_dy; }

	x -= xposition();
	y -= yposition();
	mx = x + dx;
	my = y + dy;

	for (int i = 0; i < numchildren; i++) {
		TrashRow *r = (TrashRow*)group->child(i);
		int bx = r->x(), by = r->y(), bw = r->w(), bh = r->h();

		if (bx + bw > x && bx < mx && by + bh > y && by < my) {
			if (!r->selected) { r->selected = 1; r->damage(FL_DAMAGE_ALL); redr = 1; }
		} else {
			if (r->selected) { r->selected = 0; r->damage(FL_DAMAGE_ALL); redr = 1; }
		}
	}

	group->damage(FL_DAMAGE_CHILD);
	group->show_selector(x, y, dx, dy);
	if (redr) Fl::flush();
}

void TrashScroll::resize(int X, int Y, int W, int H)
{
	int oldw = w();
	Fl_Scroll::resize(X, Y, W, H);
	if (group && W != oldw && !StatesValues.view_detail) group->relayout(W);
}

void TrashScroll::ensure_visible(TrashRow *r)
{
	if (!r) return;
	int iy = r->y(), ih = r->h();
	int ix = r->x(), iw = r->w();
	int ny = yposition();
	int nx = xposition();

	if (iy < y())
		ny = yposition() + iy - y();
	else if (iy + ih > y() + h() - 16)
		ny = yposition() + iy + ih - (y() + h() - 16);

	if (ix < x())
		nx = xposition() + ix - x();
	else if (ix + iw > x() + w() - 16)
		nx = xposition() + ix + iw - (x() + w() - 16);

	if (nx < 0) nx = 0;
	if (ny < 0) ny = 0;

	if (nx != xposition() || ny != yposition()) {
		hscrollbar.value(nx);
		scrollbar.value(ny);
		scrollbar.do_callback();
	}
}

static void trash_find(TrashScroll *scroll)
{
	const char *term = fl_input(_("Find in Trash:"), "");
	int n, found = 0;
	TrashRow *first_match = NULL;

	if (!term || !*term || !scroll->group) return;
	n = scroll->group->children();

	for (int i = 0; i < n; i++)
		((TrashRow*)scroll->group->child(i))->selected = 0;
	for (int i = 0; i < n; i++) {
		TrashRow *r = (TrashRow*)scroll->group->child(i);
		const char *slash = strrchr(r->original_path, '/');
		const char *base = slash ? slash + 1 : r->original_path;
		if (strhas_ci(base, term)) {
			r->selected = 1;
			if (!first_match) first_match = r;
			found++;
		}
	}
	scroll->group->redraw();
	if (first_match) scroll->ensure_visible(first_match);
	if (!found) fl_message(_("No results for \"%s\""), term);
}

int TrashScroll::handle(int event)
{
	if (event == FL_FOCUS || event == FL_UNFOCUS) return 1;

	if (event == FL_KEYDOWN) {
		int key = Fl::event_key();
		if (!group) return 1;
		int n = group->children();

		if ((Fl::event_state() & FL_CTRL) && key == 'f') {
			trash_find(this);
			return 1;
		}
		if (key == FL_Delete) {
			delete_selected_permanently(group, 1);
			return 1;
		}
		if (key == FL_Enter) {
			restore_selected(group);
			return 1;
		}
		if (key == FL_Down || key == FL_Up || key == FL_Left || key == FL_Right) {
			if (n == 0) return 1;
			int cur = -1;
			for (int i = 0; i < n; i++) {
				if (((TrashRow*)group->child(i))->selected) { cur = i; break; }
			}
			int next = -1;
			if (cur == -1) {
				next = 0;
			} else if (key == FL_Left) {
				next = cur > 0 ? cur - 1 : 0;
			} else if (key == FL_Right) {
				next = cur < n - 1 ? cur + 1 : cur;
			} else {
				TrashRow *cb = (TrashRow*)group->child(cur);
				int group_y = group->y();
				int cur_y = cb->y() - group_y;
				int cur_cx = cb->x() + cb->w() / 2;
				int target_y = -1;
				if (key == FL_Down) {
					for (int i = 0; i < n; i++) {
						int iy = ((TrashRow*)group->child(i))->y() - group_y;
						if (iy > cur_y && (target_y < 0 || iy < target_y))
							target_y = iy;
					}
				} else {
					for (int i = 0; i < n; i++) {
						int iy = ((TrashRow*)group->child(i))->y() - group_y;
						if (iy < cur_y && (target_y < 0 || iy > target_y))
							target_y = iy;
					}
				}
				if (target_y >= 0) {
					int best_dx = -1;
					for (int i = 0; i < n; i++) {
						TrashRow *r = (TrashRow*)group->child(i);
						if (r->y() - group_y == target_y) {
							int dx = r->x() + r->w() / 2 - cur_cx;
							if (dx < 0) dx = -dx;
							if (best_dx < 0 || dx < best_dx) {
								best_dx = dx; next = i;
							}
						}
					}
				} else {
					next = cur;
				}
			}
			for (int i = 0; i < n; i++)
				((TrashRow*)group->child(i))->selected = 0;
			TrashRow *nr = (next >= 0 && next < n) ? (TrashRow*)group->child(next) : NULL;
			if (nr) nr->selected = 1;
			if (nr) ensure_visible(nr);
			group->redraw();
			redraw();
			return 1;
		}
		if (key == FL_Home) {
			if (n > 0) {
				for (int i = 0; i < n; i++)
					((TrashRow*)group->child(i))->selected = 0;
				TrashRow *first = (TrashRow*)group->child(0);
				first->selected = 1;
				ensure_visible(first);
				group->redraw();
				redraw();
			}
			return 1;
		}
		if (key == FL_End) {
			if (n > 0) {
				for (int i = 0; i < n; i++)
					((TrashRow*)group->child(i))->selected = 0;
				TrashRow *last = (TrashRow*)group->child(n - 1);
				last->selected = 1;
				ensure_visible(last);
				group->redraw();
				redraw();
			}
			return 1;
		}
		return 0;
	}

	if (!dragging) {
		int ret = 0;

		if (event == FL_PUSH && (Fl::event_state() & FL_BUTTON1) &&
			!(Fl::event_state() & FL_CTRL))
		{
			if (group) {
				int n = group->children();
				for (int i = 0; i < n; i++)
					((TrashRow*)group->child(i))->selected = 0;
				group->redraw();
			}
		}

		ret = Fl_Scroll::handle(event);

		if (!ret && event == FL_PUSH && (Fl::event_state() & FL_BUTTON3)) {
			Fl::focus(this);
			build_canvas_menu();
			static Fl_Menu_Button *cbtn = NULL;
			if (!cbtn) cbtn = new Fl_Menu_Button(0, 0, 0, 0);
			cbtn->menu(canvas_menu);
			cbtn->popup();
			return 1;
		}
		if (!ret && event == FL_PUSH && (Fl::event_state() & FL_BUTTON1)) {
			Fl::focus(this);
			dragging = 1;
			drag_x = Fl::event_x() + xposition();
			drag_y = Fl::event_y() + yposition();
			return 1;
		}
		if (!ret && event == FL_PUSH) {
			Fl::focus(this);
			return 1;
		}
		return ret;
	} else if (event == FL_RELEASE) {
		dragging = 0;
		if (group) group->hide_selector();
		redraw();
		return 1;
	} else if (event != FL_DRAG) {
		if (!(Fl::event_state() & FL_BUTTON1)) dragging = 0;
		return Fl_Scroll::handle(event);
	}

	if (group) {
		int mx, my, ev_x, ev_y;
		Fl::get_mouse(mx, my);
		mx -= window()->x();
		my -= window()->y();
		ev_x = mx + xposition();
		ev_y = my + yposition();
		draw_selector(ev_x, ev_y);
	}
	return 1;
}

static TrashWindow *g_trash_win = NULL;

static void reload_trash_view()
{
	if (g_trash_win && g_trash_win->shown()) g_trash_win->load_trash();
}

void trash_sync_view()
{
	reload_trash_view();
}

void cb_trash(Fl_Widget*, void*)
{
	if (!g_trash_win) {
		g_trash_win = new TrashWindow();
	}
	g_trash_win->load_trash();
	g_trash_win->show();
	g_trash_win->scroll->take_focus();
}

TrashWindow::TrashWindow() : Fl_Double_Window(520, 360, _("Trash"))
{
	g_trash_win = this;
	scroll = new TrashScroll(0, 0, 520, 360);
	scroll->box(FL_FLAT_BOX);
	end();
	resizable(scroll);
	size_range(240, 200);
}

void TrashWindow::load_trash()
{
	char files_dir[900], info_dir[900];
	int count = 0;
	trash_entry *entries = list_trash_entries(&count);
	int detail = StatesValues.view_detail;

	trash_dirs(files_dir, info_dir, sizeof(files_dir));

	scroll->remove(scroll->group);
	delete scroll->group;

	scroll->begin();
	scroll->group = new TrashRowGroup(scroll->x(), scroll->y(),
		scroll->w(), scroll->h());
	scroll->group->begin();
	if (entries && count > 0) {
		for (int i = 0; i < count; i++) {
			Fl_Image *pix = trash_icon(files_dir, entries[i].id,
				detail ? 16 : ICON_SIZE);
			new TrashRow(scroll->x(), scroll->y(),
				detail ? scroll->w() - 16 : CELL_W,
				detail ? gui->size + 8 : CELL_H,
				entries[i].id, entries[i].original_path,
				entries[i].deleted_time, pix, detail);
			free(entries[i].id);
			free(entries[i].original_path);
		}
	}
	scroll->group->end();

	if (!entries || count == 0) {
		Fl_Box *empty = new Fl_Box(scroll->x() + 10, scroll->y() + 10,
			scroll->w() - 20, 26, _("Trash is empty"));
		empty->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
	}
	scroll->end();
	scroll->group->resizable(NULL);
	scroll->group->relayout(scroll->w());
	free(entries);
}
