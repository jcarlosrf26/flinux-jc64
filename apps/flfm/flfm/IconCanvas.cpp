#include "IconCanvas.h"
#include "BigIcon.h"
#include "callbacks.h"
#include <FL/fl_draw.H>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
extern void reset_selection();

int IconCanvas::onmover = 0;

IconCanvas::IconCanvas(int X, int Y, int W, int H) : Fl_Scroll(X, Y, W, H)
{
	newlist = NULL;
	list = NULL;
	newnbf = 0;
	nbf = 0;
	group = NULL;
	newinfo = NULL;
	info = NULL;
	oldgroup = NULL;
        drag_x = 0;
        drag_y = 0;
        dragging = 0;
	type(BOTH_ALWAYS);
	box(FL_FLAT_BOX);
}

void IconCanvas::resize(int X, int Y, int W, int H)
{
	int oldw = w();
	Fl_Scroll::resize(X, Y, W, H);
	if (group && W != oldw && !StatesValues.view_detail) {
		((NormalGroup*)group)->relayout(W);
	}
}

IconCanvas::~IconCanvas()
{
	Fl::remove_timeout(mover, this);
	onmover = 0;
	while (nbf > 0) {
		nbf--;
		free(list[nbf]);
		free(info[nbf]);
	}
	free(list);
	free(info);
	delete(oldgroup);
	scrollbar.parent(0);
	hscrollbar.parent(0);
	clear();
}

static int sel_all(const struct dirent *d)
{
	if (d->d_name[0] == '.' && (d->d_name[1] == '\0' ||
		(d->d_name[1] == '.' && d->d_name[2] == '\0')))
	{
		return 0;
	}
	return 1;
}

static int sel_no_hide(const struct dirent *d)
{
	if (d->d_name[0] == '.') return 0;
	return 1;
}

static int sizesort(const void *d1, const void *d2)
{
	struct file_info *f1 = *((struct file_info**) d1);
	struct file_info *f2 = *((struct file_info**) d2);
	
	if (S_ISDIR(f1->st.st_mode) && S_ISDIR(f2->st.st_mode)) {
		return 0;
	} else if (S_ISDIR(f1->st.st_mode)) {
		return -(0x7fffffff);
	} else if (S_ISDIR(f2->st.st_mode)) {
		return 0x7fffffff;
	}
	return (f2->st.st_size - f1->st.st_size);
}

static int typesort(const void *d1, const void *d2)
{
	struct file_info *f1 = *((struct file_info**) d1);
	struct file_info *f2 = *((struct file_info**) d2);
	if (S_ISDIR(f1->st.st_mode) && S_ISDIR(f2->st.st_mode)) {
		return 0;
	} else if (S_ISDIR(f1->st.st_mode)) {
		return -(0x7fffffff);
	} else if (S_ISDIR(f2->st.st_mode)) {
		return 0x7fffffff;
	}
	return ((f2->st.st_mode & S_IFMT) - (f1->st.st_mode & S_IFMT));
}

void IconCanvas::rescan()
{
	int i;
	if (!StatesValues.url) return;

	if (StatesValues.show_hide) {
		newnbf = scandir(StatesValues.url, &newlist,
				sel_all, alphasort);
	} else {
		newnbf = scandir(StatesValues.url, &newlist,
				sel_no_hide, alphasort);
	}

	newinfo = (struct file_info **) 
			malloc(sizeof(struct file_info *) * newnbf);
	i = 0;
	while (i < newnbf) {
		newinfo[i] = (struct file_info *) 
				malloc(sizeof(struct file_info));
		newinfo[i]->real_name = newlist[i]->d_name;
		if (stat(newinfo[i]->real_name, &(newinfo[i]->st))) {
			newinfo[i]->st.st_mode = 0;
			newinfo[i]->st.st_size = 0;
		}
		i++;
	}
	if (StatesValues.sort_type) {
		qsort(newinfo, newnbf, sizeof(struct file_info*), typesort);
	} else if (StatesValues.sort_size) {
		qsort(newinfo, newnbf, sizeof(struct file_info*), sizesort);
	}

	remove(group);
	delete(oldgroup);
	oldgroup = group;

	while (nbf > 0) {
		nbf--;
		free(list[nbf]);
		free(info[nbf]);
	}
	free(list);
	free(info);

	nbf = newnbf;
	list = newlist;
	info = newinfo;

	begin();
	if (StatesValues.view_detail) {
		group = new DetailGroup(x(), y(), w(), h(), this);	
	} else {
		group = new NormalGroup(x(), y(), w(), h(), this);	
	}
	end();
	resizable(NULL);

	update_status();
	Fl::flush();
	Fl::check();
}

