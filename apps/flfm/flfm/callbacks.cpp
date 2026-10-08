#include "callbacks.h"
#include "BigIcon.h"
#include "AppsWindow.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#include <FL/fl_ask.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include "xd640/Xd6XmlUtils.h"
#include <stdlib.h>

#define _(String) gettext((String))

static void apps_sync_view();

static char *shell_quote(const char *s)
{
	int len = (int)strlen(s);
	char *out = (char*)malloc((size_t)len * 4 + 3);
	char *p = out;
	*p++ = '\'';
	for (int i = 0; i < len; i++) {
		if (s[i] == '\'') {
			*p++ = '\''; *p++ = '\\'; *p++ = '\''; *p++ = '\'';
		} else {
			*p++ = s[i];
		}
	}
	*p++ = '\'';
	*p = '\0';
	return out;
}

char app_text[256]  = "editor";
char app_image[256] = "flpicsee";
char app_video[256] = "mplayer";
char app_audio[256] = "mplayer";
char app_html[256]  = "dillo";
char app_term[256]  = "xterm";
char app_other[256] = "editor";

static char *url_to_path(const char *url)
{
	const char *p = url;
	char *result;
	int len;
	if (strncmp(p, "file://", 7) == 0) p += 7;
	len = (int)strcspn(p, "\r\n");
	result = (char*)malloc(len + 1);
	memcpy(result, p, len);
	result[len] = '\0';
	url2latin1(result, len);
	return result;
}

static int path_has_ext(const char *path, const char *ext)
{
	int pl = (int)strlen(path);
	int el = (int)strlen(ext);
	if (pl < el) return 0;
	const char *p = path + pl - el;
	while (*p && *ext) {
		char c = (*p >= 'A' && *p <= 'Z') ? *p + 32 : *p;
		if (c != *ext) return 0;
		p++; ext++;
	}
	return 1;
}

static void get_exe_path(char *buf, int bufsz)
{
	ssize_t len = readlink("/proc/self/exe", buf, bufsz - 1);
	if (len > 0) { buf[len] = '\0'; }
	else { strncpy(buf, "flfm", bufsz - 1); buf[bufsz - 1] = '\0'; }
}

static void install_tcz(const char *path)
{
	const char *base = strrchr(path, '/');
	base = base ? base + 1 : path;

	char question[512];
	snprintf(question, sizeof(question), "%s\n\n%s", _("Install this extension?"), base);
	if (!fl_ask("%s", question)) return;

	char *q = shell_quote(path);
	char cmd[8192];
	snprintf(cmd, sizeof(cmd), "tce-load -i %s 2>&1", q);
	free(q);

	char output[2048];
	output[0] = '\0';
	FILE *p = popen(cmd, "r");
	int status = -1;
	if (p) {
		char line[512];
		while (fgets(line, sizeof(line), p)) {
			strncat(output, line, sizeof(output) - strlen(output) - 1);
		}
		status = pclose(p);
	}

	if (status == 0) {
		fl_message("%s\n\n%s", _("Installed successfully."), output);
	} else {
		fl_alert("%s\n\n%s", _("Installation failed."), output);
	}
}

static void open_path(const char *path)
{
	char buf[8192];
	struct stat st;
	char *q;
	if (stat(path, &st) != 0) return;

	if (path_has_ext(path, ".tcz")) {
		install_tcz(path);
		return;
	}

	q = shell_quote(path);
	if (S_ISDIR(st.st_mode)) {
		char exe[1024]; get_exe_path(exe, sizeof(exe));
		char *qexe = shell_quote(exe);
		snprintf(buf, sizeof(buf), "%s %s &", qexe, q);
		free(qexe);
	} else if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
		snprintf(buf, sizeof(buf), "%s &", q);
	} else if (path_has_ext(path, ".png")  || path_has_ext(path, ".jpg")  ||
	           path_has_ext(path, ".jpeg") || path_has_ext(path, ".gif")  ||
	           path_has_ext(path, ".bmp")  || path_has_ext(path, ".svg")  ||
	           path_has_ext(path, ".xpm")  || path_has_ext(path, ".xbm")  ||
	           path_has_ext(path, ".xcf")  || path_has_ext(path, ".eps")  ||
	           path_has_ext(path, ".ico")) {
		snprintf(buf, sizeof(buf), "%s %s &", app_image, q);
	} else if (path_has_ext(path, ".mp4")  || path_has_ext(path, ".mkv")  ||
	           path_has_ext(path, ".avi")  || path_has_ext(path, ".webm") ||
	           path_has_ext(path, ".mov")  || path_has_ext(path, ".wmv")  ||
	           path_has_ext(path, ".flv")  || path_has_ext(path, ".mpg")  ||
	           path_has_ext(path, ".mpeg") || path_has_ext(path, ".m4v")) {
		snprintf(buf, sizeof(buf), "%s %s &", app_video, q);
	} else if (path_has_ext(path, ".mp3")  || path_has_ext(path, ".wav")  ||
	           path_has_ext(path, ".ogg")  || path_has_ext(path, ".flac") ||
	           path_has_ext(path, ".m4a")  || path_has_ext(path, ".aac")) {
		snprintf(buf, sizeof(buf), "%s %s &", app_audio, q);
	} else if (path_has_ext(path, ".html") || path_has_ext(path, ".htm")) {
		snprintf(buf, sizeof(buf), "%s %s &", app_html, q);
	} else if (path_has_ext(path, ".txt")  || path_has_ext(path, ".md")   ||
	           path_has_ext(path, ".sh")   || path_has_ext(path, ".conf") ||
	           path_has_ext(path, ".cfg")  || path_has_ext(path, ".ini")  ||
	           path_has_ext(path, ".log")  || path_has_ext(path, ".xml")  ||
	           path_has_ext(path, ".json") || path_has_ext(path, ".csv")  ||
	           path_has_ext(path, ".c")    || path_has_ext(path, ".h")    ||
	           path_has_ext(path, ".cpp")  || path_has_ext(path, ".py")   ||
	           path_has_ext(path, ".js")   || path_has_ext(path, ".rtf")  ||
	           path_has_ext(path, ".doc")  || path_has_ext(path, ".docx") ||
	           path_has_ext(path, ".odt")  || path_has_ext(path, ".pdf")) {
		snprintf(buf, sizeof(buf), "%s %s &", app_text, q);
	} else {
		snprintf(buf, sizeof(buf), "%s %s &", app_other, q);
	}
	free(q);
	system(buf);
}

