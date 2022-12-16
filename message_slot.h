#ifndef MS_H
#define MS_H

#include <linux/ioctl.h>



// Set the message of the device driver
// #define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long) ====?????????====
#define MAJOR_NUM 235

#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot_dev"
#define SUCCESS 0

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#endif

