#include "BigIcon.h"
#include "callbacks.h"
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Item.H>
#include <libintl.h>
#include <sys/stat.h>

#define _(String) gettext((String))

extern Window fl_dnd_target_window;

static Fl_Pixmap *p_character = NULL;
static Fl_Pixmap *p_folder = NULL;
static Fl_Pixmap *p_link = NULL;
static Fl_Pixmap *p_pipe = NULL;
static Fl_Pixmap *p_exec = NULL;
static Fl_Pixmap *p_block = NULL;
static Fl_Pixmap *p_socket = NULL;
static Fl_Pixmap *p_unknown = NULL;

static Fl_Pixmap *p_deb = NULL;
static Fl_Pixmap *p_gz = NULL;
static Fl_Pixmap *p_html = NULL;
static Fl_Pixmap *p_image = NULL;
static Fl_Pixmap *p_pdf = NULL;
static Fl_Pixmap *p_rpm = NULL;
static Fl_Pixmap *p_src = NULL;
static Fl_Pixmap *p_tar = NULL;
static Fl_Pixmap *p_txt = NULL;
static Fl_Pixmap *p_wav = NULL;

Fl_Menu_Item *BigIcon::m_folder  = NULL;
Fl_Menu_Item *BigIcon::m_exec    = NULL;
Fl_Menu_Item *BigIcon::m_unknown = NULL;
Fl_Menu_Item *BigIcon::m_special = NULL;
Fl_Menu_Item *BigIcon::m_multi   = NULL;
Fl_Menu_Item *BigIcon::m_canvas  = NULL;

Xd6IconWindow *BigIcon::drag_window = NULL;

#include "xpm/deb.xpm"
#include "xpm/gz.xpm"
#include "xpm/html.xpm"
#include "xpm/image.xpm"
#include "xpm/pdf.xpm"
#include "xpm/rpm.xpm"
#include "xpm/src.xpm"
#include "xpm/tar.xpm"
#include "xpm/txt.xpm"
#include "xpm/wav.xpm"
#include "xpm/character.xpm"
#include "xpm/folder.xpm"
#include "xpm/link.xpm"
#include "xpm/exec.xpm"
#include "xpm/block.xpm"
#include "xpm/pipe.xpm"
#include "xpm/socket.xpm"
#include "xpm/unknown.xpm"

Fl_Widget *BigIcon::drag_widget = NULL;

BigIcon::BigIcon(int X, int Y, int W, int H) : 
	Fl_Menu_Button(X, Y, W, H)
{
	width = 0;
	height = 0;
	pix = NULL;
	real_name = NULL;
	selected = 0;
	align(FL_ALIGN_BOTTOM|FL_ALIGN_INSIDE);
	selection_color(137);
	type(1);
}

BigIcon::~BigIcon()
{
}

static void create_pixmaps()
{
	p_character = new Fl_Pixmap(character_xpm);
	p_folder = new Fl_Pixmap(folder_xpm);
	p_link = new Fl_Pixmap(link_xpm);
	p_pipe = new Fl_Pixmap(pipe_xpm);
	p_exec = new Fl_Pixmap(exec_xpm);
	p_block = new Fl_Pixmap(block_xpm);
	p_socket = new Fl_Pixmap(socket_xpm);
	p_unknown = new Fl_Pixmap(unknown_xpm);

	p_deb = new Fl_Pixmap(deb_xpm);
	p_gz = new Fl_Pixmap(gz_xpm);
	p_html = new Fl_Pixmap(html_xpm);
	p_image = new Fl_Pixmap(image_xpm);
	p_pdf = new Fl_Pixmap(pdf_xpm);
	p_rpm = new Fl_Pixmap(rpm_xpm);
	p_src = new Fl_Pixmap(src_xpm);
	p_tar = new Fl_Pixmap(tar_xpm);
	p_txt = new Fl_Pixmap(txt_xpm);
	p_wav = new Fl_Pixmap(wav_xpm);

}

