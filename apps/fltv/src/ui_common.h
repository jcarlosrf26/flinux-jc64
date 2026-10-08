#ifndef FLTV_UI_COMMON_H
#define FLTV_UI_COMMON_H

#include <FL/Fl_Wizard.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>

#define PAGE_X 0
#define PAGE_Y 50
#define PAGE_W 800
#define PAGE_H 400

extern Fl_Wizard *g_wizard;
extern Fl_Box    *g_title_lbl;
extern Fl_Box    *g_info_lbl;
extern Fl_Button *g_back_btn;
extern Fl_Box     *g_status_bar;
extern Fl_Button *g_quality_btn;
extern Fl_Group  *g_playback_ctrl_box;

extern Fl_Group *g_page_home;
extern Fl_Group *g_page_accounts;
extern Fl_Group *g_page_type;
extern Fl_Group *g_page_browse;
extern Fl_Group *g_page_episodes;
extern Fl_Group *g_page_m3u;

void show_home(void);
void show_accounts(void);
void show_type(void);
void show_browse(int ctype);
void show_episodes(const char *series_id, const char *series_name);
void show_m3u(void);
void on_back_pressed(void);

void set_status(const char *msg);
void update_quality_ui(void);

Fl_Group *build_home_page();
Fl_Group *build_type_page();
Fl_Group *build_accounts_page();
Fl_Group *build_browse_page();
Fl_Group *build_episodes_page();
Fl_Group *build_m3u_page();

#endif
