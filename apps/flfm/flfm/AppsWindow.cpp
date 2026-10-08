#include "AppsWindow.h"
#include "callbacks.h"
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <libintl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "xpm/exec.xpm"

#define _(String) gettext((String))

#define CELL_W 80
#define CELL_H 76
#define ICON_SIZE 32

#define APPS_DIR "/usr/local/share/applications"
#define PIXMAPS_DIR "/usr/local/share/pixmaps"

struct desktop_entry {
	char *name;
	char *icon;
	char *exec;
	char *desktop_path;
};

static char *clean_exec(const char *raw)
{
	char *out = (char*) malloc(strlen(raw) + 1);
	char *o = out;

	while (*raw) {
		if (*raw == '%' && raw[1]) {
			raw += 2;
			continue;
		}
		*o++ = *raw++;
	}
	*o = '\0';
	while (o > out && *(o - 1) == ' ') *(--o) = '\0';
	return out;
}

static int same_file_content(const char *a, const char *b)
{
	struct stat sa, sb;
	FILE *fa, *fb;
	char bufa[4096], bufb[4096];
	size_t na, nb;
	int same = 1;

	if (stat(a, &sa) != 0 || stat(b, &sb) != 0) return 0;
	if (sa.st_size != sb.st_size) return 0;

	fa = fopen(a, "rb");
	fb = fopen(b, "rb");
	if (!fa || !fb) {
		if (fa) fclose(fa);
		if (fb) fclose(fb);
		return 0;
	}
	while (same) {
		na = fread(bufa, 1, sizeof(bufa), fa);
		nb = fread(bufb, 1, sizeof(bufb), fb);
		if (na != nb || memcmp(bufa, bufb, na) != 0) same = 0;
		if (na == 0) break;
	}
	fclose(fa);
	fclose(fb);
	return same;
}

static int is_generic_logo(const char *path)
{
	const char *base = strrchr(path, '/');
	base = base ? base + 1 : path;
	if (!strcmp(base, "logo.png") || !strcmp(base, "logo.xpm")) return 1;
	return same_file_content(path, PIXMAPS_DIR "/logo.png");
}

static char *resolve_icon(const char *icon, const char *full_path_icon)
{
	char path[1024];
	char *result = NULL;

	if (icon && !strcmp(icon, "logo")) {
		return NULL;
	}
	if (full_path_icon && full_path_icon[0]) {
		result = strdup(full_path_icon);
	} else if (icon && icon[0] == '/') {
		if (access(icon, R_OK) == 0) result = strdup(icon);
	} else if (icon && icon[0]) {
		snprintf(path, sizeof(path), "%s/%s.png", PIXMAPS_DIR, icon);
		if (access(path, R_OK) == 0) {
			result = strdup(path);
		} else {
			snprintf(path, sizeof(path), "%s/%s.xpm", PIXMAPS_DIR, icon);
			if (access(path, R_OK) == 0) result = strdup(path);
		}
	}

	if (result && is_generic_logo(result)) {
		free(result);
		return NULL;
	}
	return result;
}

static char *resolve_binary(const char *exec)
{
	char prog[512];
	char cmd[768];
	char out[1024];
	FILE *p;
	int i = 0;

	while (exec[i] && exec[i] != ' ' && i < (int)sizeof(prog) - 1) {
		prog[i] = exec[i];
		i++;
	}
	prog[i] = '\0';
	if (!prog[0]) return strdup(_("(unknown)"));
	if (prog[0] == '/') {
		if (access(prog, X_OK) == 0) return strdup(prog);
		return strdup(_("(not found)"));
	}

	snprintf(cmd, sizeof(cmd), "which %s 2>/dev/null", prog);
	p = popen(cmd, "r");
	if (!p) return strdup(_("(not found)"));
	out[0] = '\0';
	if (fgets(out, sizeof(out), p)) {
		int l = strlen(out);
		while (l > 0 && (out[l - 1] == '\n' || out[l - 1] == '\r')) {
			out[--l] = '\0';
		}
	}
	pclose(p);
	if (!out[0]) return strdup(_("(not found in PATH)"));
	return strdup(out);
}