void BigIcon::create_menus()
{
	m_folder  = new Fl_Menu_Item[12];
	m_exec    = new Fl_Menu_Item[12];
	m_unknown = new Fl_Menu_Item[12];
	m_special = new Fl_Menu_Item[12];

	m_folder->text = NULL;
	m_exec->text = NULL;
	m_unknown->text = NULL;
	m_special->text = NULL;

	m_folder->add(_("Open"), 0, cb_open_dir, 0, 0);
	m_folder->add(_("Open Terminal"), 0, cb_terminal, 0, FL_MENU_DIVIDER);
	m_folder->add(_("Copy"), 0, cb_clip_copy, 0, 0);
	m_folder->add(_("Cut"), 0, cb_clip_cut, 0, 0);
	m_folder->add(_("Paste"), 0, cb_clip_paste, 0, 0);
	m_folder->add(_("Rename"), 0, cb_rename, 0, FL_MENU_DIVIDER);
	m_folder->add(_("Delete"), 0, cb_delete_menu, 0, FL_MENU_DIVIDER);
	m_folder->add(_("Properties"), 0, cb_properties, 0, 0);

	m_exec->add(_("Execute"), 0, cb_exec, 0, 0);
	m_exec->add(_("Open with..."), 0, cb_open_width, 0, FL_MENU_DIVIDER);
	m_exec->add(_("Copy"), 0, cb_clip_copy, 0, 0);
	m_exec->add(_("Cut"), 0, cb_clip_cut, 0, 0);
	m_exec->add(_("Paste"), 0, cb_clip_paste, 0, 0);
	m_exec->add(_("Rename"), 0, cb_rename, 0, FL_MENU_DIVIDER);
	m_exec->add(_("Delete"), 0, cb_delete_menu, 0, FL_MENU_DIVIDER);
	m_exec->add(_("Properties"), 0, cb_properties, 0, 0);

	m_unknown->add(_("Open"), 0, cb_open, 0, 0);
	m_unknown->add(_("Open with..."), 0, cb_open_width, 0, FL_MENU_DIVIDER);
	m_unknown->add(_("Copy"), 0, cb_clip_copy, 0, 0);
	m_unknown->add(_("Cut"), 0, cb_clip_cut, 0, 0);
	m_unknown->add(_("Paste"), 0, cb_clip_paste, 0, 0);
	m_unknown->add(_("Rename"), 0, cb_rename, 0, FL_MENU_DIVIDER);
	m_unknown->add(_("Delete"), 0, cb_delete_menu, 0, FL_MENU_DIVIDER);
	m_unknown->add(_("Properties"), 0, cb_properties, 0, 0);

	m_special->add(_("Delete"), 0, cb_delete_menu, 0, FL_MENU_DIVIDER);
	m_special->add(_("Properties"), 0, cb_properties, 0, 0);

	m_multi = new Fl_Menu_Item[8];
	m_multi->text = NULL;
	m_multi->add(_("Copy"),   0, cb_clip_copy,  0, 0);
	m_multi->add(_("Cut"),    0, cb_clip_cut,   0, 0);
	m_multi->add(_("Paste"),  0, cb_clip_paste, 0, FL_MENU_DIVIDER);
	m_multi->add(_("Delete"), 0, cb_delete_menu, 0, 0);

	m_canvas = new Fl_Menu_Item[8];
	m_canvas->text = NULL;
	m_canvas->add(_("Paste"),          0, cb_clip_paste, 0, FL_MENU_DIVIDER);
	m_canvas->add(_("New Directory"), 0, cb_newdir,     0, 0);
	m_canvas->add(_("New File"),      0, cb_newfile,    0, FL_MENU_DIVIDER);
	m_canvas->add(_("Open Terminal"), 0, cb_terminal,   0, FL_MENU_DIVIDER);
	m_canvas->add(_("Rescan"),        0, cb_rescan,     0, 0);

	for (int i = 0; i < 12; i++) {
		m_folder[i].labelfont(gui->font);
		m_folder[i].labelsize(gui->size);
	}
	for (int i = 0; i < 12; i++) {
		m_exec[i].labelfont(gui->font);
		m_exec[i].labelsize(gui->size);
	}
	for (int i = 0; i < 12; i++) {
		m_unknown[i].labelfont(gui->font);
		m_unknown[i].labelsize(gui->size);
	}
	for (int i = 0; i < 12; i++) {
		m_special[i].labelfont(gui->font);
		m_special[i].labelsize(gui->size);
	}
	for (int i = 0; i < 8; i++) {
		m_multi[i].labelfont(gui->font);
		m_multi[i].labelsize(gui->size);
	}
	for (int i = 0; i < 8; i++) {
		m_canvas[i].labelfont(gui->font);
		m_canvas[i].labelsize(gui->size);
	}
}

void BigIcon::destroy_menus()
{
	delete[] m_folder;  m_folder  = NULL;
	delete[] m_exec;    m_exec    = NULL;
	delete[] m_unknown; m_unknown = NULL;
	delete[] m_special; m_special = NULL;
	delete[] m_multi;   m_multi   = NULL;
	delete[] m_canvas;  m_canvas  = NULL;
}

