#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H
#define PREFIX "/usr/local"
#define SYSCONFDIR "/etc"
#define LOCALEDIR "/usr/local/share/locale"
/* fltk-utf8 → FLTK 1.3 event compat */
#ifndef FL_DROP
#  include <FL/Enumerations.H>
#  define FL_DROP FL_PASTE
#endif
#endif
