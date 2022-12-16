
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include "message_slot.h"

int main(int argc, char *argv[]){
    int fd, succ;
    long channel_id;
    char *p;
    char* buffer;
    size_t i = 0;
    
    if (argc != 3) {
        perror("wrong number of args");
        exit(1);
    }

    channel_id = strtol(argv[2], &p, 10);
    if (channel_id <= 0)
    {
        perror("wrong channel_id");
        exit(1);
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0){
        perror("Can't open file");
        exit(1);
    }
    
    succ = ioctl(fd, MSG_SLOT_CHANNEL, channel_id);

    if (succ == -1){
        perror("ioctl FAILED");
        exit(1);
    };

    buffer = malloc(BUF_LEN);
    if (!buffer){
        perror("buffer allocation failed");
        exit(1);
    }

    for (i=0; buffer[i] != '\0'; ++i);

    succ = read(fd, buffer, BUF_LEN);

    if (succ == -1){
        perror("read operation FAILED");
        exit(1);
    }
    else{
        write(STDOUT_FILENO, buffer, succ);
    }

    free(buffer);
    close(fd);

    return 0;
}