void BigIcon::create_drag_window()
{
	drag_window = new Xd6IconWindow(NULL, NULL, 32, 32);
	drag_window->end();
	drag_window->show();
	drag_window->handle(FL_HIDE);
	drag_window->position(100, 100);
}

void reset_selection()
{
        int i;
        Fl_Widget*const* a;
        BigIcon *b;

        if (!gui || !gui->icon_can || 
		!((IconCanvas*)gui->icon_can)->group) return;

        a = ((IconCanvas*)gui->icon_can)->group->array();
        i = ((IconCanvas*)gui->icon_can)->group->children();

        while (i > 0) {
                i--;
                b = (BigIcon*) a[i];
                if (b && b->selected) {
                        b->selected = 0;
                }
        }
	((IconCanvas*)gui->icon_can)->group->redraw();
}

static Fl_Pixmap *get_pixmap_by_name(const char *name) 
{
	Fl_Pixmap *pix = p_unknown;
	int len;
	
	len = strlen(name);
	if (len > 2 && name[len - 2] == '.') {
		const char *ptr = name + len - 1;
		if (ptr[0] == 'c' || ptr[0] == 'C' || ptr[0] == 'h' ||
			ptr[0] == 'H')
		{
			pix = p_src;
		}
	} else if (len > 3 && name[len - 3] == '.') {
		const char *ptr = name + len - 2;

		if (ptr[0] == 'g' && ptr[1] == 'z') {
			pix = p_gz;
		} else if (ptr[0] == 'a' && ptr[1] == 'u') {
			pix = p_wav;
		} else if (ptr[0] == 'p' && ptr[1] == 'l') {
			pix = p_src;
		} else if (ptr[0] == 'c' && ptr[1] == 'c') {
			pix = p_src;
		}
	} else if (len > 4 && name[len - 4] == '.') {
		const char *ptr = name + len - 3;

		if (ptr[0] == 'p' && ptr[1] == 'n' && ptr[2] == 'g') {
			pix = p_image;
		} else if (ptr[0] == 'g' && ptr[1] == 'i' && ptr[2] == 'f') {
			pix = p_image;
		} else if (ptr[0] == 'j' && ptr[1] == 'p' && ptr[2] == 'g') {
			pix = p_image;
		} else if (ptr[0] == 'e' && ptr[1] == 'p' && ptr[2] == 's') {
			pix = p_image;
		} else if (ptr[0] == 's' && ptr[1] == 'v' && ptr[2] == 'g') {
			pix = p_image;
		} else if (ptr[0] == 'b' && ptr[1] == 'm' && ptr[2] == 'p') {
			pix = p_image;
		} else if (ptr[0] == 'x' && ptr[1] == 'b' && ptr[2] == 'm') {
			pix = p_image;
		} else if (ptr[0] == 'x' && ptr[1] == 'c' && ptr[2] == 'f') {
			pix = p_image;
		} else if (ptr[0] == 'x' && ptr[1] == 'p' && ptr[2] == 'm') {
			pix = p_image;
		} else if (ptr[0] == 'c' && ptr[1] == 'p' && ptr[2] == 'p') {
			pix = p_src;
		} else if (ptr[0] == 'c' && ptr[1] == 'x' && ptr[2] == 'x') {
			pix = p_src;
		} else if (ptr[0] == 'a' && ptr[1] == 's' && ptr[2] == 'm') {
			pix = p_src;
		} else if (ptr[0] == 'h' && ptr[1] == 't' && ptr[2] == 'm') {
			pix = p_html;
		} else if (ptr[0] == 't' && ptr[1] == 'x' && ptr[2] == 't') {
			pix = p_txt;
		} else if (ptr[0] == 't' && ptr[1] == 'g' && ptr[2] == 'z') {
			pix = p_tar;
		} else if (ptr[0] == 't' && ptr[1] == 'a' && ptr[2] == 'r') {
			pix = p_tar;
		} else if (ptr[0] == 'd' && ptr[1] == 'e' && ptr[2] == 'b') {
			pix = p_deb;
		} else if (ptr[0] == 't' && ptr[1] == 'c' && ptr[2] == 'z') {
			pix = p_tar;
		} else if (ptr[0] == 'r' && ptr[1] == 'p' && ptr[2] == 'm') {
			pix = p_rpm;
		} else if (ptr[0] == 'p' && ptr[1] == 'd' && ptr[2] == 'f') {
			pix = p_pdf;
		} else if (ptr[0] == 'w' && ptr[1] == 'a' && ptr[2] == 'v') {
			pix = p_wav;
		} else if (ptr[0] == 'm' && ptr[1] == 'p' && ptr[2] == '3') {
			pix = p_wav;
		}
	} else if (len > 5 && name[len - 5] == '.') {
		const char *ptr = name + len - 4;

		if (ptr[0] == 'h' && ptr[1] == 't' && ptr[2] == 'm' && 
			ptr[3] == 'l') 
		{
			pix = p_html;
		} else if (ptr[0] == 'j' && ptr[1] == 'p' && ptr[2] == 'e' && 
			ptr[3] == 'g') 
		{
			pix = p_image;
		}
	} else {
		if (!strcmp("README", name)) {
			pix = p_txt;
		} else if (!strcmp("COPYING", name)) {
			pix = p_txt;
		}
	}
	return pix;
}