struct states_struct StatesValues;
static time_t mod_time;

static void save_state()
{
	char buf[80];

	snprintf(buf, 80, "%d", gui->w() + 20);
	cfg_sec->add_item("W", buf, NULL);
	snprintf(buf, 80, "%d", gui->h() + 20);
	cfg_sec->add_item("H", buf, NULL);

	cfg_sec->add_item("sort_size", 
		StatesValues.sort_size ? "1" : "0", NULL);
	cfg_sec->add_item("sort_type", 
		StatesValues.sort_type ? "1" : "0", NULL);
	cfg_sec->add_item("sort_name", 
		StatesValues.sort_name ? "1" : "0", NULL);
	cfg_sec->add_item("view_icon",
		StatesValues.view_icon ? "1" : "0", NULL);
	cfg_sec->add_item("view_detail",
		StatesValues.view_detail ? "1" : "0", NULL);
	cfg_sec->add_item("show_hide",
		StatesValues.show_hide ? "1" : "0", NULL);

	cfg_sec->add_item("app_text",  app_text,  NULL);
	cfg_sec->add_item("app_image", app_image, NULL);
	cfg_sec->add_item("app_video", app_video, NULL);
	cfg_sec->add_item("app_audio", app_audio, NULL);
	cfg_sec->add_item("app_html",  app_html,  NULL);
	cfg_sec->add_item("app_term",  app_term,  NULL);
	cfg_sec->add_item("app_other", app_other, NULL);

	cfg->write_config_section(cfg_sec);
}

char *get_selected_urls()
{
	int i;
	Fl_Widget*const* a;
	BigIcon *b;
	char *buf;
	int index;
	int alloc;

	if (!gui || !gui->icon_can ||
		!((IconCanvas*)gui->icon_can)->group) return NULL;

	a = ((IconCanvas*)gui->icon_can)->group->array();
	i = ((IconCanvas*)gui->icon_can)->group->children();

	alloc = 4096;
	buf = (char*) malloc(alloc);
	buf[0] = '\0';
	index = 0;

	char *b1 = (char*) malloc(strlen(StatesValues.url) * 3 + 1);
	latin12url(StatesValues.url, strlen(StatesValues.url), b1);

	while (i > 0) {
		i--;
		b = (BigIcon*) a[i];
		if (b && b->selected) {
			char *b2;
			if (index > alloc - 4096) {
				alloc = alloc * 2;
				buf = (char*) realloc(buf, alloc);
			}
			b2 = (char*) malloc(strlen(b->label()) * 3 + 1);
			latin12url(b->label(), strlen(b->label()), b2);
			snprintf(buf + index, 4096, "file://%s/%s\r\n", b1, b2);
			index += strlen(buf + index);
			free(b2);
		}
	}
	free(b1);
	return buf;
}


static const BigIcon *get_selected_icon()
{
	int i;
	Fl_Widget*const* a;
	BigIcon *b;

	if (!gui || !gui->icon_can ||
		!((IconCanvas*)gui->icon_can)->group) return NULL;

	a = ((IconCanvas*)gui->icon_can)->group->array();
	i = ((IconCanvas*)gui->icon_can)->group->children();

	while (i > 0) {
		i--;
		b = (BigIcon*) a[i];
		if (b && b->selected) {
			return b;
		}
	}
	return NULL;
}

static char *url2dir(char *url)
{
	char *ptr;
	char *dir;
	char buf[1024];
	char buf1[1024];

	ptr = strstr(url, "file:/");
	if (ptr) {
		dir = ptr + 5;
		while (*ptr && *ptr != '\n' && *ptr != '\r') {
			ptr++;	
		}
		*ptr = '\0';
		url2latin1(dir, strlen(dir));
		return strdup(dir);	
	}

	if (url[0] == '/') {
		ptr = url;
		dir = ptr;
		while (*ptr && *ptr != '\n' && *ptr != '\r') ptr++;
		*ptr = '\0';
		url2latin1(dir, strlen(dir));
		return strdup(dir);
	}
	
	ptr = strstr(url, "file:");
	if (ptr) {
		ptr += 5;
		url = ptr;
	} else {
		ptr = url;
	}
	while (*ptr && *ptr != '\n' && *ptr != '\r') ptr++;
	*ptr = '\0';
	url2latin1(url, strlen(url));
	buf[0] = '\0';
	getcwd(buf, 1024);
	
	snprintf(buf1, 1024, "%s/%s", buf, url);

	return (strdup(buf1));
}