static void build_detail_info(const char *path, char *info, size_t len)
{
	struct stat st;
	char perm[] = "----------";
	char *date;

	if (stat(path, &st) != 0) {
		snprintf(info, len, "%s", _("(not found)"));
		return;
	}
	if (S_ISDIR(st.st_mode)) perm[0] = 'd';
	if (st.st_mode & S_IRUSR) perm[1] = 'r';
	if (st.st_mode & S_IWUSR) perm[2] = 'w';
	if (st.st_mode & S_IXUSR) perm[3] = 'x';
	if (st.st_mode & S_IRGRP) perm[4] = 'r';
	if (st.st_mode & S_IWGRP) perm[5] = 'w';
	if (st.st_mode & S_IXGRP) perm[6] = 'x';
	if (st.st_mode & S_IROTH) perm[7] = 'r';
	if (st.st_mode & S_IWOTH) perm[8] = 'w';
	if (st.st_mode & S_IXOTH) perm[9] = 'x';

	date = ctime(&st.st_mtime);
	if (date) date[strlen(date) - 1] = '\0';
	snprintf(info, len, "%s  (%s)", perm, date ? date : "?");
}

static int name_sort(const void *a, const void *b)
{
	const desktop_entry *ea = (const desktop_entry*) a;
	const desktop_entry *eb = (const desktop_entry*) b;
	return strcmp(ea->name, eb->name);
}

static int size_sort(const void *a, const void *b)
{
	const desktop_entry *ea = (const desktop_entry*) a;
	const desktop_entry *eb = (const desktop_entry*) b;
	struct stat sa, sb;
	long za = 0, zb = 0;

	if (stat(ea->desktop_path, &sa) == 0) za = sa.st_size;
	if (stat(eb->desktop_path, &sb) == 0) zb = sb.st_size;
	return (int)(zb - za);
}

static desktop_entry *parse_desktop_apps(int *count, const char *filter)
{
	DIR *dir;
	struct dirent *de;
	desktop_entry *entries;
	int cap = 32;
	int n = 0;

	*count = 0;
	dir = opendir(APPS_DIR);
	if (!dir) return NULL;

	entries = (desktop_entry*) malloc(sizeof(desktop_entry) * cap);

	while ((de = readdir(dir))) {
		char path[1024];
		char line[1024];
		FILE *f;
		char *name = NULL;
		char *icon = NULL;
		char *exec = NULL;
		char *full_icon = NULL;
		char *type = NULL;
		int nodisplay = 0;
		int sections = 0;
		int len = strlen(de->d_name);

		if (len < 9 || strcmp(de->d_name + len - 8, ".desktop") != 0) {
			continue;
		}
		snprintf(path, sizeof(path), "%s/%s", APPS_DIR, de->d_name);
		f = fopen(path, "r");
		if (!f) continue;

		while (fgets(line, sizeof(line), f)) {
			char *p = line;
			int l = strlen(p);
			while (l > 0 && (p[l - 1] == '\n' || p[l - 1] == '\r')) {
				p[--l] = '\0';
			}
			if (p[0] == '[') {
				sections++;
				if (sections > 1) break;
				continue;
			}
			if (!strncmp(p, "Name=", 5) && !name) {
				name = strdup(p + 5);
			} else if (!strncmp(p, "Icon=", 5) && !icon) {
				icon = strdup(p + 5);
			} else if (!strncmp(p, "Exec=", 5) && !exec) {
				exec = strdup(p + 5);
			} else if (!strncmp(p, "X-FullPathIcon=", 15) && !full_icon) {
				full_icon = strdup(p + 15);
			} else if (!strncmp(p, "Type=", 5) && !type) {
				type = strdup(p + 5);
			} else if (!strncmp(p, "NoDisplay=true", 14)) {
				nodisplay = 1;
			}
		}
		fclose(f);

		if (!nodisplay && name && exec && (!type || !strcmp(type, "Application")) &&
			(!filter || !filter[0] || strhas_ci(name, filter)))
		{
			if (n >= cap) {
				cap *= 2;
				entries = (desktop_entry*) realloc(entries,
					sizeof(desktop_entry) * cap);
			}
			entries[n].name = name;
			entries[n].icon = resolve_icon(icon, full_icon);
			entries[n].exec = clean_exec(exec);
			entries[n].desktop_path = strdup(path);
			n++;
		} else {
			free(name);
		}
		free(icon);
		free(exec);
		free(full_icon);
		free(type);
	}
	closedir(dir);

	if (StatesValues.sort_size) {
		qsort(entries, n, sizeof(desktop_entry), size_sort);
	} else {
		qsort(entries, n, sizeof(desktop_entry), name_sort);
	}

	*count = n;
	return entries;
}

