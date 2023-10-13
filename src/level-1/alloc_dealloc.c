/**************** alloc_dealloc.c **************/
#include "../../include/level-1.h"

int ialloc(int dev)  // allocate an inode number from imap block
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, imap, buf);
		
       decFreeInodes(dev);

       printf("ialloc : ino=%d\n", i+1);		
       return i+1;
    }
  }
  return 0;
}



int balloc(int dev){ // allocate a FREE disk block; return its block number
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0){
       set_bit(buf,i);
       put_block(dev, bmap, buf);
    
       decFreeInodes(dev);

       printf("balloc : ino=%d\n", i+1);		
       return i+1; // return the block number
    }
  }
  return 0;
}

int idalloc(int dev, int ino){
    int i;
    char buf[BLKSIZE];
    MINODE *mip = iget(dev, ino);
    if(ino > ninodes){
        printf("inumber %d out of range\n", ino);
        return 0;
    }
    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);
    //write buf back
    put_block(dev, imap, buf);
    // update free inode count in SUPER and GD
    incFreeInodes(dev);
}

int bdalloc(int dev, int bno){ // deallocates a disk block number bno
    int i;
    char buf[BLKSIZE];
    if(bno > nblocks){
        printf("bnumber %d out of range\n", bno);
        return 0;
    }
    // get inode bitmap block
    get_block(dev, bmap, buf);
    clr_bit(buf, bno-1);
    //write buf back
    put_block(dev, bmap, buf);
    // update free inode count in SUPER and GD
    incFreeInodes(dev);
}