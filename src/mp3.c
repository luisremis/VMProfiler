#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h> /* Necessary because we use the proc fs */
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>
#include <linux/mm.h>

#include "mp3_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_30");
MODULE_DESCRIPTION("CS-423 MP3");

#define DEBUG
#define DEBUG_2

#define INCOMMING_MAX_SIZE  (sizeof(struct info_send))
#define BUFFER_SIZE (128 * 4 * 1024)

#define DEVICENAME "mp_device"

/*new code*/
struct cdev *mp_dev; /*this is the name of my char driver that i will be registering*/
int major_number; /* will store the major number extracted by dev_t*/
dev_t dev_num; /*will hold the major number that the kernel gives*/

long initial_jiffies;

/* Timer data structures */
static struct timer_list timer;

/* Lock data structure */

DEFINE_SPINLOCK( mr_lock );

/* proc filesystem and linked list data structures */

LIST_HEAD(mp_linked_list) ;

struct mp_task_struct {
     unsigned long pid;
     unsigned long long process_utilization; 
     unsigned long long major_fault_count; 
     unsigned long long minor_fault_count; 
     struct task_struct* kernel_pcb;
     //struct timer_list wakeup_timer;
     struct list_head mp_list ;
} ;

struct data_stats{

    unsigned long jiffies_mark;
    unsigned long total_minor_fault_count;
    unsigned long total_major_fault_count;
    unsigned long cpu_utilization;
};

struct data_stats* buffer;
unsigned long buffer_index = 0;

static struct workqueue_struct *my_wq;

struct mp_work{
  struct work_struct my_work;
} ;

struct mp_work *work;
int init_workq = 0;

static void my_wq_function( struct work_struct *work)
{
    int ret;
    struct list_head *pos, *q;
    struct mp_task_struct *tmp;
    long minor_fault_count;
    long major_fault_count;
    long utime;
    long stime;

    struct data_stats* data_ptr;
    //Update index
    data_ptr= buffer + (buffer_index++);

    if (buffer_index == (BUFFER_SIZE/sizeof(struct data_stats)) - 2 )
    {
      printk( "WARNING! BUFFER FULL \n");
    }
    if (buffer_index  > (BUFFER_SIZE/sizeof(struct data_stats)) - 2 )
    {
      printk( "ERROR - BUFFER FULL!!! \n");
      return;
    }

    data_ptr->jiffies_mark = jiffies;
    data_ptr->total_minor_fault_count = 0;
    data_ptr->total_major_fault_count = 0;
    data_ptr->cpu_utilization = 0;
    
    (data_ptr + 1)->jiffies_mark = -1; // for the last element
  
    #ifdef DEBUG_3
    printk( "Running Bottom-Half work \n");
    #endif

    // should put a lock here, because the registration can cause inconsistent state.
    spin_lock(&mr_lock);
    list_for_each_safe(pos, q, & mp_linked_list){
      tmp= list_entry(pos, struct mp_task_struct, mp_list);
      ret = get_cpu_use(tmp->pid, &minor_fault_count, &major_fault_count, &utime, &stime);
      if ( ret ){
        printk( "Process %ld does not exist anymore, will be removed\n", tmp->pid);
        list_del(pos);
        kfree(tmp);
        buffer_index--;
      }
      else{

        //Save state in each process
        tmp->minor_fault_count   += minor_fault_count;
        tmp->major_fault_count   += major_fault_count;
        utime = cputime_to_jiffies(utime);
        stime = cputime_to_jiffies(stime);
        tmp->process_utilization = ((utime + stime) * 10000) / (jiffies - initial_jiffies);

        //Update stats on the buffer
        data_ptr->total_minor_fault_count += tmp->minor_fault_count;
        data_ptr->total_major_fault_count += tmp->major_fault_count;
        data_ptr->cpu_utilization         += tmp->process_utilization;
      }
    }

    spin_unlock(&mr_lock);

    #ifdef DEBUG_3
    printk( "%ld %ld %ld %ld \n", data_ptr->jiffies_mark, data_ptr->total_minor_fault_count,
                                   data_ptr->total_major_fault_count,  data_ptr->cpu_utilization  );
    #endif

    return;
}

int create_work_queue(void)
{
  init_workq = 1;
  my_wq = create_workqueue("my_queue");
  if (my_wq) {

    /* Queue some work (item 1) */
    work = (struct mp_work *)kmalloc(sizeof(struct mp_work), GFP_KERNEL);
    if (work) 
    {
       INIT_WORK( (struct work_struct *)work, my_wq_function );
    }
  }
  else{
      printk( "ERROR Creating work queue \n");
      return -1;
  }

  #ifdef DEBUG_2
  printk( "Work Queue created\n");
  #endif

  return 0;
}

int delete_work_queue(void)
{
  kfree( (void *)work );   
  flush_workqueue( my_wq );
  destroy_workqueue( my_wq );
  work  = NULL;
  my_wq = NULL;
  init_workq = 0;

  return 0;
}

