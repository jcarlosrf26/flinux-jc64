#include "backend.h"
#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define UA        "IPTVSmartersPlayer"
#define CFG_DIR   "/.config/fltv"
#define ACCS_FILE "/.config/fltv/accounts.json"
#define URL_ACCS  "https://gitlab.com/AnddyCort/fltv/-/raw/main/accounts.json"
#define URL_CH    "https://gitlab.com/AnddyCort/fltv/-/raw/main/channels.json"
#define URL_REPO_TREE "https://gitlab.com/api/v4/projects/AnddyCort%2Ffltv/repository/tree?path=listas"
#define URL_RAW_BASE  "https://gitlab.com/AnddyCort/fltv/-/raw/main/listas/"

static const char *jstr(json_object *o) {
    if (!o) return "";
    const char *s = json_object_get_string(o);
    return s ? s : "";
}

Account  g_account;
SavedAcc g_personal[MAX_ACCS];
int      g_n_personal = 0;
SavedAcc g_online[MAX_ACCS];
int      g_n_online = 0;

M3uCh *g_m3u = NULL;
static int g_cap_m3u = 0;
int    g_n_m3u = 0;

typedef struct { char *data; size_t size; } Buf;

static size_t write_cb(void *ptr, size_t sz, size_t n, void *ud) {
    Buf *b = ud;
    size_t tot = sz * n;
    if (b->size + tot > 64 * 1024 * 1024) return 0;
    char *tmp = realloc(b->data, b->size + tot + 1);
    if (!tmp) return 0;
    b->data = tmp;
    memcpy(b->data + b->size, ptr, tot);
    b->size += tot;
    b->data[b->size] = '\0';
    return tot;
}

char *backend_http_get_timeout(const char *url, long timeout_secs) {
    CURL *c = curl_easy_init();
    if (!c) return NULL;
    Buf b = { malloc(1), 0 };
    b.data[0] = '\0';
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_USERAGENT, UA);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeout_secs);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    CURLcode res = curl_easy_perform(c);
    curl_easy_cleanup(c);
    if (res != CURLE_OK) { free(b.data); return NULL; }
    return b.data;
}

char *backend_http_get(const char *url) { return backend_http_get_timeout(url, 20L); }

unsigned char *backend_http_get_bytes(const char *url, long timeout_secs, size_t *out_len) {
    CURL *c = curl_easy_init();
    if (!c) return NULL;
    Buf b = { malloc(1), 0 };
    b.data[0] = '\0';
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_USERAGENT, UA);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeout_secs);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    CURLcode res = curl_easy_perform(c);
    curl_easy_cleanup(c);
    if (res != CURLE_OK || b.size == 0) { free(b.data); return NULL; }
    if (out_len) *out_len = b.size;
    return (unsigned char *)b.data;
}

void strip_emoji(char *dst, const char *src, size_t dstsz) {
    size_t di = 0;
    const unsigned char *s = (const unsigned char *)src;
    while (*s && di + 1 < dstsz) {
        unsigned char c = *s;
        int len = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
        if (len == 4) { s += 4; continue; }
        if (len == 3 && s[1] && s[2]) {
            uint32_t cp = ((uint32_t)(c & 0x0F) << 12) |
                          ((uint32_t)(s[1] & 0x3F) << 6) |
                           (uint32_t)(s[2] & 0x3F);
            if ((cp >= 0x2100 && cp <= 0x2BFF) ||
                (cp >= 0x3000 && cp <= 0x303F) ||
                (cp >= 0xFE00 && cp <= 0xFEFF)) {
                s += 3; continue;
            }
        }
        if (di + len < dstsz) {
            for (int j = 0; j < len && s[j]; j++) dst[di++] = (char)s[j];
        }
        s += len;
    }
    while (di > 0 && (unsigned char)dst[di - 1] <= ' ') di--;
    dst[di] = '\0';
}

