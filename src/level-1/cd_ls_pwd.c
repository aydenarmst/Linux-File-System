/**************** cd_ls_pwd.c file **************/
#include "../../include/level-1.h"

int cd(char* pathname)
{
  MINODE *mip = path2inode(pathname); // gets the inode of the pathname
  printf("chdir: dev=%d ino=%d\n", mip->dev, mip->ino); // prints the dev and ino of the inode
  if(mip == 0){
    printf("cd: %s does not exist", pathname);
    return -1;
  }
  if(!S_ISDIR(mip->INODE.i_mode)){
    printf("cd: %s is not a directory\n", pathname); // checks if the inode is a directory
    return -1;
  }
  printf("cd to [dev ino] = [%d %d]\n", mip->dev, mip->ino); // prints the dev and ino of the inode being cd'd to
  iput(running->cwd); // release the cwd MINODE
  running->cwd = mip; // sets the cwd to the new inode directory
  printf("after cd : cdw = [%d %d]\n", running->cwd->dev, running->cwd->ino); // prints the dev and ino of the cwd
  return 0;

}

int ls_file(MINODE *mip, char *name)
{
  char ftime[64];

  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";
    if (S_ISDIR(mip->INODE.i_mode))
      printf("d");
    if (S_ISREG(mip->INODE.i_mode))
      printf("-");
    if (S_ISLNK(mip->INODE.i_mode))
      printf("l");
    for (int i = 8; i >= 0; i--){
      if (mip->INODE.i_mode & (1 << i))
        printf("%c", t1[i]);
      else
        printf("%c", t2[i]);
    }
    // create a time_t variable
    time_t modTime = mip->INODE.i_mtime;
    // convert the time_t variable to a string
    strcpy(ftime, ctime(&modTime));
    // remove the newline character from the string
    ftime[strlen(ftime) - 1] = 0;
  

    printf("%4d ", mip->INODE.i_links_count);
    printf("%4d ", mip->INODE.i_gid);
    printf("%4d ", mip->INODE.i_uid);
    printf("%15s ",ftime);
    printf("%8d\t", mip->INODE.i_size);
    if (S_ISLNK(mip->INODE.i_mode)){
      printf("%s -> %-10s",basename(name), (char *)mip->INODE.i_block); // return string content of i_block[]
    }
    else{
      printf("%-10s", basename(name));
    }


    printf("[%d %d]", mip->dev, mip->ino);
    printf("\n");
    return 0;
}
  
int ls_dir(MINODE *pip)
{
  char sbuf[BLKSIZE], name[256];
  DIR  *dp;
  char *cp;

  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  MINODE *mip;

  get_block(dev, pip->INODE.i_block[0], sbuf);
  dp = (DIR *)sbuf;
  cp = sbuf;
   
  while (cp < sbuf + BLKSIZE){
    // clear name everytime
    memset(name, 0, 256);
    strncpy(name, dp->name, dp->name_len);
    name[dp->name_len] = 0;

    mip = iget(dev, dp->inode);
    
    ls_file(mip, name);
    iput(mip);

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

}

int ls()
{
  printf("ls %s\n", pathname);
  MINODE *mip = path2inode(pathname); // gets the inode of the pathname
  iput(mip);
  printf("i_block[0] = %d\n", mip->INODE.i_block[0]); // prints the i_block[0] of the inode
  if (S_ISDIR(mip->INODE.i_mode))
    ls_dir(mip);
  else
    ls_file(mip, basename(pathname));
}

int pwd(MINODE *wd)
{
  if(wd == root){
    printf("/\n");
    return;
  }
  else{
    rpwd(wd);
    printf("\n");
  }
}


// recursive on the directory's minode's
void rpwd(MINODE *wd)
{
  if(wd == root)return;

  MINODE *pip;
  int myino, parent_ino;
  char myname[256], sbuf[BLKSIZE];

  get_block(dev, wd->INODE.i_block[0], sbuf); // gets the block of the parent
  parent_ino = findino(wd, &myino); // gets the ino of the parent

  pip = iget(dev, parent_ino); // gets the inode of the parent
  findmyname(pip, myino, myname); // gets the name of the current directory

  rpwd(pip); // recursively calls rpwd on the parent
  iput(pip); // decrements the shared count of the parent
  printf("/%s", myname); // prints the name of the current directory
}