void timer_callback( unsigned long data )
{
    // We only trigger the work when the list is not empty, 
    // and if there is no new element, the timer is not re-shoot.
    if ( 0 == list_empty( &mp_linked_list) )
    {
      queue_work( my_wq, (struct work_struct *)work );
      
      //re-shoot the timer
      //This can be here or at the buttom half. 
      //We put it here to avoid jitter in the 20 calls per second.
      mod_timer( &timer, jiffies + msecs_to_jiffies(50) );
    }
    else{
      #ifdef DEBUG_2
      printk("Timer called with no element in list\n");
      #endif
    }
}


/*inode reffers to the actual file on disk*/
static int device_open(struct inode *inode, struct file *f) {

    #ifdef DEBUG_2
    printk("Device : device opened succesfully\n");
    #endif
    return 0;
}

static int device_close(struct inode *inode, struct file *filp) {
    #ifdef DEBUG_2
    printk("Device : device has been closed\n");
    #endif
    return 0;
}

// mmap function for character device
static int device_mmap(struct file *f, struct vm_area_struct *vma)
{
  int ret;
  unsigned long pfn, size;
  unsigned long start = vma->vm_start;
  unsigned long length = vma->vm_end - start;
  void *ptr = (void*) buffer;

  if (vma->vm_pgoff > 0 || length > BUFFER_SIZE)
    return -EIO;
  while (length > 0) {
    pfn = vmalloc_to_pfn(ptr);
    size = length < PAGE_SIZE ? length : PAGE_SIZE;
    ret = remap_pfn_range(vma, start, pfn, size, PAGE_SHARED);
    if (ret < 0)
      return ret;
    start += PAGE_SIZE;
    ptr += PAGE_SIZE;
    length -= PAGE_SIZE;
  }
  return 0;
}

struct file_operations fops = { /* these are the file operations provided by our driver */
    .owner = THIS_MODULE, /*prevents unloading when operations are in use*/
    .open = device_open,  /*to open the device*/
    .mmap = device_mmap,
    .release = device_close /*to close the device*/
    
};

int registration(unsigned int pid){

  struct mp_task_struct* new_process;
  struct task_struct* new_task;
  
  new_task = find_task_by_pid(pid);

  if ( new_task == NULL ){
    printk( "Process received %d does not exist \n", pid);
    return -1;
  }
  
  new_process = kmalloc(sizeof(struct mp_task_struct), GFP_KERNEL); 
  new_process->pid = pid;
  new_process->kernel_pcb = new_task;
  new_process->major_fault_count = 0;
  new_process->minor_fault_count = 0;
  new_process->process_utilization = 0;

 
  if ( init_workq == 0 )
  {
    create_work_queue();
    // Configure timer when we have at least one element.
    mod_timer( &timer, jiffies + msecs_to_jiffies(50) );
    
    #ifdef DEBUG_2
    printk( "Adding first element (%d) \n", pid);
    #endif
  }

  spin_lock(&mr_lock);
  list_add ( &new_process->mp_list , &mp_linked_list ) ;
  spin_unlock(&mr_lock);

  return 0;
}

int deregistration(unsigned int pid){

  struct list_head *pos, *q;
  struct mp_task_struct *tmp;  
  char flag = 0x0;

  spin_lock(&mr_lock);
  list_for_each_safe(pos, q, & mp_linked_list){
    tmp= list_entry(pos, struct mp_task_struct, mp_list);

    if (tmp->pid == pid){

      list_del(pos);
      kfree(tmp);
      flag = 0x1;
    }
    
  }

  spin_unlock(&mr_lock);
  
  if ( flag == 0x0 ){
    printk( "Process %d does not exist - NO deregistration done! \n", pid);
    return -1;
  }

  if ( 0 != list_empty( &mp_linked_list) )
  {
    delete_work_queue();
    buffer_index = 0;
    initial_jiffies = jiffies;
  }

 return 0;
}

int procfile_write(struct file *file, const char *buffer, 
                      unsigned long count, void *data)
{
    int buffer_size = 0;
    struct info_send receive;

    /* get buffer size */
    buffer_size = count;
    if (buffer_size > INCOMMING_MAX_SIZE ) {
      buffer_size = INCOMMING_MAX_SIZE;
    }

    /* write data to the buffer */
    if ( copy_from_user( (void *)&(receive), buffer, buffer_size) ) {
      return -EFAULT;
    }

    switch (receive.opcode){

        case 'R': 
                #ifdef DEBUG
                printk( "Registration (%d) \n", receive.pid);
                #endif
                if ( registration(receive.pid) ){
                  printk( "Error registering %d \n", receive.pid);
                  return -1;
                };
                break;
        case 'D':
                #ifdef DEBUG
                printk( "Deregister (%d) \n", receive.pid);
                #endif
                if ( deregistration(receive.pid) ){
                  printk( "Error deregistering %d \n", receive.pid);
                  return -1;
                };
                break;
        default:
                printk ("ERROR OPCODE!!: %c \n", receive.opcode);
                break;
    }

    return buffer_size;
}

