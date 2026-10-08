#ifndef FLTV_BACKEND_H
#define FLTV_BACKEND_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FLTV_VERSION   "2.1.0"
#define MAX_URL        2048
#define MAX_ACCS       256
#define MAX_LOCAL      128
#define MAX_VARIANTS   10

typedef struct {
    char label[128], host[512], user[128], pass[128];
} SavedAcc;

typedef struct {
    char host[512], user[128], pass[128], exp_date[32];
    int  max_conn, active_conn;
} Account;

typedef struct {
    char name[256], url[2048], cat[128], logo[2048];
} M3uCh;

typedef struct {
    int  bandwidth;
    char label[16];
    char url[2048];
} HLSVariant;

typedef struct {
    char id[64];
    char name[256];
    char ext[16];
    char logo[2048];
} ContentItem;

typedef struct {
    char id[64];
    char name[256];
} CategoryItem;

extern Account  g_account;
extern SavedAcc g_personal[MAX_ACCS];
extern int      g_n_personal;
extern SavedAcc g_online[MAX_ACCS];
extern int      g_n_online;

extern M3uCh *g_m3u;
extern int    g_n_m3u;

char *backend_http_get(const char *url);
char *backend_http_get_timeout(const char *url, long timeout_secs);
unsigned char *backend_http_get_bytes(const char *url, long timeout_secs, size_t *out_len);

void accs_load(void);
void accs_save(void);
void online_accs_load(void);
int  account_connect(const char *host, const char *user, const char *pass);

int  xtream_load_categories(int ctype, CategoryItem *out, int max);
int  xtream_load_content(int ctype, const char *cat_id, ContentItem *out, int max);
void xtream_build_play_url(int ctype, const char *id, const char *ext, char *out, size_t outsz);
void xtream_build_live_m3u8_url(const char *id, char *out, size_t outsz);

typedef struct {
    char id[64];
    char label[256];
} EpisodeItem;
int xtream_load_episodes(const char *series_id, EpisodeItem *out, int max);

typedef struct {
    char plot[2048];
    char meta[256];
    char trailer[512];
} ContentInfo;
void xtream_fetch_info(int ctype, const char *id, ContentInfo *out);

int hls_fetch_variants(const char *m3u8_url, HLSVariant *out, int max);

void m3u_reset(void);
void m3u_add(const char *name, const char *url, const char *cat, const char *logo);
void parse_m3u_buf(const char *buf, const char *default_cat);
void online_m3u_load(volatile int *gen_ptr, int my_gen, void (*status_cb)(const char *));

void strip_emoji(char *dst, const char *src, size_t dstsz);
int  str_casefind(const char *haystack, const char *needle);

#ifdef __cplusplus
}
#endif

#endif
