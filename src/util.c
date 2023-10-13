/**************** util.c file **************/
#include "../include/util.h"

int get_block(int dev, int blk, char buf[ ])
{
  lseek(dev, blk*BLKSIZE, SEEK_SET);
  int n = read(fd, buf, BLKSIZE);
  return n;
}

int put_block(int dev, int blk, char buf[ ])
{
  lseek(dev, blk*BLKSIZE, SEEK_SET);
  int n = write(fd, buf, BLKSIZE);
  return n; 
}       

int tokenize(char *pathname)
{
  char *s;
  strcpy(gline, pathname);  // copy into global gline for global use
  n = 0;
  s = strtok(gline, "/");
  while(s){
    name[n++] = s;
    s = strtok(0, "/");
  }
  name[n] = 0;
  return n;
}

MINODE *iget(int dev, int ino) // return minode pointer of (dev, ino)
{
  MINODE *mip;
  INODE *ip;
  int block, offset;
  char buf[BLKSIZE];

  // find the iblock size
  int ipb = BLKSIZE / 256; // inodes per block
  int ifactor = 2;  // factor to multiply offset to get next inode
  
 /*
  1. search cacheList for minode=(dev, ino);
  if (found){
     inc minode's cacheCount by 1;
     inc minode's shareCount by 1;
     return minode pointer;
  } */
  mip = cacheList;
  while(mip){
    if(mip->shareCount && (mip->dev == dev) && (mip->ino == ino)){
      mip->cacheCount++;
      mip->shareCount++;
      return mip;
    }
    mip = mip->next;
  }
  
  /*
  // needed (dev, ino) NOT in cacheList
  2. if (freeList NOT empty){
        remove a minode from freeList;
        set minode to (dev, ino), cacheCount=1 shareCount=1, modified=0;
 
        load INODE of (dev, ino) from disk into minode.INODE;

        enter minode into cacheList; 
        return minode pointer;
     }
     */
  if(freeList){
    mip = freeList;
    freeList = freeList->next; // remove from freeList
    mip->dev = dev; // set minode to (dev, ino)
    mip->ino = ino;
    mip->cacheCount = 1;
    mip->shareCount = 1;
    mip->modified = 0;

    // New mailmans to accomodate for 256 byte inodes
    int offset = (ino - 1) % ipb;
    int block = (ino - 1) / ipb + inodes_start;
    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + (offset * ifactor);
    mip->INODE = *ip;
    mip->next = cacheList;
    cacheList = mip;
    return mip;
  }

    /*  
  // freeList empty case:
  3. find a minode in cacheList with shareCount=0, cacheCount=SMALLest
     set minode to (dev, ino), shareCount=1, cacheCount=1, modified=0
     return minode pointer;

 NOTE: in order to do 3:
       it's better to order cacheList by INCREASING cacheCount,
       with smaller cacheCount in front ==> search cacheList
  ************/

  MINODE *prev = 0;
  MINODE *cur = cacheList;
  MINODE *next = cacheList->next;
  while(cur){
    if(next && next->cacheCount < cur->cacheCount){
      if(prev){
        prev->next = next;
      }else{
        cacheList = next;
      }
      cur->next = next->next;
      next->next = cur;
      prev = next;
      next = cur->next;
    }else{
      prev = cur;
      cur = next;
      next = next->next;
    }
  }

  // now cacheList is ordered by cacheCount
  // find smallest cacheCount of shareCount = 0
  mip = cacheList;
  MINODE *smallest = mip;
  while(mip){
    if(mip->shareCount == 0 && mip->cacheCount < smallest->cacheCount){
      smallest = mip;
    }
    mip = mip->next;
  }
  smallest->dev = dev;
  smallest->ino = ino;
  smallest->shareCount = 1;
  smallest->cacheCount = 1;
  smallest->modified = 0;

  offset = (ino - 1) % ipb;
  block = (ino - 1) / ipb + inodes_start;
  get_block(dev, block, buf);
  ip = (INODE *)buf + (offset * ifactor); // updated offset
  smallest->INODE = *ip;
  return smallest;
}

