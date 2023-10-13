/**************** open_close.c file **************/
#include "../../include/level-2.h"
#include "../../include/level-1.h"

// sets the offset in the OFT of an opened file tothe byte positon from the beginning of the file
int my_lseek(int fd, int positon)
{
    OFT *oftp = running->fd[fd];    // from fd, find the OFT entry
    if(oftp == NULL)                   // check if the OFT entry is valid
    {
        printf("ERROR: OFT entry is invalid\n");
        return -1;
    }
    if(positon < 0 || positon > oftp->inodeptr->INODE.i_size) // check if the position is valid
    {
        printf("ERROR: position is invalid\n");
        return -1;
    }
    int original_position = oftp->offset; // save the original position

    oftp->offset = positon; // set the offset to the position
    return original_position; // return the original position
}

int my_open(char* path, int mode)
{
    // get paths minode pointer
    MINODE *mip = path2inode(path);

    if(!mip){ // path does not exist
        if(mode == 0){ // READ: file must exist
            printf("READ: file must exist\n");
            return -1;
        }
        printf("in path does not exist\n");
        // mode=R|RW|APPEND: creat file first
        creat_file(path); 
        mip = path2inode(path);

        printf("MIP: [%d, %d]", mip->dev, mip->ino); // print mip's [dev ino]
    }
    if(!S_ISREG(mip->INODE.i_mode)){
        printf("MIP: [%d %d] not a regular file\n", mip->dev, mip->ino);
        return -1;
    }
    for(int i = 0; i < NFD; i++){
        if(running->fd[i] != NULL){
            if(running->fd[i]->inodeptr->ino == mip->ino && running->fd[i]->inodeptr->dev == mip->dev){
                if(running->fd[i]->mode != 0){
                    printf("ERROR: file is already open for READ\n");
                    return -1;
                }
            }
        }
    }
    // allocate a FREE OFT AND fill in values
    OFT *oftp = (OFT *)malloc(sizeof(OFT));
    oftp->inodeptr = mip; // point at the file's minode[]
    oftp->mode = mode;
    oftp->shareCount = 1;


    switch(mode){
        case 0: oftp->offset = 0;  // R: offset = 0
                break;
        case 1: my_truncate(mip);  // W: truncate file to 0 size
                oftp->offset = 0;
                break;
        case 2: oftp->offset = 0;  // RW: do NOT truncate file
                break;
        case 3: oftp->offset = mip->INODE.i_size; // APPEND mode
                break;
        default: printf("invalid mode\n");
                return (-1);
    }
    // find the SMALLEST index i in running PROC's fd[] such that fd[i] is NULL
    int i = 0;
    for(i; i < NFD; i++){
        if(running->fd[i] == NULL){
            break;
        }
    }
    printf("FD = %d\n", i);
    running->fd[i] = oftp; // let running->fd[i] point at the OFT entry

    if(mode == 0){ // if mode = READ, touch access time
        mip->INODE.i_atime = time(0L); // set atime
    }
    if(mode == 1 || mode == 2 || mode == 3){ // if mode = W|RW|APPEND
        mip->INODE.i_mtime = mip->INODE.i_atime = time(0L); // set atime and mtime
    }
    mip->modified = 1; // mark Minode[ ] MODIFIED

    if(running->fd[i]->inodeptr == mip){
        printf("OFT entry matches minode\n");
    }
    // return i as the file descriptor
    return i;
}

int close_file(char* pathname)
{
    MINODE *mip = path2inode(pathname);
    // find the file descriptor of the file 
    int fd = 0;
    for(fd; fd < NFD; fd++){
        if(running->fd[fd] != NULL){
            if(running->fd[fd]->inodeptr->ino == mip->ino && running->fd[fd]->inodeptr->dev == mip->dev){
                break; // found the file descriptor
            }
        }
    }
    // close the file
    my_close(fd);
    return 0;

}
int my_close(int fd) // close a file
{
    // verify fd is in range
    if(fd > NFD || fd < 0){
        printf("ERROR: fd is out of range\n");
        return -1;
    }
    // verify running->fd[fd] is pointing at a OFT entry
    if(running->fd[fd] == NULL){
        printf("ERROR: running->fd[fd] is not pointing at a OFT entry\n");
        return -1;
    }
    OFT *oftp = running->fd[fd];
    running->fd[fd] = 0; // set open file descriptor to 0
    oftp->shareCount--; // decrement shareCount by 1
    if(oftp->shareCount > 0){ // if shareCount > 0, return
        return 0;
    }
    // last user of this OFT entry ==> dispose of its minode
    MINODE *mip = oftp->inodeptr;
    iput(mip);
    return 0;
}




int my_truncate(MINODE *mip)
{
    // release mip->INODE's data blocks
    for(int i = 0; i < 12; i++){  // direct blocks
        if(mip->INODE.i_block[i] != 0){
            bdalloc(mip->dev, mip->INODE.i_block[i]);
        }
    }
    printf("deallocating direct blocks\n");
    if(mip->INODE.i_block[12]){ // indirect blocks
        char buf[BLKSIZE];
        get_block(dev,mip->INODE.i_block[12], buf);
        int *ip = (int *)buf; // ip points at buf[0]
        // while i < 256 and *ip is not 0
        for(int i = 0; i < 256 && *ip; i++){
            bdalloc(mip->dev, *ip);
            ip++;
        }
        bdalloc(mip->dev, mip->INODE.i_block[12]);
        printf("deallocated indirect blocks\n");
    }
    if(mip->INODE.i_block[13]){ // double indirect blocks
        char sbuf[BLKSIZE], dbuf[BLKSIZE];
        get_block(dev, mip->INODE.i_block[13], sbuf);
        int *sip = (int *)sbuf; // sip points at sbuf[0]
        // while i < 256 and *sip is not 0
        for(int i = 0; i < 256 && *sip; i++){
            get_block(dev, *sip, dbuf);
            int *dip = (int *)dbuf; // dip points at dbuf[0]
            // while i < 256 and *dip is not 0
            for(int i = 0; i < 256 && *dip; i++){
                bdalloc(mip->dev, *dip);
                dip++;
            }
            bdalloc(mip->dev, *sip);
            sip++;
        }
        bdalloc(mip->dev, mip->INODE.i_block[13]);
        printf("deallocated double indirect blocks\n");
    }
    mip->INODE.i_mtime = mip->INODE.i_atime = time(0L); // set atime and mtime
    mip->INODE.i_size = 0; // set file size to 0
    mip->modified = 1; // mark Minode[ ] MODIFIED
}

int pfd() // display all current opened files
{
    printf("fd\tmode\toffset\tINODE\n");
    printf("--\t----\t------\t-----\n");
    int num_opened_files = 0;
    for(int i = 0; i < NFD; i++){ // loop through all open file descriptors
        if(running->fd[i] == NULL){
            continue;
        }
        else{
            printf("%d\t %d\t  %d\t[%d, %d]\n", i, running->fd[i]->mode, running->fd[i]->offset, running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);
            num_opened_files++;
        }
    }
    if(num_opened_files == 0){ // if no files are currently opened
        printf("no files are currently opened\n");
    }
    return 0;
}