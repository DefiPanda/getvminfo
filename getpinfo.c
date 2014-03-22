#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/fs_struct.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/mm_types.h>

#include "getpinfo.h" /* used by both kernel module and user program */

/* The following two variables are global state shared between
 * the "call" and "return" functions.  They need to be protected
 * from re-entry caused by kernel preemption.
 */

/* The call_task variable is used to ensure that the result is
 * returned only to the process that made the call.  Only one
 * result can be pending for return at a time (any call entry 
 * while the variable is non-NULL is rejected).
 */

const struct vm_operations_struct *original_vm_ops_pointers[MAX_VMAS];
struct vm_area_struct *original_vmas[MAX_VMAS];
struct task_struct *call_task = NULL;
struct mm_struct *mm;
char *respbuf;
int vma_size;

int file_value;
struct dentry *dir, *file;

static void open_rt(struct vm_area_struct * vma) {
   int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
       if(original_vm_ops_pointers[i]->open){
         original_vm_ops_pointers[i]->open(vma);
         return;
       }
     }
   }
}

static void close_rt(struct vm_area_struct * vma) {
  int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
       if(original_vm_ops_pointers[i]->close){
         original_vm_ops_pointers[i]->close(vma);
         return;
       }
     }
   }
}

static int fault_rt(struct vm_area_struct *vma, struct vm_fault *vmf) {
   int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
       if(original_vm_ops_pointers[i]->fault){
         printk(KERN_DEBUG "The address of vma page os: %p\n", mm);
         return original_vm_ops_pointers[i]->fault(vma, vmf);
       }
     }
   }
   return -1;
}

static int page_mkwrite_rt(struct vm_area_struct *vma, struct vm_fault *vmf){
   int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
      if(original_vm_ops_pointers[i]->page_mkwrite){
       return original_vm_ops_pointers[i]->page_mkwrite(vma, vmf);
      }
     }
   }
   return -1;
}

static int access_rt(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write){
   int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
      if(original_vm_ops_pointers[i]->access){
       return original_vm_ops_pointers[i]->access(vma, addr, buf, len, write);
      }
     }
   }
  return -1;
}

#ifdef CONFIG_NUMA
static int set_policy_rt(struct vm_area_struct *vma, struct mempolicy *new){
   int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
      if(original_vm_ops_pointers[i]->set_policy){
        return original_vm_ops_pointers[i]->set_policy(vma, new);
      }
     }
   }
   return -1;
}

static struct mempolicy get_policy_rt(struct vm_area_struct *vma, unsigned long addr){
   int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
      if(original_vm_ops_pointers[i]->get_policy){
       return original_vm_ops_pointers[i]->get_policy(vma, addr);
      }
     }
   }
   return NULL;
}

static int migrate_rt(struct vm_area_struct *vma, const nodemask_t *from, const nodemask_t *to, unsigned long flags){
   int i;
   for(i = 0; i < vma_size; i++){
     if(original_vmas[i] == vma){
      if(original_vm_ops_pointers[i]->migrate){
       return original_vm_ops_pointers[i]->migrate(vma, from, to, flags);
      }
     }
   }
  return -1;
}
#endif

struct vm_operations_struct page_fault_info_class  = {
              .open = open_rt,
              .close = close_rt,
              .fault = fault_rt,
              .page_mkwrite = page_mkwrite_rt,
              .access = access_rt,   
           #ifdef CONFIG_NUMA
              .set_policy = set_policy_rt,
              .get_policy = get_policy_rt,
              .migrate = migrate_rt,
           #endif
          };

/* This function uses kernel functions for access to pid for a process 
   */
void getProcessInfo(struct task_struct *call_task, char *respbuf, int page_fault_flag) {
  pid_t cur_pid = 0;
  struct vm_area_struct *mmap;
  //repsonse line
  char resp_line[MAX_LINE];
  int i;
  struct pid *pid_p;
  {
    /* data */
  };

  pid_p = get_task_pid(call_task, PIDTYPE_PID);
  if (pid_p != NULL) {
    cur_pid = pid_vnr(pid_p);

    mm = call_task->mm;
    down_read(&(mm->mmap_sem));
    if(mm != NULL) {
      vma_size = 0;
      mmap = mm->mmap;
      while(mmap != NULL){
        mmap = mmap->vm_next;
        vma_size += 1;
      }
      //printk(KERN_DEBUG "The number of vma pages are: %i\n", vma_size);
      i = 0;
      if(page_fault_flag == 1){
        mmap = mm->mmap;
        while(mmap != NULL){
          if(mmap->vm_ops != NULL){
            original_vm_ops_pointers[i] = mmap->vm_ops;
            original_vmas[i] = mmap;
            mmap->vm_ops = &page_fault_info_class;
          }  
          mmap = mmap->vm_next;
          i++;
        }
      }
      // i = 0;
      // if(page_fault_flag == 1){
      //   mmap = mm->mmap;
      //   while(mmap != NULL){
      //     printk(KERN_DEBUG "vma: %p", original_vmas[i]);
      //     printk(KERN_DEBUG "op: %p", original_vm_ops_pointers[i]);
      //     // mmap->vm_ops = &page_fault_info_class;
      //     mmap = mmap->vm_next;
      //     i++;
      //   }
      // }


      mmap = mm->mmap;
      while(mmap != NULL){
        
        sprintf(resp_line, " start linear address    %lu\n", mmap->vm_start);
        strcat(respbuf, resp_line);
        sprintf(resp_line, " start end address    %lu\n", mmap->vm_end);
        strcat(respbuf, resp_line);
        sprintf(resp_line, " vm_flags %08lx\n", mmap->vm_flags);
        strcat(respbuf, resp_line);
        mmap = mmap->vm_next;
      }
    }
    up_read(&(mm->mmap_sem));
  }

  else cur_pid = 0;

  sprintf(resp_line, " PID %d\n", cur_pid);
  strcat(respbuf, resp_line);
}