void cb_rescan(Fl_Widget*, void*)
{
	char *dir;
	char buf[1024];
	struct stat st;

	Fl::remove_timeout(cb_directory, 0);

	if (!StatesValues.newurl) {
		if (StatesValues.url) {
			StatesValues.newurl = strdup(StatesValues.url);
		} else {
			StatesValues.newurl = strdup(cfg->home_dir);
		}
	}

	dir = url2dir(StatesValues.newurl);
	buf[0] = '\0';
	chdir(dir);
	getcwd(buf, 1024);

	if (StatesValues.url && strcmp(buf, StatesValues.url)) {
		gui->loc_inp->new_url(StatesValues.url);
	}

	free(StatesValues.url);
	StatesValues.url = strdup(buf);
	gui->loc_inp->value(StatesValues.url);
	stat(StatesValues.url, &st);

	((IconCanvas*)gui->icon_can)->rescan();

	free(StatesValues.newurl);
	StatesValues.newurl = NULL;
	free(dir);
	Fl::redraw();
	if (gui && gui->icon_can)
		Fl::focus(gui->icon_can);

	mod_time = st.st_mtime;
	Fl::add_timeout(1, cb_directory, 0);
}

void cb_newdir(Fl_Widget*, void*)
{
	const char *dir;
	if ((dir = fl_input(_("New Directory :")))) {
		mkdir(dir, 0777);
	}
	cb_rescan(NULL, NULL);
}

void cb_newfile(Fl_Widget*, void*)
{
	const char *name;
	char buf[2048];
	if ((name = fl_input(_("New File (name.ext):")))) {
		snprintf(buf, sizeof(buf), "%s/%s", StatesValues.url, name);
		FILE *f = fopen(buf, "a");
		if (f) fclose(f);
	}
	cb_rescan(NULL, NULL);
}

static char *clip_urls = NULL;
static int  clip_is_move = 0;

void cb_clip_copy(Fl_Widget*, void*)
{
	free(clip_urls);
	clip_urls = get_selected_urls();
	clip_is_move = 0;
}

void cb_clip_cut(Fl_Widget*, void*)
{
	free(clip_urls);
	clip_urls = get_selected_urls();
	clip_is_move = 1;
}

void cb_clip_paste(Fl_Widget*, void*)
{
	if (!clip_urls || !clip_urls[0]) return;
	char *urls = strdup(clip_urls);
	char *p = urls;
	char *qdest = shell_quote(StatesValues.url);
	while (p && *p) {
		char *end = strstr(p, "\r\n");
		if (end) *end = '\0';
		char *src = url_to_path(p);
		if (src && src[0]) {
			char cmd[8192];
			char *qsrc = shell_quote(src);
			if (clip_is_move)
				snprintf(cmd, sizeof(cmd), "mv %s %s/", qsrc, qdest);
			else
				snprintf(cmd, sizeof(cmd), "cp -r %s %s/", qsrc, qdest);
			system(cmd);
			free(qsrc);
			free(src);
		}
		if (end) p = end + 2;
		else break;
	}
	free(qdest);
	free(urls);
	if (clip_is_move) { free(clip_urls); clip_urls = NULL; }
	cb_rescan(NULL, NULL);
}

void cb_rename(Fl_Widget*, void*)
{
	char buf[8192];
	char *q1, *q2;
	const BigIcon *b = get_selected_icon();
	if (!b) return;
	const char *newname = fl_input(_("Rename to:"), b->label());
	if (!newname || !newname[0]) return;
	char src[2048], dst[2048];
	snprintf(src, sizeof(src), "%s/%s", StatesValues.url, b->label());
	snprintf(dst, sizeof(dst), "%s/%s", StatesValues.url, newname);
	q1 = shell_quote(src);
	q2 = shell_quote(dst);
	snprintf(buf, sizeof(buf), "mv %s %s", q1, q2);
	free(q1);
	free(q2);
	system(buf);
	cb_rescan(NULL, NULL);
}

void cb_exit(Fl_Widget*, void*)
{
	exit(0);
}

void cb_sort(Fl_Widget *w, void *d)
{
	int itm = *((int*) &d);

	switch (itm) {
	case 1:
		StatesValues.sort_name = 1;
		StatesValues.sort_type = 0;
		StatesValues.sort_size = 0;
		break;
	case 2:
		StatesValues.sort_type = 1;
		StatesValues.sort_size = 0;
		StatesValues.sort_name = 0;
		break;
	case 3:
		StatesValues.sort_size = 1;
		StatesValues.sort_name = 0;
		StatesValues.sort_type = 0;
		break;
	case 4:
		StatesValues.view_icon = 1;
		StatesValues.view_detail = 0;
		break;
	case 6:
		StatesValues.view_detail = 1;
		StatesValues.view_icon = 0;
		break;
	case 7:
		StatesValues.show_hide = (char) !(StatesValues.show_hide);
		break;
	default:
		break;
	}
	save_state();
	cb_rescan(w, d);
	apps_sync_view();
	trash_sync_view();
}

void cb_about(Fl_Widget*, void*)
{
	fl_message(
		"FLFM v1.0.6\n"
		"Copyright (c) 2000-2002 O'ksi'D\n"
		"Updated for FLinux — 2026\n"
		"Open Source  (FLTK 1.3)");
}

void cb_terminal(Fl_Widget*, void*)
{
	char cmd[8192];
	char *q = shell_quote(StatesValues.url);
	snprintf(cmd, sizeof(cmd), "cd %s && %s &", q, app_term);
	free(q);
	system(cmd);
}

