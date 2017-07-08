/*
 * inotify.c
 *
 *  Created on: 8/5/2017
 *      Author: utnso
 */

#include "inotify.h"

void watchFile(char* path,char* file, void  (*function)()) {
	char buffer[BUF_LEN];

	int file_descriptor = inotify_init();
	if (file_descriptor < 0) {
		return;
	}

	int watch_descriptor = inotify_add_watch(file_descriptor, path, IN_CLOSE_WRITE);

	while(1){
		int length = read(file_descriptor, buffer, BUF_LEN);
		if (length < 0) {
			return;
		}

		int offset = 0;

		while (offset < length) {

			struct inotify_event *event = (struct inotify_event *) &buffer[offset];

			if (event->len && strcmp(event->name,file)==0) {
				if (event->mask & IN_CLOSE_WRITE)
					function();
			}
			offset += sizeof (struct inotify_event) + event->len;
		}
	}
	inotify_rm_watch(file_descriptor, watch_descriptor);
	close(file_descriptor);
}
