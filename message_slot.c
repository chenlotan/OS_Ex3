/* I USED REC6 CODE SKELETON */

#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include<linux/slab.h>      /*for kmalloc*/
#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/


MODULE_LICENSE("GPL");

#include "message_slot.h"

/*===================================== CHANNEL AND SLOT STRUCTS AND FUNCTIONS ==============================================*/

struct channel
{
    unsigned long channel_id;
    char* message;
    size_t length_message;
    struct channel* next_channel;
};

/**** STRUCT FOR SAVING OPEN SLOTS ****/
static struct channel* slots[256]; 

int create_new_channel(struct channel* new_channel, int channel_num){
  new_channel->channel_id = channel_num;
  new_channel->message = NULL;
  new_channel->length_message = 0;
  new_channel->next_channel = NULL;

  return SUCCESS;
  }

struct channel* find_channel(struct file* file, unsigned long channel_num)
{
  struct channel* curr;
  curr = slots[iminor(file->f_inode)];
  printk("curr was iniitialized");

  if (curr == NULL){
    printk("curr == NULL \n");
  }

  while (curr != NULL)
  {
    if (curr->channel_id == channel_num){
      printk("channel_id = %ld\n", curr->channel_id);
      return curr;
    } 
    curr = curr->next_channel;
  }

  return NULL;
}

int add_channel(struct channel* first_channel, struct channel *new_channel, unsigned long channel_num){
  struct channel* curr;
  int succ;

  curr = first_channel;
  if (curr == NULL){
    printk("channel list wasnt initialized\n");
  }
  succ = create_new_channel(new_channel, channel_num);
  if (succ != 0) {
    printk("couldn't create new channel\n");
    return -1;}

  while (curr->next_channel != NULL)
  {
    curr = curr->next_channel;
  }

  curr->next_channel = new_channel;
  return SUCCESS;
}

int delete_channels_list(struct channel* channel){
  struct channel* tmp;
  if (channel == NULL) return SUCCESS;

  while (channel->next_channel != NULL){
    tmp = channel->next_channel;
    channel->next_channel = tmp->next_channel;
    kfree(tmp);
  }

  kfree(channel);
  return SUCCESS; 
}



/*=========================================== DEVICE FUNCTIONS =================================================*/
static int device_open( struct inode* inode,
                        struct file*  file )
{
    
    struct channel* new_channel, *channel;
    int succ, minor;
    printk("Invoking device_open(%p)\n", file);

    minor = iminor(file->f_inode);
    printk("minor = %d\n", minor);
    printk("privat_data = %p\n", file->private_data);

    if (slots[minor] == NULL){
      new_channel = (struct channel*) kmalloc(sizeof(struct channel), GFP_KERNEL);
      if (!new_channel) {
        printk("kmalloc FAILED\n");
        return -1;
        } 
      succ = create_new_channel(new_channel, -1);
      if (succ != 0) {
          printk("couldn't create new channel\n");
          return -1; 
          }
        printk("channel was created successfully\n");

      slots[minor] = new_channel;
    }

    printk("------CHANNEL LIST OPEN----------");
    channel = slots[minor];
    while(channel!=NULL){
      printk("channel_id = %ld\n", channel->channel_id);
      channel = channel->next_channel;
    }
    printk("### OPEN end successfully ###\n");
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    unsigned long channel_num;
    struct channel* channel;
    ssize_t i;

    printk("Invoking device_read(%p)\n", file);

    channel_num = (unsigned long) file->private_data;

    if (channel_num == 0) return -EINVAL; /* ERROR: no channel has been set on fd */

    printk("$ channel_id to read is %ld, in slot %d\n", channel_num, iminor(file->f_inode));
    channel = find_channel(file, channel_num);


    /* ERROR CASES */

    if (channel == NULL) return -EWOULDBLOCK;                                               /* ERROR: no message exists on channel (channel was not created yet) */
    if ((channel->length_message == 0) || (channel->message == NULL)) return -EWOULDBLOCK; /* ERROR: no message exists on channel */
    if (length < channel->length_message) return -ENOSPC;                                 /* ERROR: provided buffer is too small */

    for( i = 0; i < channel->length_message; ++i ) {
        put_user(channel->message[i], &buffer[i]);
    }

    printk("### READ end successfully ###\n");
    return i;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    ssize_t i;
    unsigned long channel_num;
    int add_succ;
    struct channel* channel;

    printk("Invoking device_write(%p,%ld)\n", file, length);

    channel_num = (unsigned long) file->private_data;
    printk("$ channel_id to write is %ld, in slot %d\n", channel_num, iminor(file->f_inode));

    if (channel_num == 0) return -EINVAL;

    channel = find_channel(file, channel_num);

/* If the channel is not intialized, add channel to slot */
    
    if (channel == NULL){
      printk("channel is NULL\n");
      channel = (struct channel*) kmalloc(sizeof(struct channel), GFP_KERNEL);
      if (!channel) {
        printk("kmalloc FAILED\n");
        return -1;
        } 
      add_succ = add_channel(slots[iminor(file->f_inode)], channel, channel_num);
      if (add_succ != 0) {
        printk("Failed openning channel on write");
        return -1; 
        }
    }

    if ((length == 0) || (length > BUF_LEN)) return -EMSGSIZE;

    if (channel->message != NULL) kfree(channel->message);

    channel->message = (char *) kmalloc(BUF_LEN, GFP_KERNEL);
    if (channel->message == NULL) return -1;

    for( i = 0; i < length; ++i ) {
        get_user(channel->message[i], &buffer[i]);
    }

    channel->length_message = i;
    printk("MESSEGE= %s\n", channel->message);

    channel = slots[iminor(file->f_inode)];
    while(channel!=NULL){
      printk("channel_id = %ld\n", channel->channel_id);
      channel = channel->next_channel;
    }
    printk("### WRITE end successfully ###\n");

    return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
   printk("Invoking device_ioctl(%p,%d,%ld)\n", file, ioctl_command_id, ioctl_param);

  if (MSG_SLOT_CHANNEL == ioctl_command_id)
  {
    printk("MSG_SLOT_CHANNEL is the command\n");
    if (ioctl_param == 0) return -EINVAL;

    /* Set file channel id to be the one given in arg*/
    file->private_data = (void *) ioctl_param; 
    printk("file->private is now %p\n", file->private_data);
    
    return SUCCESS;
  }

  return -EINVAL;
}


/*=========================================== DEVICE SETUP ===================================================*/

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  // .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;

  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 ) {
    printk( KERN_ALERT "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
  }

  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    int i;
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);

    for (i=0; i<256; i++){
        delete_channels_list(slots[i]);
    }

}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