void cb_help(Fl_Widget*, void*)
{
	system("dillo 'https://loc-os.com/flfm-help.html' &");
}

void cb_exec(Fl_Widget*, void  *d)
{
	char buf[8192];
	char *q;

	const BigIcon *b = get_selected_icon();
	if (!b) return;
	char target[2048];
	snprintf(target, sizeof(target), "%s/%s", StatesValues.url, b->label());
	q = shell_quote(target);
	snprintf(buf, sizeof(buf), "%s &", q);
	free(q);
	system(buf);
}

void cb_open(Fl_Widget*, void  *d)
{
	char buf[2048];
	struct stat st;

	const BigIcon *b = get_selected_icon();
	if (!b) return;
	snprintf(buf, 2048, "%s/%s", StatesValues.url, b->label());
	if (stat(buf, &st) == 0 && S_ISDIR(st.st_mode)) {
		cb_change_dir(NULL, NULL);
		return;
	}
	open_path(buf);
}

void cb_open_width(Fl_Widget*, void*)
{
	const BigIcon *b = NULL;
	char buf[8192];
	static char *last_cmd = NULL;
	char *r;
	char *q;

	if (!last_cmd) {
		last_cmd = strdup("");
	}
	b = get_selected_icon();
	if (!b) return;
	snprintf(buf, sizeof(buf), "%s/%s", StatesValues.url, b->label());
	r = (char *)fl_input(_("Open \"%s\" with :"), last_cmd, buf);

	if (!r) return;
	free(last_cmd);
	last_cmd = strdup(r);

	q = shell_quote(buf);
	snprintf(buf, sizeof(buf), "%s %s &", r, q);
	free(q);
	system(buf);
}

void cb_new_fm(Fl_Widget*, void*)
{
	char buf[8192];
	char exe[1024];
	char *qexe, *qurl;
	get_exe_path(exe, sizeof(exe));
	qexe = shell_quote(exe);
	qurl = shell_quote(StatesValues.url);
	snprintf(buf, sizeof(buf), "%s %s &", qexe, qurl);
	free(qexe);
	free(qurl);
	system(buf);
}

static void pref_ok_cb(Fl_Widget *w, void *v)
{
	*((int*)v) = 1;
	w->window()->hide();
}

static void pref_cancel_cb(Fl_Widget *w, void*)
{
	w->window()->hide();
}

static void boot_autostart_path(char *path, size_t len)
{
	const char *home = getenv("HOME");
	snprintf(path, len, "%s/.X.d/flfm-autostart", home ? home : "");
}

static int boot_autostart_enabled()
{
	char path[1024];
	boot_autostart_path(path, sizeof(path));
	return access(path, F_OK) == 0;
}

static void boot_autostart_set(int enable)
{
	char path[1024];
	char dir[1024];
	const char *home = getenv("HOME");
	FILE *f;
	int currently = boot_autostart_enabled();

	if (!home || enable == currently) return;
	boot_autostart_path(path, sizeof(path));

	if (!enable) {
		unlink(path);
	} else {
		snprintf(dir, sizeof(dir), "%s/.X.d", home);
		mkdir(dir, 0755);
		f = fopen(path, "w");
		if (!f) return;
		fprintf(f, "#!/bin/sh\nflfm &\n");
		fclose(f);
		chmod(path, 0755);
	}
}

static void hotkey_autostart_path(char *path, size_t len)
{
	const char *home = getenv("HOME");
	snprintf(path, len, "%s/.X.d/flfm-hotkey-autostart", home ? home : "");
}

static int hotkey_autostart_enabled()
{
	char path[1024];
	hotkey_autostart_path(path, sizeof(path));
	return access(path, F_OK) == 0;
}

static void hotkey_autostart_set(int enable)
{
	char path[1024];
	char dir[1024];
	const char *home = getenv("HOME");
	FILE *f;
	int currently = hotkey_autostart_enabled();

	if (!home || enable == currently) return;
	hotkey_autostart_path(path, sizeof(path));

	if (!enable) {
		unlink(path);
		system("pkill -f flfm-hotkey >/dev/null 2>&1");
	} else {
		snprintf(dir, sizeof(dir), "%s/.X.d", home);
		mkdir(dir, 0755);
		f = fopen(path, "w");
		if (!f) return;
		fprintf(f, "#!/bin/sh\nflfm-hotkey &\n");
		fclose(f);
		chmod(path, 0755);
		system("flfm-hotkey >/dev/null 2>&1 &");
	}
}

static void volwatch_autostart_path(char *path, size_t len)
{
	const char *home = getenv("HOME");
	snprintf(path, len, "%s/.X.d/flfm-volwatch-autostart", home ? home : "");
}

static int volwatch_autostart_enabled()
{
	char path[1024];
	volwatch_autostart_path(path, sizeof(path));
	return access(path, F_OK) == 0;
}

static void volwatch_autostart_set(int enable)
{
	char path[1024];
	char dir[1024];
	const char *home = getenv("HOME");
	FILE *f;
	int currently = volwatch_autostart_enabled();

	if (!home || enable == currently) return;
	volwatch_autostart_path(path, sizeof(path));

	if (!enable) {
		unlink(path);
		system("pkill -f flfm-volwatch >/dev/null 2>&1");
	} else {
		snprintf(dir, sizeof(dir), "%s/.X.d", home);
		mkdir(dir, 0755);
		f = fopen(path, "w");
		if (!f) return;
		fprintf(f, "#!/bin/sh\nflfm-volwatch &\n");
		fclose(f);
		chmod(path, 0755);
		system("flfm-volwatch >/dev/null 2>&1 &");
	}
}

