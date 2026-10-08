#include "player.h"
#include "backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

#define MPLAYER_INPUT_CONF "/tmp/fltv_input.conf"
#define MAX_RECONNECTS 5
#define FEED_START_TIMEOUT 20
#define STALL_RESTART 90
#define FEED_SCRIPT "while :; do curl -s -L -A \"$FLTV_UA\" --connect-timeout 10 -y 20 -Y 1000 \"$FLTV_URL\"; r=$?; if [ $r -eq 23 ] || [ $r -gt 128 ]; then exit 0; fi; sleep 1; done"

static pid_t g_mpv_pid = -1;
static int   g_mpv_ready = 0;
static char  g_cur_url[MAX_URL] = "";
static char  g_cur_name[256] = "";
static int   g_is_live = 0;
static int   g_reconnect_count = 0;
static time_t g_play_start = 0;
static time_t g_reconnect_at = 0;
static int   g_quality = 1;
static HLSVariant g_variants[MAX_VARIANTS];
static int   g_n_variants = 0;
static int   g_cur_variant = 0;
static PlayerStatusCb g_status_cb = NULL;
static int   g_out_fd = -1;
static char  g_out_buf[1024];
static int   g_out_len = 0;
static double g_pos = 0;
static double g_len = 0;
static double g_resume = 0;
static int   g_ever_played = 0;
static double g_screen_aspect = 16.0 / 9.0;
static int   g_feed_ok = 1;
static int   g_feed_mode = 0;
static int   g_paused = 0;
static int   g_buffering = 0;
static int   g_announced = 0;
static time_t g_last_progress = 0;

void player_set_status_cb(PlayerStatusCb cb) { g_status_cb = cb; }

void player_set_screen_size(int w, int h) {
    if (w > 0 && h > 0) g_screen_aspect = (double)w / h;
}

static void report(const char *msg) { if (g_status_cb) g_status_cb(msg); }

static void safe_kill(pid_t *pid) {
    if (*pid <= 0) return;
    kill(-*pid, SIGTERM);
    kill(-*pid, SIGKILL);
    waitpid(*pid, NULL, 0);
    *pid = -1;
}

static void close_output(void) {
    if (g_out_fd >= 0) close(g_out_fd);
    g_out_fd = -1;
    g_out_len = 0;
}

static void parse_output_line(const char *ln) {
    while (*ln == ' ') ln++;
    if (strncmp(ln, "ID_LENGTH=", 10) == 0) {
        g_len = atof(ln + 10);
    } else if (strncmp(ln, "A:", 2) == 0 || strncmp(ln, "V:", 2) == 0) {
        double t = atof(ln + 2);
        g_paused = 0;
        if (t > 0 && t != g_pos) {
            g_pos = t;
            g_ever_played = 1;
            g_last_progress = time(NULL);
        }
    } else if (strstr(ln, "PAUSE")) {
        g_paused = 1;
    }
}

static void read_output(void) {
    if (g_out_fd < 0) return;
    for (;;) {
        int room = (int)sizeof(g_out_buf) - 1 - g_out_len;
        if (room <= 0) { g_out_len = 0; room = (int)sizeof(g_out_buf) - 1; }
        ssize_t n = read(g_out_fd, g_out_buf + g_out_len, room);
        if (n <= 0) break;
        g_out_len += (int)n;
        g_out_buf[g_out_len] = '\0';
        char *start = g_out_buf, *p;
        while ((p = strpbrk(start, "\r\n")) != NULL) {
            *p = '\0';
            parse_output_line(start);
            start = p + 1;
        }
        g_out_len = (int)strlen(start);
        memmove(g_out_buf, start, g_out_len + 1);
    }
}

static void kill_player(void) {
    safe_kill(&g_mpv_pid);
    close_output();
    g_cur_url[0] = '\0';
    g_mpv_ready = 0;
}

int player_get_quality(void) { return g_quality; }

