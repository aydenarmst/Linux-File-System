/**************** mkdir_creat.c file **************/
#include "../../include/level-1.h"

int make_dir(char *pathname)
{
    char parent_copy[128], child_copy[128];
    char *parent, *child;
    MINODE *pip;

    strcpy(parent_copy, pathname);
    strcpy(child_copy, pathname);

    parent = dirname(parent_copy);
    child = basename(child_copy);

    pip = path2inode(parent);
    printf("parent = %s\n", parent);
    if(!pip)
    {
        printf("ERROR: parent does not exist\n");
        return -1;
    }
    if(!S_ISDIR(pip->INODE.i_mode))
    {
        printf("ERROR: parent is not a directory\n");
        return -1;
    }

    // check if child exists in parent
    if(search(pip, child))
    {
        printf("ERROR: child already exists in parent\n");
        return -1;
    }

    mymkdir(pip, child);
    pip->INODE.i_links_count++; // inc parent inodes's links count by 1
    // touch pip's atime
    pip->INODE.i_atime = time(0L);
    // mark it modified
    pip->modified = 1;
    iput(pip);
} 

int mymkdir(MINODE *pip, char *name){
    int ino = ialloc(pip->dev);
    int bno = balloc(pip->dev);
    printf("ino = %d\tblock = %d\n", ino, bno);
    MINODE *mip = iget(pip->dev, ino); // load inode into a minode[]
    printf("mkdir : dev = %d ino = %d\n", pip->dev, ino);
    // write the contents to mip->INODE to make it a DIR INODE, mark it modified
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED; // OR 040755: DIR type and permissions
    ip->i_uid = running->uid; // Owner uid
    ip->i_gid = running->gid; // Group Id
    ip->i_size = BLKSIZE; // Size in bytes
    ip->i_links_count = 2; // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
    ip->i_block[0] = bno; // new DIR has one data block
    for(int i = 1; i < 15; i++)
        ip->i_block[i] = 0;

    mip->modified = 1; // mark minode MODIFIED
    iput(mip); // write INODE to disk
    printf("writing data block %d to disk\n", bno);

    // write . and .. entries to a buf[ ] of BLKSIZE
    char buf[BLKSIZE];
    DIR *dp = (DIR *)buf;
    char *cp = buf;
    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    cp += dp->rec_len;
    // make .. entry
    dp = (DIR *)cp;
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE - 12;
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';

    put_block(pip->dev, bno, buf); // write buf[ ] to disk block bno

    enter_child(pip, ino, name); // enter name into parent's directory
}



// similar to mkdir except the inode.i mode field is set to a reg file type, permission bits to 0644 = rw r r, and no data block is allocated for it, so the file size is 0, links count = 1, do not increment parent's link count
int creat_file(char *pathname){
    char parent_copy[128], child_copy[128];
    char *parent, *child;
    MINODE *pip;

    strcpy(parent_copy, pathname);
    strcpy(child_copy, pathname);

    parent = dirname(parent_copy);
    child = basename(child_copy);

    pip = path2inode(parent);
    printf("parent = %s\n", parent);
    if(!pip)
    {
        printf("ERROR: parent does not exist\n");
        return -1;
    }
    if(!S_ISDIR(pip->INODE.i_mode))
    {
        printf("ERROR: parent is not a directory\n");
        return -1;
    }

    // check if child exists in parent
    if(search(pip, child))
    {
        printf("ERROR: child already exists in parent\n");
        return -1;
    }

    mycreat(pip, child);
    // touch pip's atime
    pip->INODE.i_atime = time(0L);
    // mark it modified
    pip->modified = 1;
    iput(pip);
    return 0;
}

int mycreat(MINODE *pip, char *child){
    int ino = ialloc(pip->dev);
    printf("ino = %d\n", ino);
    MINODE *mip = iget(pip->dev, ino); // load inode into a minode[]
    printf("creat : dev = %d ino = %d\n", pip->dev, ino);
    // write the contents to mip->INODE to make it a DIR INODE, mark it modified
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4; // OR 0100644: REG type and permissions
    ip->i_uid = running->uid; // Owner uid
    ip->i_gid = running->gid; // Group Id
    ip->i_size = 0; // Size in bytes
    ip->i_links_count = 1; // Links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time
    ip->i_blocks = 0; // LINUX: Blocks count in 512-byte chunks
    for(int i = 0; i < 15; i++)
        ip->i_block[i] = 0;

    mip->modified = 1; // mark minode MODIFIED
    iput(mip); // write INODE to disk

    enter_child(pip, ino, child); // enter name into parent's directory
}