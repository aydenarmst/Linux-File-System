#pragma once
#include "util.h"

/**************** level-2 function prototypes **************/

int my_lseek(int fd, int position);


// Open File
int my_open(char *pathname, int mode);
int my_truncate(MINODE *mip);
int pfd();

// Close File
int close_file(char *pathname);
int my_close(int fd);

// Read File
int read_file();
int my_read(int fd, char *buf, int nbytes);