static void apps_hotkey_autostart_path(char *path, size_t len)
{
	const char *home = getenv("HOME");
	snprintf(path, len, "%s/.X.d/flfm-apps-hotkey-autostart", home ? home : "");
}

static int apps_hotkey_autostart_enabled()
{
	char path[1024];
	apps_hotkey_autostart_path(path, sizeof(path));
	return access(path, F_OK) == 0;
}

static void apps_hotkey_autostart_set(int enable)
{
	char path[1024];
	char dir[1024];
	const char *home = getenv("HOME");
	FILE *f;
	int currently = apps_hotkey_autostart_enabled();

	if (!home || enable == currently) return;
	apps_hotkey_autostart_path(path, sizeof(path));

	if (!enable) {
		unlink(path);
		system("pkill -f flfm-apps-hotkey >/dev/null 2>&1");
	} else {
		snprintf(dir, sizeof(dir), "%s/.X.d", home);
		mkdir(dir, 0755);
		f = fopen(path, "w");
		if (!f) return;
		fprintf(f, "#!/bin/sh\nflfm-apps-hotkey &\n");
		fclose(f);
		chmod(path, 0755);
		system("flfm-apps-hotkey >/dev/null 2>&1 &");
	}
}

void apply_default_autostart_once()
{
	char path[1024];
	char dir[1024];
	const char *home = getenv("HOME");
	FILE *f;

	if (!home) return;
	snprintf(path, sizeof(path), "%s/.xd640/etc/flfm_defaults_done", home);
	if (access(path, F_OK) == 0) return;

	if (!hotkey_autostart_enabled()) hotkey_autostart_set(1);
	if (!volwatch_autostart_enabled()) volwatch_autostart_set(1);
	if (!apps_hotkey_autostart_enabled()) apps_hotkey_autostart_set(1);

	snprintf(dir, sizeof(dir), "%s/.xd640", home);
	mkdir(dir, 0755);
	snprintf(dir, sizeof(dir), "%s/.xd640/etc", home);
	mkdir(dir, 0755);
	f = fopen(path, "w");
	if (f) fclose(f);

	system("sudo -n filetool.sh -b >/dev/null 2>&1");
}

void cb_pref(Fl_Widget*, void*)
{
	int result = 0;
	Fl_Window *win = new Fl_Window(420, 424, _("Default Applications"));
	win->begin();

	Fl_Input *inp[7];
	const char *labels[] = {
		_("Images:"), _("Video:"), _("Audio:"), _("Text editor:"),
		_("Terminal:"), _("HTML viewer:"), _("Other files:")
	};
	const char *vals[] = { app_image, app_video, app_audio, app_text,
		app_term, app_html, app_other };
	for (int i = 0; i < 7; i++) {
		inp[i] = new Fl_Input(130, 10 + i * 34, 280, 26, labels[i]);
		inp[i]->value(vals[i]);
	}

	Fl_Check_Button *chk_boot = new Fl_Check_Button(20, 256, 380, 26,
		_("Launch FLFM at system boot"));
	chk_boot->value(boot_autostart_enabled());

	Fl_Check_Button *chk_hotkey = new Fl_Check_Button(20, 286, 380, 26,
		_("Enable keyboard shortcut (Ctrl+Alt+F) to open FLFM"));
	chk_hotkey->value(hotkey_autostart_enabled());

	Fl_Check_Button *chk_volwatch = new Fl_Check_Button(20, 316, 380, 26,
		_("Detect new mounted volumes automatically"));
	chk_volwatch->value(volwatch_autostart_enabled());

	Fl_Check_Button *chk_apps_hotkey = new Fl_Check_Button(20, 346, 380, 26,
		_("Enable keyboard shortcut (Ctrl+Alt+Space) to open Apps launcher"));
	chk_apps_hotkey->value(apps_hotkey_autostart_enabled());

	Fl_Button *ok  = new Fl_Button(230, 388, 80, 26, _("OK"));
	Fl_Button *can = new Fl_Button(320, 388, 90, 26, _("Cancel"));
	ok->callback(pref_ok_cb, &result);
	can->callback(pref_cancel_cb, NULL);

	win->end();
	win->set_modal();
	win->show();
	while (win->shown()) Fl::wait();

	if (result) {
		strncpy(app_image, inp[0]->value(), 255); app_image[255] = '\0';
		strncpy(app_video, inp[1]->value(), 255); app_video[255] = '\0';
		strncpy(app_audio, inp[2]->value(), 255); app_audio[255] = '\0';
		strncpy(app_text,  inp[3]->value(), 255); app_text[255]  = '\0';
		strncpy(app_term,  inp[4]->value(), 255); app_term[255]  = '\0';
		strncpy(app_html,  inp[5]->value(), 255); app_html[255]  = '\0';
		strncpy(app_other, inp[6]->value(), 255); app_other[255] = '\0';
		boot_autostart_set(chk_boot->value());
		hotkey_autostart_set(chk_hotkey->value());
		volwatch_autostart_set(chk_volwatch->value());
		apps_hotkey_autostart_set(chk_apps_hotkey->value());
		system("sudo -n filetool.sh -b >/dev/null 2>&1");
		save_state();
	}
	delete win;
}