/* This function emulates the handling of a system call by
 * accessing the call string from the user program, executing
 * the requested function and preparing a response.
 */

static ssize_t getpinfo_call(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
  int rc;
  int scan_count;
  int page_fault_flag;
  char callbuf[MAX_CALL];
  char parameter1[MAX_CALL];
  // struct task_struct *parent;
  // struct list_head *sibling, *sibling_list;

  /* the user's write() call should not include a count that exceeds
   * the size of the module's buffer for the call string.
   */

  if(count >= MAX_CALL)
     return -EINVAL;

  /* The preempt_disable() and preempt_enable() functions are used in the
   * kernel for preventing preemption.  They are used here to protect
   * global state.
   */
  
  preempt_disable();

  if (call_task != NULL) {
     preempt_enable(); 
     return -EAGAIN;
  }

  respbuf = kmalloc(MAX_RESP, GFP_ATOMIC);
  if (respbuf == NULL) {
     preempt_enable(); 
     return -ENOSPC;
  }
  strcpy(respbuf,""); /* initialize buffer with null string */

  /* current is global for the kernel and contains a pointer to the
   * running process
   */
  call_task = current;

  /* Use the kernel function to copy from user space to kernel space.
   */

  rc = copy_from_user(callbuf, buf, count);
  callbuf[MAX_CALL - 1] = '\0'; /* make sure it is a valid string */

  scan_count = sscanf(callbuf, "%s", parameter1); 

  if (scan_count != 1 || kstrtoint(parameter1, 10, &page_fault_flag) != 0) {
      strcpy(respbuf, "Failed: invalid operation\n");
      printk(KERN_DEBUG "getpinfo: call %s will return %s\n", callbuf, respbuf);
      preempt_enable();
      return count;  /* write() calls return the number of bytes written */
  }

  sprintf(respbuf, "Success:\n");
  
  getProcessInfo(call_task, respbuf, page_fault_flag);
  
  /* Here the response has been generated and is ready for the user
   * program to access it by a read() call.
   */
  printk(KERN_DEBUG "getpinfo: call %s will return %s", callbuf, respbuf);
  preempt_enable();
  
  *ppos = 0;  /* reset the offset to zero */
  return count;  /* write() calls return the number of bytes written */
}

/* This function emulates the return from a system call by returning
 * the response to the user.
 */

static ssize_t getpinfo_return(struct file *file, char __user *userbuf,
                                size_t count, loff_t *ppos)
{
  int rc; 

  preempt_disable();

  if (current != call_task) {
     preempt_enable();
     return 0;
  }

  rc = strlen(respbuf) + 1; /* length includes string termination */

  /* return at most the user specified length with a string 
   * termination as the last byte.  Use the kernel function to copy
   * from kernel space to user space.
   */

  if (count < rc) {
     respbuf[count - 1] = '\0';
     rc = copy_to_user(userbuf, respbuf, count);
  }
  else 
     rc = copy_to_user(userbuf, respbuf, rc);

  kfree(respbuf);

  respbuf = NULL;
  call_task = NULL;

  preempt_enable();

  *ppos = 0;  /* reset the offset to zero */
  return rc;  /* read() calls return the number of bytes read */
} 

static const struct file_operations my_fops = {
        .read = getpinfo_return,
        .write = getpinfo_call,
};

/* This function is called when the module is loaded into the kernel
 * with insmod.  It creates the directory and file in the debugfs
 * file system that will be used for communication between programs
 * in user space and the kernel module.
 */

static int __init getpinfo_module_init(void)
{

  /* create a directory to hold the file */

  dir = debugfs_create_dir(dir_name, NULL);
  if (dir == NULL) {
    printk(KERN_DEBUG "getpinfo: error creating %s directory\n", dir_name);
     return -ENODEV;
  }

  /* create the in-memory file used for communication;
   * make the permission read+write by "world"
   */


  file = debugfs_create_file(file_name, 0666, dir, &file_value, &my_fops);
  if (file == NULL) {
    printk(KERN_DEBUG "getpinfo: error creating %s file\n", file_name);
     return -ENODEV;
  }

  printk(KERN_DEBUG "getpinfo: created new debugfs directory and file\n");

  return 0;
}

/* This function is called when the module is removed from the kernel
 * with rmmod.  It cleans up by deleting the directory and file and
 * freeing any memory still allocated.
 */

static void __exit getpinfo_module_exit(void)
{
  debugfs_remove(file);
  debugfs_remove(dir);
  if (respbuf != NULL)
     kfree(respbuf);
}

/* Declarations required in building a module */

module_init(getpinfo_module_init);
module_exit(getpinfo_module_exit);
MODULE_LICENSE("GPL");
