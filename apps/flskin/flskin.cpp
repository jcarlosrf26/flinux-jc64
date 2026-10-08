// FLSkin — interruptor de temas de FLinux-JC (Tiny Core 17.1 x86)
// FLTK 1.3 / C++ i686. Corre como usuario tc, sin root.
//
// Controles con botones ON/OFF (el activo va resaltado):
//  - Tema FLinux-JC64 por defecto:
//      ON  = fondo de lava + iconos Adwaita + conky lava (el aspecto de la ISO)
//      OFF = aspecto base neutro (fondo solido, GTK Raleigh, iconos hicolor,
//            conky gris azulado)
//  - Modo oscuro:
//      ON  = Adwaita-dark real para GTK2 + GTK3 (viene en el paquete) y,
//            a la vez, paleta oscura FLTK escrita como bloque marcado en
//            ~/.Xdefaults, para que las apps FLTK (FLFM, FLRadio, FLTube,
//            FLWriter, FLPlayer, FLConnect...) tambien abran oscuras.
//      OFF = GTK claro segun el tema y bloque FLTK retirado de ~/.Xdefaults.
//
// La barra FLWM con degradado de 3 colores NO se toca en ningun modo:
// es el flwm parcheado y va siempre.
//
// Estado en ~/.flskin.conf (tema=0/1, dark=0/1); todo lo que escribe vive
// bajo $HOME, asi que filetool lo conserva con el mecanismo normal de TC.
// Los temas GTK y FLTK se aplican en las apps que se abren tras el cambio.
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

static Fl_Button *btnTemaOn, *btnTemaOff, *btnDarkOn, *btnDarkOff;
static Fl_Box *lblEstado;

static std::string home() {
    const char *h = getenv("HOME");
    return h ? h : "/home/tc";
}
static std::string cfgPath() { return home() + "/.flskin.conf"; }

static int g_tema = 1, g_dark = 0;

static void saveState() {
    FILE *f = fopen(cfgPath().c_str(), "w");
    if (!f) return;
    fprintf(f, "tema=%d\ndark=%d\n", g_tema, g_dark);
    fclose(f);
}

static void loadState(bool &existed) {
    existed = false;
    FILE *f = fopen(cfgPath().c_str(), "r");
    if (!f) return;
    existed = true;
    char buf[128];
    while (fgets(buf, sizeof(buf), f)) {
        if (sscanf(buf, "tema=%d", &g_tema) == 1) continue;
        if (sscanf(buf, "dark=%d", &g_dark) == 1) continue;
    }
    fclose(f);
}

static bool readFile(const std::string &path, std::string &out) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    char buf[4096];
    size_t n;
    out.clear();
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    fclose(f);
    return true;
}

static void writeFile(const std::string &path, const char *content) {
    FILE *f = fopen(path.c_str(), "w");
    if (!f) return;
    fputs(content, f);
    fclose(f);
}

static void writeFileStr(const std::string &path, const std::string &content) {
    FILE *f = fopen(path.c_str(), "w");
    if (!f) return;
    fwrite(content.data(), 1, content.size(), f);
    fclose(f);
}

// --- Paleta oscura FLTK en ~/.Xdefaults -------------------------------------
// FLTK 1.3 lee al arrancar cada app los recursos X "background",
// "foreground", "Text.background" y "selectBackground" de la base de
// datos de recursos (en este sistema: ~/.Xdefaults; no hace falta xrdb).
// El bloque va marcado para poder retirarlo sin tocar el resto del
// fichero (ajustes propios del usuario incluidos).
static const char *kMarkBegin = "! FLSKIN-DARK-BEGIN";
static const char *kMarkEnd = "! FLSKIN-DARK-END";
static const char *kDarkBlock =
    "! FLSKIN-DARK-BEGIN\n"
    "*background: #242424\n"
    "*foreground: #eeeeec\n"
    "*Text.background: #1e1e1e\n"
    "*Text.selectBackground: #15539e\n"
    "*selectBackground: #15539e\n"
    "! FLSKIN-DARK-END\n";

