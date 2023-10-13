#pragma once 
#include "type.h"

/*********** globals in main.c ***********/
extern PROC   proc[NPROC];
extern PROC   *running;

extern MINODE minode[NMINODE];   // minodes
extern MINODE *freeList;         // free minodes list
extern MINODE *cacheList;        // cacheCount minodes list

extern MINODE *root;

extern OFT    oft[NOFT];

extern char gline[256];   // global line hold token strings of pathname
extern char *name[64];    // token string pointers
extern int  n;            // number of token strings                    

extern int ninodes, nblocks;
extern int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

extern int  fd, dev;
extern char cmd[16], pathname[128], parameter[128];
extern int  requests, hits;

/*********** util.c function prototypes ***********/
int get_block(int dev, int blk, char buf[ ]);
int put_block(int dev, int blk, char buf[ ]);

int tokenize(char *pathname);

MINODE *iget(int dev, int ino);
int iput(MINODE *mip);


int search(MINODE *mip, char *name);
MINODE *path2inode(char *pathname);
int findmyname(MINODE *pip, int myino, char myname[ ]);
int findino(MINODE *mip, int *myino) ;
int tst_bit(char *buf, int bit);
int clr_bit(char *buf, int bit);
int decFreeInodes(int dev);
int incFreeInodes(int dev);
int enter_child(MINODE *pip, int myino, char *myname);