int str_casefind(const char *haystack, const char *needle) {
    if (!needle || !*needle) return 1;
    size_t hn = strlen(haystack), nn = strlen(needle);
    if (nn > hn) return 0;
    for (size_t i = 0; i + nn <= hn; i++) {
        size_t j = 0;
        for (; j < nn; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) break;
        }
        if (j == nn) return 1;
    }
    return 0;
}

static void cfg_ensure(void) {
    const char *h = getenv("HOME");
    if (!h) return;
    char d[512];
    snprintf(d, sizeof(d), "%s%s", h, CFG_DIR);
    mkdir(d, 0700);
}

static int parse_accs(const char *buf, SavedAcc *arr, int max) {
    json_object *a = json_tokener_parse(buf);
    if (!a || !json_object_is_type(a, json_type_array)) { if (a) json_object_put(a); return 0; }
    int n = 0, total = json_object_array_length(a);
    for (int i = 0; i < total && n < max; i++) {
        json_object *obj = json_object_array_get_idx(a, i), *o;
        SavedAcc *s = &arr[n];
        memset(s, 0, sizeof(SavedAcc));
        if (json_object_object_get_ex(obj, "label", &o))    strncpy(s->label, jstr(o), 127);
        if (json_object_object_get_ex(obj, "host", &o))     strncpy(s->host, jstr(o), 511);
        if (json_object_object_get_ex(obj, "username", &o)) strncpy(s->user, jstr(o), 127);
        if (json_object_object_get_ex(obj, "password", &o)) strncpy(s->pass, jstr(o), 127);
        if (s->host[0] && s->user[0]) n++;
    }
    json_object_put(a);
    return n;
}

void accs_load(void) {
    const char *h = getenv("HOME");
    if (!h) return;
    char f[512];
    snprintf(f, sizeof(f), "%s%s", h, ACCS_FILE);
    FILE *fp = fopen(f, "r");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    if (sz <= 0 || sz > 256 * 1024) { fclose(fp); return; }
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, fp);
    fclose(fp);
    buf[sz] = '\0';
    g_n_personal = parse_accs(buf, g_personal, MAX_ACCS);
    free(buf);
}

void accs_save(void) {
    const char *h = getenv("HOME");
    if (!h) return;
    char f[512];
    snprintf(f, sizeof(f), "%s%s", h, ACCS_FILE);
    cfg_ensure();
    FILE *fp = fopen(f, "w");
    if (!fp) return;
    fprintf(fp, "[\n");
    for (int i = 0; i < g_n_personal; i++) {
        SavedAcc *a = &g_personal[i];
        fprintf(fp, "  {\"label\":\"%s\",\"host\":\"%s\",\"username\":\"%s\",\"password\":\"%s\"}%s\n",
                a->label, a->host, a->user, a->pass, i < g_n_personal - 1 ? "," : "");
    }
    fprintf(fp, "]\n");
    fclose(fp);
}

void online_accs_load(void) {
    g_n_online = 0;
    char *resp = backend_http_get(URL_ACCS);
    if (!resp) return;
    g_n_online = parse_accs(resp, g_online, MAX_ACCS);
    free(resp);
}

int account_connect(const char *host, const char *user, const char *pass) {
    char url[MAX_URL];
    snprintf(url, MAX_URL, "%s/player_api.php?username=%s&password=%s", host, user, pass);
    char *resp = backend_http_get(url);
    if (!resp) return 0;
    json_object *root = json_tokener_parse(resp);
    free(resp);
    if (!root) return 0;
    json_object *ui = json_object_object_get(root, "user_info");
    if (!ui) { json_object_put(root); return 0; }
    json_object *auth = json_object_object_get(ui, "auth");
    if (!auth || json_object_get_int(auth) != 1) { json_object_put(root); return 0; }
    strncpy(g_account.host, host, 511);
    strncpy(g_account.user, user, 127);
    strncpy(g_account.pass, pass, 127);
    json_object *o;
    g_account.max_conn    = (o = json_object_object_get(ui, "max_connections")) ? json_object_get_int(o) : 0;
    g_account.active_conn = (o = json_object_object_get(ui, "active_cons"))     ? json_object_get_int(o) : 0;
    o = json_object_object_get(ui, "exp_date");
    if (o) {
        const char *es = jstr(o);
        if (es && *es && strcmp(es, "null") != 0) {
            time_t t = (time_t)atol(es);
            strftime(g_account.exp_date, 32, "%Y-%m-%d", localtime(&t));
        } else {
            strncpy(g_account.exp_date, "Sin limite", 31);
        }
    }
    json_object_put(root);
    return 1;
}

