#ifndef callbacks_h
#define callbacks_h

#include <FL/Fl_Widget.H>
#include "gui.h"
#include "xd640/Xd6ConfigFile.h"

struct states_struct {
	char sort_size;
	char sort_type;
	char sort_name;
	char view_icon;
	char view_detail;
	char show_hide;
	char *url;
	char *newurl;
	char *status;
	char *history[10];
};

extern struct states_struct StatesValues;
extern GUI *gui;
extern Xd6ConfigFile *cfg;
extern Xd6ConfigFileSection *cfg_sec;

extern char app_text[256];
extern char app_image[256];
extern char app_video[256];
extern char app_audio[256];
extern char app_html[256];
extern char app_term[256];
extern char app_other[256];

char *get_selected_urls();
void trash_dirs(char *files_dir, char *info_dir, size_t len);
void trash_sync_view();
int strhas_ci(const char *hay, const char *needle);

void cb_rescan(Fl_Widget*, void*);
void cb_newdir(Fl_Widget*, void*);
void cb_newfile(Fl_Widget*, void*);
void cb_clip_copy(Fl_Widget*, void*);
void cb_clip_cut(Fl_Widget*, void*);
void cb_clip_paste(Fl_Widget*, void*);
void cb_rename(Fl_Widget*, void*);
void cb_pref(Fl_Widget*, void*);
void cb_exit(Fl_Widget*, void*);
void cb_sort(Fl_Widget*, void*);
void cb_about(Fl_Widget*, void*);
void cb_help(Fl_Widget*, void*);
void cb_terminal(Fl_Widget*, void*);

void cb_open(Fl_Widget*, void*);
void cb_exec(Fl_Widget*, void*);
void cb_open_width(Fl_Widget*, void*);
void cb_open_dir(Fl_Widget*, void*);
void cb_change_dir(Fl_Widget*, void*);
void cb_delete(Fl_Widget*, void*);
void cb_delete_menu(Fl_Widget*, void*);
void cb_properties(Fl_Widget*, void*);

void cb_move_link_copy(Fl_Widget*, void*);

void cb_find(Fl_Widget*, void*);

void cb_back(Fl_Widget*, void*);
void cb_up(Fl_Widget*, void*);
void cb_home(Fl_Widget*, void*);
void cb_new_fm(Fl_Widget*, void*);
void cb_loc_input(Fl_Widget*, void*);

void cb_apps(Fl_Widget*, void*);
void cb_trash(Fl_Widget*, void*);

void cb_directory(void*);

void refresh_volumes_menu();
void start_volume_watch();
void apply_default_autostart_once();

#endif
