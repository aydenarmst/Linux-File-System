#pragma once
#include "util.h"

/**************** level-1 function prototypes **************/


// Alloc_dealloc
int ialloc(int dev);
int balloc(int dev);
int idalloc(int dev, int ino);
int bdalloc(int dev, int bno);

// Cd_ls_pwd
int cd(char *pathname);
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *pip);
int ls();
int pwd(MINODE *wd);
void rpwd(MINODE *wd);

// link_unlink
int link();
int unlink();
int truncate(MINODE *mip);

// mkdir_creat
int make_dir(char *pathname);
int mymkdir(MINODE *pip, char *name);
int creat_file(char *pathname);
int mycreat(MINODE *pip, char *child);

// rmdir
int rm_dir();
int rm_child(MINODE *parent, char *name);
int verify_dir(MINODE *mip);
int dir_empty(MINODE *mip);

// symlink
int symlink();
int readlink(const char *pathname, char *buf, int bufsize);
