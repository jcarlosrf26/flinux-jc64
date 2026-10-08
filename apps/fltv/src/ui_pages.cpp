#include "ui_common.h"
#include "backend.h"
#include "player.h"

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Tabs.H>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>

static int g_ctype = 0;

static void on_home_xtream(Fl_Widget *, void *) { show_accounts(); }
static void on_home_m3u(Fl_Widget *, void *)    { show_m3u(); }

Fl_Group *build_home_page() {
    Fl_Group *g = new Fl_Group(PAGE_X, PAGE_Y, PAGE_W, PAGE_H);
    g->begin();
    Fl_Box *title = new Fl_Box(0, PAGE_Y + 90, PAGE_W, 60, "FLTV");
    title->labelfont(FL_BOLD);
    title->labelsize(42);
    Fl_Box *sub = new Fl_Box(0, PAGE_Y + 155, PAGE_W, 24, "Elige el modo de conexion");
    sub->labelcolor(FL_DARK3);

    int bw = 170, bh = 54, gap = 18;
    int total = bw * 2 + gap;
    int startx = (PAGE_W - total) / 2;
    Fl_Button *b1 = new Fl_Button(startx, PAGE_Y + 210, bw, bh, "Xtream Codes");
    Fl_Button *b2 = new Fl_Button(startx + bw + gap, PAGE_Y + 210, bw, bh, "M3U");
    b1->callback(on_home_xtream);
    b2->callback(on_home_m3u);
    g->end();
    return g;
}

void update_back_btn(const char *page) {
    if (strcmp(page, "home") == 0) {
        g_back_btn->hide();
    } else if (strcmp(page, "type") == 0) {
        g_back_btn->copy_label("Cerrar sesion");
        g_back_btn->show();
    } else {
        g_back_btn->copy_label("< Atras");
        g_back_btn->show();
    }
}

void show_home(void) {
    g_title_lbl->copy_label("FLTV");
    g_info_lbl->copy_label("");
    g_wizard->value(g_page_home);
    update_back_btn("home");
    set_status("");
    g_playback_ctrl_box->hide();
}

void on_back_pressed(void) {
    Fl_Widget *cur = g_wizard->value();
    if (cur == g_page_type) { show_home(); return; }
    if (cur == g_page_browse) { show_type(); return; }
    if (cur == g_page_episodes) { show_browse(2); return; }
    if (cur == g_page_accounts) { show_home(); return; }
    if (cur == g_page_m3u) { show_home(); return; }
    show_home();
}

static void on_type_live(Fl_Widget *, void *) { show_browse(0); }
static void on_type_vod(Fl_Widget *, void *)  { show_browse(1); }
static void on_type_ser(Fl_Widget *, void *)  { show_browse(2); }

Fl_Group *build_type_page() {
    Fl_Group *g = new Fl_Group(PAGE_X, PAGE_Y, PAGE_W, PAGE_H);
    g->begin();
    int bw = 240, bh = 50, gap = 14;
    int starty = PAGE_Y + (PAGE_H - (bh * 3 + gap * 2)) / 2;
    int startx = (PAGE_W - bw) / 2;
    Fl_Button *b1 = new Fl_Button(startx, starty, bw, bh, "TV en Vivo");
    Fl_Button *b2 = new Fl_Button(startx, starty + bh + gap, bw, bh, "Peliculas");
    Fl_Button *b3 = new Fl_Button(startx, starty + 2 * (bh + gap), bw, bh, "Series");
    b1->callback(on_type_live);
    b2->callback(on_type_vod);
    b3->callback(on_type_ser);
    g->end();
    return g;
}

void show_type(void) {
    char info[320];
    snprintf(info, sizeof(info), "%s   vence: %s   %d/%d con.",
             g_account.user, g_account.exp_date, g_account.active_conn, g_account.max_conn);
    g_title_lbl->copy_label("FLTV");
    g_info_lbl->copy_label(info);
    g_wizard->value(g_page_type);
    update_back_btn("type");
    set_status("Selecciona el tipo de contenido");
    g_playback_ctrl_box->hide();
}

static Fl_Hold_Browser *g_online_browser = nullptr;
static Fl_Hold_Browser *g_personal_browser = nullptr;

static void rebuild_acc_browser(Fl_Hold_Browser *b, SavedAcc *arr, int n, const char *empty_msg) {
    b->clear();
    if (n == 0) {
        b->add(empty_msg);
        return;
    }
    for (int i = 0; i < n; i++) {
        char line[700];
        snprintf(line, sizeof(line), "@b%s\t%s",
                 arr[i].label[0] ? arr[i].label : "Sin nombre", arr[i].host);
        b->add(line, (void *)(intptr_t)i);
    }
}

