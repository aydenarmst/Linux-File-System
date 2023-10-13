/**************** read.c file **************/
#include "../../include/level-2.h"
#include "../../include/level-1.h"

int read_file()
{
    int fd, nbytes;
    char buf[BLKSIZE];
    printf("Enter fd: ");
    scanf("%d", &fd);
    printf("Enter nbytes: ");
    scanf("%d", &nbytes);

    // verify fd is indeed opened for READ or RW
    if(running->fd[fd] == NULL){
        printf("ERROR: fd is not opened\n");
        return -1;
    }
    if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2){
        printf("ERROR: fd is not opened for READ or RW\n");
        return -1;
    }
    return my_read(fd, buf, nbytes);
}


int my_read(int fd, char *buf, int nbytes)
{
    
}