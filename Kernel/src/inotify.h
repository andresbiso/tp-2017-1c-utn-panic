/*
 * inotify.h
 *
 *  Created on: 8/5/2017
 *      Author: utnso
 */

#ifndef SRC_INOTIFY_H_
#define SRC_INOTIFY_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <string.h>

#define EVENT_SIZE  ( sizeof (struct inotify_event) + 24 )

#define BUF_LEN ( EVENT_SIZE )

void watchFile(char* path,char* file, void  (*function)());




#endif /* SRC_INOTIFY_H_ */
