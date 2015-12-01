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

static char *buffer_d;
static char *buffer_v;
static ssize_t curr_size_d;
static ssize_t curr_size_v;
static int damas;
static int varones;

static int damas_pos;
static int varones_pos;

// TODO el ejemplo falla en (14) se llega a damas negativo. Me tinca que algo incorrecto esta pasando en epilog dado que se debe a una interrupcion antes de tiempo
// Se deberia actualziar el valor de dmas o varones en epilog?

/* El mutex y la condicion para escribir */
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
  curr_size_d = 0;
  curr_size_v = 0;
  damas_pos = 0;
  varones_pos = 0;
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

  printk("<1>Inserting bano module varones y damas\n");
  return 0;
}

void bano_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(bano_major, "damas");
  unregister_chrdev(bano_major, "varones");

  /* Freeing buffers */
  if (buffer_d) {
    kfree(buffer_d);
  }
  if (buffer_v) {
    kfree(buffer_v);
  }

  printk("<1>Removing bano module\n");
}

int bano_open(struct inode *inode, struct file *filp) {
	int rc= 0;
  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    int dv = iminor(filp->f_inode);
    int rc;
    if (dv == 0) {
      // Esta entrando una dama
      printk("<1>open request for write dama\n");
      while (varones > 0) {
        printk("no cacho 1");
        if (c_wait(&cond, &mutex)) {
          c_broadcast(&cond);
          rc= -EINTR;
          if (damas == 0) {
            damas_pos = 0;
          }
          damas++;
          goto epilog;
        }
      }
      if (damas == 0) {
        damas_pos = 0;
      }
      damas++;
      curr_size_d = 0;
      printk("no cacho 2");
      c_broadcast(&cond);
      printk("<1>open for write successful damas: %d \n", damas);
    } else {
      // Esta entrando un varon
      printk("<1>open request for write varon\n");
      while (damas > 0) {
        if (c_wait(&cond, &mutex)) {
          c_broadcast(&cond);
          rc= -EINTR;
          if (varones == 0) {
            varones_pos = 0;
          }
          varones++;
          goto epilog;
        }
      }
      if (varones == 0) {
        varones_pos = 0;
      }
      varones++;
      curr_size_v= 0;
      c_broadcast(&cond);
      printk("<1>open for write successful varones: %d\n", varones);
    }

  }
  else if (filp->f_mode & FMODE_READ) {
    printk("<1>open for read\n");
  }

epilog:
  m_unlock(&mutex);
  return rc;
}

int bano_release(struct inode *inode, struct file *filp) {
	 m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    int dv = iminor(filp->f_inode);
    if (dv == 0) {
      // Esta saliendo una dama
      printk("<1>close for write successful dama\n");
      damas--;
    } else {
      // Esta saliendo un varon
      printk("<1>close for write successful varon\n");
      varones--;
    }
    c_broadcast(&cond);
  }
  else if (filp->f_mode & FMODE_READ) {
    printk("<1>close for read \n");
  }

  m_unlock(&mutex);
  return 0;
}

ssize_t bano_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;
  int dv;
  m_lock(&mutex);


  dv = iminor(filp->f_inode);
  if (dv == 0) {
    // Dama
    while (curr_size_d <= *f_pos && damas > 0) {
      printk("What? \n");
      /* si el lector esta en el final del archivo pero hay un proceso
       * escribiendo todavia en el archivo, el lector espera.
       */
      if (c_wait(&cond, &mutex)) {
        printk("<1>read interrupted\n");
        rc= -EINTR;
        goto epilog;
      }
    }
    printk("Curresize: %d\n", (int) curr_size_d); 
    printk("Damas: %d\n", damas);

    if (count > curr_size_d-*f_pos) {
      count= curr_size_d-*f_pos;
    }

    printk("<1>read %d bytes at (damas) %d\n", (int)count, (int)*f_pos);

    /* Transfiriendo datos hacia el espacio del usuario */
    if (copy_to_user(buf, buffer_d+*f_pos, count)!=0) {
      /* el valor de buf es una direccion invalida */
      rc= -EFAULT;
      goto epilog;
    }

  } else {
    // Varon
    while (curr_size_v <= *f_pos && varones > 0) {
      printk("What? \n");
      /* si el lector esta en el final del archivo pero hay un proceso
       * escribiendo todavia en el archivo, el lector espera.
       */
      if (c_wait(&cond, &mutex)) {
        printk("<1>read interrupted\n");
        rc= -EINTR;
        goto epilog;
      }
    }
    printk("Curresize: %d\n", (int) curr_size_v); 
    printk("Varones: %d\n", varones);

    if (count > curr_size_v-*f_pos) {
      count= curr_size_v-*f_pos;
    }

    printk("<1>read %d bytes at (varones) %d\n", (int)count, (int)*f_pos);

    /* Transfiriendo datos hacia el espacio del usuario */
    if (copy_to_user(buf, buffer_v+*f_pos, count)!=0) {
      /* el valor de buf es una direccion invalida */
      rc= -EFAULT;
      goto epilog;
    }
  }


  *f_pos+= count;
  rc= count;

epilog:
  m_unlock(&mutex);
  return rc;
}

ssize_t bano_write( struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
  int rc;
  int copyFromUserResult;
  int dv;

  loff_t last;

  m_lock(&mutex);
  dv = iminor(filp->f_inode);
  if (dv == 0) {
    last = damas_pos + count;
    if (last > MAX_SIZE) {
      count -= last-MAX_SIZE;
    }


    printk("<1>write %d bytes at (damas) %d\n", (int)count, (int)*f_pos);
    copyFromUserResult = copy_from_user(buffer_d+damas_pos, buf, count);
    /* Transfiriendo datos desde el espacio del usuario */
    if (copyFromUserResult!=0) {
      /* el valor de buf es una direccion invalida */
      rc= -EFAULT;
      goto epilog;
    }

    damas_pos += count;
    curr_size_d= damas_pos;
  } else {
    last = varones_pos + count;
    if (last > MAX_SIZE) {
      count -= last-MAX_SIZE;
    }
    printk("<1>write %d bytes at (varones) %d\n", (int)count, (int)*f_pos);
    copyFromUserResult = copy_from_user(buffer_v+varones_pos, buf, count);
    /* Transfiriendo datos desde el espacio del usuario */
    if (copyFromUserResult!=0) {
      /* el valor de buf es una direccion invalida */
      rc= -EFAULT;
      goto epilog;
    }

    varones_pos += count;
    curr_size_v = varones_pos;
  }

  rc= count;
  c_broadcast(&cond);

epilog:
  m_unlock(&mutex);
  return rc;
}


