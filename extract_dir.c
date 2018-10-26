
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fileio.h"
#include "resource.h"
#include "util.h"

int main(int argc, char *argv[]) {
	DIR *d;
	struct dirent *de;
	struct stat st;

	g_util_debug_mask = DBG_RESOURCE;

	for (int i = 1; i < argc; ++i) {
		if (stat(argv[i], &st) == 0) {
			if (S_ISDIR(st.st_mode)) { /* extract all .DIR files */
				fio_init(argv[i]);
				d = opendir(argv[i]);
				if (d) {
					while ((de = readdir(d)) != NULL) {
						const char *ext = strrchr(de->d_name, '.');
						if (ext && strcasecmp(ext + 1, "DIR") == 0) {
							resource_dump_files(de->d_name);
						}
					}
					closedir(d);
				}
				fio_fini();
			} else if (S_ISREG(st.st_mode)) { /* extract a single .DIR file */
				const char *ext = strrchr(argv[i], '.');
				if (ext && strcasecmp(ext + 1, "DIR") == 0) {
					char *p = strrchr(argv[i], '/');
					if (p) {
						*p = 0;
						fio_init(argv[i]);
					} else {
						fio_init(".");
					}
					resource_dump_files(p + 1);
					fio_fini();
				}
			}
		}
	}
	return 0;
}
