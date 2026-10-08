// ============================================================================
// FLConnect - Gestor de VPN (64-bit)
// Interfaz grafica FLTK - Programado en C++
// Soporta perfiles WireGuard (.conf) y OpenVPN (.ovpn)
// Importacion estilo NetworkManager: los archivos referenciados por un .ovpn
// (ca, cert, key, tls-auth, etc.) se copian junto al perfil para que no se
// rompan las rutas; si el .ovpn pide usuario/contrasena se solicitan al
// conectar de forma segura.
// ============================================================================

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#define APP_NAME    "FLConnect"
#define APP_VERSION "1.0"

// ---------------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------------

static std::string shell_quote(const std::string &s) {
    std::string r = "'";
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else r += c;
    }
    r += "'";
    return r;
}

static std::string trim(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b-1])) b--;
    return s.substr(a, b - a);
}

static std::string home_dir() {
    const char *h = getenv("HOME");
    return h ? h : "/root";
}

static std::string profiles_dir() {
    return home_dir() + "/.flconnect/profiles";
}

static void ensure_dir(const std::string &p) {
    std::string cur;
    for (size_t i = 0; i < p.size(); i++) {
        cur += p[i];
        if (p[i] == '/' && cur.size() > 1) mkdir(cur.c_str(), 0755);
    }
    mkdir(p.c_str(), 0755);
}

static std::string basename_of(const std::string &p) {
    size_t i = p.find_last_of('/');
    return (i == std::string::npos) ? p : p.substr(i + 1);
}

static std::string dirname_of(const std::string &p) {
    size_t i = p.find_last_of('/');
    return (i == std::string::npos) ? "." : p.substr(0, i);
}

static std::string strip_ext(const std::string &n) {
    size_t i = n.find_last_of('.');
    return (i == std::string::npos) ? n : n.substr(0, i);
}

static std::string sanitize(const std::string &n) {
    std::string r;
    for (char c : n) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_') r += c;
        else if (c == ' ') r += '_';
    }
    if (r.empty()) r = "vpn";
    return r;
}