void IconCanvas::mover(void* data)
{
        IconCanvas *w = (IconCanvas *) data;
        Fl::remove_timeout(mover);
        onmover = 0;
        w->handle(FL_DRAG);
}

void IconCanvas::draw_selector(int ev_x, int ev_y)
{
	int redr = 0;
        const int numchildren = group->children();
        BigIcon *b;
        int x=0, y=0, dx=0, dy=0;
	int drag_dx, drag_dy, mx, my;

        drag_dx = ev_x - drag_x;
        drag_dy = ev_y - drag_y;

        if (drag_x < ev_x) {
                x = drag_x;
                dx = drag_dx;
        } else {
                x = ev_x;
                dx = -drag_dx;
        }
        if (drag_y < ev_y) {
                y = drag_y;
                dy = drag_dy;
        } else {
                y = ev_y;
                dy = -drag_dy;
        }

	x -= xposition();
	y -= yposition();
        mx = x + dx;
        my = y + dy;

        for (int i=0; i < numchildren; ++i) {
                int bx, by, bw, bh;
                b = (BigIcon *)group->child(i);
                bx = b->x();
                by = b->y();
                bw = b->w();
                bh = b->h();

                if (bx + bw > x && bx < mx && by + bh > y && by < my) {
			if (!b->selected) {
				b->selected = 1;
				b->damage(FL_DAMAGE_ALL);
				redr = 1;
			}
                } else {
			if (b->selected) {
				b->selected = 0;
				b->damage(FL_DAMAGE_ALL);
				redr = 1;
			}
                }
      }

	group->damage(FL_DAMAGE_CHILD);
	group->show_selector(x, y, dx, dy);
	if (redr) {
		update_status();
		Fl::flush();
	}
}