struct ConnectJob { char host[512], user[128], pass[128]; int ok; };

static void connect_done_awake(void *data) {
    ConnectJob *job = (ConnectJob *)data;
    if (job->ok) {
        show_type();
    } else {
        set_status("No se pudo conectar - credenciales invalidas o sin red");
    }
    delete job;
}

static void *connect_thread(void *data) {
    ConnectJob *job = (ConnectJob *)data;
    job->ok = account_connect(job->host, job->user, job->pass);
    Fl::awake(connect_done_awake, job);
    return nullptr;
}

static void auto_connect(const SavedAcc *a) {
    set_status("Conectando...");
    ConnectJob *job = new ConnectJob();
    strncpy(job->host, a->host, sizeof(job->host) - 1);
    strncpy(job->user, a->user, sizeof(job->user) - 1);
    strncpy(job->pass, a->pass, sizeof(job->pass) - 1);
    pthread_t th;
    pthread_create(&th, nullptr, connect_thread, job);
    pthread_detach(th);
}

static void on_online_browser_cb(Fl_Widget *, void *) {
    int line = g_online_browser->value();
    if (line <= 0) return;
    intptr_t idx = (intptr_t)g_online_browser->data(line);
    if (idx >= 0 && idx < g_n_online) auto_connect(&g_online[idx]);
}

static void on_personal_browser_cb(Fl_Widget *, void *) {
    int line = g_personal_browser->value();
    if (line <= 0) return;
    intptr_t idx = (intptr_t)g_personal_browser->data(line);
    if (idx >= 0 && idx < g_n_personal) auto_connect(&g_personal[idx]);
}

static void on_add_account(Fl_Widget *, void *) {
    Fl_Window *dlg = new Fl_Window(380, 230, "Nueva cuenta");
    dlg->begin();
    Fl_Input *label_in = new Fl_Input(110, 15, 250, 25, "Nombre:");
    Fl_Input *host_in  = new Fl_Input(110, 50, 250, 25, "Host URL:");
    Fl_Input *user_in  = new Fl_Input(110, 85, 250, 25, "Usuario:");
    Fl_Secret_Input *pass_in = new Fl_Secret_Input(110, 120, 250, 25, "Password:");
    host_in->tooltip("http://host:puerto");
    Fl_Button *save_btn = new Fl_Button(110, 165, 100, 30, "Guardar");
    Fl_Button *cancel_btn = new Fl_Button(220, 165, 100, 30, "Cancelar");
    dlg->end();
    dlg->set_modal();

    struct Ctx { Fl_Window *dlg; Fl_Input *label_in, *host_in, *user_in; Fl_Secret_Input *pass_in; bool saved; };
    static Ctx ctx;
    ctx = { dlg, label_in, host_in, user_in, pass_in, false };

    save_btn->callback([](Fl_Widget *, void *data) {
        Ctx *c = (Ctx *)data;
        if (strlen(c->host_in->value()) == 0 || strlen(c->user_in->value()) == 0) return;
        if (g_n_personal < MAX_ACCS) {
            SavedAcc *a = &g_personal[g_n_personal];
            memset(a, 0, sizeof(SavedAcc));
            strncpy(a->label, c->label_in->value(), 127);
            strncpy(a->host, c->host_in->value(), 511);
            strncpy(a->user, c->user_in->value(), 127);
            strncpy(a->pass, c->pass_in->value(), 127);
            g_n_personal++;
            accs_save();
            rebuild_acc_browser(g_personal_browser, g_personal, g_n_personal, "Sin cuentas guardadas");
        }
        c->saved = true;
        c->dlg->hide();
    }, &ctx);
    cancel_btn->callback([](Fl_Widget *, void *data) {
        Ctx *c = (Ctx *)data;
        c->dlg->hide();
    }, &ctx);

    dlg->show();
    while (dlg->shown()) Fl::wait();
    delete dlg;
}

static void on_delete_account(Fl_Widget *, void *) {
    int line = g_personal_browser->value();
    if (line <= 0) { set_status("Selecciona una cuenta para eliminar"); return; }
    intptr_t idx = (intptr_t)g_personal_browser->data(line);
    if (idx < 0 || idx >= g_n_personal) return;
    for (int i = (int)idx; i < g_n_personal - 1; i++) g_personal[i] = g_personal[i + 1];
    g_n_personal--;
    accs_save();
    rebuild_acc_browser(g_personal_browser, g_personal, g_n_personal, "Sin cuentas guardadas");
    set_status("Cuenta eliminada");
}

static void online_accs_done_awake(void *) {
    rebuild_acc_browser(g_online_browser, g_online, g_n_online, "Sin cuentas en linea");
    set_status("Selecciona una cuenta o agrega una nueva");
}