int xtream_load_categories(int ctype, CategoryItem *out, int max) {
    static const char *acts[] = { "get_live_categories", "get_vod_categories", "get_series_categories" };
    char url[MAX_URL];
    snprintf(url, MAX_URL, "%s/player_api.php?username=%s&password=%s&action=%s",
             g_account.host, g_account.user, g_account.pass, acts[ctype]);
    char *resp = backend_http_get(url);
    if (!resp) return 0;
    json_object *arr = json_tokener_parse(resp);
    free(resp);
    if (!arr || !json_object_is_type(arr, json_type_array)) { if (arr) json_object_put(arr); return 0; }
    int n = json_object_array_length(arr);
    int count = 0;
    for (int i = 0; i < n && count < max; i++) {
        json_object *c = json_object_array_get_idx(arr, i), *o;
        if (!json_object_object_get_ex(c, "category_id", &o)) continue;
        strncpy(out[count].id, jstr(o), 63);
        if (json_object_object_get_ex(c, "category_name", &o)) {
            strip_emoji(out[count].name, jstr(o), sizeof(out[count].name));
        }
        count++;
    }
    json_object_put(arr);
    return count;
}

int xtream_load_content(int ctype, const char *cat_id, ContentItem *out, int max) {
    static const char *acts[] = { "get_live_streams", "get_vod_streams", "get_series" };
    char url[MAX_URL];
    snprintf(url, MAX_URL, "%s/player_api.php?username=%s&password=%s&action=%s&category_id=%s",
             g_account.host, g_account.user, g_account.pass, acts[ctype], cat_id);
    char *resp = backend_http_get(url);
    if (!resp) return 0;
    json_object *arr = json_tokener_parse(resp);
    free(resp);
    if (!arr || !json_object_is_type(arr, json_type_array)) { if (arr) json_object_put(arr); return 0; }
    const char *id_key   = (ctype == 2) ? "series_id" : "stream_id";
    const char *logo_key = (ctype == 2) ? "cover"     : "stream_icon";
    int n = json_object_array_length(arr);
    int count = 0;
    for (int i = 0; i < n && count < max; i++) {
        json_object *s = json_object_array_get_idx(arr, i), *id_o, *nm_o, *ex_o, *lg_o;
        if (!json_object_object_get_ex(s, id_key, &id_o)) continue;
        if (!json_object_object_get_ex(s, "name", &nm_o)) continue;
        snprintf(out[count].id, sizeof(out[count].id), "%d", json_object_get_int(id_o));
        strip_emoji(out[count].name, jstr(nm_o), sizeof(out[count].name));
        out[count].ext[0] = '\0';
        out[count].logo[0] = '\0';
        if (ctype == 1 && json_object_object_get_ex(s, "container_extension", &ex_o)) {
            strncpy(out[count].ext, jstr(ex_o), sizeof(out[count].ext) - 1);
        }
        if (json_object_object_get_ex(s, logo_key, &lg_o)) {
            strncpy(out[count].logo, jstr(lg_o), sizeof(out[count].logo) - 1);
        }
        count++;
    }
    json_object_put(arr);
    return count;
}

