#ifndef FLTV_PLAYER_H
#define FLTV_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PlayerStatusCb)(const char *msg);
void player_set_status_cb(PlayerStatusCb cb);
void player_set_screen_size(int w, int h);

void player_play(const char *url, int is_live, const char *display_name);
void player_stop(void);
int  player_is_playing(void);

void player_poll(void);

void player_set_quality(int q);
int  player_get_quality(void);
const char *player_quality_label(void);

void player_set_variants(int n, void *variants);
void player_cycle_variant_or_quality(void);

#ifdef __cplusplus
}
#endif

#endif
