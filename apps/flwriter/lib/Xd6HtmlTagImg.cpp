/******************************************************************************
 *   "$Id:  $"
 *
 *                 Copyright (c) 2001  O'ksi'D
 *
 *                      All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *      Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *      Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *      Neither the name of O'ksi'D nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER 
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *   Author : Jean-Marc Lienher ( http://oksid.ch )
 *
 ******************************************************************************/


#include "Xd6Std.h"
#include <FL/fl_draw.h>
#include "Xd6HtmlTagImg.h"
#include "Xd6Png.h"
#include "Xd6Jpeg.h"
#include "Xd6Gif.h"
#include <stdio.h>
#include <stdlib.h>
#include "Xd6Base64.h"

Xd6HtmlTagImg::Xd6HtmlTagImg(int i, Xd6XmlTreeElement *e, Xd6HtmlFrame *u) : 
	Xd6HtmlDisplay(i, NULL , 0, e ? e->stl : NULL)
{
	const char *ptr;

	parent = u;
	text = (char *)malloc(2);
	len = 1;
	gif = new Xd6Png();
	source = NULL;
	attr_w = attr_h = attr_border = 0;
	if (e) {
		ptr = e->get_attr_value("width");
		if (ptr) attr_w = (int) atol(ptr);
		ptr = e->get_attr_value("height");
		if (ptr) attr_h = (int) atol(ptr);
		ptr = e->get_attr_value("border");
		if (ptr) attr_border = (int) atol(ptr);
		while (u->display != DISPLAY_FRAME &&
			u->display != DISPLAY_IFRAME &&
			u->display != DISPLAY_TOP_FRAME)
		{
			u = u->parent;
		}
		
		downloader->request(e->get_attr_value("src"),
			NULL, NULL, (Xd6HtmlFrame*)u, this);
	}
}

Xd6HtmlTagImg::~Xd6HtmlTagImg()
{
	destroy();
}

void Xd6HtmlTagImg::destroy()
{
	if (source) fl_unlink(source);
	free(source);
	delete(gif);
}

void Xd6HtmlTagImg::load(const char *url, const char *file)
{
	char header[6];
	FILE *fp;

//	printf("url %s file %s\n", url, file);

	free(source);
	source = strdup(file);

	fp = fl_fopen(file, "r");
	if (!fp) {
		return;
	}
	int r = fread(header, 1, 6, fp);
	fclose(fp);
	if (r != 6) return;

	delete(gif);

	if (memcmp(header, "GIF87a", 6) == 0 ||
		memcmp(header, "GIF89a", 6) == 0)
	{
		gif = new Xd6Gif();
	} else if (memcmp(header, "\211PNG", 4) == 0) {
		gif = new Xd6Png();
	} else {
		gif = new Xd6Jpeg();
	}
	gif->load(file);

	damage(FL_DAMAGE_ALL);
}

void Xd6HtmlTagImg::measure() 
{
	width = height = 0;
	if (attr_w && attr_h) {
		width = attr_w + 2 * attr_border;
		height = attr_h + 2 * attr_border;
		gif->set_size(attr_w, attr_h);
	} else if (gif->data) {
		width = gif->w + 2 * attr_border;
		height = gif->h + 2 * attr_border;
		attr_w = width;
		attr_h = height;
	} else  {
		width = 32;
		height = 32;
		attr_w = width;
		attr_h = height;
	}
	descent = fl_descent(); 
}

void Xd6HtmlTagImg::draw(int X, int Y)
{
	int i = attr_border;
	X += left;
	Y += top;
	
	fl_color(FL_BLUE);
	for (i--;i > -1; i--) {
		fl_rect(X + i, Y + i, width - 2 * i, height - 2 * i);
	}
	if (!gif->data) {
		fl_color(COLOR_RED);
		fl_rectf(X + attr_border,  Y + attr_border, 
			width - 2 * attr_border, height -  2 * attr_border);
	} else {
		gif->draw(X + attr_border,  Y + attr_border);
	}
}

/* PostScript image printing is unimplemented: the original implementation
 * depended on Fl_Fltk, a class from the old fltk-utf8 fork that doesn't
 * exist in the FLTK 1.3 this project builds against. Images are skipped
 * when printing a document. */
void Xd6HtmlTagImg::print(Xd6HtmlPrint *p, int X, int Y)
{
}

void Xd6HtmlTagImg::to_html(FILE *fp)
{
	int l;
	char buf[1024];
	char *out;
	FILE *fi, *fo;

	if (!gif) return;

	fi = fl_fopen(source, "r");
	if (!fi) return;
	out = Xd6ConfigFile::temp();
	fo = fl_fopen(out, "w");
	if (!fo) { fclose(fi); free(out); return; }
	Xd6Encode_base64(fi, fo);
	fclose(fi);
	fclose(fo);

	fi = fopen(out, "r");
	if (!fi) { free(out); return; }
	l = fread(buf, 1, 1024, fi);

	if (l < 6) { fclose(fi); free(out); return; }

	fprintf(fp, "<img src=\"data:%s;base64,\n", gif->mime());

	while (l > 0) {
		fwrite(buf, l, 1, fp);
		l = fread(buf, 1, 1024, fi);
	}
	fclose(fi);
	fprintf(fp, "\" border=\"%d\" width=\"%d\" height=\"%d\" \n/>",
		attr_border, attr_w, attr_h);
	fl_unlink(out);
	free(out);
}

/*
 * "$Id: $"
 */