void cb_open_dir(Fl_Widget*, void*)
{
	const BigIcon *b;
	char buf[8192];
	char target[2048];
	char *q;

	b = get_selected_icon();

	if (b) {
		snprintf(target, sizeof(target), "%s/%s", StatesValues.url, b->label());
	} else {
		snprintf(target, sizeof(target), "%s", StatesValues.url);
	}
	q = shell_quote(target);
	snprintf(buf, sizeof(buf), "flfm %s &", q);
	free(q);
	system(buf);
}

void cb_change_dir(Fl_Widget*, void*)
{
	const BigIcon *b;
	char buf[2048];
	char *b1, *b2;
	b = get_selected_icon();
	if (!b) return;  
	b1 = (char*) malloc(strlen(StatesValues.url) * 3 + 1);
	b2 = (char*) malloc(strlen(b->label()) * 3 + 1);
	latin12url(StatesValues.url, strlen(StatesValues.url), b1);
	latin12url(b->label(), strlen(b->label()), b2);
	snprintf(buf, 2048, "file://%s/%s\r\n", b1, b2);
	StatesValues.newurl = strdup(buf);
	cb_rescan(NULL, NULL);
	free(b1);
	free(b2);
};
	
void cb_up(Fl_Widget*, void*)
{
	char buf[2048];
	int l;
	char *b1 = (char*) malloc(strlen(StatesValues.url) * 3 + 1);
	latin12url(StatesValues.url, strlen(StatesValues.url), b1);
	snprintf(buf, 2000, "file://%s", b1);
	l = strlen(buf);
	while (l > 0) {
		l--;
		if (buf[l] == '/') {
			buf[l] = '\0';
			break;
		}
	}
	snprintf(buf + l, 5, "/\r\n");
	StatesValues.newurl = strdup(buf);
	cb_rescan(NULL, NULL);
	free(b1);
}

void cb_home(Fl_Widget*, void*)
{
	StatesValues.newurl = strdup(cfg->home_dir);
	cb_rescan(NULL, NULL);
}

void cb_back(Fl_Widget*, void*)
{
	int i;

	i = 10;
	while (i > 0) {
		i--;
		if (StatesValues.history[i]) break;
	}
	if (!StatesValues.history[i]) return;

	StatesValues.newurl = StatesValues.history[i];
	StatesValues.history[i] = NULL;
	free(StatesValues.url);
	StatesValues.url = NULL;
	cb_rescan(NULL, NULL);
}

static AppsWindow *apps_win = NULL;

void cb_apps(Fl_Widget*, void*)
{
	if (!apps_win) {
		apps_win = new AppsWindow();
	}
	apps_win->search->value("");
	apps_win->load_apps();
	apps_win->show();
	apps_win->search->take_focus();
}

static void apps_sync_view()
{
	if (apps_win && apps_win->shown()) {
		apps_win->load_apps();
	}
}

#define MAX_VOLUMES 16

struct volume_entry {
	char device[128];
	char mountpoint[512];
};

static int list_volumes(struct volume_entry *out, int max)
{
	FILE *f = fopen("/proc/mounts", "r");
	if (!f) return 0;

	char line[1024];
	int n = 0;
	while (n < max && fgets(line, sizeof(line), f)) {
		char dev[128], mnt[512], fs[32];
		if (sscanf(line, "%127s %511s %31s", dev, mnt, fs) != 3) continue;
		if (strncmp(dev, "/dev/", 5) != 0) continue;
		if (!strcmp(fs, "squashfs")) continue;
		if (!strncmp(mnt, "/tmp/tcloop/", 12)) continue;
		strncpy(out[n].device, dev, sizeof(out[n].device) - 1);
		out[n].device[sizeof(out[n].device) - 1] = '\0';
		strncpy(out[n].mountpoint, mnt, sizeof(out[n].mountpoint) - 1);
		out[n].mountpoint[sizeof(out[n].mountpoint) - 1] = '\0';
		n++;
	}
	fclose(f);
	return n;
}

static void cb_open_volume(Fl_Widget*, void *v)
{
	char *path = (char*) v;
	char *enc = (char*) malloc(strlen(path) * 3 + 1);
	char buf[2048];

	latin12url(path, strlen(path), enc);
	snprintf(buf, sizeof(buf), "file://%s\r\n", enc);
	free(StatesValues.newurl);
	StatesValues.newurl = strdup(buf);
	cb_rescan(NULL, NULL);
	free(enc);
}

static char g_vol_paths[MAX_VOLUMES][512];
static char g_vol_labels[MAX_VOLUMES][600];
static Fl_Menu_Item g_vol_items[MAX_VOLUMES + 1];

static void build_volumes_submenu()
{
	struct volume_entry vols[MAX_VOLUMES];
	int n = list_volumes(vols, MAX_VOLUMES);

	if (n == 0) {
		g_vol_items[0].text = _("(no volumes mounted)");
		g_vol_items[0].shortcut_ = 0;
		g_vol_items[0].callback_ = 0;
		g_vol_items[0].user_data_ = 0;
		g_vol_items[0].flags = FL_MENU_INACTIVE;
		g_vol_items[1].text = NULL;
		return;
	}
	for (int i = 0; i < n; i++) {
		strncpy(g_vol_paths[i], vols[i].mountpoint, sizeof(g_vol_paths[i]) - 1);
		g_vol_paths[i][sizeof(g_vol_paths[i]) - 1] = '\0';
		snprintf(g_vol_labels[i], sizeof(g_vol_labels[i]), "%s (%s)",
			vols[i].mountpoint, vols[i].device);
		g_vol_items[i].text = g_vol_labels[i];
		g_vol_items[i].shortcut_ = 0;
		g_vol_items[i].callback_ = (Fl_Callback*)cb_open_volume;
		g_vol_items[i].user_data_ = (void*)g_vol_paths[i];
		g_vol_items[i].flags = 0;
	}
	g_vol_items[n].text = NULL;
}

