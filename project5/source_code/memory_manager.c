#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched/signal.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hunter bowman");
MODULE_DESCRIPTION("project 5: Memory Manager kernel module.");

// pid: int, pid of a process (a non-negative number)
static unsigned int pid = 0;
module_param(pid, unsigned int, 0);
MODULE_PARM_DESC(pid, "ID of a process")

// addr: unsigned long long, a virtual address of the process (a non-negative number)
static unsigned long long int addr = 0;
module_param(addr, unsigned long long int, 0);
MODULE_PARM_DESC(addr, "The virtual address of a process")

// Module Init
static int __init memory_manager_init(void) {
    return 0;
}

// Module Exit
static void __exit memory_manager_exit(void) {

}

module_init(memory_manager_init);
module_exit(memory_manager_exit);