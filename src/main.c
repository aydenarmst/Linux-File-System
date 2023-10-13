// start up files
#include "../include/type.h"
#include "../include/util.h"
#include "../include/level-1.h"
#include "../include/level-2.h"

/********** globals **************/
PROC   proc[NPROC];
PROC   *running;

MINODE minode[NMINODE];   // in memory INODES
MINODE *freeList;         // free minodes list
MINODE *cacheList;        // cached minodes list

MINODE *root;             // root minode pointer

OFT    oft[NOFT];         // for level-2 only

char gline[256];          // global line hold token strings of pathname
char *name[64];           // token string pointers
int  n;                   // number of token strings                    

int ninodes, nblocks;     // ninodes, nblocks from SUPER block
int bmap, imap, inodes_start, iblk;  // bitmap, inodes block numbers

int  fd, dev;
char cmd[16], pathname[128], parameter[128];
int  requests, hits;


int init()
{
  int i, j;
  // initialize minodes into a freeList
  for (i=0; i<NMINODE; i++){
    MINODE *mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->id = i;
    mip->next = &minode[i+1];
  }
  minode[NMINODE-1].next = 0;
  freeList = &minode[0];       // free minodes list

  cacheList = 0;               // cacheList = 0

  for (i=0; i<NOFT; i++)
    oft[i].shareCount = 0;     // all oft are FREE
 
  for (i=0; i<NPROC; i++){     // initialize procs
     PROC *p = &proc[i];    
     p->uid = p->gid = i;      // uid=0 for SUPER user
     p->pid = i+1;             // pid = 1,2,..., NPROC-1

     for (j=0; j<NFD; j++)
       p->fd[j] = 0;           // open file descritors are 0
  }
  
  running = &proc[0];          // P1 is running
  requests = hits = 0;         // for hit_ratio of minodes cache
}

int mount_root(){
  printf("mount_root()\n");
  root = iget(dev, 2);         // get root inode
}

char *disk = "diskimage";

int main(int argc, char *argv[ ]) 
{
  char line[128];
  char buf[BLKSIZE];

  init();
  
  fd = dev = open(disk, O_RDWR);
  printf("dev = %d\n", dev);  // YOU should check dev value: exit if < 0

  // get super block of dev
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;  // you should check s_magic for EXT2 FS
  if(sp->s_magic != 0xEF53){
    printf("NOT an EXT2 FS\n");
    printf("s_magic = %x\n", sp->s_magic);
    exit(1);
  }
  printf("check: superblock magic = %#x OK\n", sp->s_magic);
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;
  printf("ninodes=%d  nblocks=%d  inode_size=%d\n", ninodes, nblocks, sp->s_inode_size);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = inodes_start = gp->bg_inode_table;

  printf("bmap=%d  imap=%d  iblk=%d\n", bmap, imap, iblk);


  // HERE =========================================================
  mount_root();
  running->cwd = iget(dev, 2);
  printf("creating P%d as running process\n", running->pid);
  printf("root shareCount = %d\n", root->shareCount);
  // Endhere ====================================================
  
 /********* write code for iget()/iput() in util.c **********
          Replace code between HERE and ENDhere with

  root         = iget(dev, 2);  
  running->cwd = iget(dev, 2);
 **********************************************************/

  
  while(1){
     pathname[0] = parameter[0] = 0;
     memset(pathname, 0, sizeof(pathname));
     memset(parameter, 0, sizeof(parameter));

     printf("P%d running: input command [cd|ls|pwd|mkdir|creat|rmdir|link|unlink|symlink][open|close|pfd][show|hits|exit] : ", running->pid);
     fgets(line, 128, stdin);
     line[strlen(line)-1] = 0;    // kill \n at end

     if (line[0]==0)
        continue;
      
     sscanf(line, "%s %s %64c", cmd, pathname, parameter);
     if (pathname[0] == 0)
  strcpy(pathname, "./");

     printf("pathname=%s parameter=%s\n", pathname, parameter);
     // if pathname is null, set it to root
     if (strcmp(cmd, "ls")==0)
	ls();
     if (strcmp(cmd, "cd")==0)
	cd(pathname);
     if (strcmp(cmd, "pwd")==0)
        pwd(running->cwd);

     
     if (strcmp(cmd, "show")==0)
	show_dir(running->cwd);
     if (strcmp(cmd, "hits")==0)
	hit_ratio();
     if (strcmp(cmd, "mkdir")==0)
  make_dir(pathname);
     if (strcmp(cmd, "creat")==0)
  creat_file(pathname);
      if (strcmp(cmd, "rmdir")==0)
  rm_dir();
      if (strcmp(cmd, "link")==0)
  link();
      if (strcmp(cmd, "unlink")==0)
  unlink();
      if (strcmp(cmd, "symlink")==0)
  symlink();
      if (strcmp(cmd, "open")==0)
  my_open(pathname, 1);
      if (strcmp(cmd, "pfd")==0)
  pfd();
      if (strcmp(cmd, "close")==0)
  close_file(pathname);
     if (strcmp(cmd, "exit")==0)
	quit();
  }
}

int show_dir(MINODE *mip)
{
  // show contents of mip DIR; same as in LAB5
  int ino = search(mip, ".");
  MINODE *mip2 = iget(dev, ino);
  char sbuf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  get_block(mip->dev, mip2->INODE.i_block[0], sbuf);
  dp = (DIR *)sbuf;
  cp = sbuf;
  printf("  %3s %9s %12s %10s\n", "i_number", "rec_len", "name_len", "name");
  while(cp < sbuf + BLKSIZE){
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;
    printf("  %5d %10d %10d \t\t%-1s\n", dp->inode, dp->rec_len, dp->name_len, temp);
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
}

/*typedef struct minode{		
  INODE INODE;            // disk INODE
  int   dev, ino;
  int   cacheCount;       // minode in cache count
  int   shareCount;       // number of users on this minode
  int   modified;         // modified while in memory
  int   id;               // index ID
  struct minode *next;    // pointer to next minode

}MINODE;*/
int hit_ratio()
{
  // print cacheList;
  // compute and print hit_ratio
  MINODE *mip = running->cwd;  // get cwd of running process
  int count = 0;
  char sbuf[BLKSIZE];
  // loop through cacheList and print
  hits = 0;
  requests = 0;
  printf("cacheList =");
  // for every minode in cacheList
  for(mip = cacheList; mip; mip = mip->next){
    printf("c%d[%d %d]s%d->", mip->cacheCount, mip->dev, mip->ino, mip->shareCount);
    // decrement cacheCount once for each time it is in cache
    mip->cacheCount--;
    if(mip->cacheCount > 0){
      count++;
      hits += mip->cacheCount;
    }
    requests += mip->shareCount;
  }
  printf("NULL\n");
  printf("requests=%d hits=%d hit_ratio=%f\n", requests, hits, (float)hits/requests);
}

int quit()
{
   MINODE *mip = cacheList;
   while(mip){
     if (mip->shareCount){
        mip->shareCount = 1;
        iput(mip);    // write INODE back if modified
     }
     mip = mip->next;
   }
   exit(0);
}







