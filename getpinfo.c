#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/fs_struct.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
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

struct task_struct *call_task = NULL;
char *respbuf;

int file_value;
struct dentry *dir, *file;

/* This function uses kernel functions for access to pid for a process 
   */
void getProcessInfo(struct task_struct *call_task, char *respbuf, char *process_descriptor) {
  pid_t cur_pid = 0;
  //real parent's pid
  pid_t parent_pid = 0;
  //state of the process
  unsigned long state = 0;
  //flags of the process
  unsigned int flags = 0;
  //priority of the processc
  int prio = 0;
  //the command called by the process
  char comm[TASK_COMM_LEN];
  //the struct for current process;
  struct fs_struct *fs;
  //current working directory name
  unsigned char *relative_dirname = kmalloc(DNAME_INLINE_LEN_MIN, GFP_ATOMIC);
  //virtual memory struct for the process
  struct mm_struct *mm;
  struct vm_area_struct *mmap;
  //number of VMAs allocated to the process
  int map_count = 0;
  //total pages mapped 
  unsigned long total_vm = 0;
  //Shared pages (files)
  unsigned long shared_vm = 0;
  //VM_EXEC & ~VM_WRITE
  unsigned long exec_vm = 0;
  //VM_GROWSUP/DOWN
  unsigned long stack_vm = 0;
  //repsonse line
  char resp_line[MAX_LINE];
  struct pid *pid_p;
  {
    /* data */
  };

  pid_p = get_task_pid(call_task, PIDTYPE_PID);
  if (pid_p != NULL) {
    cur_pid = pid_vnr(pid_p);
    parent_pid = call_task->real_parent->pid;
    state = call_task->state;
    flags = call_task->flags;
    prio = call_task->prio;
    strcpy(comm, call_task->comm);
    strcat(comm, "\0");
    
    fs = call_task->fs;
    if(fs != NULL) {
      struct dentry *relative_dentry = fs->pwd.dentry;
      relative_dirname = relative_dentry->d_iname;
    }

    mm = call_task->mm;
    if(mm != NULL) {
      mmap = mm->mmap;
      while(mmap != NULL){
        
        sprintf(resp_line, " start linear address    %lu\n", mmap->vm_start);
        strcat(respbuf, resp_line);
        sprintf(resp_line, " start end address    %lu\n", mmap->vm_end);
        strcat(respbuf, resp_line);
        mmap = mmap->vm_next;
      }
    }
  }

  else cur_pid = 0;

  sprintf(resp_line, "     %s", process_descriptor);
  strcat(respbuf, resp_line);
  sprintf(resp_line, " PID %d\n", cur_pid);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     parent %d\n", parent_pid);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     state %lu\n", state);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     flags %08x\n", flags);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     priority %d\n", prio);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     command %s\n", comm);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     cwd %s\n", relative_dirname);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     number of VMAs allocated %d\n", map_count);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     total VM logical pages allocated %lu\n", total_vm);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     number of shared pages allocated %lu\n", shared_vm);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     number of exec pages allocated %lu\n", exec_vm);
  strcat(respbuf, resp_line);
  sprintf(resp_line, "     number of stack pages allocated %lu\n\n", stack_vm);
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
  struct task_struct *parent;
  struct list_head *sibling, *sibling_list;

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
  
  rcu_read_lock();
  getProcessInfo(call_task, respbuf, "Current");
  // parent = call_task->real_parent;
  // if(parent == NULL){
  //   strcpy(respbuf, "Failed: failed in the process of getting sibling info\n");
  // }
  // sibling_list = &(parent->children);
  // if(sibling_list == NULL){
  //   strcpy(respbuf, "Failed: failed in the process of getting sibling info\n");
  // }
  // else{
  //     list_for_each(sibling, sibling_list){
  //     struct task_struct *sibling_task;
  //     sibling_task = list_entry(sibling,struct task_struct,sibling);
  //     if(sibling_task != current){
  //       getProcessInfo(sibling_task, respbuf, "Sibling");
  //     }
  //   }
  // }
  rcu_read_unlock();
  
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
