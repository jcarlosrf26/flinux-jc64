#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Wizard.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Slider.H>
#include <FL/fl_ask.H>
#include <curl/curl.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ui_common.h"
#include "backend.h"
#include "player.h"

class FixedSizeWindow : public Fl_Window {
public:
    FixedSizeWindow(int W, int H, const char *title) : Fl_Window(W, H, title) {}
    void resize(int X, int Y, int W, int H) override {
        (void)W; (void)H;
        Fl_Window::resize(X, Y, w(), h());
    }
};

Fl_Wizard *g_wizard = nullptr;
Fl_Box    *g_title_lbl = nullptr;
Fl_Box    *g_info_lbl = nullptr;
Fl_Button *g_back_btn = nullptr;
Fl_Box    *g_status_bar = nullptr;
Fl_Button *g_quality_btn = nullptr;
Fl_Slider *g_vol_slider = nullptr;
Fl_Group  *g_playback_ctrl_box = nullptr;

Fl_Group *g_page_home = nullptr;
Fl_Group *g_page_accounts = nullptr;
Fl_Group *g_page_type = nullptr;
Fl_Group *g_page_browse = nullptr;
Fl_Group *g_page_episodes = nullptr;
Fl_Group *g_page_m3u = nullptr;

void set_status(const char *msg) {
    g_status_bar->copy_label(msg ? msg : "");
    Fl::check();
}

static void back_btn_cb(Fl_Widget *, void *) { on_back_pressed(); }

static void quality_btn_cb(Fl_Widget *, void *) {
    player_cycle_variant_or_quality();
    update_quality_ui();
}

static int read_current_volume(void) {
    FILE *fp = popen("amixer -c 0 sget Master 2>/dev/null", "r");
    if (!fp) return 70;
    char line[256];
    int vol = 70;
    while (fgets(line, sizeof(line), fp)) {
        char *p = strstr(line, "[");
        if (p && strchr(p, '%')) { vol = atoi(p + 1); break; }
    }
    pclose(fp);
    return vol;
}

static void vol_slider_cb(Fl_Widget *w, void *) {
    int v = (int)((Fl_Slider *)w)->value();
    char cmd[80];
    snprintf(cmd, sizeof(cmd), "amixer -c 0 sset Master %d%% unmute >/dev/null 2>&1", v);
    system(cmd);
}

static void salir_btn_cb(Fl_Widget *, void *) {
    player_stop();
    exit(0);
}

void update_quality_ui(void) {
    g_quality_btn->copy_label(player_quality_label());
}

static void poll_timer_cb(void *) {
    player_poll();
    Fl::repeat_timeout(0.25, poll_timer_cb);
}

int main(int argc, char **argv) {
    Fl::lock();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    accs_load();
    player_set_screen_size(Fl::w(), Fl::h());

    Fl_Window *win = new FixedSizeWindow(PAGE_W, 500, "FLTV");

    Fl_Group *top = new Fl_Group(0, 0, PAGE_W, 46);
    top->begin();
    g_title_lbl = new Fl_Box(10, 4, PAGE_W - 20, 20, "FLTV");
    g_title_lbl->labelfont(FL_BOLD);
    g_title_lbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    g_info_lbl = new Fl_Box(10, 24, PAGE_W - 20, 18, "");
    g_info_lbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    g_info_lbl->labelsize(11);
    g_info_lbl->labelcolor(FL_DARK3);
    top->end();
    top->resizable(nullptr);

    Fl_Box *sep1 = new Fl_Box(0, 46, PAGE_W, 2);
    sep1->box(FL_THIN_DOWN_BOX);

    g_wizard = new Fl_Wizard(PAGE_X, 50, PAGE_W, PAGE_H);
    g_wizard->begin();
    g_page_home     = build_home_page();
    g_page_accounts = build_accounts_page();
    g_page_type     = build_type_page();
    g_page_browse   = build_browse_page();
    g_page_episodes = build_episodes_page();
    g_page_m3u      = build_m3u_page();
    g_wizard->end();
    g_wizard->value(g_page_home);

    Fl_Box *sep2 = new Fl_Box(0, 450, PAGE_W, 2);
    sep2->box(FL_THIN_DOWN_BOX);

    Fl_Group *bot = new Fl_Group(0, 454, PAGE_W, 40);
    bot->begin();
    g_back_btn = new Fl_Button(8, 458, 110, 30, "< Atras");
    g_back_btn->callback(back_btn_cb);
    g_back_btn->hide();

    Fl_Button *salir_btn = new Fl_Button(PAGE_W - 80, 458, 72, 30, "Salir");
    salir_btn->callback(salir_btn_cb);

    Fl_Box *ver_lbl = new Fl_Box(PAGE_W - 138, 458, 56, 30, "v" FLTV_VERSION);
    ver_lbl->labelsize(10);
    ver_lbl->labelcolor(FL_DARK3);

    g_playback_ctrl_box = new Fl_Group(PAGE_W - 306, 454, 164, 40);
    g_playback_ctrl_box->begin();
    g_quality_btn = new Fl_Button(PAGE_W - 306, 458, 70, 30, "Cal: 360p");
    g_quality_btn->callback(quality_btn_cb);
    g_vol_slider = new Fl_Slider(PAGE_W - 232, 458, 90, 30);
    g_vol_slider->type(FL_HOR_NICE_SLIDER);
    g_vol_slider->range(0, 100);
    g_vol_slider->value(read_current_volume());
    g_vol_slider->tooltip("Volumen");
    g_vol_slider->callback(vol_slider_cb);
    g_playback_ctrl_box->end();
    g_playback_ctrl_box->hide();

    g_status_bar = new Fl_Box(126, 458, PAGE_W - 438, 30, "");
    g_status_bar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    bot->resizable(nullptr);
    bot->end();

    win->end();
    win->show(argc, argv);

    Fl::add_timeout(0.25, poll_timer_cb, nullptr);

    int ret = Fl::run();
    player_stop();
    curl_global_cleanup();
    return ret;
}