static bool file_exists(const std::string &p) {
    struct stat st;
    return stat(p.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static bool copy_file(const std::string &src, const std::string &dst) {
    FILE *in = fopen(src.c_str(), "rb");
    FILE *ou = fopen(dst.c_str(), "wb");
    if (!in || !ou) {
        if (in) fclose(in);
        if (ou) fclose(ou);
        return false;
    }
    char buf[8192]; size_t r;
    while ((r = fread(buf, 1, sizeof(buf), in)) > 0) fwrite(buf, 1, r, ou);
    fclose(in); fclose(ou);
    chmod(dst.c_str(), 0600);
    return true;
}

// ---------------------------------------------------------------------------
// Registro (log) en pantalla
// ---------------------------------------------------------------------------

static Fl_Text_Buffer *g_logbuf = 0;

static void log_msg(const std::string &s) {
    if (!g_logbuf) return;
    g_logbuf->append(s.c_str());
    if (s.empty() || s[s.size()-1] != '\n') g_logbuf->append("\n");
}

static std::string run_capture(const std::string &cmd, int *exitcode = 0) {
    std::string out;
    FILE *fp = popen((cmd + " 2>&1").c_str(), "r");
    if (!fp) return out;
    char buf[512];
    while (fgets(buf, sizeof(buf), fp)) out += buf;
    int rc = pclose(fp);
    if (exitcode) *exitcode = WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
    return out;
}

// ---------------------------------------------------------------------------
// Perfiles VPN
// ---------------------------------------------------------------------------

struct Profile {
    std::string name;
    std::string file;   // ruta completa al .conf / .ovpn
    std::string type;   // "wireguard" | "openvpn"
    bool        active = false;
};

static std::vector<Profile> g_profiles;

static std::string wg_iface(const Profile &p) {
    return strip_ext(basename_of(p.file));
}

static std::string ovpn_pidfile(const Profile &p) {
    return profiles_dir() + "/" + p.name + ".pid";
}

static std::string ovpn_logfile(const Profile &p) {
    return profiles_dir() + "/" + p.name + ".log";
}

static std::string ovpn_credfile(const Profile &p) {
    return profiles_dir() + "/" + p.name + ".cred";
}

static bool pid_alive(const std::string &pidfile) {
    FILE *f = fopen(pidfile.c_str(), "r");
    if (!f) return false;
    int pid = 0;
    if (fscanf(f, "%d", &pid) != 1) pid = 0;
    fclose(f);
    if (pid <= 0) return false;
    return kill(pid, 0) == 0;
}

static bool check_active(const Profile &p) {
    if (p.type == "wireguard") {
        int rc = 0;
        run_capture("wg show " + shell_quote(wg_iface(p)) + " dump >/dev/null", &rc);
        return rc == 0;
    } else {
        return pid_alive(ovpn_pidfile(p));
    }
}

static void load_profiles() {
    g_profiles.clear();
    ensure_dir(profiles_dir());
    DIR *d = opendir(profiles_dir().c_str());
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != 0) {
        std::string n = e->d_name;
        if (n == "." || n == "..") continue;
        std::string full = profiles_dir() + "/" + n;
        struct stat st;
        if (stat(full.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) continue;
        std::string low = n;
        for (char &c : low) c = tolower((unsigned char)c);
        Profile p;
        p.file = full;
        if (low.size() > 5 && low.substr(low.size()-5) == ".conf") {
            p.type = "wireguard";
            p.name = strip_ext(n);
        } else if (low.size() > 5 && low.substr(low.size()-5) == ".ovpn") {
            p.type = "openvpn";
            p.name = strip_ext(n);
        } else {
            continue;
        }
        p.active = check_active(p);
        g_profiles.push_back(p);
    }
    closedir(d);
}

// ---------------------------------------------------------------------------
// Importacion estilo NetworkManager
// ---------------------------------------------------------------------------

// Directivas de .ovpn que referencian archivos externos
static bool is_ovpn_file_directive(const std::string &d) {
    static const char *dirs[] = {
        "ca", "cert", "key", "dh",
        "tls-auth", "tls-crypt", "tls-crypt-v2",
        "pkcs12", "crl-verify", "auth-user-pass", 0
    };
    for (int i = 0; dirs[i]; i++)
        if (d == dirs[i]) return true;
    return false;
}

// Extrae (directiva, argumento) de una linea ya sin comentarios de bloque.
// Devuelve false si es linea vacia o comentario.
static bool parse_ovpn_line(const std::string &t, std::string &dir, std::string &arg) {
    if (t.empty() || t[0] == '#' || t[0] == ';') return false;
    size_t sp = t.find_first_of(" \t");
    dir = (sp == std::string::npos) ? t : t.substr(0, sp);
    arg = (sp == std::string::npos) ? "" : trim(t.substr(sp + 1));
    if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"')
        arg = arg.substr(1, arg.size() - 2);
    return true;
}

// true si es inicio de bloque <tag> (no cierre)
static bool is_block_start(const std::string &t, std::string &tag) {
    if (t.size() > 2 && t.front() == '<' && t.back() == '>' &&
        t.find(' ') == std::string::npos && t[1] != '/') {
        tag = t.substr(1, t.size() - 2);
        return true;
    }
    return false;
}

// Importa un .ovpn copiando los archivos externos junto al perfil.
// Las rutas relativas se resuelven respecto al directorio del .ovpn origen.
static bool import_ovpn_file(const std::string &src, const std::string &dst,
                             const std::string &base) {
    FILE *in = fopen(src.c_str(), "r");
    FILE *ou = fopen(dst.c_str(), "w");
    if (!in || !ou) {
        if (in) fclose(in);
        if (ou) fclose(ou);
        return false;
    }
    std::string srcdir = dirname_of(src);
    char buf[4096];
    bool in_block = false;
    std::string block_tag;
    int copied = 0;
    bool has_remote = false;
    while (fgets(buf, sizeof(buf), in)) {
        std::string line(buf);
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
            line.pop_back();
        std::string t = trim(line);
        if (in_block) {
            fprintf(ou, "%s\n", line.c_str());
            if (t == "</" + block_tag + ">") in_block = false;
            continue;
        }
        std::string tag;
        if (is_block_start(t, tag)) {
            block_tag = tag;
            in_block = true;
            fprintf(ou, "%s\n", line.c_str());
            continue;
        }
        std::string dir, arg;
        if (parse_ovpn_line(t, dir, arg)) {
            if (dir == "remote") has_remote = true;
            // auth-user-pass sin argumento: se pedira al conectar, se deja igual
            if (is_ovpn_file_directive(dir) && !arg.empty() && arg != "[inline]") {
                std::string fpath = arg;
                if (!fpath.empty() && fpath[0] != '/')
                    fpath = srcdir + "/" + fpath;
                if (file_exists(fpath)) {
                    std::string dest = profiles_dir() + "/" + base + "_" + dir +
                                       "_" + basename_of(fpath);
                    if (copy_file(fpath, dest)) {
                        copied++;
                        fprintf(ou, "%s %s\n", dir.c_str(),
                                shell_quote(dest).c_str());
                        continue;
                    }
                    log_msg("Aviso: no se pudo copiar " + fpath);
                } else {
                    log_msg("Aviso: no se encontro " + dir + " -> " + arg +
                            " (se conserva la ruta original)");
                }
            }
        }
        fprintf(ou, "%s\n", line.c_str());
    }
    fclose(in); fclose(ou);
    chmod(dst.c_str(), 0600);
    if (copied > 0)
        log_msg("Archivos externos integrados al perfil: " + std::to_string(copied));
    if (!has_remote)
        log_msg("Aviso: el .ovpn no contiene directiva 'remote'.");
    return true;
}

static bool file_contains_directive(const std::string &path, const std::string &want) {
    FILE *f = fopen(path.c_str(), "r");
    if (!f) return false;
    char buf[4096];
    bool found = false;
    while (fgets(buf, sizeof(buf), f)) {
        std::string t = trim(buf);
        std::string dir, arg;
        if (parse_ovpn_line(t, dir, arg) && dir == want) { found = true; break; }
    }
    fclose(f);
    return found;
}

// true si el .ovpn tiene "auth-user-pass" sin archivo (pedir credenciales)
static bool ovpn_needs_auth(const std::string &file) {
    FILE *f = fopen(file.c_str(), "r");
    if (!f) return false;
    char buf[4096];
    bool in_block = false, need = false;
    std::string tag;
    while (fgets(buf, sizeof(buf), f)) {
        std::string t = trim(buf);
        if (in_block) {
            if (t == "</" + tag + ">") in_block = false;
            continue;
        }
        if (is_block_start(t, tag)) { in_block = true; continue; }
        std::string dir, arg;
        if (parse_ovpn_line(t, dir, arg) && dir == "auth-user-pass" && arg.empty()) {
            need = true; break;
        }
    }
    fclose(f);
    return need;
}

// ---------------------------------------------------------------------------
// Widget: interruptor moderno estilo "toggle"
// ---------------------------------------------------------------------------

class ToggleSwitch : public Fl_Widget {
public:
    ToggleSwitch(int x, int y, int w, int h, const char *l = 0)
        : Fl_Widget(x, y, w, h, l), on_(false) {
        box(FL_NO_BOX);
    }
    void set(bool v) { if (v != on_) { on_ = v; redraw(); } }
    bool value() const { return on_; }

    void draw() override {
        int X = x(), Y = y(), W = w(), H = h();
        int r = H / 2;
        fl_color(on_ ? fl_rgb_color(52, 199, 89) : fl_rgb_color(178, 178, 178));
        fl_rectf(X + r, Y, W - 2 * r, H);
        fl_pie(X, Y, H, H, 0.0, 360.0);
        fl_pie(X + W - H, Y, H, H, 0.0, 360.0);
        int kd = H - 10;
        int kx = on_ ? (X + W - 5 - kd) : (X + 5);
        int ky = Y + 5;
        fl_color(fl_rgb_color(210, 210, 210));
        fl_pie(kx + 1, ky + 2, kd, kd, 0.0, 360.0);
        fl_color(FL_WHITE);
        fl_pie(kx, ky, kd, kd, 0.0, 360.0);
        fl_color(FL_WHITE);
        fl_font(FL_HELVETICA_BOLD, 13);
        const char *t = on_ ? "ON" : "OFF";
        int tw = 0, th = 0;
        fl_measure(t, tw, th);
        int tx = on_ ? (X + 12) : (X + W - 12 - tw);
        fl_draw(t, tx, Y + (H + th) / 2 - 2);
    }

    int handle(int e) override {
        if (e == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE) {
            on_ = !on_;
            redraw();
            do_callback();
            return 1;
        }
        if (e == FL_KEYBOARD && (Fl::event_key() == ' ' || Fl::event_key() == FL_Enter)) {
            on_ = !on_;
            redraw();
            do_callback();
            return 1;
        }
        return Fl_Widget::handle(e);
    }

private:
    bool on_;
};

// ---------------------------------------------------------------------------
// Interfaz principal
// ---------------------------------------------------------------------------

static Fl_Window      *g_win     = 0;
static Fl_Hold_Browser *g_list   = 0;
static ToggleSwitch   *g_toggle = 0;
static Fl_Box         *g_status = 0;
static Fl_Box         *g_info   = 0;

static int selected_index() {
    if (!g_list) return -1;
    int v = g_list->value();
    return (v >= 1 && v <= (int)g_profiles.size()) ? v - 1 : -1;
}

static void refresh_list() {
    g_list->clear();
    for (size_t i = 0; i < g_profiles.size(); i++) {
        const Profile &p = g_profiles[i];
        std::string tag = (p.type == "wireguard") ? "WireGuard" : "OpenVPN";
        std::string mark = p.active ? "* " : "";
        g_list->add((mark + p.name + "   [" + tag + "]").c_str());
    }
    if (!g_profiles.empty() && g_list->value() < 1) g_list->value(1);
}

static void update_detail() {
    int idx = selected_index();
    if (idx < 0) {
        g_toggle->set(false);
        g_toggle->deactivate();
        g_status->label("Sin perfiles: importa un archivo .conf u .ovpn");
        g_info->label("");
        return;
    }
    const Profile &p = g_profiles[idx];
    g_toggle->activate();
    g_toggle->set(p.active);
    std::string s = p.active ? "Conectado" : "Desconectado";
    g_status->copy_label(s.c_str());
    g_status->labelcolor(p.active ? fl_rgb_color(30, 140, 60) : fl_rgb_color(150, 30, 30));
    std::string info = std::string(p.type == "wireguard" ? "WireGuard" : "OpenVPN") + "\n" + p.file;
    g_info->copy_label(info.c_str());
    g_info->redraw();
    g_status->redraw();
}

// Espera el resultado real de openvpn leyendo su log.
// Devuelve true con "Initialization Sequence Completed", false con el error.
static bool wait_ovpn_result(const std::string &logf, const std::string &pidf,
                             int timeout_s, std::string &err) {
    err = "tiempo de espera agotado (revisa el registro)";
    static const char *fatals[] = {
        "AUTH_FAILED",
        "Exiting due to fatal error",
        "Cannot resolve host address",
        "Options error",
        "Cannot open TUN/TAP",
        "TLS Error",
        0
    };
    for (int i = 0; i < timeout_s * 2; i++) {
        std::string tail;
        FILE *f = fopen(logf.c_str(), "r");
        if (f) {
            std::vector<std::string> lines;
            char buf[1024];
            while (fgets(buf, sizeof(buf), f)) lines.push_back(buf);
            fclose(f);
            size_t start = lines.size() > 40 ? lines.size() - 40 : 0;
            for (size_t k = start; k < lines.size(); k++) tail += lines[k];
        }
        if (tail.find("Initialization Sequence Completed") != std::string::npos)
            return true;
        for (int k = 0; fatals[k]; k++) {
            size_t pos = tail.find(fatals[k]);
            if (pos != std::string::npos) {
                size_t a = tail.rfind('\n', pos);
                a = (a == std::string::npos) ? 0 : a + 1;
                size_t b = tail.find('\n', pos);
                b = (b == std::string::npos) ? tail.size() : b;
                err = trim(tail.substr(a, b - a));
                return false;
            }
        }
        if (i > 2 && !pid_alive(pidf)) {
            err = "el proceso openvpn termino inesperadamente (revisa el registro)";
            return false;
        }
        Fl::wait(0.5);
    }
    return false;
}

static void kill_ovpn(const Profile &p) {
    std::string pidf = ovpn_pidfile(p);
    FILE *f = fopen(pidf.c_str(), "r");
    if (f) {
        int pid = 0;
        if (fscanf(f, "%d", &pid) != 1) pid = 0;
        fclose(f);
        if (pid > 0) kill(pid, SIGTERM);
        unlink(pidf.c_str());
    }
}

static void activate_profile(Profile &p) {
    log_msg("$ activando " + p.name + " ...");
    if (p.type == "wireguard") {
        int rc0 = 0;
        run_capture("command -v wg-quick >/dev/null", &rc0);
        if (rc0 != 0) {
            log_msg("Fallo: wg-quick no encontrado. Instala el paquete wireguard-tools.");
            p.active = false;
            return;
        }
        int rc = 0;
        run_capture("modprobe wireguard >/dev/null 2>&1", &rc);
        std::string out = run_capture("wg-quick up " + shell_quote(p.file), &rc);
        log_msg(out);
        p.active = check_active(p);
        log_msg(p.active ? "WireGuard conectado." : "Fallo al conectar WireGuard.");
    } else {
        int rc0 = 0;
        run_capture("command -v openvpn >/dev/null", &rc0);
        if (rc0 != 0) {
            log_msg("Fallo: openvpn no encontrado. Instala el paquete openvpn.");
            p.active = false;
            return;
        }
        std::string pidf = ovpn_pidfile(p);
        std::string logf = ovpn_logfile(p);
        std::string credf = ovpn_credfile(p);
        FILE *lf = fopen(logf.c_str(), "w"); // truncar log del intento
        if (lf) fclose(lf);

        std::string extra;
        bool have_cred = false;
        if (ovpn_needs_auth(p.file)) {
            const char *user = fl_input("Usuario VPN para '%s':", 0, p.name.c_str());
            if (!user) { log_msg("Conexion cancelada."); p.active = false; return; }
            const char *pass = fl_password("Contrasena VPN para '%s':", 0, p.name.c_str());
            if (!pass) { log_msg("Conexion cancelada."); p.active = false; return; }
            FILE *cf = fopen(credf.c_str(), "w");
            if (!cf) {
                log_msg("Fallo: no se pudo crear el archivo temporal de credenciales.");
                p.active = false;
                return;
            }
            fprintf(cf, "%s\n%s\n", user, pass);
            fclose(cf);
            chmod(credf.c_str(), 0600);
            extra = " --auth-user-pass " + shell_quote(credf);
            have_cred = true;
        }

        std::string cmd = "openvpn --daemon --config " + shell_quote(p.file) +
                          " --writepid " + shell_quote(pidf) +
                          " --log-append " + shell_quote(logf) + extra;
        int rc = 0;
        run_capture(cmd, &rc);

        std::string err;
        bool ok = wait_ovpn_result(logf, pidf, 15, err);
        p.active = ok;
        if (ok) {
            log_msg("OpenVPN conectado.");
        } else {
            log_msg("Fallo al conectar OpenVPN: " + err);
            kill_ovpn(p);
            if (have_cred) unlink(credf.c_str());
        }
    }
}

static void deactivate_profile(Profile &p) {
    log_msg("$ desactivando " + p.name + " ...");
    if (p.type == "wireguard") {
        int rc = 0;
        std::string out = run_capture("wg-quick down " + shell_quote(p.file), &rc);
        log_msg(out);
    } else {
        kill_ovpn(p);
        unlink(ovpn_credfile(p).c_str()); // borrar credenciales temporales
        log_msg("Proceso OpenVPN detenido.");
    }
    p.active = check_active(p);
    if (!p.active) log_msg("Desconectado.");
}

static void toggle_cb(Fl_Widget *, void *) {
    int idx = selected_index();
    if (idx < 0) { g_toggle->set(false); return; }
    Profile &p = g_profiles[idx];
    if (g_toggle->value()) activate_profile(p);
    else deactivate_profile(p);
    g_toggle->set(p.active); // sincroniza con el estado real
    refresh_list();
    g_list->value(idx + 1);
    update_detail();
}

static void list_cb(Fl_Widget *, void *) {
    update_detail();
}

static void import_cb(Fl_Widget *w, void *) {
    const char *kind = (const char *)w->user_data(); // "wireguard" | "openvpn"
    bool is_wg = strcmp(kind, "wireguard") == 0;
    Fl_Native_File_Chooser fc;
    fc.title("Importar archivo de configuracion VPN");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fc.filter(is_wg ? "WireGuard\t*.conf\n" : "OpenVPN\t*.ovpn\n");
    if (fc.show() != 0) return;
    std::string src = fc.filename();
    std::string base = sanitize(strip_ext(basename_of(src)));
    std::string ext = is_wg ? ".conf" : ".ovpn";
    ensure_dir(profiles_dir());

    // Nombre destino unico
    std::string dst = profiles_dir() + "/" + base + ext;
    int n = 1;
    while (file_exists(dst)) {
        char tmp[32]; snprintf(tmp, sizeof(tmp), "_%d", ++n);
        dst = profiles_dir() + "/" + base + tmp + ext;
    }
    std::string final_base = strip_ext(basename_of(dst));

    bool ok;
    if (is_wg) {
        ok = copy_file(src, dst);
        if (ok && !file_contains_directive(src, "[Interface]"))
            log_msg("Aviso: el .conf no contiene seccion [Interface].");
    } else {
        ok = import_ovpn_file(src, dst, final_base);
    }
    if (!ok) {
        fl_alert("No se pudo importar el archivo.");
        return;
    }
    log_msg("Perfil importado: " + dst);
    load_profiles();
    refresh_list();
    for (size_t i = 0; i < g_profiles.size(); i++)
        if (g_profiles[i].file == dst) { g_list->value((int)i + 1); break; }
    update_detail();
}

static void delete_cb(Fl_Widget *, void *) {
    int idx = selected_index();
    if (idx < 0) return;
    Profile &p = g_profiles[idx];
    if (p.active) {
        if (!fl_choice("El perfil esta activo. Desconectarlo y eliminarlo?", "Cancelar", "Eliminar", 0))
            return;
        deactivate_profile(p);
    } else {
        if (!fl_choice("%s", "Cancelar", "Eliminar", 0,
                       ("Eliminar el perfil '" + p.name + "'?").c_str()))
            return;
    }
    unlink(p.file.c_str());
    unlink(ovpn_pidfile(p).c_str());
    unlink(ovpn_logfile(p).c_str());
    unlink(ovpn_credfile(p).c_str());
    // archivos externos integrados al perfil (prefijo nombre_)
    DIR *d = opendir(profiles_dir().c_str());
    if (d) {
        struct dirent *e;
        std::string pre = p.name + "_";
        while ((e = readdir(d)) != 0) {
            std::string n = e->d_name;
            if (n.compare(0, pre.size(), pre) == 0)
                unlink((profiles_dir() + "/" + n).c_str());
        }
        closedir(d);
    }
    log_msg("Perfil eliminado: " + p.name);
    load_profiles();
    refresh_list();
    update_detail();
}

static void timer_cb(void *) {
    bool changed = false;
    for (size_t i = 0; i < g_profiles.size(); i++) {
        bool a = check_active(g_profiles[i]);
        if (a != g_profiles[i].active) { g_profiles[i].active = a; changed = true; }
    }
    if (changed) { refresh_list(); update_detail(); }
    Fl::repeat_timeout(3.0, timer_cb);
}

static Fl_Button *style_btn(Fl_Button *b, Fl_Color c) {
    b->box(FL_ROUNDED_BOX);
    b->color(c);
    b->labelcolor(FL_WHITE);
    b->labelfont(FL_HELVETICA_BOLD);
    b->labelsize(13);
    return b;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    // Entorno autocontenido: todo vive en /usr/local
    setenv("PATH", "/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin", 1);
    Fl::scheme("gtk+");

    g_win = new Fl_Window(620, 600, APP_NAME);
    g_win->color(fl_rgb_color(245, 245, 247));

    int m = 14; // margen

    Fl_Box *title = new Fl_Box(m, m, 400, 34, "FLConnect");
    title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    title->labelfont(FL_HELVETICA_BOLD);
    title->labelsize(26);
    title->labelcolor(fl_rgb_color(30, 30, 40));

    Fl_Box *sub = new Fl_Box(m, m + 34, 400, 20, "Gestor de VPN");
    sub->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    sub->labelsize(13);
    sub->labelcolor(fl_rgb_color(110, 110, 120));

    int list_y = m + 74;
    int list_h = 280;
    g_list = new Fl_Hold_Browser(m, list_y, 360, list_h, "Perfiles");
    g_list->align(FL_ALIGN_TOP_LEFT);
    g_list->labelfont(FL_HELVETICA_BOLD);
    g_list->textfont(FL_HELVETICA);
    g_list->textsize(14);
    g_list->callback(list_cb);

    // Panel derecho: interruptor + estado
    int px = m + 360 + 16;
    int pw = 620 - px - m;

    Fl_Box *swlabel = new Fl_Box(px, list_y, pw, 22, "Conexion");
    swlabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    swlabel->labelfont(FL_HELVETICA_BOLD);
    swlabel->labelsize(14);

    g_toggle = new ToggleSwitch(px, list_y + 28, 150, 52);
    g_toggle->callback(toggle_cb);
    g_toggle->tooltip("Activar / desactivar la VPN seleccionada");

    g_status = new Fl_Box(px, list_y + 88, pw, 24, "Desconectado");
    g_status->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    g_status->labelfont(FL_HELVETICA_BOLD);
    g_status->labelsize(15);

    g_info = new Fl_Box(px, list_y + 118, pw, 56, "");
    g_info->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    g_info->labelsize(11);
    g_info->labelcolor(fl_rgb_color(100, 100, 110));

    Fl_Button *b_wg = style_btn(new Fl_Button(px, list_y + 186, pw, 34, "+  Importar WireGuard"),
                               fl_rgb_color(0, 122, 255));
    b_wg->user_data((void *)"wireguard");
    b_wg->callback(import_cb);

    Fl_Button *b_ov = style_btn(new Fl_Button(px, list_y + 186 + 42, pw, 34, "+  Importar OpenVPN"),
                               fl_rgb_color(88, 86, 214));
    b_ov->user_data((void *)"openvpn");
    b_ov->callback(import_cb);

    Fl_Button *b_del = style_btn(new Fl_Button(px, list_y + 186 + 84, pw, 34, "Eliminar perfil"),
                                fl_rgb_color(200, 60, 60));
    b_del->callback(delete_cb);

    // Registro (debajo de la columna mas alta: lista o botones)
    int log_y = list_y + 316;
    Fl_Box *loglabel = new Fl_Box(m, log_y, 200, 20, "Registro");
    loglabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    loglabel->labelfont(FL_HELVETICA_BOLD);
    loglabel->labelsize(13);

    g_logbuf = new Fl_Text_Buffer();
    Fl_Text_Display *logview = new Fl_Text_Display(m, log_y + 22, 620 - 2 * m, 600 - (log_y + 22) - 34);
    logview->buffer(g_logbuf);
    logview->textfont(FL_COURIER);
    logview->textsize(11);
    logview->box(FL_DOWN_BOX);

    if (geteuid() != 0) {
        Fl_Box *warn = new Fl_Box(m, 600 - 26, 620 - 2 * m, 20,
            "Aviso: ejecutalo como root (sudo flconnect) para poder activar las VPN.");
        warn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        warn->labelsize(11);
        warn->labelcolor(fl_rgb_color(170, 110, 20));
    }

    g_win->end();
    g_win->show(argc, argv);

    log_msg(APP_NAME " " APP_VERSION " - listo.");
    log_msg("Perfiles en: " + profiles_dir());

    load_profiles();
    refresh_list();
    update_detail();

    Fl::add_timeout(3.0, timer_cb);
    return Fl::run();
}