static void *online_accs_thread(void *) {
    online_accs_load();
    Fl::awake(online_accs_done_awake, nullptr);
    return nullptr;
}

Fl_Group *build_accounts_page() {
    Fl_Group *g = new Fl_Group(PAGE_X, PAGE_Y, PAGE_W, PAGE_H);
    g->begin();

    int colw = (PAGE_W - 30) / 2;
    int rx = 20 + colw;
    Fl_Box *lt = new Fl_Box(10, PAGE_Y + 8, colw, 20, "En Linea");
    lt->labelfont(FL_BOLD);
    lt->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    g_online_browser = new Fl_Hold_Browser(10, PAGE_Y + 32, colw, PAGE_H - 42);
    g_online_browser->column_widths((const int[]){180, 0});
    g_online_browser->callback(on_online_browser_cb);

    Fl_Box *rt = new Fl_Box(rx, PAGE_Y + 8, colw - 170, 20, "Mis Cuentas");
    rt->labelfont(FL_BOLD);
    rt->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Button *add_btn = new Fl_Button(PAGE_W - 170, PAGE_Y + 6, 80, 24, "+ Agregar");
    Fl_Button *del_btn = new Fl_Button(PAGE_W - 88, PAGE_Y + 6, 78, 24, "Eliminar");
    add_btn->callback(on_add_account);
    del_btn->callback(on_delete_account);
    g_personal_browser = new Fl_Hold_Browser(rx, PAGE_Y + 32, colw, PAGE_H - 42);
    g_personal_browser->column_widths((const int[]){180, 0});
    g_personal_browser->callback(on_personal_browser_cb);

    g->end();
    return g;
}

void show_accounts(void) {
    g_title_lbl->copy_label("FLTV -- Cuentas");
    g_info_lbl->copy_label("");
    g_wizard->value(g_page_accounts);
    update_back_btn("accounts");
    g_playback_ctrl_box->hide();
    rebuild_acc_browser(g_personal_browser, g_personal, g_n_personal, "Sin cuentas guardadas");
    g_online_browser->clear();
    g_online_browser->add("Cargando cuentas en linea...");
    set_status("Cargando cuentas en linea...");
    pthread_t th;
    pthread_create(&th, nullptr, online_accs_thread, nullptr);
    pthread_detach(th);
}

#define MAX_CATS 300
#define MAX_CONTENT 2000

static CategoryItem g_categories[MAX_CATS];
static int g_n_categories = 0;
static ContentItem g_content_items[MAX_CONTENT];
static int g_n_content_items = 0;
static char g_cur_cat_id[64] = "";
static char g_cur_cat_name[128] = "";

static Fl_Hold_Browser *g_cat_browser = nullptr;
static Fl_Hold_Browser *g_content_browser = nullptr;
static Fl_Input *g_cat_search = nullptr;
static Fl_Input *g_content_search = nullptr;
static Fl_Box *g_cover_box = nullptr;
static Fl_Box *g_info_title_box = nullptr;
static Fl_Box *g_info_meta_box = nullptr;
static Fl_Text_Display *g_info_plot_box = nullptr;
static Fl_Text_Buffer  *g_info_plot_buf = nullptr;
static Fl_Button *g_trailer_btn = nullptr;
static Fl_Image *g_cover_image = nullptr;
static char g_cover_url_cur[2048] = "";
static char g_info_trailer[512] = "";
static char g_info_id_cur[64] = "";

static void refilter_cat_browser() {
    const char *q = g_cat_search->value();
    g_cat_browser->clear();
    for (int i = 0; i < g_n_categories; i++) {
        if (q && *q && !str_casefind(g_categories[i].name, q)) continue;
        g_cat_browser->add(g_categories[i].name, (void *)(intptr_t)i);
    }
}

static void refilter_content_browser() {
    const char *q = g_content_search->value();
    g_content_browser->clear();
    for (int i = 0; i < g_n_content_items; i++) {
        if (q && *q && !str_casefind(g_content_items[i].name, q)) continue;
        g_content_browser->add(g_content_items[i].name, (void *)(intptr_t)i);
    }
}

static void cat_search_cb(Fl_Widget *, void *) { refilter_cat_browser(); }
static void content_search_cb(Fl_Widget *, void *) { refilter_content_browser(); }

static void info_clear() {
    g_info_id_cur[0] = '\0';
    g_info_trailer[0] = '\0';
    g_info_title_box->copy_label("");
    g_info_meta_box->copy_label("");
    g_info_plot_buf->text("");
    g_trailer_btn->hide();
}

struct CoverJob { char url[2048]; unsigned char *bytes; size_t len; };