int IconCanvas::handle(int event)
{
        int move = 0;
        int ypos = yposition();
        int xpos = xposition();
      	int ev_x;
       	int ev_y;
	int mx, my;

	if (event == FL_FOCUS || event == FL_UNFOCUS) return 1;
	if (event == FL_KEYDOWN) {
		int key = Fl::event_key();
		int ctrl = Fl::event_state(FL_CTRL);
		if (ctrl && key == 'a') {
			if (group) {
				int n = group->children();
				for (int i = 0; i < n; i++)
					((BigIcon*)group->child(i))->selected = 1;
				update_status();
				group->redraw();
			}
			return 1;
		}
		if (ctrl && key == 'c') { cb_clip_copy(NULL, NULL); return 1; }
		if (ctrl && key == 'x') { cb_clip_cut(NULL, NULL); return 1; }
		if (ctrl && key == 'v') { cb_clip_paste(NULL, NULL); return 1; }
		if (ctrl && key == 'h') {
			StatesValues.show_hide = !StatesValues.show_hide;
			cb_rescan(NULL, NULL);
			return 1;
		}
		if (key == FL_Escape)    { window()->do_callback(); return 1; }
		if (key == FL_BackSpace) { cb_up(NULL, NULL); return 1; }
		if (key == FL_Delete) {
			cb_delete(NULL, (void*)(Fl::event_state() & FL_SHIFT ? 1 : 0));
			return 1;
		}
		if (key == FL_F + 2)     { cb_rename(NULL, NULL); return 1; }
		if (key == FL_F + 4)     { cb_terminal(NULL, NULL); return 1; }
		if (key == FL_Enter) { cb_open(NULL, NULL); return 1; }
		if (key == FL_Down || key == FL_Up || key == FL_Left || key == FL_Right) {
			if (!group) return 1;
			int n = group->children();
			int cur = -1;
			for (int i = 0; i < n; i++) {
				if (((BigIcon*)group->child(i))->selected) { cur = i; break; }
			}
			int next = -1;
			if (cur == -1) {
				next = 0;
			} else if (key == FL_Left) {
				next = cur > 0 ? cur-1 : 0;
			} else if (key == FL_Right) {
				next = cur < n-1 ? cur+1 : cur;
			} else {
				/* UP/DOWN: navigate by row using group-relative y (scroll-stable) */
				BigIcon *cb = (BigIcon*)group->child(cur);
				int group_y = group->y();
				int cur_y  = cb->y() - group_y;
				int cur_cx = cb->x() + cb->width/2;
				int target_y = -1;
				if (key == FL_Down) {
					for (int i = 0; i < n; i++) {
						int iy = ((BigIcon*)group->child(i))->y() - group_y;
						if (iy > cur_y && (target_y < 0 || iy < target_y))
							target_y = iy;
					}
				} else {
					for (int i = 0; i < n; i++) {
						int iy = ((BigIcon*)group->child(i))->y() - group_y;
						if (iy < cur_y && (target_y < 0 || iy > target_y))
							target_y = iy;
					}
				}
				if (target_y >= 0) {
					int best_dx = -1;
					for (int i = 0; i < n; i++) {
						BigIcon *b = (BigIcon*)group->child(i);
						if (b->y() - group_y == target_y) {
							int dx = b->x() + b->width/2 - cur_cx;
							if (dx < 0) dx = -dx;
							if (best_dx < 0 || dx < best_dx) {
								best_dx = dx; next = i;
							}
						}
					}
				} else {
					next = cur; /* already at first/last row */
				}
			}
			for (int i = 0; i < n; i++)
				((BigIcon*)group->child(i))->selected = 0;
			BigIcon *nb = (next >= 0 && next < n) ? (BigIcon*)group->child(next) : NULL;
			if (nb) nb->selected = 1;
			update_status();
			group->hide_selector();
			if (nb) ensure_visible(nb);
			/* redraw AFTER scroll so FL_DAMAGE_ALL overrides FL_DAMAGE_SCROLL blit */
			group->redraw();
			redraw();
			return 1;
		}
		if (key == FL_Home) {
			if (!group) return 1;
			int n = group->children();
			if (n > 0) {
				for (int i = 0; i < n; i++)
					((BigIcon*)group->child(i))->selected = 0;
				BigIcon *first = (BigIcon*)group->child(0);
				first->selected = 1;
				update_status();
				group->hide_selector();
				group->redraw();
				ensure_visible(first);
			}
			return 1;
		}
		if (key == FL_End) {
			if (!group) return 1;
			int n = group->children();
			if (n > 0) {
				for (int i = 0; i < n; i++)
					((BigIcon*)group->child(i))->selected = 0;
				BigIcon *last = (BigIcon*)group->child(n-1);
				last->selected = 1;
				update_status();
				group->hide_selector();
				group->redraw();
				ensure_visible(last);
			}
			return 1;
		}
		if (ctrl && key == 'f') { cb_find(NULL, NULL); return 1; }
		return 0;
	}

        if (!dragging) {
                int ret = 0;
                onmover = 0;
                if (event == FL_PUSH && 
			Fl::event_state() & FL_BUTTON1  &&
			!(Fl::get_key(FL_Control_L) ||
                        Fl::get_key(FL_Control_R)))
                {
                        int i = group->children();
                        while (i) {
                                i--;
                                ((BigIcon*)group->child(i))->selected = 0;
                        }
			update_status();
			group->damage(FL_DAMAGE_CHILD);
                }
                if (event != FL_FOCUS) {
                        ret = Fl_Scroll::handle(event);
                }
                if (!ret && event == FL_PUSH && (Fl::event_state() & FL_BUTTON3)) {
			Fl::focus(this);
			if (!BigIcon::m_canvas) BigIcon::create_menus();
			static Fl_Menu_Button *cbtn = NULL;
			if (!cbtn) cbtn = new Fl_Menu_Button(0, 0, 0, 0);
			cbtn->menu(BigIcon::m_canvas);
			cbtn->popup();
			return 1;
		}
                if (!ret && event == FL_PUSH) {
			int i = group->children();
                        Fl::focus(this);
			if (!(Fl::get_key(FL_Control_L) ||
				Fl::get_key(FL_Control_R)))
			{
				while (i) {
					i--;
					((BigIcon*)group->child(i))->
						selected = 0;
				}
				group->redraw();
			}
                        dragging = 1;
                        drag_x = Fl::event_x() + xposition();
                        drag_y = Fl::event_y() + yposition();
                        return 1;
                }
                return ret;
        } else if (event == FL_RELEASE) {
                dragging = 0;
                Fl::remove_timeout(mover);
		group->hide_selector();
                group->redraw();
                return 1;
        } else if (event != FL_DRAG) {
		if (!(Fl::event_state() & FL_BUTTON1)) {
			dragging = 0;
		}
                return Fl_Scroll::handle(event);
        }

       	if (onmover) {
               	Fl::remove_timeout(mover, this);
		onmover = 0;
	}

	Fl::get_mouse(mx, my);
	mx -= window()->x();
	my -= window()->y();
      	ev_x = mx + xposition();
       	ev_y = my + yposition();

	if (!(drag_x - 3 >= ev_x || drag_x + 3 <= ev_x || drag_y - 3 >= ev_y ||
		drag_y + 3 <= ev_y))
	{
		return 1;
	}
	if (Fl::has_timeout(mover, this)) return 1;

        if (mx > scrollbar.x() - 16 &&
                xposition() < group->w() - 16 - w())
        {
                xpos += 18;
                move = 1;
        } else if (my > hscrollbar.y() - 16 &&
                yposition() < group->h() - 16 - h())
        {
                ypos += 18;
                move = 1;
        } else if (mx < x() + 16 && xposition() > 15) {
                xpos -= 18;
                move = 1;
        } else if (my < y() + 16 && yposition() > 15) {
                ypos -= 18;
                move = 1;
        } else {
		group->hide_selector();
                draw_selector(ev_x, ev_y);
        }
        if (move) {
                hscrollbar.value(xpos);
                scrollbar.value(ypos);
                scrollbar.do_callback();
                draw_selector(ev_x, ev_y);
		group->hide_selector();
		group->redraw();
 	}
        if (move && !onmover) {
                Fl::add_timeout(.1, mover, this);
        }
        return 1;
}