const char *player_quality_label(void) {
    static char lbl[32];
    if (g_n_variants > 0) {
        snprintf(lbl, sizeof(lbl), "Cal: %s", g_variants[g_cur_variant].label);
    } else {
        static const char *ql[] = { "Cal: 240p", "Cal: 360p", "Cal: 480p", "Cal: 720p", "Cal: Orig" };
        strncpy(lbl, ql[g_quality], sizeof(lbl) - 1);
    }
    return lbl;
}

void player_set_variants(int n, void *variants) {
    g_n_variants = n;
    g_cur_variant = 0;
    if (n > 0) memcpy(g_variants, variants, sizeof(HLSVariant) * n);
}

static void launch_player(void) {
    safe_kill(&g_mpv_pid);
    g_mpv_ready = 0;
    g_play_start = time(NULL);

    FILE *f = fopen(MPLAYER_INPUT_CONF, "w");
    if (f) { fputs("q quit 42\nQ quit 42\nESC quit 42\nCLOSE_WIN quit 42\n", f); fclose(f); }

    static const int scale_h[] = { 240, 360, 480, 720, 0 };
    static const char *lavd[] = {
        "fast:skiploopfilter=all:skipidct=nonref",
        "fast:skiploopfilter=all:skipidct=nonref",
        "skiploopfilter=all:skipidct=nonref",
        "skiploopfilter=all:skipidct=nonref",
        "skiploopfilter=all:skipidct=nonref"
    };
    char icarg[80], vf_arg[160], lavdopts_arg[80], ss_arg[32], expand_arg[48];
    snprintf(icarg, sizeof(icarg), "conf=%s", MPLAYER_INPUT_CONF);

    const char *args[48];
    int i = 0;
    args[i++] = "mplayer";
    args[i++] = "-vo"; args[i++] = "xv,x11";
    args[i++] = "-ao"; args[i++] = "alsa,pulse,null";
    args[i++] = "-user-agent"; args[i++] = "IPTVSmartersPlayer";
    args[i++] = "-input"; args[i++] = icarg;
    args[i++] = "-mc"; args[i++] = "10";
    args[i++] = "-identify";
    args[i++] = "-zoom";

    snprintf(expand_arg, sizeof(expand_arg), "expand=:::::%.4f", g_screen_aspect);
    if (g_n_variants > 0) {
        args[i++] = "-vf"; args[i++] = expand_arg;
    } else {
        strncpy(lavdopts_arg, lavd[g_quality], sizeof(lavdopts_arg) - 1);
        args[i++] = "-lavdopts"; args[i++] = lavdopts_arg;
        vf_arg[0] = '\0';
        if (scale_h[g_quality] > 0)
            snprintf(vf_arg + strlen(vf_arg), sizeof(vf_arg) - strlen(vf_arg), "scale=-2:%d,", scale_h[g_quality]);
        snprintf(vf_arg + strlen(vf_arg), sizeof(vf_arg) - strlen(vf_arg), "%s", expand_arg);
        args[i++] = "-vf"; args[i++] = vf_arg;
    }
    args[i++] = "-sws"; args[i++] = "0";
    args[i++] = "-framedrop";

    static char cache_arg[16];
    static char lavfdopts_arg[400];
    if (g_is_live) {
        strncpy(cache_arg, "16384", sizeof(cache_arg) - 1);
        args[i++] = "-cache"; args[i++] = cache_arg;
        args[i++] = "-cache-min"; args[i++] = "15";
        args[i++] = "-correct-pts";
        args[i++] = "-autosync"; args[i++] = "30";
        snprintf(lavfdopts_arg, sizeof(lavfdopts_arg),
                 "o=reconnect=1,reconnect_streamed=1,reconnect_at_eof=1,"
                 "reconnect_delay_max=3,reconnect_on_http_error=400,401,403,404,500,502,503,504,"
                 "timeout=12000000,live_start_index=-1,fflags=discardcorrupt:analyzeduration=1000000");
        args[i++] = "-lavfdopts"; args[i++] = lavfdopts_arg;
    } else {
        strncpy(cache_arg, "8192", sizeof(cache_arg) - 1);
        args[i++] = "-cache"; args[i++] = cache_arg;
        args[i++] = "-correct-pts";
        args[i++] = "-autosync"; args[i++] = "30";
        args[i++] = "-lavfdopts"; args[i++] = "o=reconnect=1";
        if (g_resume > 1) {
            snprintf(ss_arg, sizeof(ss_arg), "%.0f", g_resume);
            args[i++] = "-ss"; args[i++] = ss_arg;
        }
    }
    g_resume = 0;

    const char *url = (g_n_variants > 0) ? g_variants[g_cur_variant].url : g_cur_url;
    const char *feed_url = url;
    g_feed_mode = g_is_live && g_n_variants == 0 && g_feed_ok && !strstr(url, ".m3u8");
    if (g_feed_mode) {
        args[i++] = "-noconsolecontrols";
        url = "-";
    }
    g_paused = 0;
    g_buffering = 0;
    g_last_progress = time(NULL);
    static char url_buf[MAX_URL + 16];
    if (!g_is_live) {
        snprintf(url_buf, sizeof(url_buf), "ffmpeg://%s", url);
        url = url_buf;
    }
    args[i++] = url;
    args[i] = NULL;

    static char env_url[MAX_URL + 16];
    snprintf(env_url, sizeof(env_url), "FLTV_URL=%s", feed_url);
    char *feed_env[] = { env_url, (char *)"FLTV_UA=IPTVSmartersPlayer",
                         (char *)"PATH=/usr/local/bin:/usr/bin:/bin", NULL };

    close_output();
    int pfd[2] = { -1, -1 };
    if (pipe(pfd) == 0) {
        fcntl(pfd[0], F_SETFL, O_NONBLOCK);
        fcntl(pfd[0], F_SETFD, FD_CLOEXEC);
        fcntl(pfd[1], F_SETFL, O_NONBLOCK);
    }

    pid_t pid = fork();
    if (pid == 0) {
        setpgid(0, 0);
        int dn = open("/dev/null", O_WRONLY);
        if (dn >= 0) { dup2(dn, STDERR_FILENO); dup2(dn, STDOUT_FILENO); close(dn); }
        if (pfd[1] >= 0) { dup2(pfd[1], STDOUT_FILENO); close(pfd[1]); }
        if (g_feed_mode) {
            int dp[2];
            if (pipe(dp) == 0) {
                pid_t f = fork();
                if (f == 0) {
                    close(dp[0]);
                    dup2(dp[1], STDOUT_FILENO);
                    close(dp[1]);
                    int nul = open("/dev/null", O_RDONLY);
                    if (nul >= 0) { dup2(nul, STDIN_FILENO); close(nul); }
                    execle("/bin/sh", "sh", "-c", FEED_SCRIPT, (char *)NULL, feed_env);
                    _exit(1);
                }
                close(dp[1]);
                dup2(dp[0], STDIN_FILENO);
                close(dp[0]);
            }
        }
        execvp("mplayer", (char *const *)args);
        _exit(1);
    }
    if (pfd[1] >= 0) close(pfd[1]);
    if (pid > 0) { g_mpv_pid = pid; g_out_fd = pfd[0]; }
    else if (pfd[0] >= 0) close(pfd[0]);
}

