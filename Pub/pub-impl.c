/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration de funciones de banno */
int bano_open(struct inode *inode, struct file *filp);
int bano_release(struct inode *inode, struct file *filp);
ssize_t bano_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t bano_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void bano_exit(void);
int bano_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations bano_fops = {
  read: bano_read,
  write: bano_write,
  open: bano_open,
  release: bano_release
};

/* Declaration of the init and exit functions */
module_init(bano_init);
module_exit(bano_exit);

/*** El driver para el bano *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int bano_major = 62;     /* Major number */

/* Buffer to store data */
#define MAX_SIZE 8192

static char *buffer_v;
static char *buffer_d;
static ssize_t curr_size;
static int damas;
static int varones;
static int pend_open_damas;
static int pend_open_varones;

/* El mutex y la condicion para syncread */
static KMutex mutex;
static KCondition cond;

int bano_init(void) {
  int b;

  /* Registering device */
  b = register_chrdev(bano_major, "bano", &bano_fops);
  if (b < 0) {
    printk("<1>Bano: cannot obtain major number %d\n", bano_major);
    return b;
  }

  damas = 0;
  varones = 0;
  pend_open_damas= 0;
  pend_open_varones = 0;
  curr_size= 0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating syncread_buffer */
  buffer_d = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (buffer_d == NULL) {
    bano_exit();
    return -ENOMEM;
  }
  memset(buffer_d, 0, MAX_SIZE);

  buffer_v = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (buffer_v == NULL) {
    bano_exit();
    return -ENOMEM;
  }
  memset(buffer_v, 0, MAX_SIZE);

  printk("<1>Inserting bano module varones y damas yei!\n");
  return 0;
}

void bano_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(bano_major, "damas");
  unregister_chrdev(bano_major, "varones");

  /* Freeing buffer syncread */
  /*if (syncread_buffer) {
    kfree(syncread_buffer);
  }*/

  printk("<1>Removing bano module\n");
}

int bano_open(struct inode *inode, struct file *filp) {
	return 0;
}

int bano_release(struct inode *inode, struct file *filp) {
	return 0;
}

ssize_t bano_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	ssize_t rc = -EFAULT;;
	return rc;
}

ssize_t bano_write( struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	ssize_t rc = -EFAULT;;
	return rc;
}