void xtream_build_play_url(int ctype, const char *id, const char *ext, char *out, size_t outsz) {
    if (ctype == 0) {
        snprintf(out, outsz, "%s/live/%s/%s/%s.ts", g_account.host, g_account.user, g_account.pass, id);
    } else if (ctype == 1) {
        snprintf(out, outsz, "%s/movie/%s/%s/%s.%s", g_account.host, g_account.user, g_account.pass,
                 id, (ext && *ext) ? ext : "mp4");
    } else {
        snprintf(out, outsz, "%s/series/%s/%s/%s.%s", g_account.host, g_account.user, g_account.pass,
                 id, (ext && *ext) ? ext : "mkv");
    }
}

void xtream_build_live_m3u8_url(const char *id, char *out, size_t outsz) {
    snprintf(out, outsz, "%s/live/%s/%s/%s.m3u8", g_account.host, g_account.user, g_account.pass, id);
}

int xtream_load_episodes(const char *series_id, EpisodeItem *out, int max) {
    char url[MAX_URL];
    snprintf(url, MAX_URL, "%s/player_api.php?username=%s&password=%s&action=get_series_info&series_id=%s",
             g_account.host, g_account.user, g_account.pass, series_id);
    char *resp = backend_http_get(url);
    if (!resp) return 0;
    json_object *root = json_tokener_parse(resp);
    free(resp);
    if (!root) return 0;
    int count = 0;
    json_object *eps_obj = json_object_object_get(root, "episodes");
    if (eps_obj && json_object_is_type(eps_obj, json_type_object)) {
        json_object_object_foreach(eps_obj, skey, sarr) {
            if (!json_object_is_type(sarr, json_type_array)) continue;
            int sn = atoi(skey), n = json_object_array_length(sarr);
            for (int i = 0; i < n && count < max; i++) {
                json_object *ep = json_object_array_get_idx(sarr, i), *o;
                const char *eid = "", *ext = "mkv", *title = "";
                int en = 0;
                if (json_object_object_get_ex(ep, "id", &o))                  eid   = jstr(o);
                if (json_object_object_get_ex(ep, "container_extension", &o)) ext   = jstr(o);
                if (json_object_object_get_ex(ep, "title", &o))               title = jstr(o);
                if (json_object_object_get_ex(ep, "episode_num", &o))         en    = json_object_get_int(o);
                if (!eid || !*eid) continue;
                char clean_title[256];
                strip_emoji(clean_title, title && *title ? title : "", sizeof(clean_title));
                char idbuf[300];
                snprintf(idbuf, sizeof(idbuf), "%s|%s", eid, ext ? ext : "mkv");
                strncpy(out[count].id, idbuf, sizeof(out[count].id) - 1);
                if (clean_title[0]) {
                    snprintf(out[count].label, sizeof(out[count].label), "S%02dE%02d  %s", sn, en, clean_title);
                } else {
                    snprintf(out[count].label, sizeof(out[count].label), "S%02dE%02d", sn, en);
                }
                count++;
            }
        }
    }
    json_object_put(root);
    return count;
}

