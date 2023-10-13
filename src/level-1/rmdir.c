/**************** rmdir.c file **************/
#include "../../include/level-1.h"

int rm_dir(){
    // get cwd and add to pathname if not absolute
    printf("pathname = %s", pathname);

    MINODE *mip = path2inode(pathname);

    if(verify_dir(mip) == -1){ // verify that mip is a directory and is empty and is not busy
        return -1;
    }
    // get parent DIR pino in mip->INODE.i_block[0]
    int pino = findino(mip, &mip->ino);

    // deallocate its block and inode
    for(int i = 0; i < 12; i++){
        if(mip->INODE.i_block[i] == 0)
            continue;
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip); // clears mip->shareCount to 0, still in cache

    // get parent minode
    MINODE *pip = iget(mip->dev, pino);
    rm_child(pip, name[n-1]); // remove child from parent
    pip->INODE.i_atime = time(0L); // touch parent's atime
    pip->INODE.i_mtime = time(0L); // touch parent's mtime
    pip->INODE.i_links_count--; // dec parent's link count by 1
    pip->modified = 1; // mark pip modified
    iput(pip); // this will write parent INODE out eventually
    return 0;
}



int rm_child(MINODE *parent, char *name) {
    DIR *prev = NULL, *dp;
    char *cp;
    char temp[128], buf[BLKSIZE];

    // Loop through the first 12 blocks of the parent directory
    for(int i = 0; i < 12 && parent->INODE.i_block[i] != 0; i++) {
        get_block(dev, parent->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        cp = buf;

        // Traverse through the current block
        while (cp < buf + BLKSIZE) {
            // Copy the name of the current directory entry into a temporary buffer
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            // If the name of the directory entry matches the given name
            if (strcmp(temp, name) == 0) {
                // If it is not the first entry in the block
                if (prev != NULL) {
                    prev->rec_len += dp->rec_len; // Add the rlen to predecessor's rlen
                    put_block(dev, parent->INODE.i_block[i], buf); // Write the block back to disk
                }
                // If it is the first and only entry in the block
                else if (cp == buf && dp->rec_len == BLKSIZE) {
                    bdalloc(dev, parent->INODE.i_block[i]); // Deallocate the block
                    parent->INODE.i_size -= BLKSIZE; // Reduce the size of the parent directory by the block size

                    // Compact the parent's data blocks
                    for (int j = i; j < 11 && parent->INODE.i_block[j + 1] != 0; j++) {
                        get_block(dev, parent->INODE.i_block[j + 1], buf); // Get the next block
                        put_block(dev, parent->INODE.i_block[j], buf); // Write the block to the current block
                        parent->INODE.i_block[j + 1] = 0; // Clear the next block
                    }
                }
                // If it is not the first and only entry in the block
                else {
                    char *last_cp = buf;
                    DIR *last_dp = (DIR *)buf;

                    // Traverse to the end of the block to find the last entry
                    while (last_cp + last_dp->rec_len < buf + BLKSIZE) {
                        last_cp += last_dp->rec_len;
                        last_dp = (DIR *)last_cp;
                    }

                    last_dp->rec_len += dp->rec_len; // Add the rec length of the current entry to the last entry
                    char *memLocation = cp + dp->rec_len; // Get the memory location of the current entry
                    int numBytes = buf + BLKSIZE - (cp + dp->rec_len); // Get the number of bytes to move\

                    memcpy(cp, memLocation, numBytes); // Move the trailing entries LEFT
                    put_block(dev, parent->INODE.i_block[i], buf); // Write the block back to disk
                }
                parent->modified = 1; // mark the parent's minode MODIFIED for write back
                return 0; // Return success
            }

            // Update the previous directory entry, current position and directory entry
            prev = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return -1; // Return failure
}


int verify_dir(MINODE *mip)
{
    char buf[BLKSIZE], *cp;
    DIR *dp;
    if(!mip){
        printf("ERROR: pathname does not exist\n");
        return -1;
    }
    if(!S_ISDIR(mip->INODE.i_mode)){
        printf("ERROR: pathname is not a directory\n");
        return -1;
    }
    // verify minode is not busy
    if(mip->shareCount > 1){
        printf("ERROR: minode is busy\n");
        return -1;
    }
    // verify directory is empty
    if(!dir_empty(mip)){
        printf("ERROR: directory is not empty\n");
        return -1;
    }
}

int dir_empty(MINODE *mip)
{
    char buf[BLKSIZE], *cp;
    DIR *dp;
    int i;

    if(mip->INODE.i_links_count > 2){  // if Links count is greater than 2, it is definitely not empty
        printf("ERROR: directory is not empty\n");
        return 0;
    }
    else if(mip->INODE.i_links_count == 2){  // if links count ==2 may still have FILEs, so check data blocks
        for(i = 0; i < 12; i++){ 
            if(mip->INODE.i_block[i] == 0)
                continue;
            get_block(mip->dev, mip->INODE.i_block[i], buf);
            dp = (DIR *)buf;
            cp = buf;
            while(cp < buf + BLKSIZE){
                if(strcmp(dp->name, ".") && strcmp(dp->name, "..")) // if name is not . or .. then directory is not empty
                    return 0;
                cp += dp->rec_len;
                dp = (DIR *)cp;
            }
        }

    }
    return 1;
}