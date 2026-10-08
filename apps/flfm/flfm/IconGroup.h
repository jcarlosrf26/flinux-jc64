#ifndef IconGroup_h
#define IconGroup_h


class IconCanvas;

class IconGroup : public Fl_Group {
public:
	IconCanvas *canvas;
	int sx, sy, sw, sh;
	int lx, ly;
	int px, py , pw, ph;
	int draw_sel;

	IconGroup(int X, int Y, int W, int H, IconCanvas *C);
	~IconGroup();
	int handle(int e);
	void draw(void);
	void show_selector(int x, int y, int w, int h);
	void hide_selector(void);
	void overlay_rect(void);
};


class NormalGroup : public IconGroup {
public:
	NormalGroup(int X, int Y, int W, int H, IconCanvas *C);
	~NormalGroup();
	void relayout(int neww);
};

class DetailGroup : public IconGroup {
public:
	DetailGroup(int X, int Y, int W, int H, IconCanvas *C);
	~DetailGroup();
};

#endif