void xtream_fetch_info(int ctype, const char *id, ContentInfo *out) {
    memset(out, 0, sizeof(*out));
    char url[MAX_URL];
    if (ctype == 0) {
        snprintf(url, MAX_URL, "%s/player_api.php?username=%s&password=%s&action=get_short_epg&stream_id=%s&limit=2",
                 g_account.host, g_account.user, g_account.pass, id);
        char *resp = backend_http_get(url);
        strncpy(out->meta, "TV en Vivo", sizeof(out->meta) - 1);
        if (resp) {
            json_object *root = json_tokener_parse(resp);
            free(resp);
            if (root) {
                json_object *lst = json_object_object_get(root, "epg_listings");
                if (lst && json_object_is_type(lst, json_type_array) && json_object_array_length(lst) > 0) {
                    json_object *cur = json_object_array_get_idx(lst, 0), *o;
                    if (json_object_object_get_ex(cur, "title", &o)) {
                        strncpy(out->plot, jstr(o), sizeof(out->plot) - 1);
                    }
                }
                json_object_put(root);
            }
        }
    } else {
        if (ctype == 1) {
            snprintf(url, MAX_URL, "%s/player_api.php?username=%s&password=%s&action=get_vod_info&vod_id=%s",
                     g_account.host, g_account.user, g_account.pass, id);
        } else {
            snprintf(url, MAX_URL, "%s/player_api.php?username=%s&password=%s&action=get_series_info&series_id=%s",
                     g_account.host, g_account.user, g_account.pass, id);
        }
        char *resp = backend_http_get(url);
        if (resp) {
            json_object *root = json_tokener_parse(resp);
            free(resp);
            if (root) {
                json_object *info = json_object_object_get(root, "info"), *o;
                if (info) {
                    const char *pk = (ctype == 1) ? "description" : "plot";
                    const char *s = jstr(json_object_object_get(info, pk));
                    if (s) strncpy(out->plot, s, sizeof(out->plot) - 1);
                    if (!out->plot[0]) {
                        s = jstr(json_object_object_get(info, "plot"));
                        if (s) strncpy(out->plot, s, sizeof(out->plot) - 1);
                    }
                    char yr4[8] = "", rating[20] = "", dur[32] = "", genre[128] = "";
                    if (json_object_object_get_ex(info, "releasedate", &o)) {
                        const char *rd = jstr(o);
                        if (rd && strlen(rd) >= 4) { memcpy(yr4, rd, 4); yr4[4] = '\0'; }
                    }
                    #define CPJS(key,dst) do { const char *_s=jstr(json_object_object_get(info,(key))); if(_s) strncpy((dst),_s,sizeof(dst)-1); } while(0)
                    CPJS("rating", rating);
                    if (ctype == 1) CPJS("duration", dur);
                    CPJS("genre", genre);
                    CPJS("youtube_trailer", out->trailer);
                    #undef CPJS
                    char meta[256] = "";
                    if (yr4[0]) snprintf(meta + strlen(meta), sizeof(meta) - strlen(meta), "%s", yr4);
                    if (rating[0] && strcmp(rating, "0") != 0 && strcmp(rating, "0.0") != 0)
                        snprintf(meta + strlen(meta), sizeof(meta) - strlen(meta), "%s%s*", meta[0] ? " - " : "", rating);
                    if (dur[0]) snprintf(meta + strlen(meta), sizeof(meta) - strlen(meta), "%s%s", meta[0] ? " - " : "", dur);
                    if (genre[0]) snprintf(meta + strlen(meta), sizeof(meta) - strlen(meta), "\n%s", genre);
                    strncpy(out->meta, meta, sizeof(out->meta) - 1);
                }
                json_object_put(root);
            }
        }
    }
}

static const char *bw_to_label(int bw, const char *res) {
    if (res && *res) {
        int w = atoi(res);
        if (w >= 1920) return "1080p";
        if (w >= 1280) return "720p";
        if (w >= 854)  return "480p";
        if (w >= 640)  return "360p";
        if (w >= 426)  return "240p";
        if (w > 0)     return "Baja";
    }
    if (bw >= 3000000) return "1080p";
    if (bw >= 1500000) return "720p";
    if (bw >= 800000)  return "480p";
    if (bw >= 400000)  return "360p";
    return "Baja";
}