int iput(MINODE *mip)  // release a mip
{
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];
  int ipb = BLKSIZE / 256;
  int ifactor = 2;

  if(mip == 0){
      return -1;
    }
  mip->shareCount--; // dec shareCount by 1
  if(mip->shareCount > 0){ // still has user
    return -1;
  }
  if(mip->modified == 0){ // no need to write back
    return -1;
  }
/*
 2. // last user, INODE modified: MUST write back to disk
    Use Mailman's algorithm to write minode.INODE back to disk)
    // NOTE: minode still in cacheList;
    *****************/

  block = (mip->ino - 1) / ipb + inodes_start;
  offset = (mip->ino - 1) % ipb;
  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + (offset * ifactor); // ip points at INODE
  *ip = mip->INODE; // copy INODE to inode in block
  put_block(mip->dev, block, buf); // write back to disk
  mip->modified = 0; // set modified to 0
} 

int search(MINODE *mip, char *name)
{
  /******************
  search mip->INODE data blocks for name:
  if (found) return its inode number;
  else       return 0;
  ******************/
  char sbuf[BLKSIZE], temp[256], *cp;
  DIR *dp;
  printf("search for %s in inode# %d\n", name, mip->ino);

  for(int i = 0; i < 12; i++){  // search DIR direct blocks only
    if(mip->INODE.i_block[i] == 0){
      return 0;
    }

    get_block(mip->dev, mip->INODE.i_block[i], sbuf); // get data block 0
    dp = (DIR *)sbuf; // set dp to start of block
    cp = sbuf; // set cp char pointer to start of block
    printf("  %3s %9s %12s %10s\n", "i_number", "rec_len", "name_len", "name");


    while(cp < sbuf + BLKSIZE){
      strncpy(temp, dp->name, dp->name_len); // copy name from DIR to temp
      temp[dp->name_len] = 0; // set end of string
      printf("  %5d %10d %10d \t\t%-1s\n", dp->inode, dp->rec_len, dp->name_len, temp);
      if(strcmp(temp, name) == 0){
        printf("found %s : inumber = %d\n", name, dp->inode);
        return dp->inode; // return inode number
      }
      cp += dp->rec_len; // move the cp to the next entry
      dp = (DIR *)cp; // set the dp to the next entry
    }

  }
  return 0;
}

MINODE *path2inode(char *pathname) 
{
  /*******************
  return minode pointer of pathname;
  return 0 if pathname invalid;
  
  This is same as YOUR main loop in LAB5 without printing block numbers
  *******************/
  int ino, i;
  MINODE *mip, *pip;
  if(pathname[0] == '/'){
    mip = iget(root->dev, 2); // get root minode
  }else{
    mip = iget(running->cwd->dev, running->cwd->ino); // get cwd minode
  }
  mip->shareCount++; // in order to iput(mip) later

  tokenize(pathname); // tokenize pathname into name[ ]
  for(i = 0; i < n; i++){
    if(!S_ISDIR(mip->INODE.i_mode)){ // check if INODE is a DIR
      printf("%s is not a directory\n", name[i]);
      iput(mip);
      return 0;
    }
    ino = search(mip, name[i]); // search for name[i] in mip->INODE
    if(ino == 0){ // if name[i] does not exist
      printf("no such component name %s\n", name[i]);
      iput(mip);
      return 0;
    }
    iput(mip); // release current minode
    mip = iget(dev, ino); // get minode of name[i]
  }
  iput(mip);
  return mip;
}