static Fl_Image *load_app_icon(const char *path, int size)
{
	static int images_ready = 0;
	Fl_Image *img;

	if (!images_ready) {
		fl_register_images();
		images_ready = 1;
	}
	img = path ? Fl_Shared_Image::get(path) : NULL;
	if (!img) {
		img = new Fl_Pixmap(exec_xpm);
	}
	if (img->w() != size || img->h() != size) {
		return img->copy(size, size);
	}
	return img;
}

static void cb_app_open(Fl_Widget *w, void*)
{
	((AppIcon*)w)->open_app();
}

static void cb_app_properties(Fl_Widget *w, void*)
{
	((AppIcon*)w)->show_properties();
}

static Fl_Menu_Item *app_menu = NULL;

static void build_app_menu()
{
	if (app_menu) return;
	app_menu = new Fl_Menu_Item[3];
	app_menu->text = NULL;
	app_menu->add(_("Open"), 0, cb_app_open, 0, 0);
	app_menu->add(_("Properties"), 0, cb_app_properties, 0, 0);
}

AppIcon::AppIcon(int X, int Y, int W, int H, const char *t,
	const char *c, const char *ip, const char *dp, Fl_Image *p, int d)
	: Fl_Menu_Button(X, Y, W, H)
{
	title = strdup(t);
	cmd = strdup(c);
	icon_path = ip ? strdup(ip) : NULL;
	desktop_path = dp ? strdup(dp) : NULL;
	pix = p;
	selected = 0;
	detail_mode = d;
	icon_size = pix ? pix->w() : ICON_SIZE;
	align(d ? (FL_ALIGN_LEFT|FL_ALIGN_INSIDE) :
		(FL_ALIGN_BOTTOM|FL_ALIGN_INSIDE));
	selection_color(137);
	type(1);
	box(FL_FLAT_BOX);
	label(title);
	build_app_menu();
	menu(app_menu);

	info[0] = '\0';
	info_len = 0;
	offset = 200;
	if (detail_mode) {
		build_detail_info(desktop_path ? desktop_path : "", info,
			sizeof(info));
		info_len = strlen(info);
		fl_font(labelfont(), labelsize());
		offset = (int) fl_width(title) + icon_size + 24;
		if (offset < 200) offset = 200;
		width = W;
		height = H;
	} else {
		fl_font(labelfont(), labelsize());
		width = (int) fl_width(title) + 8;
		if (width < 48) width = 48;
		height = labelsize() + 8 + icon_size;
	}
}

AppIcon::~AppIcon()
{
	free(title);
	free(cmd);
	free(icon_path);
	free(desktop_path);
}

int AppIcon::handle(int e)
{
	switch (e) {
	case FL_PUSH:
		if (Fl::event_state() & FL_BUTTON3) {
			Fl::event_clicks(0);
			popup();
			return 1;
		}
		if (Fl::event_clicks() > 0) {
			Fl::event_clicks(0);
			open_app();
			return 1;
		}
		if (parent()) {
			int n = parent()->children();
			for (int i = 0; i < n; i++) {
				AppIcon *a = (AppIcon*)parent()->child(i);
				if (a != this && a->selected) {
					a->selected = 0;
					a->redraw();
				}
			}
			Fl::focus(parent());
		}
		selected = 1;
		redraw();
		return 1;
	case FL_RELEASE:
		return 1;
	default:
		break;
	}
	return Fl_Menu_Button::handle(e);
}

