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

  while (curr != NULL)
  {
    if (curr->channel_id == channel_num){
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
    return -1;
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
    if (tmp->message != NULL){
      kfree(tmp->message);
    }
    kfree(tmp);
  }

  if (channel->message != NULL){
    kfree(channel->message);
  }
  kfree(channel);
  return SUCCESS; 
}

static int device_open_flag = 0;

/*=========================================== DEVICE FUNCTIONS =================================================*/
static int device_open( struct inode* inode,
                        struct file*  file )
{
    
    struct channel* new_channel;
    int succ, minor;
    printk("Invoking device_open(%p)\n", file);

    if (device_open_flag == 1){
      return -EBUSY;
    }

    minor = iminor(file->f_inode);

    if (slots[minor] == NULL){
      new_channel = (struct channel*) kmalloc(sizeof(struct channel), GFP_KERNEL);
      if (!new_channel) {
        printk("ERROR: kmalloc FAILED\n");
        return -1;
        } 
      succ = create_new_channel(new_channel, -1);
      if (succ != 0) {
          printk("ERROR: couldn't create new channel\n");
          return -1; 
          }

      slots[minor] = new_channel;
    }

    /* printk("------CHANNEL LIST OPEN----------");
    channel = slots[minor];
    while(channel!=NULL){
      printk("channel_id = %ld\n", channel->channel_id);
      channel = channel->next_channel;
    }*/

    device_open_flag++;
    printk("### OPEN end successfully ###\n");
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  printk("Invoking device_release(%p,%p)\n", inode, file);

  // ready for our next caller
  --device_open_flag;
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
    // unsigned long channel_num;
    struct channel* channel;
    ssize_t i;
    int succ;

    printk("Invoking device_read(%p)\n", file);

    channel = (struct channel*) file->private_data;

    if (channel == NULL) return -EINVAL; /* ERROR: no channel has been set on fd */

    // printk("$ channel_id to read is %ld, in slot %d\n", channel->channel_id, iminor(file->f_inode));

    /* ERROR CASES */

    if ((channel->length_message == 0) || (channel->message == NULL)) return -EWOULDBLOCK; /* ERROR: no message exists on channel */
    if (length < channel->length_message) return -ENOSPC;                                 /* ERROR: provided buffer is too small */
    if (buffer == NULL) return -EINVAL;

    for( i = 0; i < channel->length_message; ++i ) {
        succ  = put_user(channel->message[i], &buffer[i]);
    }

    if (succ == -1){
      return succ;
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
    int get_succ;
    struct channel* channel;

    printk("Invoking device_write(%p,%ld)\n", file, length);

    channel = (struct channel*) file->private_data;
    if (channel == NULL) return -EINVAL;

    // printk("$ channel_id to write is %ld, in slot %d\n", channel->channel_id, iminor(file->f_inode));

    if ((length == 0) || (length > BUF_LEN)) return -EMSGSIZE;
    if (buffer == NULL) return -EINVAL;

    if (channel->message != NULL) kfree(channel->message);

    channel->message = (char *) kmalloc(BUF_LEN, GFP_KERNEL);
    if (channel->message == NULL) return -1;

    for( i = 0; i < length; ++i ) {
        get_succ = get_user(channel->message[i], &buffer[i]);
    }

    if (get_succ == -1){
      return get_succ;
    }

    channel->length_message = i;
    printk("MESSEGE= %s\n", channel->message);

    printk("### WRITE end successfully ###\n");

    return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  struct channel* channel;
  int add_succ;
  printk("Invoking device_ioctl(%p,%d,%ld)\n", file, ioctl_command_id, ioctl_param);

  if (MSG_SLOT_CHANNEL == ioctl_command_id)
  {
    if (ioctl_param == 0) return -EINVAL;

/* If the channel is not intialized, add channel to slot */
    channel = find_channel(file, ioctl_param);

    if (channel == NULL){
      printk("channel is NULL\n");
      channel = (struct channel*) kmalloc(sizeof(struct channel), GFP_KERNEL);
      if (!channel) {
        printk("ERROR: kmalloc FAILED\n");
        return -1;
        } 
      add_succ = add_channel(slots[iminor(file->f_inode)], channel, ioctl_param);
      if (add_succ != 0) {
        printk("ERROR: Failed openning channel");
        return -1; 
        }
    }

    /* Set file channel id to be the one given in arg*/
    file->private_data = (void *) channel; 
  
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
  .release        = device_release,
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
      printk("free channel list of slot %d", i);
      delete_channels_list(slots[i]);
    }
    printk(" ### FREE MEMORY end successfully ### ");

}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