static int parse_hls_master(const char *data, const char *base, HLSVariant *out, int maxn) {
    int n = 0;
    const char *p = data;
    while (*p && n < maxn) {
        if (strncmp(p, "#EXT-X-STREAM-INF:", 18) == 0) {
            int bw = 0;
            char res[24] = "";
            const char *bwp = strstr(p, "BANDWIDTH=");
            if (bwp) bw = atoi(bwp + 10);
            const char *rp = strstr(p, "RESOLUTION=");
            if (rp) {
                rp += 11;
                const char *re = strpbrk(rp, ",\r\n ");
                size_t rl = re ? (size_t)(re - rp) : strlen(rp);
                if (rl < sizeof(res)) { memcpy(res, rp, rl); res[rl] = '\0'; }
            }
            p = strchr(p, '\n');
            if (!p) break;
            p++;
            while (*p == '\r') p++;
            if (*p == '#' || *p == '\0') continue;
            const char *ue = strpbrk(p, "\r\n");
            size_t ul = ue ? (size_t)(ue - p) : strlen(p);
            if (ul < 4 || ul >= sizeof(out[n].url)) { p = ue ? ue : p + ul; continue; }
            out[n].bandwidth = bw;
            strncpy(out[n].label, bw_to_label(bw, res), sizeof(out[n].label) - 1);
            if (strncmp(p, "http", 4) == 0) {
                memcpy(out[n].url, p, ul);
                out[n].url[ul] = '\0';
            } else {
                snprintf(out[n].url, sizeof(out[n].url), "%s/%.*s", base, (int)ul, p);
            }
            n++;
        }
        p = strchr(p, '\n');
        if (p) p++; else break;
    }
    for (int a = 0; a < n - 1; a++)
        for (int b = a + 1; b < n; b++)
            if (out[b].bandwidth > out[a].bandwidth) {
                HLSVariant tmp = out[a]; out[a] = out[b]; out[b] = tmp;
            }
    return n;
}

int hls_fetch_variants(const char *m3u8_url, HLSVariant *out, int max) {
    char *resp = backend_http_get(m3u8_url);
    if (!resp) return 0;
    int n = 0;
    if (strstr(resp, "#EXT-X-STREAM-INF")) {
        char base[2048];
        strncpy(base, m3u8_url, sizeof(base) - 1);
        base[sizeof(base) - 1] = '\0';
        char *sl = strrchr(base, '/');
        if (sl) *sl = '\0';
        n = parse_hls_master(resp, base, out, max);
    }
    free(resp);
    return n;
}

void m3u_reset(void) { g_n_m3u = 0; }

void m3u_add(const char *name, const char *url, const char *cat, const char *logo) {
    if (g_n_m3u >= g_cap_m3u) {
        g_cap_m3u = g_cap_m3u ? g_cap_m3u * 2 : 256;
        g_m3u = realloc(g_m3u, g_cap_m3u * sizeof(M3uCh));
    }
    M3uCh *ch = &g_m3u[g_n_m3u++];
    strncpy(ch->name, name, 255); ch->name[255] = '\0';
    strncpy(ch->url, url, 2047);  ch->url[2047] = '\0';
    strncpy(ch->cat, cat, 127);   ch->cat[127] = '\0';
    strncpy(ch->logo, logo ? logo : "", 2047); ch->logo[2047] = '\0';
}

void parse_m3u_buf(const char *buf, const char *default_cat) {
    const char *p = buf;
    char name[256] = "", cat[128] = "", logo[2048] = "";
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        char line[4096];
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, p, len);
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ')) len--;
        line[len] = '\0';
        if (strncmp(line, "#EXTINF", 7) == 0) {
            name[0] = '\0'; cat[0] = '\0';
            char *gt = strrchr(line, ',');
            if (gt) { char tmp[256]; strip_emoji(tmp, gt + 1, sizeof(tmp)); strncpy(name, tmp, 255); }
            char *gts = strstr(line, "group-title=\"");
            if (gts) {
                gts += 13;
                char *gte = strchr(gts, '"');
                if (gte) {
                    size_t gl = gte - gts; if (gl > 127) gl = 127;
                    char tmp[128]; memcpy(tmp, gts, gl); tmp[gl] = '\0';
                    strip_emoji(cat, tmp, sizeof(cat));
                }
            }
            char *lgs = strstr(line, "tvg-logo=\"");
            if (lgs) {
                lgs += 10;
                char *lge = strchr(lgs, '"');
                if (lge) {
                    size_t ll = lge - lgs; if (ll > 2047) ll = 2047;
                    memcpy(logo, lgs, ll); logo[ll] = '\0';
                }
            } else logo[0] = '\0';
        } else if (line[0] && line[0] != '#') {
            if (name[0]) m3u_add(name, line, cat[0] ? cat : default_cat, logo);
            name[0] = '\0'; logo[0] = '\0';
        }
        p = eol ? eol + 1 : p + strlen(p);
    }
}

