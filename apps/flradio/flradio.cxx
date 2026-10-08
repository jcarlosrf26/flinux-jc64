/*
 * FlRadio Internet radio - versión adaptada para ALSA
 *
 * Basado en código original de Georg Potthast
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Slider.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <alsa/asoundlib.h>

#define MAX_VOLUME 100
#define MIN_VOLUME 0

// --- Variables ALSA
snd_mixer_t *mixerHandle = nullptr;
snd_mixer_elem_t* mixerElem = nullptr;
long minVolume = 0, maxVolume = 100;

// --- Variables globales
FILE *fr_fp = nullptr;
Fl_Slider *volumeslider = nullptr;

// --- Funciones
bool initALSA() {
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    if (snd_mixer_open(&mixerHandle, 0) < 0) return false;
    if (snd_mixer_attach(mixerHandle, card) < 0) return false;
    if (snd_mixer_selem_register(mixerHandle, nullptr, nullptr) < 0) return false;
    if (snd_mixer_load(mixerHandle) < 0) return false;

    snd_mixer_selem_id_malloc(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    mixerElem = snd_mixer_find_selem(mixerHandle, sid);
    snd_mixer_selem_id_free(sid);

    if (!mixerElem) return false;

    snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume);
    long currentVol = 0;
    snd_mixer_selem_get_playback_volume(mixerElem, SND_MIXER_SCHN_FRONT_LEFT, &currentVol);

    double normalized = (double)(currentVol - minVolume) / (double)(maxVolume - minVolume) * 100.0;
    volumeslider->value(normalized);

    return true;
}

void Update_CB(Fl_Widget*, void*) {
    double rawVolume = volumeslider->value();
    long alsaVol = (long)((rawVolume / 100.0) * (maxVolume - minVolume) + minVolume);

    if (mixerElem) {
        snd_mixer_selem_set_playback_volume_all(mixerElem, alsaVol);
    }
}

void Close_CB(Fl_Widget *, void*) {
    system("pkill mpg123");
    if (fr_fp != nullptr) {
        pclose(fr_fp);
        fr_fp = nullptr;
    }
}

void Exit_CB(Fl_Widget*, void*) {
    Close_CB(nullptr, nullptr);
    if (mixerHandle) {
        snd_mixer_close(mixerHandle);
    }
    exit(0);
}

void Help_CB(Fl_Widget*, void*) {
    fl_alert("Click on a station to start streaming.\n"
             "Click Stop to stop.\n"
             "Use File to open an MP3 file.\n"
             "Some stations may no longer work.");
}

void Open_CB(Fl_Widget*, void*) {
    const char *filename = fl_file_chooser("Open ...", "*.{mp3,mpeg,MP3,MPEG}", ".");
    char cmdline[512];
    if (filename) {
        snprintf(cmdline, sizeof(cmdline), "mpg123 -q \"%s\"", filename);
        if ((fr_fp = popen(cmdline, "r")) == nullptr) {
            fl_message("popen failed");
        }
    } else {
        fl_alert("Could not load file");
    }
}

void BrowserCallback(Fl_Widget *w, void*) {
    Fl_Browser *fbrow = (Fl_Browser*)w;
    int index = fbrow->value();
    if (index == 0) return;

    char url[256], city[256], radioname[256];
    char *tabpos = strchr((char*)fbrow->text(index), '\t');
    char *tabpos2 = strchr(tabpos + 1, '\t');
    char *commentpos = strchr((char*)fbrow->text(index), '#');

    strcpy(url, tabpos2 + 1);
    if (commentpos != nullptr)
        url[commentpos - 1 - tabpos2] = '\0';

    strcpy(city, tabpos + 1);
    city[tabpos2 - 1 - tabpos] = '\0';

    strncpy(radioname, fbrow->text(index), tabpos - fbrow->text(index));
    radioname[tabpos - fbrow->text(index)] = '\0';

    char cmdline[512];
    if (strstr(url, ".m3u")) {
        snprintf(cmdline, sizeof(cmdline), "wget -O - `wget -O - %s` | mpg123 -", url);
    } else if (strstr(url, ".pls")) {
        snprintf(cmdline, sizeof(cmdline), "mpg123 -q -@ %s", url);
    } else {
        snprintf(cmdline, sizeof(cmdline), "mpg123 -q %s", url);
    }

    Close_CB(nullptr, nullptr);

    if ((fr_fp = popen(cmdline, "r")) == nullptr) {
        fl_message("popen failed");
        return;
    }
}

int main() {
    Fl::scheme("OXY");

    Fl_Window *w = new Fl_Window(620, 520, "FlRadio (ALSA)");

    Fl_Browser *b = new Fl_Browser(10, 10, w->w() - 20, w->h() - 70);
    static int widths[] = { 240, 120, 0 };
    b->column_widths(widths);
    b->type(FL_HOLD_BROWSER);
    b->load("/home/tc/webstations.lst");
    b->callback(BrowserCallback);

    Fl_Button *close = new Fl_Button(10, w->h() - 50, 50, 20, "Stop");
    close->callback(Close_CB);

    Fl_Button *file = new Fl_Button(80, w->h() - 50, 50, 20, "File");
    file->callback(Open_CB);

    Fl_Button *help = new Fl_Button(150, w->h() - 50, 50, 20, "Help");
    help->callback(Help_CB);

    Fl_Button *exit = new Fl_Button(w->w() - 60, w->h() - 50, 50, 20, "Exit");
    exit->callback(Exit_CB);

    volumeslider = new Fl_Slider(263, w->h() - 50, 230, 20, "0                 Volume                  100");
    volumeslider->type(FL_HOR_SLIDER);
    volumeslider->range(MIN_VOLUME, MAX_VOLUME);
    volumeslider->step(1);
    volumeslider->value(50);
    volumeslider->labelfont(0);
    volumeslider->labelcolor(0);
    volumeslider->callback(Update_CB);

    w->resizable(b);
    w->end();

    if (!initALSA()) {
        fl_alert("No se pudo inicializar el control de volumen ALSA.");
    }

    w->show();
    return Fl::run();
}
