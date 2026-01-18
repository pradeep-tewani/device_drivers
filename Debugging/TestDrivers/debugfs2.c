#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/debugfs.h> /* this is for DebugFS libraries */ 
#include <linux/fs.h>   
#define len 200

u64 intvalue, hexvalue; 
struct dentry *dirret,*fileret,*u64int,*u64hex; 
char ker_buf[len]; 
int filevalue; 

/* read file operation */
static ssize_t myreader(struct file *fp, char __user *user_buffer, 
                                size_t count, loff_t *position) 
{ 
     return simple_read_from_buffer(user_buffer, count, position, ker_buf, len);
} 
 
/* write file operation */
static ssize_t mywriter(struct file *fp, const char __user *user_buffer, 
                                size_t count, loff_t *position) 
{ 
        if(count > len ) 
                return -EINVAL; 
  
        return simple_write_to_buffer(ker_buf, len, position, user_buffer, count); 
} 

//TODO 1: Register the file operations 
static const struct file_operations fops_debug = { 
        .read = myreader, 
        .write = mywriter, 
}; 
 
static int __init init_debug(void) 
{ 
    //TODO 2: Create a directory
    dirret = debugfs_create_dir("timmins", NULL); 
      
    //TODO 3: Create a file under above directory
    /* create a file in the above directory 
    This requires read and write file operations */
    debugfs_create_file("text", 0644, dirret, &filevalue, &fops_debug);
 
    //TODO 4: Create a file which takes int(64) value
    debugfs_create_u64("number", 0644, dirret, &intvalue); 

    //TODO 5: Create a file which takes hexadecimal value
    debugfs_create_x64("hexnum", 0644, dirret, &hexvalue ); 
 
    return (0); 
} 
 
static void __exit exit_debug(void) 
{ 
    /* removing the directory recursively which 
    in turn cleans all the file */
    debugfs_remove_recursive(dirret); 
} 

module_init(init_debug); 
module_exit(exit_debug);
MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("sample code for DebugFS functionality"); 