void online_m3u_load(volatile int *gen_ptr, int my_gen, void (*status_cb)(const char *)) {
    char *resp = backend_http_get_timeout(URL_CH, 20L);
    if (*gen_ptr != my_gen) { free(resp); return; }
    if (resp) {
        json_object *arr = json_tokener_parse(resp);
        free(resp);
        if (arr && json_object_is_type(arr, json_type_array)) {
            int total = json_object_array_length(arr);
            for (int i = 0; i < total; i++) {
                if (*gen_ptr != my_gen) { json_object_put(arr); return; }
                json_object *obj = json_object_array_get_idx(arr, i), *o;
                const char *nm = "", *url = "", *cat = "", *logo = "";
                if (json_object_object_get_ex(obj, "name", &o))     nm = jstr(o);
                if (json_object_object_get_ex(obj, "url", &o))      url = jstr(o);
                if (json_object_object_get_ex(obj, "category", &o)) cat = jstr(o);
                if (json_object_object_get_ex(obj, "logo", &o))     logo = jstr(o);
                if (nm[0] && url[0]) m3u_add(nm, url, cat[0] ? cat : "General", logo);
            }
        }
        if (arr) json_object_put(arr);
    }

    char *tree_resp = backend_http_get_timeout(URL_REPO_TREE, 20L);
    if (*gen_ptr != my_gen) { free(tree_resp); return; }
    if (tree_resp) {
        json_object *tree = json_tokener_parse(tree_resp);
        free(tree_resp);
        if (tree && json_object_is_type(tree, json_type_array)) {
            int n = json_object_array_length(tree);
            for (int i = 0; i < n; i++) {
                if (*gen_ptr != my_gen) { json_object_put(tree); return; }
                json_object *obj = json_object_array_get_idx(tree, i), *o;
                const char *fname = "", *ftype = "";
                if (json_object_object_get_ex(obj, "name", &o)) fname = jstr(o);
                if (json_object_object_get_ex(obj, "type", &o)) ftype = jstr(o);
                if (strcmp(ftype, "blob") != 0) continue;
                size_t nl = strlen(fname);
                int is_m3u = (nl > 4 && strcasecmp(fname + nl - 4, ".m3u") == 0) ||
                             (nl > 5 && strcasecmp(fname + nl - 5, ".m3u8") == 0);
                if (!is_m3u) continue;
                char defcat[128];
                const char *us = strrchr(fname, '_');
                const char *dot = strrchr(fname, '.');
                if (us && dot && us < dot) {
                    size_t l = dot - (us + 1); if (l > 127) l = 127;
                    memcpy(defcat, us + 1, l); defcat[l] = '\0';
                } else {
                    size_t l = dot ? (size_t)(dot - fname) : strlen(fname); if (l > 127) l = 127;
                    memcpy(defcat, fname, l); defcat[l] = '\0';
                }
                char raw_url[512];
                snprintf(raw_url, sizeof(raw_url), "%s%s", URL_RAW_BASE, fname);
                if (status_cb) {
                    char msg[300];
                    snprintf(msg, sizeof(msg), "Descargando %s...", fname);
                    status_cb(msg);
                }
                char *buf = backend_http_get_timeout(raw_url, 120L);
                if (buf) {
                    if (*gen_ptr == my_gen) parse_m3u_buf(buf, defcat);
                    free(buf);
                }
            }
        }
        if (tree) json_object_put(tree);
    }
}
