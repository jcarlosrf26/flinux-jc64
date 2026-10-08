#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_VOLUMES 32

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

static char *shell_quote(const char *s)
{
	char *out = (char*) malloc(strlen(s) * 4 + 3);
	char *o = out;
	*o++ = '\'';
	for (; *s; s++) {
		if (*s == '\'') {
			*o++ = '\''; *o++ = '\\'; *o++ = '\''; *o++ = '\'';
		} else {
			*o++ = *s;
		}
	}
	*o++ = '\'';
	*o = '\0';
	return out;
}

int main()
{
	struct volume_entry known[MAX_VOLUMES];
	int known_count = -1;

	while (1) {
		struct volume_entry vols[MAX_VOLUMES];
		int n = list_volumes(vols, MAX_VOLUMES);

		if (known_count < 0) {
			memcpy(known, vols, sizeof(vols));
			known_count = n;
		} else {
			for (int i = 0; i < n; i++) {
				int found = 0;
				for (int j = 0; j < known_count; j++) {
					if (!strcmp(vols[i].mountpoint, known[j].mountpoint)) {
						found = 1;
						break;
					}
				}
				if (!found) {
					char q[700];
					snprintf(q, sizeof(q), "%s\n\n%s",
						"New volume detected. Open with FLFM?",
						vols[i].mountpoint);
					int answer = fl_ask("%s", q);
					Fl::flush();
					if (answer) {
						char *qpath = shell_quote(vols[i].mountpoint);
						char buf[600];
						snprintf(buf, sizeof(buf), "flfm %s &", qpath);
						system(buf);
						free(qpath);
					}
				}
			}
			memcpy(known, vols, sizeof(vols));
			known_count = n;
		}

		sleep(3);
	}
	return 0;
}