// almost the same as search() but search by ino number
int findmyname(MINODE *pip, int myino, char myname[ ]) 
{
  /****************
  pip points to parent DIR minode: 
  search for myino;    // same as search(pip, name) but search by ino
  copy name string into myname[256]
  ******************/
  char sbuf[BLKSIZE], temp[256];
  char *cp;
  DIR *dp;
  int i;
  get_block(pip->dev, pip->INODE.i_block[0], sbuf);
  dp = (DIR *)sbuf; // set the dp to the start of the block
  cp = sbuf; // set the cp char pointer to the start of the block
  while(cp < sbuf + BLKSIZE){
    strncpy(temp, dp->name, dp->name_len);
    temp[dp->name_len] = 0;
    if(dp->inode == myino){
      strcpy(myname, temp);
      printf("findmyname: myino = %d\n",dp->inode);
      return dp->inode;
    }
    cp += dp->rec_len; // move the cp to the next entry
    dp = (DIR *)cp; // set the dp to the next entry
  }
  printf("findmyname: can't find my name\n");
  return 0;
}
 
int findino(MINODE *mip, int *myino) 
{
  /*****************
  mip points at a DIR minode
  i_block[0] contains .  and  ..
  get myino of .
  return parent_ino of ..
  *******************/
  char sbuf[BLKSIZE];
  char *cp;
  DIR *dp;
  get_block(mip->dev, mip->INODE.i_block[0], sbuf);
  dp = (DIR *)sbuf; // set the dp to the start of the block
  cp = sbuf; // set the cp to the start of the block
  *myino = dp->inode; // set myino to the inode number of the first entry
  cp += dp->rec_len; // move the cp to the next entry
  dp = (DIR *)cp; // set the dp to the next entry
  return dp->inode; // return the inode number of the next entry
}


// Start of mkdir


int tst_bit(char *buf, int bit){
  // return 1 if bit is set in buf; 0 if not
  int i = bit / 8;
  int j = bit % 8;

  if(buf[i] & (1 << j)){
    return 1; // bit is set
  }
  return 0; // bit is not set
}

int set_bit(char *buf, int bit){
  // set bit in buf[ ]
  int i = bit / 8;
  int j = bit % 8;
  buf[i] |= (1 << j);
  return 0;
}

int clr_bit(char *buf, int bit) // clear bit in char buf[BLKSIZE]
{ buf[bit/8] &= ~(1 << (bit % 8)); }



int decFreeInodes(int dev) // decrement the free inodes count in SUPER and GD
{
  char buf[BLKSIZE];

  // dec free inodes count by 1 in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}


int incFreeInodes(int dev){
  char buf[BLKSIZE];

  // inc free inodes count by 1 in SUPER and GD
  get_block(dev, 1, buf);
  SUPER *sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);

  get_block(dev, 2, buf);
  GD *gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int enter_child(MINODE *pip, int myino, char *myname){
    int i;
    char buf[BLKSIZE], *cp;
    DIR *dp;
    int NEED_LEN = 4*((8+strlen(myname)+3)/4); // a multiple of 4
    printf("Enter: parent = [%d %d]\n", pip->dev, pip->ino);
    printf("need_len for %s = %d\n", myname, NEED_LEN);
    

    for(int i = 0; i < 12; i++){ // for each data block of parent do (assume only 12 direct blocks)
        if(pip->INODE.i_block[i] == 0) // if no data block
            break; // break
        printf("i = %d i_block[%d] = %d\n", i, i, pip->INODE.i_block[i]);
        get_block(pip->dev, pip->INODE.i_block[i], buf); // get parent's data block into buf[]
        dp = (DIR *)buf;
        cp = buf;
        while(cp < buf + BLKSIZE){
            int IDEAL_LEN = 4*((8+dp->name_len+3)/4); // a multiple of 4
            int REMAIN = dp->rec_len - IDEAL_LEN;
            if(REMAIN >= NEED_LEN){
                // enter the new entry as the LAST entry and trim the previous entry to its IDEAL_LENGTH
                printf("rec_len = %d ideal = %d remain = %d\n", dp->rec_len, IDEAL_LEN, REMAIN);
                dp->rec_len = IDEAL_LEN;
                cp += dp->rec_len;
                dp = (DIR *)cp;
                dp->inode = myino;
                dp->rec_len = REMAIN;
                dp->name_len = strlen(myname);
                strcpy(dp->name, myname);

                put_block(pip->dev, pip->INODE.i_block[i], buf); // write to disk block
                return 1;
            }
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
}