Fl_Pixmap *resolve_icon_for_path(const char *path)
{
	struct stat st;
	const char *base;

	if (!p_link) create_pixmaps();

	if (lstat(path, &st) != 0) return p_unknown;
	if (S_ISLNK(st.st_mode)) return p_link;
	if (S_ISDIR(st.st_mode)) return p_folder;
	if (S_ISSOCK(st.st_mode)) return p_socket;
	if (S_ISBLK(st.st_mode)) return p_block;
	if (S_ISCHR(st.st_mode)) return p_character;
	if (S_ISFIFO(st.st_mode)) return p_pipe;
	if (st.st_mode & S_IXUSR) return p_exec;

	base = strrchr(path, '/');
	base = base ? base + 1 : path;
	return get_pixmap_by_name(base);
}

void BigIcon::set_data(struct file_info *fi)
{
	struct stat st;

	fl_font(gui->font, gui->size);
	width = (int) fl_width(fi->real_name) + 8;
	if (width < 48) width = 48;
	
	height = labelsize() + 8 + 32;
	
	real_name = fi->real_name;
	label(fi->real_name);	

	if (!p_link) {
		create_pixmaps();
	}
	if (!m_exec) {
		create_menus();
	}
	if (!drag_window) {
		window()->end();
		create_drag_window();
		((Fl_Group*)parent())->begin();
	}

	switch(fi->st.st_mode & S_IFMT) {
	case S_IFSOCK:
		menu(m_special);
		pix = p_socket; break;
	case S_IFLNK:
		if (!stat(real_name, &st)) {
			switch(st.st_mode & S_IFMT) {
			case S_IFREG:
				if (st.st_mode & S_IXUSR) {
					menu(m_exec);
					callback(cb_exec);
				} else {
					menu(m_unknown);
					callback(cb_open);
				}
				break;
			case S_IFDIR:
				callback(cb_change_dir);
				menu(m_folder); break;
			default:
				menu(m_special);
			}
		} else {
			menu(m_special);
		}
		pix = p_link; break;
	case S_IFREG:
		if (fi->st.st_mode & S_IXUSR) {
			menu(m_exec);
			pix = p_exec; 
			callback(cb_exec);
		} else {
			menu(m_unknown);
			pix = get_pixmap_by_name(real_name);
			callback(cb_open);
		}
		break;
	case S_IFBLK:
		menu(m_special);
		pix = p_block; break;
	case S_IFDIR:
		menu(m_folder);
		callback(cb_change_dir);
		pix = p_folder; break;
	case S_IFCHR:
		menu(m_special);
		pix = p_character; break;
	case S_IFIFO:
		menu(m_special);
		pix = p_pipe; break;
	default:
		menu(m_special);
		pix = p_unknown;		
	}

	resize(x(), y(), width, height);
}

int BigIcon::is_inside()
{
	int ix;

	ix = x() + (w() - 32) / 2;
	if (Fl::event_x() >= ix && Fl::event_x() <= ix + 32 &&
		Fl::event_y() >= y() && Fl::event_y() <= y() + 32)
	{
		return 1;
	}
	return 0;
}