void IconCanvas::update_status(void)
{
	const int numchildren = group->children();
	static char buf[80];
	unsigned long dsize = 0;
	unsigned long ssize = 0;
	int nbs = 0;

	for (int i = 0; i < numchildren; ++i) {
		if (i >= nbf) break;
		if (((BigIcon*)group->child(i))->selected) {
			ssize += info[i]->st.st_size;
			nbs++;
		}
		dsize += info[i]->st.st_size;
	}

	if (nbs) {
		snprintf(buf, 80, " %d selected  /  %ld kb", nbs, ssize / 1024);
	} else {
		snprintf(buf, 80, " %d files  /  %ld kb", numchildren, dsize / 1024);
	}
	gui->stat_bar->value(buf);
}

void IconCanvas::draw()
{
	Fl_Scroll::draw();
}

void IconCanvas::ensure_visible(BigIcon *b)
{
	if (!b || !group) return;
	int iy = b->y(), ih = b->h();
	int ix = b->x(), iw = b->w();
	int ny = yposition();
	int nx = xposition();

	if (iy < y())
		ny = yposition() + iy - y();
	else if (iy + ih > y() + h() - 16)
		ny = yposition() + iy + ih - (y() + h() - 16);

	if (ix < x())
		nx = xposition() + ix - x();
	else if (ix + iw > x() + w() - 16)
		nx = xposition() + ix + iw - (x() + w() - 16);

	if (nx < 0) nx = 0;
	if (ny < 0) ny = 0;

	if (nx != xposition() || ny != yposition()) {
		hscrollbar.value(nx);
		scrollbar.value(ny);
		scrollbar.do_callback();
	}
}

