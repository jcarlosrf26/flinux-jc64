#include "gui.h"
#include "callbacks.h"
#include "xd640/Xd6ConfigFile.h"
#include "xd640/Xd6DefaultFonts.h"
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include "xd640/Xd6IconWindow.h"
#include "fltk_internals.h"

#include "xpm/flfm.xpm"

GUI *gui;
Xd6ConfigFile *cfg;
Xd6ConfigFileSection *cfg_sec;
Atom XdndActionAsk;
Atom XdndActionMove;
Atom XdndActionLink;
Atom XdndActionCopy;

static void restore_values()
{
	Xd6ConfigFileItem *itm;
	const char *val;
        itm = cfg_sec->get_item("sort_size", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) StatesValues.sort_size = (int) strtoul(val, NULL, 0);
       
        itm = cfg_sec->get_item("sort_type", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) StatesValues.sort_type = (int) strtoul(val, NULL, 0);

        itm = cfg_sec->get_item("sort_name", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) StatesValues.sort_name = (int) strtoul(val, NULL, 0);

        itm = cfg_sec->get_item("view_icon", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) StatesValues.view_icon = (int) strtoul(val, NULL, 0);

        itm = cfg_sec->get_item("view_detail", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) StatesValues.view_detail = (int) strtoul(val, NULL, 0);

        itm = cfg_sec->get_item("show_hide", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) StatesValues.show_hide = (int) strtoul(val, NULL, 0);

        itm = cfg_sec->get_item("app_text", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) {
            if (strcmp(val, "flwrite") == 0 || strcmp(val, "flwriter") == 0)
                val = "editor";
            strncpy(app_text, val, 255); app_text[255] = '\0';
        }

        itm = cfg_sec->get_item("app_image", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) { strncpy(app_image, val, 255); app_image[255] = '\0'; }

        itm = cfg_sec->get_item("app_video", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) { strncpy(app_video, val, 255); app_video[255] = '\0'; }

        itm = cfg_sec->get_item("app_audio", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) { strncpy(app_audio, val, 255); app_audio[255] = '\0'; }

        itm = cfg_sec->get_item("app_html", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) { strncpy(app_html, val, 255); app_html[255] = '\0'; }

        itm = cfg_sec->get_item("app_term", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) { strncpy(app_term, val, 255); app_term[255] = '\0'; }

        itm = cfg_sec->get_item("app_other", NULL);
        val = NULL; if (itm) val = itm->get_value();
        if (val) {
            if (strcmp(val, "flwrite") == 0 || strcmp(val, "flwriter") == 0)
                val = "editor";
            strncpy(app_other, val, 255); app_other[255] = '\0';
        }

}

int main(int argc, char **argv, char **environ)
{
	int index = 0;
	int apps_only = 0;
	Xd6ConfigFileItem *itm;
	const char *val;
	Fl_Font font = (Fl_Font) 0;
	int size = 16;
	int x = 50;
	int y = 50;
	int w = 500;
	int h = 350;

	if (argc >= 2 && !strcmp(argv[1], "--apps")) {
		apps_only = 1;
		argc--;
		for (int i = 1; i < argc; i++) argv[i] = argv[i + 1];
	}

	Fl::args(argc, argv, index);

	fl_open_display();
	fl_init_xdnd_atoms();
	XdndActionCopy     = XInternAtom(fl_display, "XdndActionCopy",     0);
	XdndActionAsk     = XInternAtom(fl_display, "XdndActionAsk",     0);
	XdndActionMove     = XInternAtom(fl_display, "XdndActionMove",     0);
	XdndActionLink     = XInternAtom(fl_display, "XdndActionLink",     0);

	cfg = new Xd6ConfigFile("flfm", "Utilities");
	cfg_sec = cfg->get_config_section(cfg->app_name);

	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	Xd6DefaultFonts::load(cfg);
        if (cfg_sec) {
                itm = cfg_sec->get_item("FontId", NULL);
                val = NULL; if (itm) val = itm->get_value();
                if (val) font = (Fl_Font) strtoul(val, NULL, 0);
                itm = cfg_sec->get_item("FontSize", NULL);
                val = NULL; if (itm) val = itm->get_value();
                if (val) size = (int) strtoul(val, NULL, 0);
                itm = cfg_sec->get_item("W", NULL);
                val = NULL; if (itm) val = itm->get_value();
                if (val) w = (int) strtoul(val, NULL, 0);
                itm = cfg_sec->get_item("H", NULL);
                val = NULL; if (itm) val = itm->get_value();
                if (val) h = (int) strtoul(val, NULL, 0);
        }
	if ((int)font >= (int) Xd6DefaultFonts::free_font) font = (Fl_Font) 0;

	gui = new GUI(x, y, w, h);
	gui->callback((Fl_Callback*)cb_exit);
	gui->font = font;
	gui->size = size;
        if (cfg_sec) {
		restore_values();
	}

	if (apps_only) {
		cb_apps(NULL, NULL);
		return Fl::run();
	}

    	gui->create_layout();

	if (index != argc) {
		StatesValues.newurl = strdup(argv[index]);
	} else {
		StatesValues.newurl = strdup(cfg->home_dir);
	}

	gui->show(argc, argv);
	cb_rescan(NULL, NULL); /* sets Fl::focus(icon_can) internally */

	set_wm_icon(flfm_xpm, gui);
	apply_default_autostart_once();
	start_volume_watch();

    	return Fl::run();
}

