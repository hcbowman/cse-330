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
MODULE_DESCRIPTION("project 4: Zombie Killer kernel module.");

static int prod = 1;
module_param(prod, int, 0);
MODULE_PARM_DESC(prod, "Number of producer threads");

static int cons = 0;
module_param(cons, int, 0);
MODULE_PARM_DESC(cons, "Number of consumer threads");

static int size = 0;
module_param(size, int, 0);
MODULE_PARM_DESC(size, "Size of the shared buffer");

static int uid = 0;
module_param(uid, int, 0);
MODULE_PARM_DESC(uid, "UID of the test user");

static struct semaphore empty;
static struct semaphore full;
// static struct semaphore mutex;
static spinlock_t buffer_lock;  // test

static struct task_struct **producer_threads;
static struct task_struct **consumer_threads;

static struct task_struct **buffer;
static int buffer_size;
static int in = 0;
static int out = 0;

//#define EXIT_ZOMBIE 0x00000020

// producer thread
static int producer_thread_fn(void *data) {
    int id = (int)(long)data;
    struct task_struct *task;

    while (!kthread_should_stop()) {
		// iterate over all processes
        for_each_process(task) {
            // check if the process is a zombie and belongs to the specified UID
            if ((task->exit_state & EXIT_ZOMBIE) && (task->cred->uid.val == uid)) {
                // wait for an empty slot in the buffer
                if (down_interruptible(&empty) != 0) {
                    return -ERESTARTSYS;
                }

                // Critical Section: Add task to buffer
                spin_lock(&buffer_lock);
                buffer[in] = task;
                in = (in + 1) % buffer_size;
                spin_unlock(&buffer_lock);

                // signal that there's a new full slot
                up(&full);

                // log the produced zombie
                printk(KERN_INFO "[Producer-%d] has produced a zombie process with pid %d and parent pid %d\n", id, task->pid, task->parent->pid);
            }
        }

        // to prevent the producer from using too much CPU time
        msleep(250);
    }
    return 0;
}

// consumer thread
static int consumer_thread_fn(void *data) {
    int id = (int)(long)data;
    struct task_struct *task;

    while (!kthread_should_stop()) {
		// wait for a full slot in the buffer
        if (down_interruptible(&full) != 0) {
            return -ERESTARTSYS;
        }

        // Critical section: Remove task from buffer
        spin_lock(&buffer_lock);
        task = buffer[out];
        out = (out + 1) % buffer_size;
        spin_unlock(&buffer_lock);

        // signal that there's a new empty slot
        up(&empty);

        // kill the parent process of the zombie
        if (task && task->parent) {
            kill_pid(task->parent->thread_pid, SIGKILL, 0);

            // log the consumed zombie
            printk(KERN_INFO "[Consumer-%d] has consumed a zombie process with pid %d and parent pid %d\n", id, task->pid, task->parent->pid);
        }
    }
    return 0;
}

// modlue initialization
static int __init producer_consumer_init(void) {
    int i;

    // DEBUG: check that prod is 1
    if (prod != 1) {
        printk(KERN_ERR "Error: Number of producer threads must be 1.\n");
        return -EINVAL;
    }

    // separate the external input variabel (size) from the internal implementation (buffer_size)
    buffer_size = size;
    // DEBUG:
    if (buffer_size <= 0) {
        printk(KERN_ERR "Error: Buffer size must be positive.\n");
        return -EINVAL;
    }

    // allocate memory for the buffer
    // static inline void *kmalloc_array(unsigned n, size_t s, gfp_t gfp)
    buffer = kmalloc_array(buffer_size, sizeof(struct task_struct *), GFP_KERNEL);
    // DEBUG:
    if (!buffer) {
        printk(KERN_ERR "Error: Could not allocate memory for buffer.\n");
        return -ENOMEM;
    }

    // init semaphores and spinlock
    sema_init(&empty, buffer_size);
    sema_init(&full, 0);
    //sema_init(&mutex, 1);
    // spin_lock_init(&xxx_lock);
    spin_lock_init(&buffer_lock);

    // memory for thread arrays
    producer_threads = kmalloc_array(prod, sizeof(struct task_struct *), GFP_KERNEL);
    consumer_threads = kmalloc_array(cons, sizeof(struct task_struct *), GFP_KERNEL);
    // DEBUG:
    if (!producer_threads || !consumer_threads) {
        printk(KERN_ERR "Error: Failed to allocate memory for thread arrays\n");
        kfree(buffer);
        return -ENOMEM;
    }

    // Create producer thread
    producer_threads[0] = kthread_run(producer_thread_fn, (void *)(long)(1), "Producer-%d", 1);
    // DEBUG:
    if (IS_ERR(producer_threads[0])) {
        printk(KERN_ERR "Error: Could not create producer thread.\n");
        producer_threads[0] = NULL;
        kfree(buffer);
        kfree(producer_threads);
        kfree(consumer_threads);
        return PTR_ERR(producer_threads[0]);
    }

    // create consumer threads
    for (i = 0; i < cons; i++) {
        consumer_threads[i] = kthread_run(consumer_thread_fn, (void *)(long)(i + 1), "Consumer-%d", i + 1);
        // DEBUG:
        if (IS_ERR(consumer_threads[i])) {
            printk(KERN_ERR "Error: Could not create consumer thread %d.\n", i + 1);
            consumer_threads[i] = NULL;
        }
    }

    printk(KERN_INFO "Producer-consumer module loaded.\n");
    return 0;
}

// module exit function
static void __exit producer_consumer_exit(void) {
    int i;

    // stop producer thread
    if (producer_threads && producer_threads[0]) {
        kthread_stop(producer_threads[0]);
    }

    // stop consumer threads
    if (consumer_threads) {
        for (i = 0; i < cons; i++) {
            if (consumer_threads[i])
                kthread_stop(consumer_threads[i]);
        }
    }

    // free allocated memory
    kfree(buffer);
    kfree(producer_threads);
    kfree(consumer_threads);

    printk(KERN_INFO "Producer-consumer module unloaded.\n");
}

module_init(producer_consumer_init);
module_exit(producer_consumer_exit);