void refresh_volumes_menu()
{
	if (!gui || !gui->mnu_bar || !gui->mnu) return;
	int idx = gui->mnu_bar->find_index("File/Volumes");
	if (idx < 0) return;

	build_volumes_submenu();
	gui->mnu[idx].user_data((void*)g_vol_items);
}

static void volumes_menu_tick(void*)
{
	refresh_volumes_menu();
	Fl::repeat_timeout(5.0, volumes_menu_tick, 0);
}

void start_volume_watch()
{
	Fl::add_timeout(5.0, volumes_menu_tick, 0);
}

static int is_under_home(const char *path)
{
	const char *home = getenv("HOME");
	size_t hl;
	if (!home || !home[0]) return 0;
	hl = strlen(home);
	if (strncmp(path, home, hl) != 0) return 0;
	return path[hl] == '\0' || path[hl] == '/';
}

void trash_dirs(char *files_dir, char *info_dir, size_t len)
{
	char base[900];
	const char *home = getenv("HOME");
	if (!home) home = "/root";
	snprintf(base, sizeof(base), "%s/.xd640", home);
	mkdir(base, 0755);
	snprintf(base, sizeof(base), "%s/.xd640/trash", home);
	mkdir(base, 0755);
	snprintf(files_dir, len, "%s/files", base);
	mkdir(files_dir, 0755);
	snprintf(info_dir, len, "%s/info", base);
	mkdir(info_dir, 0755);
}

static void unique_trash_name(const char *files_dir, const char *base, char *out, size_t outlen)
{
	struct stat st;
	char full[1200];
	int n = 1;

	snprintf(out, outlen, "%s", base);
	snprintf(full, sizeof(full), "%s/%s", files_dir, out);
	while (stat(full, &st) == 0) {
		n++;
		snprintf(out, outlen, "%s.%d", base, n);
		snprintf(full, sizeof(full), "%s/%s", files_dir, out);
	}
}

static int trash_item(const char *path)
{
	char files_dir[900], info_dir[900];
	char trash_name[900], dest[1200], info_path[1200];
	const char *base;
	FILE *f;

	trash_dirs(files_dir, info_dir, sizeof(files_dir));
	base = strrchr(path, '/');
	base = base ? base + 1 : path;
	unique_trash_name(files_dir, base, trash_name, sizeof(trash_name));

	snprintf(dest, sizeof(dest), "%s/%s", files_dir, trash_name);
	if (rename(path, dest) != 0) return 0;

	snprintf(info_path, sizeof(info_path), "%s/%s.trashinfo", info_dir, trash_name);
	f = fopen(info_path, "w");
	if (f) {
		fprintf(f, "%s\n%ld\n", path, (long) time(NULL));
		fclose(f);
	}
	return 1;
}

static void perform_delete(int permanent)
{
	char buf[8192];
	char *urls;
	char *p;
	pid_t pid;

	urls = get_selected_urls();
	if (!urls) return;

	pid = fork();
	if (pid < 0) { free(urls); return; }
	if (pid > 0) {
		waitpid(pid, NULL, 0);
		free(urls);
		return;
	}

	if (fork() > 0) _exit(0);

	p = urls;
	while (p && *p) {
		char *end = strstr(p, "\r\n");
		if (end) *end = '\0';
		char *path = url_to_path(p);
		if (path && path[0]) {
			if (!permanent && is_under_home(path)) {
				trash_item(path);
			} else {
				char *q = shell_quote(path);
				snprintf(buf, sizeof(buf), "rm -rf %s", q);
				system(buf);
				free(q);
			}
			free(path);
		}
		if (end) p = end + 2;
		else break;
	}
	free(urls);
	_exit(0);
}

void cb_delete(Fl_Widget*, void *data)
{
	int permanent = *((int*) &data);

	if (permanent) {
		if (!fl_ask("%s", _("Permanently delete selected items? This cannot be undone."))) return;
	} else {
		if (!fl_ask("%s", _("Move selected items to Trash?"))) return;
	}
	perform_delete(permanent);
}

void cb_delete_menu(Fl_Widget*, void*)
{
	char *urls;
	char *p;
	int home_only = 1;
	int result;

	urls = get_selected_urls();
	if (!urls) return;

	p = urls;
	while (p && *p) {
		char *end = strstr(p, "\r\n");
		if (end) *end = '\0';
		char *path = url_to_path(p);
		if (path && !is_under_home(path)) home_only = 0;
		free(path);
		if (end) p = end + 2;
		else break;
	}
	free(urls);

	char msg[256];
	snprintf(msg, sizeof(msg), "%-56s", _("Delete selected items:"));

	if (home_only) {
		result = fl_choice("%s", _("Cancel"), _("Move to Trash"),
			_("Delete Permanently"), msg);
	} else {
		result = fl_choice("%s", _("Cancel"), NULL,
			_("Delete Permanently"), msg);
	}

	if (result == 1) perform_delete(0);
	else if (result == 2) perform_delete(1);
}