static void applyFltkDark() {
    std::string path = home() + "/.Xdefaults";
    std::string all;
    // Si el fichero no existe, se crea con el esquema de la ISO para no
    // perder el aspecto gtk+ de FLTK.
    if (!readFile(path, all)) all = "fltk*scheme: gtk+\n";

    // Quita cualquier bloque anterior (linea a linea) y repone si toca.
    std::string res;
    bool skip = false;
    size_t pos = 0;
    while (pos < all.size()) {
        size_t nl = all.find('\n', pos);
        std::string line = (nl == std::string::npos)
                               ? all.substr(pos)
                               : all.substr(pos, nl - pos);
        pos = (nl == std::string::npos) ? all.size() : nl + 1;
        if (line == kMarkBegin) { skip = true; continue; }
        if (line == kMarkEnd) { skip = false; continue; }
        if (!skip) { res += line; res += '\n'; }
    }
    if (g_dark) res += kDarkBlock;
    writeFileStr(path, res);
}

// --- Tema GTK + fondo/conky --------------------------------------------------
static int s_lastTemaApplied = -1;

static void applyTema() {
    std::string H = home();
    const char *gtkTheme, *gtkIcon, *gtk3Theme, *conkyVar, *wall;
    if (g_dark) {
        gtkTheme = "Adwaita-dark"; gtkIcon = "Adwaita"; gtk3Theme = "Adwaita-dark";
    } else if (g_tema) {
        gtkTheme = "Adwaita";      gtkIcon = "Adwaita"; gtk3Theme = "Adwaita";
    } else {
        gtkTheme = "Raleigh";      gtkIcon = "hicolor"; gtk3Theme = "Adwaita";
    }
    char rc[256], ini[320], cmd[640];

    // GTK2
    snprintf(rc, sizeof(rc),
             "gtk-icon-theme-name=\"%s\"\ngtk-theme-name=\"%s\"\n", gtkIcon, gtkTheme);
    writeFile(H + "/.gtkrc-2.0", rc);

    // GTK3
    std::string g3dir = H + "/.config/gtk-3.0";
    std::string mkc = "mkdir -p " + g3dir;
    system(mkc.c_str());
    snprintf(ini, sizeof(ini),
             "[Settings]\ngtk-icon-theme-name=%s\ngtk-theme-name=%s\n", gtkIcon, gtk3Theme);
    writeFile(g3dir + "/settings.ini", ini);

    // Paleta FLTK (apps FLTK oscuras con el mismo interruptor)
    applyFltkDark();

    // Fondo de escritorio (lava o neutro solido) + conky + wbar:
    // solo cuando cambia el tema (el wbar usa transparencia falsa y
    // hay que reiniciarlo para que recoja el nuevo fondo raiz).
    if (g_tema != s_lastTemaApplied) {
        s_lastTemaApplied = g_tema;
        wall = g_tema ? "/opt/backgrounds/flinux-lava.bmp"
                      : "/usr/local/share/flskin/neutral.bmp";
        snprintf(cmd, sizeof(cmd), "/usr/local/bin/flbg %s >/dev/null 2>&1", wall);
        system(cmd);
        conkyVar = g_tema ? "/usr/local/share/flskin/conkyrc-lava"
                          : "/usr/local/share/flskin/conkyrc-neutro";
        snprintf(cmd, sizeof(cmd), "cp %s %s/.conkyrc 2>/dev/null", conkyVar, H.c_str());
        system(cmd);
        system("killall conky >/dev/null 2>&1");
        system("killall wbar >/dev/null 2>&1");
        system("(sleep 1; conky -d >/dev/null 2>&1; wbar.sh >/dev/null 2>&1) &");
    }
}