static void cover_done_awake(void *data) {
    CoverJob *job = (CoverJob *)data;
    if (strcmp(job->url, g_cover_url_cur) == 0) {
        if (g_cover_image) { delete g_cover_image; g_cover_image = nullptr; }
        if (job->bytes && job->len > 0) {
            Fl_Image *img = nullptr;
            Fl_JPEG_Image *jpg = new Fl_JPEG_Image(job->url, job->bytes);
            if (jpg->w() > 0) {
                img = jpg;
            } else {
                delete jpg;
                Fl_PNG_Image *png = new Fl_PNG_Image(job->url, job->bytes, (int)job->len);
                if (png->w() > 0) img = png; else delete png;
            }
            if (img) {
                int tw = g_cover_box->w() - 8, th = g_cover_box->h() - 8;
                double s = (double)tw / img->w();
                double s2 = (double)th / img->h();
                if (s2 < s) s = s2;
                Fl_Image *scaled = img->copy((int)(img->w() * s), (int)(img->h() * s));
                delete img;
                g_cover_image = scaled;
            }
        }
        g_cover_box->image(g_cover_image);
        g_cover_box->redraw();
    }
    free(job->bytes);
    delete job;
}

static void *cover_fetch_thread(void *data) {
    CoverJob *job = (CoverJob *)data;
    job->bytes = backend_http_get_bytes(job->url, 8L, &job->len);
    Fl::awake(cover_done_awake, job);
    return nullptr;
}

static void cover_request(const char *url) {
    if (!url || !*url) {
        g_cover_url_cur[0] = '\0';
        if (g_cover_image) { delete g_cover_image; g_cover_image = nullptr; }
        g_cover_box->image(nullptr);
        g_cover_box->redraw();
        return;
    }
    strncpy(g_cover_url_cur, url, sizeof(g_cover_url_cur) - 1);
    CoverJob *job = new CoverJob();
    strncpy(job->url, url, sizeof(job->url) - 1);
    job->bytes = nullptr;
    job->len = 0;
    pthread_t th;
    pthread_create(&th, nullptr, cover_fetch_thread, job);
    pthread_detach(th);
}

struct InfoJob { char id[64]; int ctype; ContentInfo info; };

static void info_done_awake(void *data) {
    InfoJob *job = (InfoJob *)data;
    if (strcmp(job->id, g_info_id_cur) == 0) {
        g_info_meta_box->copy_label(job->info.meta);
        g_info_plot_buf->text(job->info.plot);
        if (job->info.trailer[0]) {
            strncpy(g_info_trailer, job->info.trailer, sizeof(g_info_trailer) - 1);
            g_trailer_btn->show();
        } else {
            g_trailer_btn->hide();
        }
    }
    delete job;
}

static void *info_fetch_thread(void *data) {
    InfoJob *job = (InfoJob *)data;
    xtream_fetch_info(job->ctype, job->id, &job->info);
    Fl::awake(info_done_awake, job);
    return nullptr;
}

static void info_request(const char *id, int ctype, const char *title) {
    info_clear();
    if (!id || !*id) return;
    strncpy(g_info_id_cur, id, sizeof(g_info_id_cur) - 1);
    g_info_title_box->copy_label(title ? title : "");
    InfoJob *job = new InfoJob();
    strncpy(job->id, id, sizeof(job->id) - 1);
    job->ctype = ctype;
    pthread_t th;
    pthread_create(&th, nullptr, info_fetch_thread, job);
    pthread_detach(th);
}

static void on_trailer_clicked(Fl_Widget *, void *) {
    if (!g_info_trailer[0]) return;
    char uri[640];
    if (strncmp(g_info_trailer, "http", 4) == 0)
        snprintf(uri, sizeof(uri), "%s", g_info_trailer);
    else
        snprintf(uri, sizeof(uri), "https://www.youtube.com/watch?v=%s", g_info_trailer);
    char cmd[720];
    snprintf(cmd, sizeof(cmd), "xdg-open '%s' >/dev/null 2>&1 &", uri);
    system(cmd);
}

static void on_content_selected(Fl_Widget *, void *) {
    int line = g_content_browser->value();
    if (line <= 0) { cover_request(nullptr); info_clear(); return; }
    intptr_t idx = (intptr_t)g_content_browser->data(line);
    if (idx < 0 || idx >= g_n_content_items) return;
    ContentItem *it = &g_content_items[idx];
    cover_request(it->logo);
    info_request(it->id, g_ctype, it->name);
}

