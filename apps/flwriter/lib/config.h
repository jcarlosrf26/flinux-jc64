#define HAVE_FLTK_UTF 0
#define HAVE_LIBPNG 1
#define HAVE_LIBZ 1
#define HAVE_LIBJPEG 1
#define HAVE_PNG_H 1
#define HAVE_PNG_GET_VALID 1
#define HAVE_PNG_SET_TRNS_TO_ALPHA 1
#define SYSCONFDIR "/etc"
#define PREFIX "/usr/local"

/* FLTK 1.1 → 1.3 compat shims */
#ifdef __cplusplus
extern "C" {
#endif
int fl_ucs2utf(unsigned ucs, char *buf);
int fl_utf2ucs(const unsigned char *buf, int len, unsigned int *ucs);
#ifdef __cplusplus
}
#endif
