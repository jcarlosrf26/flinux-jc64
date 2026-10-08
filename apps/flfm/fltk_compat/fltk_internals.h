/* fltk-utf8 internal symbols not exported in FLTK 1.3 */
#pragma once
#include <FL/x.H>
#include <FL/Fl.H>

#ifdef __cplusplus
extern "C" {
#endif

extern Atom fl_dnd_action;
extern Atom fl_XdndActionCopy;
extern Atom fl_XdndStatus;
extern Atom fl_XdndAware;
extern Atom fl_XdndSelection;
extern Window fl_dnd_target_window;
extern int fl_font_;

#ifdef __cplusplus
}
extern Fl_Widget *fl_selection_requestor;
extern int (*fl_local_grab)(int);
void fl_init_xdnd_atoms();
#endif