static void on_content_activated(Fl_Widget *, void *) {
    int line = g_content_browser->value();
    if (line <= 0) return;
    intptr_t idx = (intptr_t)g_content_browser->data(line);
    if (idx < 0 || idx >= g_n_content_items) return;
    ContentItem *it = &g_content_items[idx];
    if (g_ctype == 2) {
        show_episodes(it->id, it->name);
        return;
    }
    char url[MAX_URL];
    xtream_build_play_url(g_ctype, it->id, it->ext, url, sizeof(url));
    char st[300];
    snprintf(st, sizeof(st), "Cargando: %s...", it->name);
    set_status(st);
    player_play(url, g_ctype == 0, it->name);
    g_playback_ctrl_box->show();
    update_quality_ui();
    if (g_ctype == 0) {
        static char m3u8[MAX_URL];
        xtream_build_live_m3u8_url(it->id, m3u8, sizeof(m3u8));
        struct VJob { char url[MAX_URL]; HLSVariant variants[MAX_VARIANTS]; int n; };
        VJob *vj = new VJob();
        strncpy(vj->url, m3u8, sizeof(vj->url) - 1);
        pthread_t th;
        pthread_create(&th, nullptr, [](void *data) -> void * {
            VJob *vj = (VJob *)data;
            vj->n = hls_fetch_variants(vj->url, vj->variants, MAX_VARIANTS);
            Fl::awake([](void *d) {
                VJob *vj = (VJob *)d;
                if (vj->n > 0) { player_set_variants(vj->n, vj->variants); update_quality_ui(); }
                delete vj;
            }, vj);
            return nullptr;
        }, vj);
        pthread_detach(th);
    }
}

static void on_cat_selected(Fl_Widget *, void *) {
    int line = g_cat_browser->value();
    if (line <= 0) return;
    intptr_t idx = (intptr_t)g_cat_browser->data(line);
    if (idx < 0 || idx >= g_n_categories) return;
    strncpy(g_cur_cat_id, g_categories[idx].id, sizeof(g_cur_cat_id) - 1);
    strncpy(g_cur_cat_name, g_categories[idx].name, sizeof(g_cur_cat_name) - 1);

    char st[300];
    snprintf(st, sizeof(st), "Cargando: %s...", g_cur_cat_name);
    set_status(st);
    g_content_search->value("");
    cover_request(nullptr);
    info_clear();
    player_set_variants(0, nullptr);

    g_n_content_items = xtream_load_content(g_ctype, g_cur_cat_id, g_content_items, MAX_CONTENT);
    refilter_content_browser();
    snprintf(st, sizeof(st), "%d elementos", g_n_content_items);
    set_status(st);
}