void cb_properties(Fl_Widget*, void*)
{
	char info[2048];
	char line[2048];
	char *urls;
	char *path;
	char *end;
	struct stat st;
	int len;

	urls = get_selected_urls();
	if (!urls) return;

	end = strstr(urls, "\r\n");
	len = end ? (int)(end - urls) : (int)strlen(urls);
	if (len >= (int)sizeof(line)) len = (int)sizeof(line) - 1;
	memcpy(line, urls, len);
	line[len] = '\0';
	free(urls);

	path = url_to_path(line);
	if (!path) return;

	if (stat(path, &st) == 0) {
		const char *type = S_ISDIR(st.st_mode) ? _("Directory") :
		                   S_ISLNK(st.st_mode) ? _("Symlink") : _("File");
		snprintf(info, sizeof(info),
			"%s: %s\n%s: %s\n%s: %ld\n%s: %04o",
			_("Path"), path,
			_("Type"), type,
			_("Size"), (long)st.st_size,
			_("Mode"), (unsigned)(st.st_mode & 0777));
		fl_message("%s", info);
	} else {
		fl_alert(_("Cannot access: %s"), path);
	}
	free(path);
}

void cb_move_link_copy(Fl_Widget*, void *d)
{
	char dest[2048];
	int a = *((int*) &d);
	Fl_Widget *w = Fl::belowmouse();
	const char *dir = "";
	pid_t pid;
	char *src_text;
	int src_len;

	if (Fl::e_length < 2) return;
	if (gui->loc_inp == w || gui->loc_inp->contains(w)) {
		StatesValues.newurl = strdup(Fl::e_text);
		cb_rescan(NULL, NULL);
		return;
	}

	if (w == ((IconCanvas*)gui->icon_can)->group) {
		dir = "";
	} else if (((IconCanvas*)gui->icon_can)->group->contains(w)) {
		BigIcon *b = (BigIcon*) w;
		dir = b->label();
	} else {
		return;
	}
	if (dir[0]) {
		snprintf(dest, sizeof(dest), "%s/%s", StatesValues.url, dir);
	} else {
		strncpy(dest, StatesValues.url, sizeof(dest) - 1);
		dest[sizeof(dest) - 1] = '\0';
	}

	src_len = Fl::e_length;
	src_text = (char*)malloc(src_len + 1);
	memcpy(src_text, Fl::e_text, src_len);
	src_text[src_len] = '\0';

	pid = fork();
	if (pid < 0) { free(src_text); return; }
	if (pid > 0) {
		waitpid(pid, NULL, 0);
		free(src_text);
		return;
	}

	if (fork() > 0) _exit(0);

	char *p = src_text;
	char *qdest = shell_quote(dest);
	while (p && *p) {
		char *end = strstr(p, "\r\n");
		if (end) *end = '\0';
		char *src = url_to_path(p);
		if (src && src[0]) {
			char cmd[8192];
			char *qsrc = shell_quote(src);
			switch(a) {
			default:
			case DND_COPY:
				snprintf(cmd, sizeof(cmd), "cp -r %s %s", qsrc, qdest);
				break;
			case DND_MOVE:
				snprintf(cmd, sizeof(cmd), "mv %s %s", qsrc, qdest);
				break;
			case DND_LINK:
				snprintf(cmd, sizeof(cmd), "ln -s %s %s", qsrc, qdest);
				break;
			}
			system(cmd);
			free(qsrc);
			free(src);
		}
		if (end) p = end + 2;
		else break;
	}
	free(qdest);
	free(src_text);
	_exit(0);
}

void cb_loc_input(Fl_Widget*, void *d)
{
	Location *l = (Location*) d;
	if (!d) return;
	if (!l->url || (l->loc->value() && strcmp(l->url, l->loc->value()))) {
		StatesValues.newurl = strdup(l->loc->value());
		cb_rescan(NULL, NULL);
	}
}

int strhas_ci(const char *hay, const char *needle) {
	int hlen = (int)strlen(hay);
	int nlen = (int)strlen(needle);
	if (!nlen) return 1;
	for (int i = 0; i <= hlen - nlen; i++) {
		int j;
		for (j = 0; j < nlen; j++) {
			char hc = hay[i+j], nc = needle[j];
			if (hc >= 'A' && hc <= 'Z') hc += 32;
			if (nc >= 'A' && nc <= 'Z') nc += 32;
			if (hc != nc) break;
		}
		if (j == nlen) return 1;
	}
	return 0;
}

void cb_find(Fl_Widget*, void*)
{
	const char *term = fl_input(_("Find in current directory:"), "");
	if (!term || !*term) return;
	if (!gui || !gui->icon_can ||
		!((IconCanvas*)gui->icon_can)->group) return;
	IconCanvas *ic = (IconCanvas*)gui->icon_can;
	int n = ic->group->children();
	int found = 0;
	BigIcon *first_match = NULL;
	for (int i = 0; i < n; i++)
		((BigIcon*)ic->group->child(i))->selected = 0;
	for (int i = 0; i < n; i++) {
		BigIcon *b = (BigIcon*)ic->group->child(i);
		if (!b->real_name) continue;
		if (strhas_ci(b->real_name, term)) {
			b->selected = 1;
			if (!first_match) first_match = b;
			found++;
		}
	}
	ic->update_status();
	ic->group->redraw();
	if (first_match) ic->ensure_visible(first_match);
	if (!found)
		fl_message(_("No results for \"%s\""), term);
}

void cb_directory(void*)
{
	struct stat st;
	if (!StatesValues.url) {
		Fl::add_timeout(.5, cb_directory, 0);
		return;
	}
	stat(StatesValues.url, &st);
	if (st.st_mtime != mod_time) {
		cb_rescan(NULL, NULL);
		return;
	}
	Fl::add_timeout(.5, cb_directory, 0);
}
