
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "message_slot.h"

int main(int argc, char *argv[]){
    int fd, succ; 
    char *p;
    int channel_id;
    size_t i = 0;
    
    if (argc != 4) {
        perror("wrong number of args");
        exit(1);
    }

    channel_id = strtol(argv[2], &p, 10);

    i = strlen(argv[3]);

    fd = open(argv[1], O_RDWR);
    if (fd < 0){
        printf("fd = %d\n", fd);
        perror("Can't open file");
        exit(1);
    }

    succ = ioctl(fd, MSG_SLOT_CHANNEL, channel_id);

    if (succ == -1){
        perror("ioctl Failed");
        exit(1);
    };

    succ = write(fd, argv[3], i);

    if (succ == -1){
        perror("write Failed");
        exit(1);
    };

    close(fd);


    return 0;
}

