#include <FL/x.H>
#include <FL/Fl.H>
#include "fltk_internals.h"

Atom fl_dnd_action       = 0;
Atom fl_XdndActionCopy   = 0;
Atom fl_XdndStatus       = 0;
Atom fl_XdndAware        = 0;
Atom fl_XdndSelection    = 0;
Window fl_dnd_target_window = 0;
int fl_font_             = -1;
Fl_Widget *fl_selection_requestor = NULL;
int (*fl_local_grab)(int) = NULL;

void fl_init_xdnd_atoms() {
    if (!fl_display) { fl_open_display(); }
    if (fl_XdndAware) return;
    fl_XdndAware        = XInternAtom(fl_display, "XdndAware",      False);
    fl_XdndStatus       = XInternAtom(fl_display, "XdndStatus",     False);
    fl_XdndSelection    = XInternAtom(fl_display, "XdndSelection",  False);
    fl_XdndActionCopy   = XInternAtom(fl_display, "XdndActionCopy", False);
    fl_dnd_action       = fl_XdndActionCopy;
}

struct _XdndInit {
    _XdndInit() { }
} _xdnd_init_instance;