int BigIcon::handle(int e)
{
	static int px, py;
	static int dragging = 0;
	static char *buffer = NULL;
	static Window last_window = 0;
	
	if (e == FL_PUSH) {
		if (!is_inside()) {
			((IconCanvas*)gui->icon_can)->update_status();
			return 0;
		}
		if (Fl::event_state() & FL_BUTTON1 &&
			!(Fl::get_key(FL_Control_L) ||
				Fl::get_key(FL_Control_R)))
		{
			reset_selection();
		}
		selected = 1;
		((IconCanvas*)gui->icon_can)->update_status();
		if (menu() && Fl::event_state() & FL_BUTTON3) {
			Fl::event_clicks(0);
			int sel_count = 0;
			if (gui && gui->icon_can &&
				((IconCanvas*)gui->icon_can)->group) {
				int n = ((IconCanvas*)gui->icon_can)->group->children();
				Fl_Widget*const* arr = ((IconCanvas*)gui->icon_can)->group->array();
				for (int i = 0; i < n; i++) {
					if (((BigIcon*)arr[i])->selected) sel_count++;
				}
			}
			if (sel_count > 1 && m_multi) {
				static Fl_Menu_Button *mbtn = NULL;
				if (!mbtn) mbtn = new Fl_Menu_Button(0, 0, 0, 0);
				mbtn->menu(m_multi);
				mbtn->popup();
			} else {
				popup();
			}
			return 1;
		} else if (Fl::event_state() & FL_BUTTON1 && 
			Fl::event_clicks() > 0) 
		{
			Fl::event_clicks(0);
			redraw();
			Fl::flush();
			do_callback();
			redraw();
			return 1;
		}
		
		px = Fl::event_x_root();
		py = Fl::event_y_root();
		redraw();
		return 1;
	}
	switch(e) {
	case FL_ENTER:
		if (is_inside()) {
			struct stat st;
			char sbuf[256];
			if (real_name && stat(real_name, &st) == 0) {
				snprintf(sbuf, sizeof(sbuf), " %s  (%ld kb)",
					real_name, (long)(st.st_size / 1024));
			} else if (real_name) {
				snprintf(sbuf, sizeof(sbuf), " %s", real_name);
			} else {
				sbuf[0] = '\0';
			}
			gui->stat_bar->value(sbuf);
			return 1;
		}
		return 0;
	case FL_PUSH:
		break;
	case FL_DRAG:
  		if (px - 5 >= Fl::event_x_root() ||
                        px + 5 <= Fl::event_x_root() ||
                        py - 5 >= Fl::event_y_root() ||
                        py + 5 <= Fl::event_y_root())
                {
			px = -10;
			py = -10;
		} else {
			return selected;
		}

		if (selected && !dragging) {
			dragging = 1;
			last_window = 0;
			drag_window->position(Fl::event_x_root() + 2,
				Fl::event_y_root() + 2);
			drag_window->show();
			free(buffer);
			buffer = get_selected_urls();
			Fl::copy(buffer, strlen(buffer), 0);
			if (Fl::event_state() & FL_BUTTON1) {
				fl_XdndActionCopy = XdndActionCopy;
			} else {
				fl_XdndActionCopy = XdndActionAsk;
			}
			drag_widget = this;
			Fl::dnd();
			drag_widget = NULL;
			fl_XdndActionCopy = XdndActionCopy;
			drag_window->handle(FL_HIDE);
			XUnmapWindow(fl_display, drag_window->win);
			reset_selection();
			dragging = 0;
			return 1;
		}
		return 0;
	case FL_RELEASE:
		return 1;
	case FL_DND_LEAVE:
		return 1;
	case 0:
		if (!(Fl::event_state() & (FL_BUTTON1|FL_BUTTON2|FL_BUTTON2))) {
			return 0;
		}
		if (!dragging) return 0;
		drag_window->position(Fl::event_x_root() + 2,
				Fl::event_y_root() + 2);
		if (fl_xevent->type == ClientMessage) {
			XClientMessageEvent message = fl_xevent->xclient;
			if (message.message_type == fl_XdndStatus) {
				if (!(message.data.l[1] & 0x1) ||
					message.data.l[4] == 0)
				{
					Fl::first_window()->
						cursor((Fl_Cursor)21);	
					return 1;	
				}
				if (fl_XdndActionCopy == XdndActionAsk) {
					Fl::first_window()->
						cursor((Fl_Cursor)47);
				} else {
					Fl::first_window()->
						cursor((Fl_Cursor)18);
				}
			}
		}
		return 0;
	default:
		break;
	}
	return Fl_Menu_Button::handle(e);
}

void BigIcon::draw()
{
	if (!damage()) return;
	draw_box(FL_FLAT_BOX, color());
	if (selected) {
		fl_color(selection_color());
		fl_rectf(x() + (w() / 2) - 16, y(), 32, 32);
	}
	if (!pix) return;
	pix->draw(x() + (w() / 2) - 16, y());
	draw_label();
}

