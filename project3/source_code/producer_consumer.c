#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

static int prod = 0;
module_param(prod, int, 0);
MODULE_PARM_DESC(prod, "Number of producer threads");

static int cons = 0;
module_param(cons, int, 0);
MODULE_PARM_DESC(cons, "Number of consumer threads");

static int size = 0;
module_param(size, int, 0);
MODULE_PARM_DESC(size, "Initial value for the counting semaphore");

static struct semaphore empty;
static struct semaphore full;

static struct task_struct **producer_threads;
static struct task_struct **consumer_threads;

// producer thread
static int producer_thread_fn(void *data) {
    int id = (int)(long)data;

    while (!kthread_should_stop()) {
        //printk(KERN_DEBUG "Producer-%d attempting to acquire 'empty' semaphore\n", id);
        if (down_interruptible(&empty) == 0) {
            printk(KERN_INFO "An item has been produced by Producer-%d\n", id);
            up(&full);
            //printk(KERN_DEBUG "Producer-%d released 'full' semaphore\n", id);
        } else {
            break;
        }
    }
    return 0;
}

// consumer thread
static int consumer_thread_fn(void *data) {
    int id = (int)(long)data;

    while (!kthread_should_stop()) {
        //printk(KERN_DEBUG "Consumer-%d attempting to acquire 'full' semaphore\n", id);
        if (down_interruptible(&full) == 0) {
            // Critical section: consuming an item
            printk(KERN_INFO "An item has been consumed by Consumer-%d\n", id);
            up(&empty);
            //printk(KERN_DEBUG "Consumer-%d released 'empty' semaphore\n", id);
        } else {
            break;
        }
    }
    return 0;
}

// modlue initialization
static int __init producer_consumer_init(void) {
    int i;

    // init semaphores
    sema_init(&empty, size);
    sema_init(&full, 0);
    printk(KERN_INFO "Semaphores initialized: empty=%d, full=%d\n", size, 0);

    // memory for thread arrays
    producer_threads = kmalloc_array(prod, sizeof(struct task_struct *), GFP_KERNEL);
    consumer_threads = kmalloc_array(cons, sizeof(struct task_struct *), GFP_KERNEL);

    if ((!producer_threads && prod > 0) || (!consumer_threads && cons > 0)) {
        printk(KERN_ERR "Failed to allocate memory for thread arrays\n");
        kfree(producer_threads);
        kfree(consumer_threads);
        return -ENOMEM;
    }

    // create producer threads
    for (i = 0; i < prod; i++) {
        producer_threads[i] = kthread_run(producer_thread_fn, (void *)(long)(i + 1), "Producer-%d", i + 1);
        if (IS_ERR(producer_threads[i])) {
            printk(KERN_ERR "Failed to create Producer-%d\n", i + 1);
            producer_threads[i] = NULL;
        }
    }

    // create consumer threads
    for (i = 0; i < cons; i++) {
        consumer_threads[i] = kthread_run(consumer_thread_fn, (void *)(long)(i + 1), "Consumer-%d", i + 1);
        if (IS_ERR(consumer_threads[i])) {
            printk(KERN_ERR "Failed to create Consumer-%d\n", i + 1);
            consumer_threads[i] = NULL;
        }
    }

    printk(KERN_INFO "Producer-consumer module loaded\n");
    return 0;
}

// module exit function
static void __exit producer_consumer_exit(void) {
    int i;

    // stop producer threads
    for (i = 0; i < prod; i++) {
        if (producer_threads[i]) {
            kthread_stop(producer_threads[i]);
        }
    }

    // stop consumer threads
    for (i = 0; i < cons; i++) {
        if (consumer_threads[i]) {
            kthread_stop(consumer_threads[i]);
        }
    }

    // free allocated memory
    kfree(producer_threads);
    kfree(consumer_threads);

    printk(KERN_INFO "Producer-consumer module unloaded\n");
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Producer-Consumer Kernel Module with Semaphores");