void AppIcon::draw()
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
		fl_draw(title, x() + icon_size + 12,
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

void AppIcon::open_app()
{
	char buf[2048];
	snprintf(buf, sizeof(buf), "%s &", cmd);
	system(buf);
	if (window()) window()->hide();
}

void AppIcon::show_properties()
{
	char *binary = resolve_binary(cmd);
	char info[2048];

	snprintf(info, sizeof(info), "%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s",
		_("Name"), title,
		_("Command"), cmd,
		_("Binary"), binary,
		_("Icon"), icon_path ? icon_path : _("(no icon)"),
		_("Desktop file"), desktop_path ? desktop_path : "");
	fl_message("%s", info);
	free(binary);
}

AppsIconGroup::AppsIconGroup(int X, int Y, int W, int H) : Fl_Group(X, Y, W, H)
{
	box(FL_FLAT_BOX);
}

void AppsIconGroup::relayout(int neww)
{
	int n = children();
	int ww, hh;

	if (StatesValues.view_detail) {
		int row_h = gui->size + 4;
		if (row_h < 18) row_h = 18;
		int row_w = neww - 16;
		for (int i = 0; i < n; i++) {
			AppIcon *a = (AppIcon*)child(i);
			a->resize(x(), y() + i * row_h, row_w, row_h);
		}
		ww = neww;
		hh = n * row_h + 15;
	} else {
		int xx = x() + 15;
		int yy = y() + 15;
		int mx = neww + x() - 30;
		int lineh = 0;

		for (int i = 0; i < n; i++) {
			AppIcon *a = (AppIcon*)child(i);
			if (a->height > lineh) lineh = a->height;
			if (xx + a->width > mx) {
				yy += lineh + 15;
				lineh = 0;
				xx = x() + 15;
			}
			a->resize(xx, yy, a->width, a->height);
			xx += a->width + 15;
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

AppsScroll::AppsScroll(int X, int Y, int W, int H) : Fl_Scroll(X, Y, W, H)
{
	group = NULL;
	type(BOTH_ALWAYS);
	box(FL_FLAT_BOX);
}

void AppsScroll::resize(int X, int Y, int W, int H)
{
	int oldw = w();
	Fl_Scroll::resize(X, Y, W, H);
	if (group && W != oldw && !StatesValues.view_detail) group->relayout(W);
}

void AppsScroll::ensure_visible(AppIcon *a)
{
	if (!a) return;
	int iy = a->y(), ih = a->h();
	int ix = a->x(), iw = a->w();
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

int AppsScroll::handle(int event)
{
	if (event == FL_FOCUS || event == FL_UNFOCUS) return 1;
	if (event == FL_KEYDOWN) {
		int key = Fl::event_key();
		if (!group) return 1;
		int n = group->children();

		if (key == FL_Enter) {
			for (int i = 0; i < n; i++) {
				AppIcon *a = (AppIcon*)group->child(i);
				if (a->selected) { a->open_app(); break; }
			}
			return 1;
		}
		if (key == FL_Down || key == FL_Up || key == FL_Left || key == FL_Right) {
			if (n == 0) return 1;
			int cur = -1;
			for (int i = 0; i < n; i++) {
				if (((AppIcon*)group->child(i))->selected) { cur = i; break; }
			}
			int next = -1;
			if (cur == -1) {
				next = 0;
			} else if (key == FL_Left) {
				next = cur > 0 ? cur - 1 : 0;
			} else if (key == FL_Right) {
				next = cur < n - 1 ? cur + 1 : cur;
			} else {
				/* UP/DOWN: navigate by row using group-relative y (scroll-stable) */
				AppIcon *cb = (AppIcon*)group->child(cur);
				int group_y = group->y();
				int cur_y = cb->y() - group_y;
				int cur_cx = cb->x() + cb->w() / 2;
				int target_y = -1;
				if (key == FL_Down) {
					for (int i = 0; i < n; i++) {
						int iy = ((AppIcon*)group->child(i))->y() - group_y;
						if (iy > cur_y && (target_y < 0 || iy < target_y))
							target_y = iy;
					}
				} else {
					for (int i = 0; i < n; i++) {
						int iy = ((AppIcon*)group->child(i))->y() - group_y;
						if (iy < cur_y && (target_y < 0 || iy > target_y))
							target_y = iy;
					}
				}
				if (target_y >= 0) {
					int best_dx = -1;
					for (int i = 0; i < n; i++) {
						AppIcon *a = (AppIcon*)group->child(i);
						if (a->y() - group_y == target_y) {
							int dx = a->x() + a->w() / 2 - cur_cx;
							if (dx < 0) dx = -dx;
							if (best_dx < 0 || dx < best_dx) {
								best_dx = dx; next = i;
							}
						}
					}
				} else {
					next = cur; /* already at first/last row */
				}
			}
			for (int i = 0; i < n; i++)
				((AppIcon*)group->child(i))->selected = 0;
			AppIcon *nb = (next >= 0 && next < n) ? (AppIcon*)group->child(next) : NULL;
			if (nb) nb->selected = 1;
			if (nb) ensure_visible(nb);
			/* redraw AFTER scroll so FL_DAMAGE_ALL overrides FL_DAMAGE_SCROLL blit */
			group->redraw();
			redraw();
			return 1;
		}
		if (key == FL_Home) {
			if (n > 0) {
				for (int i = 0; i < n; i++)
					((AppIcon*)group->child(i))->selected = 0;
				AppIcon *first = (AppIcon*)group->child(0);
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
					((AppIcon*)group->child(i))->selected = 0;
				AppIcon *last = (AppIcon*)group->child(n - 1);
				last->selected = 1;
				ensure_visible(last);
				group->redraw();
				redraw();
			}
			return 1;
		}
		return 0;
	}

	if (event == FL_PUSH && (Fl::event_state() & FL_BUTTON1) &&
		!(Fl::event_state() & FL_CTRL))
	{
		if (group) {
			int n = group->children();
			for (int i = 0; i < n; i++)
				((AppIcon*)group->child(i))->selected = 0;
			group->redraw();
		}
	}

	int ret = Fl_Scroll::handle(event);

	if (!ret && event == FL_PUSH) {
		Fl::focus(this);
		return 1;
	}
	return ret;
}

static void cb_search_changed(Fl_Widget *w, void *data)
{
	((AppsWindow*)data)->load_apps();
}

AppsSearchInput::AppsSearchInput(int X, int Y, int W, int H) : Fl_Input(X, Y, W, H)
{
}

int AppsSearchInput::handle(int e)
{
	if (e == FL_KEYDOWN) {
		int key = Fl::event_key();
		if (key == FL_Up || key == FL_Down || key == FL_Left ||
			key == FL_Right || key == FL_Enter)
		{
			AppsWindow *win = (AppsWindow*) window();
			if (win && win->scroll) return win->scroll->handle(e);
		}
	}
	return Fl_Input::handle(e);
}

AppsWindow::AppsWindow() : Fl_Double_Window(520, 360, _("Apps"))
{
	search = new AppsSearchInput(0, 0, 520, 26);
	search->callback(cb_search_changed, this);
	search->when(FL_WHEN_CHANGED);
	scroll = new AppsScroll(0, 26, 520, 334);
	scroll->box(FL_FLAT_BOX);
	end();
	resizable(scroll);
	size_range(240, 200);
}

void AppsWindow::load_apps()
{
	int count = 0;
	desktop_entry *entries = parse_desktop_apps(&count, search->value());
	int detail = StatesValues.view_detail;

	scroll->remove(scroll->group);
	delete scroll->group;

	scroll->begin();
	scroll->group = new AppsIconGroup(scroll->x(), scroll->y(),
		scroll->w(), scroll->h());
	scroll->group->begin();
	if (entries && count > 0 && detail) {
		for (int i = 0; i < count; i++) {
			Fl_Image *pix = load_app_icon(entries[i].icon, 16);
			new AppIcon(scroll->x(), scroll->y(), 200, 24,
				entries[i].name, entries[i].exec,
				entries[i].icon, entries[i].desktop_path, pix, 1);
			free(entries[i].name);
			free(entries[i].icon);
			free(entries[i].exec);
			free(entries[i].desktop_path);
		}
	} else if (entries && count > 0) {
		for (int i = 0; i < count; i++) {
			Fl_Image *pix = load_app_icon(entries[i].icon, ICON_SIZE);
			new AppIcon(scroll->x(), scroll->y(), CELL_W, CELL_H,
				entries[i].name, entries[i].exec,
				entries[i].icon, entries[i].desktop_path, pix, 0);
			free(entries[i].name);
			free(entries[i].icon);
			free(entries[i].exec);
			free(entries[i].desktop_path);
		}
	}
	scroll->group->end();

	if (!entries || count == 0) {
		Fl_Box *empty = new Fl_Box(scroll->x() + 10, scroll->y() + 10,
			scroll->w() - 20, 26,
			_("No applications registered in " APPS_DIR));
		empty->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP);
	}
	scroll->end();
	scroll->group->resizable(NULL);
	scroll->group->relayout(scroll->w());
	free(entries);

	if (search->value()[0] && scroll->group->children() > 0) {
		AppIcon *first = (AppIcon*) scroll->group->child(0);
		first->selected = 1;
		scroll->ensure_visible(first);
	}
}