// --- UI: pares de botones ON/OFF con el activo resaltado ---------------------
static void paintPair(Fl_Button *on, Fl_Button *off, int state) {
    Fl_Button *act = state ? on : off;
    Fl_Button *ina = state ? off : on;
    act->box(FL_DOWN_BOX);                        // presionado
    act->color(fl_rgb_color(255, 103, 0));        // naranja fluorescente
    act->labelcolor(FL_BLACK);
    ina->box(FL_UP_BOX);
    ina->color(fl_rgb_color(70, 76, 83));
    ina->labelcolor(FL_WHITE);
    act->redraw();
    ina->redraw();
}

static void showState() {
    char buf[256];
    if (g_dark)
        snprintf(buf, sizeof(buf), "Modo oscuro activo (GTK + FLTK)%s",
                 g_tema ? " · tema FLinux-JC64 activo" : " · tema base");
    else
        snprintf(buf, sizeof(buf), "%s",
                 g_tema ? "Aspecto FLinux-JC64 por defecto" : "Aspecto base neutro");
    lblEstado->copy_label(buf);
}

static void refreshUI() {
    paintPair(btnTemaOn, btnTemaOff, g_tema);
    paintPair(btnDarkOn, btnDarkOff, g_dark);
    showState();
}

struct SetCmd { int which; int val; };  // which: 0=tema, 1=dark
static void cbSet(Fl_Widget *, void *ud) {
    SetCmd *c = (SetCmd *)ud;
    if (c->which == 0) g_tema = c->val;
    else               g_dark = c->val;
    saveState();
    applyTema();
    refreshUI();
}

static SetCmd cmdTemaOn = {0, 1}, cmdTemaOff = {0, 0},
              cmdDarkOn = {1, 1}, cmdDarkOff = {1, 0};

static Fl_Button *mkBtn(int x, int y, const char *label, SetCmd *c) {
    Fl_Button *b = new Fl_Button(x, y, 72, 26, label);
    b->labelsize(13);
    b->labelfont(FL_BOLD);
    b->callback(cbSet, c);
    return b;
}

int main(int argc, char **argv) {
    bool existed;
    loadState(existed);

    Fl_Window win(430, 212, "FLSkin");
    win.color(fl_rgb_color(43, 48, 51));

    Fl_Box title(15, 8, 400, 28, "FLSkin \xE2\x80\x94 Temas FLinux-JC64");
    title.labelfont(FL_BOLD);
    title.labelsize(16);
    title.labelcolor(FL_WHITE);
    title.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Box lblTema(15, 48, 245, 26, "Tema FLinux-JC64 por defecto");
    lblTema.labelcolor(FL_WHITE);
    lblTema.labelsize(14);
    lblTema.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    btnTemaOn = mkBtn(262, 48, "ON", &cmdTemaOn);
    btnTemaOff = mkBtn(342, 48, "OFF", &cmdTemaOff);

    Fl_Box lblDark(15, 84, 245, 26, "Modo oscuro (GTK + FLTK)");
    lblDark.labelcolor(FL_WHITE);
    lblDark.labelsize(14);
    lblDark.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    btnDarkOn = mkBtn(262, 84, "ON", &cmdDarkOn);
    btnDarkOff = mkBtn(342, 84, "OFF", &cmdDarkOff);

    lblEstado = new Fl_Box(15, 122, 400, 24, "");
    lblEstado->labelcolor(fl_rgb_color(255, 199, 154));
    lblEstado->labelsize(13);
    lblEstado->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Box note(15, 150, 400, 48,
                "La barra degradada de 3 colores se mantiene siempre.\n"
                "El tema (GTK y FLTK) se aplica en las apps al abrirlas.");
    note.labelcolor(fl_rgb_color(180, 190, 198));
    note.labelsize(11);
    note.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_TOP);

    win.end();
    win.show(argc, argv);

    // Deja el sistema coherente con el estado guardado (sin forzar el
    // reinicio visual de tema salvo primera vez o cambio real).
    s_lastTemaApplied = existed ? g_tema : -1;
    saveState();
    applyTema();
    refreshUI();
    return Fl::run();
}