void player_play(const char *url, int is_live, const char *display_name) {
    kill_player();
    g_reconnect_count = 0;
    strncpy(g_cur_url, url, MAX_URL - 1);
    strncpy(g_cur_name, display_name ? display_name : "", sizeof(g_cur_name) - 1);
    g_is_live = is_live;
    g_n_variants = 0;
    g_pos = 0;
    g_len = 0;
    g_resume = 0;
    g_ever_played = 0;
    g_announced = 0;
    g_feed_ok = 1;
    launch_player();
}

void player_stop(void) {
    kill_player();
}

int player_is_playing(void) { return g_mpv_pid > 0; }

void player_set_quality(int q) {
    if (q < 0 || q > 4) return;
    g_quality = q;
    if (g_mpv_pid > 0 && g_cur_url[0]) {
        if (!g_is_live) g_resume = g_pos;
        launch_player();
    }
}

void player_cycle_variant_or_quality(void) {
    if (g_n_variants > 0) {
        g_cur_variant = (g_cur_variant + 1) % g_n_variants;
        if (g_mpv_pid > 0) launch_player();
    } else {
        g_quality = (g_quality + 1) % 5;
        if (g_mpv_pid > 0 && g_cur_url[0]) {
            if (!g_is_live) g_resume = g_pos;
            launch_player();
        }
    }
}

