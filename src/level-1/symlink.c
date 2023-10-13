/**************** symlink.c file **************/
#include "../../include/level-1.h"

int symlink()
{
    // check if pathname exists
    MINODE *mip = path2inode(pathname);
    if(!mip){
        printf("symlink: %s does not exist", pathname);
        return -1;
    }
    // check if dir or regular file
    if(S_ISDIR(mip->INODE.i_mode) || S_ISREG(mip->INODE.i_mode)){
        printf("symlink: %s is a DIR or REG file\n", pathname);
        iput(mip);
    }
    else{
        printf("symlink: %s isn't reg or DIR", pathname);
        iput(mip);
        return -1;
    }
    // creat a new file named parameter
    if(creat_file(parameter) == -1){
        printf("symlink: %s error creating file", parameter);
        return -1;
    }

    // change the new file's mode to 0120000 (symbolic link)
    mip = path2inode(parameter);
    mip->INODE.i_mode = mip->INODE.i_mode | 0120000; // set mode to symbolic link wihtout changing other mode bits
    strncpy(mip->INODE.i_block, pathname, 60); // copy pathname into i_block (which has room for 60 chars)
    mip->INODE.i_size = strlen(pathname); // set size to pathname length + null terminator
    mip->modified = 1;
    iput(mip);
}

int readlink(const char *pathname, char *buf, int bufsize) {
    // check if pathname exists
    MINODE *mip = path2inode(pathname);
    if(!mip){
        printf("readlink: %s does not exist", pathname);
        return -1;
    }
    // check if symbolic link
    if(!S_ISLNK(mip->INODE.i_mode)){
        printf("readlink: %s is not a symbolic link", pathname);
        return -1;
    }
    // copy i_block into buf
    strncpy(buf, mip->INODE.i_block, bufsize);
    // return success
    return 0;
}