//Part 2 - Kernel Module
//Group 18
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple module featuring proc read");

#define ENTRY_NAME "timed"
#define PERMS 0644
#define PARENT NULL
static struct file_operations fops;

static char *message;
static char *message2;
static int read_p;
static int firstread;
static long prevsec;
static long prevnano;


int hello_proc_open(struct inode *sp_inode, struct file *sp_file) {
	printk(KERN_INFO "proc called open\n");

	
	read_p = 1;
	message = kmalloc(sizeof(char) * 70, __GFP_RECLAIM | __GFP_WRITE | __GFP_IO | __GFP_FS);
	message2 = kmalloc(sizeof(char) * 70, __GFP_RECLAIM | __GFP_WRITE | __GFP_IO | __GFP_FS);
	if (message == NULL) {
	printk(KERN_WARNING "hello_proc_open");
	return -ENOMEM;
	}
	strcpy(message, "current time: 0000000000.000000000\n                         \n");
	strcpy(message2,"current time: 0000000000.000000000\nelapsed time:\n                    \n");
	return 0;
}

ssize_t hello_proc_read(struct file *sp_file, char __user *buf, 
size_t size, loff_t *offset) {


	int len = strlen(message);
	int len2 = strlen(message2);
	struct timespec time;
	time=current_kernel_time();
	time_t seconds;
	long nanoseconds;
	seconds=time.tv_sec;
	nanoseconds=time.tv_nsec;
	read_p = !read_p;
	if (read_p) 
		return 0;

	printk(KERN_INFO "proc called read\n");
	printk(KERN_INFO "current time: %ld.%ld", seconds,nanoseconds);
	if(firstread)
	{
		//need to move the seconds and nanoseconds into message
		sprintf(message,"current time: %ld.%ld",seconds,nanoseconds);
		//strcpy(message,"YES");
		copy_to_user(buf,message,len);
		firstread=0;
		prevsec=(long)seconds;
		prevnano=nanoseconds;
	}
	else
	{
		long temp_s;
		long temp_n;
		temp_s=(long)seconds-prevsec;
		temp_n=nanoseconds-prevnano;
		if(temp_n<0)
		{	
			temp_s=temp_s-1;
			temp_n=temp_n+1000000000;
		}
		sprintf(message2,"current time: %ld.%ld elapsed time: %ld.%ld\n",seconds,nanoseconds,temp_s,temp_n);
		//sprintf(message2,"time: ");
		//copy_to_user(buf,message,len);
		prevsec=(long)seconds;
		prevnano=nanoseconds;
		copy_to_user(buf,message2,len2);
	}
	//copy_to_user(buf, message, len);
	return len;
}


int hello_proc_release(struct inode *sp_inode,
struct file *sp_file) {
	printk(KERN_INFO "proc called release\n");
	 kfree(message);
 	return 0;
}


static int hello_proc_init(void) {
	firstread=1;
	 printk(KERN_NOTICE "/proc/%s create\n", ENTRY_NAME);
	 fops.open = hello_proc_open;
	 fops.read = hello_proc_read;
	 fops.release = hello_proc_release;

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)) {

		 printk("ERROR! proc_create\n");
		 remove_proc_entry(ENTRY_NAME, NULL);
	 	return -ENOMEM;
	 }
	 return 0;
}

module_init(hello_proc_init);


static void hello_proc_exit(void) {
	remove_proc_entry(ENTRY_NAME,NULL);
printk(KERN_NOTICE "Removing /proc/%s.\n", ENTRY_NAME);
}

module_exit(hello_proc_exit);