static int status_proc_show(struct seq_file *m, void *v) {

  struct mp_task_struct* tmp; 

  #ifdef DEBUG_2
  printk("calling status_proc_show \n");
  #endif

  // should put a lock here, because the registration can cause inconsisten state.
  spin_lock(&mr_lock);

  //We used seq_file to print because it is a tool that allow us no to
  //worry about paging. 
  list_for_each_entry ( tmp , & mp_linked_list, mp_list ) 
  { 
    seq_printf(m, "%ld \n" , tmp->pid  ); 
  }

  spin_unlock(&mr_lock);

  return 0;
}

static int status_proc_open(struct inode *inode, struct  file *file) {

   #ifdef DEBUG_2
   printk("calling status_proc_open \n" );
   #endif
   return single_open(file, status_proc_show, NULL);
}

static const struct file_operations status_proc_fops = {
  .owner = THIS_MODULE,
  .open = status_proc_open,
  .read = seq_read,         // This simplifies the reading process
  .write = procfile_write,  // Write our own writing method
  .llseek = seq_lseek,
  .release = single_release,
};

// mp_init - Called when module is loaded
int __init mp_init(void)
{
    int ret;
    int i;
    struct proc_dir_entry *proc_parent; 
    struct proc_dir_entry *proc_write_entry;

    #ifdef DEBUG_2
    printk(KERN_ALERT "MP3 MODULE LOADING\n");
    #endif

    buffer = vmalloc(BUFFER_SIZE);
    if (!buffer ) 
    {
        return -1;
    }

    /* Creation proc filesytems dir and file */
    proc_parent = proc_mkdir("mp3",NULL);
    proc_write_entry =proc_create("status", 0666, proc_parent, &status_proc_fops);

    work = NULL;
    my_wq = NULL;

    /* Timer inicialization */
    setup_timer( &timer, timer_callback, 0 );

    /*Character Device */
        /* we will get the major number dynamically this is recommended please read ldd3*/
    ret = alloc_chrdev_region(&dev_num,0,1,DEVICENAME);
    if(ret < 0) {
        printk("Device : failed to allocate major number\n");
        return ret;
    }

    major_number = MAJOR(dev_num);
    #ifdef DEBUG
    printk("Device : major number of our device is %d\n",major_number);
    printk("Device : to use mknod /dev/%s c %d 0\n",DEVICENAME,major_number);
    #endif

    mp_dev = cdev_alloc(); /*create, allocate and initialize our cdev structure*/
    mp_dev->ops = &fops;   /*fops stand for our file operations*/

    /*we have created and initialized our cdev structure now we need to add it to the kernel*/
    ret = cdev_add(mp_dev,dev_num,1);
    if(ret < 0) {
        printk("Device : device adding to the kerknel failed\n");
        return ret;
    }

    initial_jiffies = jiffies;
 
    #ifdef DEBUG
    printk(KERN_ALERT "MP3 MODULE LOADED\n");
    #endif

    return 0;   
}

// mp1_exit - Called when module is unloaded
void __exit mp_exit(void)
{
    
    struct list_head *pos, *q;
    struct mp_task_struct *tmp;

    #ifdef DEBUG_2
    printk(KERN_ALERT "MP3 MODULE UNLOADING\n");
    #endif
    // Insert your code here ...

    if (my_wq != NULL)
    {
      kfree( (void *)work );   
      flush_workqueue( my_wq );
      destroy_workqueue( my_wq );
    }

    /* Deleting timer */ 
    if ( del_timer( &timer) ) printk("Deleting an active timer\n");

    /* Deleting list */

    /* Since we will be removing items
    * off the list using list_del() we need to use a safer version of the list_for_each() 
    * macro aptly named list_for_each_safe(). Note that you MUST use this macro if the loop 
    * involves deletions of items (or moving items from one list to another).
    */
    list_for_each_safe(pos, q, & mp_linked_list)
    {
     tmp= list_entry(pos, struct mp_task_struct, mp_list);

     #ifdef DEBUG_2
     printk("freeing item pid= %ld \n", tmp->pid);
     #endif

     list_del(pos);
     kfree(tmp);
    }

    vfree(buffer);

    cdev_del(mp_dev);
    unregister_chrdev_region(dev_num,1);

    /* Removing entrys in proc filesystem */
    
    remove_proc_entry("mp3/status",NULL);
    remove_proc_entry("mp3",NULL);

    #ifdef DEBUG
    printk(KERN_ALERT "MP3 MODULE UNLOADED\n");
    #endif
}

// Register init and exit funtions
module_init(mp_init);
module_exit(mp_exit);