void player_poll(void) {
    if (g_reconnect_at > 0) {
        if (time(NULL) >= g_reconnect_at) {
            g_reconnect_at = 0;
            launch_player();
        }
        return;
    }
    if (g_mpv_pid <= 0) return;
    read_output();
    int st;
    pid_t r = waitpid(g_mpv_pid, &st, WNOHANG);
    if (r == 0) {
        g_mpv_ready++;
        time_t now = time(NULL);
        char msg[300];
        if (g_ever_played && !g_announced) {
            g_announced = 1;
            snprintf(msg, sizeof(msg), "Reproduciendo: %s", g_cur_name);
            report(msg);
        }
        if (!g_is_live) return;
        if (g_feed_mode && !g_ever_played && now - g_play_start > FEED_START_TIMEOUT) {
            g_feed_ok = 0;
            report("Conectando en modo directo...");
            launch_player();
            return;
        }
        if (!g_ever_played || g_paused) return;
        time_t idle = now - g_last_progress;
        if (idle >= 3 && !g_buffering) {
            g_buffering = 1;
            report("Cargando buffer...");
        } else if (idle < 3 && g_buffering) {
            g_buffering = 0;
            snprintf(msg, sizeof(msg), "Reproduciendo: %s", g_cur_name);
            report(msg);
        }
        if (idle > STALL_RESTART && g_reconnect_count < MAX_RECONNECTS) {
            g_reconnect_count++;
            snprintf(msg, sizeof(msg), "Reconectando (%d/%d): %s...", g_reconnect_count, MAX_RECONNECTS, g_cur_name);
            report(msg);
            launch_player();
        }
        return;
    }
    kill(-g_mpv_pid, SIGKILL);
    read_output();
    close_output();
    g_mpv_pid = -1;
    g_mpv_ready = 0;
    int user_quit = (WIFEXITED(st) && WEXITSTATUS(st) == 42);
    if (user_quit || r < 0 || !g_cur_url[0]) {
        g_cur_url[0] = '\0';
        g_reconnect_count = 0;
        if (user_quit) report("");
        return;
    }
    time_t uptime = time(NULL) - g_play_start;
    if (uptime >= 60) g_reconnect_count = 0;
    int finished = !g_is_live && g_len > 0 && g_pos >= g_len - 20;
    if (finished) {
        g_cur_url[0] = '\0';
        g_reconnect_count = 0;
        report("Fin de la reproduccion");
        return;
    }
    int can_retry = g_is_live ? (g_ever_played || uptime >= 5) : g_ever_played;
    if (can_retry && g_reconnect_count < MAX_RECONNECTS) {
        g_reconnect_count++;
        if (!g_is_live) g_resume = g_pos > 5 ? g_pos - 3 : 0;
        char msg[300];
        snprintf(msg, sizeof(msg), "Reconectando (%d/%d): %s...", g_reconnect_count, MAX_RECONNECTS, g_cur_name);
        report(msg);
        g_reconnect_at = time(NULL) + (g_is_live ? 4 : 2);
        return;
    }
    g_cur_url[0] = '\0';
    g_reconnect_count = 0;
    if (!g_ever_played)
        report(g_is_live ? "No se pudo abrir el canal - intenta con otro"
                         : "El servidor rechazo el contenido - intenta con otro");
    else
        report(g_is_live ? "Sin senal - no se pudo reconectar, intenta con otro canal"
                         : "Se perdio la conexion - no se pudo reanudar");
}
