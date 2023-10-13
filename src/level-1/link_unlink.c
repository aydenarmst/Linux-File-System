/**************** link_unlink_symlink.c file **************/
#include "../../include/level-1.h"

int link()
{
    MINODE *mip;
    char parent[128], child[128];
    printf("link: %s %s\n", pathname, parameter);
    mip = path2inode(pathname);

    if(mip == 0){
        printf("link: %s does not exist", pathname);
        return -1;
    }
    // check if is a DIR
    if(S_ISDIR(mip->INODE.i_mode)){
        printf("%s is a DIR, can't link to DIR\n", pathname);
        return -1;
    }

    //break into parent and child
    strcpy(parent, pathname);
    strcpy(child, parameter);
    strcpy(parent, dirname(parent));
    strcpy(child, basename(child));

    // check parent exists and is a DIR
    MINODE *pmip = path2inode(parent);
    if(pmip == 0){
        printf("link: %s does not exist", parent);
        return -1;
    }
    if(!S_ISDIR(pmip->INODE.i_mode)){
        printf("link: %s is not a directory\n", parent);
        return -1;
    }
    // check child does not exist yet in parent
    int ino = search(pmip, child);
    if(ino != 0){
        printf("link: %s already exists in %s\n", child, parent);
        return -1;
    }
    // add an entry to parent DIR's data block
    enter_child(pmip, mip->ino, child);
    // increment mip's link count by 1
    mip->INODE.i_links_count++;
    printf("link: %s linked to %s at [%d %d] links_count = %d\n", pathname, parameter, mip->dev, mip->ino, mip->INODE.i_links_count);
    // mark mip modified
    mip->modified = 1;
    // write mip back to disk
    iput(mip);
    // write pmip back to disk
    iput(pmip);
    return 0;
}

int unlink()
{
    MINODE *mip;
    char parent[128], child[128];
    printf("unlink: %s", pathname);
    mip = path2inode(pathname);
    // check if its a REG or symbolic link file, cannot unlink a DIR
    if(!S_ISREG(mip->INODE.i_mode) && !S_ISLNK(mip->INODE.i_mode)){
        printf("unlink: %s is not a REG or symbolic link file", pathname);
        return -1;
    }
    // decrement mip's link count by 1
    mip->INODE.i_links_count--;
    // if link count == 0 ==> rm pathname by calling truncate()
    if(mip->INODE.i_links_count == 0){
        truncate(mip);
        idalloc(mip->dev, mip->ino); // deallocate its INODE
    }
    //remove childname 
    strcpy(parent, pathname);
    strcpy(child, pathname);
    strcpy(parent, dirname(parent));
    strcpy(child, basename(child));
    MINODE *parentInodePtr = path2inode(parent);
    rm_child(parentInodePtr, child);
    printf("unlink: %s unlinked from %s at [%d %d] links_count = %d\n", pathname, parent, mip->dev, mip->ino, mip->INODE.i_links_count);
    // write mip back to disk
    iput(mip);
    // write pmip back to disk
    iput(parentInodePtr);
    return 0;
}


int truncate(MINODE *mip)
{
    // Deallocate all data blocks of INODE 
    for (int i = 0; i < 12 && mip->INODE.i_block[i]; i++) {
        bdalloc(mip->dev, mip->INODE.i_block[i]);
        mip->INODE.i_block[i] = 0;
    }

    // Update inode fields and mark as modified
    mip->INODE.i_size = 0;
    mip->INODE.i_blocks = 0;
    mip->modified = 1;

    // Write inode back to disk and return success
    iput(mip);
    return 0;
}