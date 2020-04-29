#include <linux/linkage.h>
#include <linux/kernel.h>
#include <sys/time.h>
#include <linux/uaccess.h>

asmlinkage void sys_print(char *input)
{
	char kernel_info[60];
	copy_from_user(kernel_info,input,sizeof kernel_info);
	printk(KERN_INFO "%s\n" ,kernel_info);
} 
