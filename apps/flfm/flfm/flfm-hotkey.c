#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>

int main(void)
{
	Display *d;
	Window root;
	XEvent ev;
	KeyCode kc;
	unsigned int mask = ControlMask | Mod1Mask;

	d = XOpenDisplay(NULL);
	if (!d) return 1;

	root = DefaultRootWindow(d);
	kc = XKeysymToKeycode(d, XK_f);
	if (kc == 0) return 1;

	XGrabKey(d, kc, mask, root, True, GrabModeAsync, GrabModeAsync);
	XGrabKey(d, kc, mask | LockMask, root, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(d, kc, mask | Mod2Mask, root, True,
		GrabModeAsync, GrabModeAsync);
	XGrabKey(d, kc, mask | LockMask | Mod2Mask, root, True,
		GrabModeAsync, GrabModeAsync);

	while (1) {
		XNextEvent(d, &ev);
		if (ev.type == KeyPress && ev.xkey.keycode == kc &&
			(ev.xkey.state & mask) == mask)
		{
			system("flfm &");
		}
	}

	return 0;
}