Fl_Group *build_browse_page() {
    Fl_Group *g = new Fl_Group(PAGE_X, PAGE_Y, PAGE_W, PAGE_H);
    g->begin();

    int py = PAGE_Y;
    int catw = 220;
    int cw = 220;
    int mx = 20 + catw;
    int mw = PAGE_W - mx - cw - 20;
    int cx = mx + mw + 10;
    g_cat_search = new Fl_Input(10, py + 6, catw, 24, "");
    g_cat_search->when(FL_WHEN_CHANGED);
    g_cat_search->callback(cat_search_cb);
    g_cat_search->tooltip("Filtrar categorias...");

    g_content_search = new Fl_Input(mx, py + 6, mw, 24, "");
    g_content_search->when(FL_WHEN_CHANGED);
    g_content_search->callback(content_search_cb);
    g_content_search->tooltip("Filtrar contenido...");

    g_cat_browser = new Fl_Hold_Browser(10, py + 36, catw, PAGE_H - 46);
    g_cat_browser->callback(on_cat_selected);

    g_content_browser = new Fl_Hold_Browser(mx, py + 36, mw, PAGE_H - 46);
    g_content_browser->callback([](Fl_Widget *w, void *d) {
        on_content_selected(w, d);
        if (Fl::event_clicks()) on_content_activated(w, d);
    });

    g_cover_box = new Fl_Box(cx, py + 6, cw, 190);
    g_cover_box->box(FL_DOWN_BOX);
    g_cover_box->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_IMAGE_OVER_TEXT);

    g_info_title_box = new Fl_Box(cx, py + 200, cw, 32, "");
    g_info_title_box->labelfont(FL_BOLD);
    g_info_title_box->align(FL_ALIGN_TOP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    g_info_meta_box = new Fl_Box(cx, py + 236, cw, 28, "");
    g_info_meta_box->labelsize(11);
    g_info_meta_box->labelcolor(FL_DARK3);
    g_info_meta_box->align(FL_ALIGN_TOP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    Fl_Box *sep = new Fl_Box(cx, py + 268, cw, 2);
    sep->box(FL_THIN_DOWN_BOX);

    g_info_plot_buf = new Fl_Text_Buffer();
    g_info_plot_box = new Fl_Text_Display(cx, py + 272, cw, 68);
    g_info_plot_box->buffer(g_info_plot_buf);
    g_info_plot_box->wrap_mode(Fl_Text_Display::WRAP_AT_BOUNDS, 0);
    g_info_plot_box->textsize(11);

    g_trailer_btn = new Fl_Button(cx, py + 348, cw, 22, "> Ver Trailer");
    g_trailer_btn->callback(on_trailer_clicked);
    g_trailer_btn->hide();

    g->end();
    g->resizable(nullptr);
    return g;
}

void show_browse(int ctype) {
    g_ctype = ctype;
    static const char *names[] = { "TV en Vivo", "Peliculas", "Series" };
    char tm[64];
    snprintf(tm, sizeof(tm), "FLTV -- %s", names[ctype]);
    g_title_lbl->copy_label(tm);
    g_info_lbl->copy_label("");
    g_wizard->value(g_page_browse);
    update_back_btn("browse");
    g_playback_ctrl_box->show();
    update_quality_ui();

    g_cat_search->value("");
    g_content_search->value("");
    g_content_browser->clear();
    cover_request(nullptr);
    info_clear();

    set_status("Cargando categorias...");
    g_n_categories = xtream_load_categories(ctype, g_categories, MAX_CATS);
    refilter_cat_browser();
    char st[64];
    snprintf(st, sizeof(st), "%d categorias", g_n_categories);
    set_status(st);
}

#define MAX_EPISODES 500
static EpisodeItem g_episodes[MAX_EPISODES];
static int g_n_episodes = 0;
static char g_ser_id[64] = "";
static char g_ser_name[256] = "";
static Fl_Hold_Browser *g_ep_browser = nullptr;

static void on_ep_activated(Fl_Widget *, void *) {
    int line = g_ep_browser->value();
    if (line <= 0) return;
    intptr_t idx = (intptr_t)g_ep_browser->data(line);
    if (idx < 0 || idx >= g_n_episodes) return;
    char idbuf[300];
    strncpy(idbuf, g_episodes[idx].id, sizeof(idbuf) - 1);
    char *bar = strchr(idbuf, '|');
    const char *ext = "mkv";
    if (bar) { *bar = '\0'; ext = bar + 1; }
    char url[MAX_URL];
    xtream_build_play_url(2, idbuf, ext, url, sizeof(url));
    char st[300];
    snprintf(st, sizeof(st), "Cargando: %s...", g_episodes[idx].label);
    set_status(st);
    player_play(url, 0, g_episodes[idx].label);
    g_playback_ctrl_box->show();
    update_quality_ui();
}

Fl_Group *build_episodes_page() {
    Fl_Group *g = new Fl_Group(PAGE_X, PAGE_Y, PAGE_W, PAGE_H);
    g->begin();
    g_ep_browser = new Fl_Hold_Browser(10, PAGE_Y + 8, PAGE_W - 20, PAGE_H - 16);
    g_ep_browser->callback([](Fl_Widget *w, void *d) {
        if (Fl::event_clicks()) on_ep_activated(w, d);
    });
    g->end();
    return g;
}

void show_episodes(const char *series_id, const char *series_name) {
    strncpy(g_ser_id, series_id, sizeof(g_ser_id) - 1);
    strncpy(g_ser_name, series_name, sizeof(g_ser_name) - 1);
    char tm[320];
    snprintf(tm, sizeof(tm), "FLTV -- %s", series_name);
    g_title_lbl->copy_label(tm);
    g_wizard->value(g_page_episodes);
    update_back_btn("episodes");
    g_playback_ctrl_box->show();

    char st[300];
    snprintf(st, sizeof(st), "Cargando: %s...", series_name);
    set_status(st);
    g_ep_browser->clear();
    g_n_episodes = xtream_load_episodes(series_id, g_episodes, MAX_EPISODES);
    for (int i = 0; i < g_n_episodes; i++) {
        g_ep_browser->add(g_episodes[i].label, (void *)(intptr_t)i);
    }
    snprintf(st, sizeof(st), "%s - %d episodios", series_name, g_n_episodes);
    set_status(st);
}

#define MAX_M3U_GROUPS 400
static char g_m3u_groups[MAX_M3U_GROUPS][128];
static int  g_n_m3u_groups = 0;
static char g_m3u_cur_group[128] = "";

static Fl_Hold_Browser *g_m3u_grp_browser = nullptr;
static Fl_Hold_Browser *g_m3u_ch_browser = nullptr;
static Fl_Input *g_m3u_grp_search = nullptr;
static Fl_Input *g_m3u_ch_search = nullptr;
static Fl_Box *g_m3u_cover_box = nullptr;
static Fl_Image *g_m3u_cover_image = nullptr;
static char g_m3u_cover_url_cur[2048] = "";

static volatile int g_m3u_online_gen = 0;

static void m3u_cover_request(const char *url) {
    if (!url || !*url) {
        g_m3u_cover_url_cur[0] = '\0';
        if (g_m3u_cover_image) { delete g_m3u_cover_image; g_m3u_cover_image = nullptr; }
        g_m3u_cover_box->image(nullptr);
        g_m3u_cover_box->redraw();
        return;
    }
    strncpy(g_m3u_cover_url_cur, url, sizeof(g_m3u_cover_url_cur) - 1);
    CoverJob *job = new CoverJob();
    strncpy(job->url, url, sizeof(job->url) - 1);
    job->bytes = nullptr;
    job->len = 0;
    pthread_t th;
    pthread_create(&th, nullptr, [](void *data) -> void * {
        CoverJob *job = (CoverJob *)data;
        job->bytes = backend_http_get_bytes(job->url, 8L, &job->len);
        Fl::awake([](void *d) {
            CoverJob *job = (CoverJob *)d;
            if (strcmp(job->url, g_m3u_cover_url_cur) == 0) {
                if (g_m3u_cover_image) { delete g_m3u_cover_image; g_m3u_cover_image = nullptr; }
                if (job->bytes && job->len > 0) {
                    Fl_Image *img = nullptr;
                    Fl_JPEG_Image *jpg = new Fl_JPEG_Image(job->url, job->bytes);
                    if (jpg->w() > 0) img = jpg;
                    else { delete jpg; Fl_PNG_Image *png = new Fl_PNG_Image(job->url, job->bytes, (int)job->len);
                           if (png->w() > 0) img = png; else delete png; }
                    if (img) {
                        int tw = g_m3u_cover_box->w() - 8, th2 = g_m3u_cover_box->h() - 8;
                        double s = (double)tw / img->w();
                        double s2 = (double)th2 / img->h();
                        if (s2 < s) s = s2;
                        Fl_Image *scaled = img->copy((int)(img->w() * s), (int)(img->h() * s));
                        delete img;
                        g_m3u_cover_image = scaled;
                    }
                }
                g_m3u_cover_box->image(g_m3u_cover_image);
                g_m3u_cover_box->redraw();
            }
            free(job->bytes);
            delete job;
        }, job);
        return nullptr;
    }, job);
    pthread_detach(th);
}

static void refilter_m3u_groups() {
    const char *q = g_m3u_grp_search->value();
    g_m3u_grp_browser->clear();
    for (int i = 0; i < g_n_m3u_groups; i++) {
        if (q && *q && !str_casefind(g_m3u_groups[i], q)) continue;
        g_m3u_grp_browser->add(g_m3u_groups[i]);
    }
}

static void refilter_m3u_channels() {
    const char *q = g_m3u_ch_search->value();
    g_m3u_ch_browser->clear();
    for (int i = 0; i < g_n_m3u; i++) {
        if (strcmp(g_m3u[i].cat, g_m3u_cur_group) != 0) continue;
        if (q && *q && !str_casefind(g_m3u[i].name, q)) continue;
        g_m3u_ch_browser->add(g_m3u[i].name, (void *)(intptr_t)i);
    }
}

static void rebuild_m3u_groups() {
    g_n_m3u_groups = 0;
    char last[128] = "\x01";
    for (int i = 0; i < g_n_m3u && g_n_m3u_groups < MAX_M3U_GROUPS; i++) {
        if (strcmp(g_m3u[i].cat, last) != 0) {
            strncpy(last, g_m3u[i].cat, 127); last[127] = '\0';
            strncpy(g_m3u_groups[g_n_m3u_groups], last, 127);
            g_m3u_groups[g_n_m3u_groups][127] = '\0';
            g_n_m3u_groups++;
        }
    }
    refilter_m3u_groups();
}

static void on_m3u_grp_selected(Fl_Widget *, void *) {
    int line = g_m3u_grp_browser->value();
    if (line <= 0) return;
    const char *txt = g_m3u_grp_browser->text(line);
    strncpy(g_m3u_cur_group, txt, sizeof(g_m3u_cur_group) - 1);
    g_m3u_ch_search->value("");
    refilter_m3u_channels();
}

static void on_m3u_ch_selected(Fl_Widget *, void *) {
    int line = g_m3u_ch_browser->value();
    if (line <= 0) { m3u_cover_request(nullptr); return; }
    intptr_t idx = (intptr_t)g_m3u_ch_browser->data(line);
    if (idx < 0 || idx >= g_n_m3u) return;
    m3u_cover_request(g_m3u[idx].logo);
}

static void on_m3u_ch_activated(Fl_Widget *, void *) {
    int line = g_m3u_ch_browser->value();
    if (line <= 0) return;
    intptr_t idx = (intptr_t)g_m3u_ch_browser->data(line);
    if (idx < 0 || idx >= g_n_m3u) return;
    char st[300];
    snprintf(st, sizeof(st), "Cargando: %s...", g_m3u[idx].name);
    set_status(st);
    player_play(g_m3u[idx].url, 1, g_m3u[idx].name);
    g_playback_ctrl_box->show();
    update_quality_ui();
}

static void m3u_online_status_cb(const char *msg) {
    Fl::awake([](void *d) {
        char *m = (char *)d;
        set_status(m);
        free(m);
    }, strdup(msg));
}

static void *m3u_online_load_thread(void *data) {
    int my_gen = (int)(intptr_t)data;
    online_m3u_load(&g_m3u_online_gen, my_gen, m3u_online_status_cb);
    Fl::awake([](void *d) {
        int gen = (int)(intptr_t)d;
        if (gen != g_m3u_online_gen) { set_status(""); return; }
        if (g_n_m3u > 0) {
            rebuild_m3u_groups();
            char st[64];
            snprintf(st, sizeof(st), "%d canales en linea", g_n_m3u);
            set_status(st);
        } else {
            set_status("Sin conexion o sin canales disponibles");
        }
    }, (void *)(intptr_t)my_gen);
    return nullptr;
}

Fl_Group *build_m3u_page() {
    Fl_Group *g = new Fl_Group(PAGE_X, PAGE_Y, PAGE_W, PAGE_H);
    g->begin();

    Fl_Tabs *tabs = new Fl_Tabs(PAGE_X, PAGE_Y, PAGE_W, PAGE_H);
    tabs->begin();

    Fl_Group *online_tab = new Fl_Group(PAGE_X, PAGE_Y + 24, PAGE_W, PAGE_H - 24, "En Linea");
    online_tab->begin();
    int gw = 180;
    int covw = 150;
    int chx = 16 + gw;
    int chw = PAGE_W - chx - covw - 16;
    g_m3u_grp_search = new Fl_Input(6, PAGE_Y + 30, gw, 24, "");
    g_m3u_grp_search->tooltip("Filtrar grupos...");
    g_m3u_grp_search->when(FL_WHEN_CHANGED);
    g_m3u_grp_search->callback([](Fl_Widget *, void *) { refilter_m3u_groups(); });

    g_m3u_ch_search = new Fl_Input(chx, PAGE_Y + 30, chw, 24, "");
    g_m3u_ch_search->tooltip("Filtrar canales...");
    g_m3u_ch_search->when(FL_WHEN_CHANGED);
    g_m3u_ch_search->callback([](Fl_Widget *, void *) { refilter_m3u_channels(); });

    g_m3u_grp_browser = new Fl_Hold_Browser(6, PAGE_Y + 60, gw, PAGE_H - 94);
    g_m3u_grp_browser->callback(on_m3u_grp_selected);

    g_m3u_ch_browser = new Fl_Hold_Browser(chx, PAGE_Y + 60, chw, PAGE_H - 94);
    g_m3u_ch_browser->callback([](Fl_Widget *w, void *d) {
        on_m3u_ch_selected(w, d);
        if (Fl::event_clicks()) on_m3u_ch_activated(w, d);
    });

    g_m3u_cover_box = new Fl_Box(chx + chw + 10, PAGE_Y + 60, covw, 192);
    g_m3u_cover_box->box(FL_DOWN_BOX);
    g_m3u_cover_box->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_IMAGE_OVER_TEXT);
    online_tab->end();

    Fl_Group *local_tab = new Fl_Group(PAGE_X, PAGE_Y + 24, PAGE_W, PAGE_H - 24, "Local");
    local_tab->begin();
    Fl_Box *pending = new Fl_Box(PAGE_X, PAGE_Y + 24, PAGE_W, PAGE_H - 24,
        "Proximamente: agregar tus propias listas M3U o enlaces directos.\n"
        "Por ahora usa la pestana 'En Linea'.");
    pending->align(FL_ALIGN_CENTER | FL_ALIGN_WRAP);
    pending->labelcolor(FL_DARK3);
    local_tab->end();

    tabs->end();
    g->end();
    return g;
}

void show_m3u(void) {
    g_title_lbl->copy_label("FLTV -- M3U");
    g_info_lbl->copy_label("");
    g_wizard->value(g_page_m3u);
    update_back_btn("m3u");
    g_playback_ctrl_box->show();
    update_quality_ui();
    if (g_n_m3u == 0) {
        g_m3u_online_gen++;
        int my_gen = g_m3u_online_gen;
        set_status("Cargando canales en linea...");
        pthread_t th;
        pthread_create(&th, nullptr, m3u_online_load_thread, (void *)(intptr_t)my_gen);
        pthread_detach(th);
    }
}
