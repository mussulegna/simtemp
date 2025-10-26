
// Basic for module creation
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/device.h>
#include <linux/miscdevice.h> // For device.
#include <linux/fs.h> // For file operations.
#include <linux/sysfs.h> // For sysfs create file.
#include <linux/uaccess.h> // For copy_to_user/copy_from_user.

#include <linux/hrtimer.h> // For high resolution timer.
#include <linux/ktime.h>
#include <linux/poll.h> // For poll/select/epoll.
#include <linux/wait.h>

// Project specific headers
#include "nxp_simtemp_data.h"
#include "nxp_simtemp_ioctl.h"


// --------------------------------
// --------------------------------
// Module information.
// --------------------------------
MODULE_AUTHOR("Buenrostro, Angel");
MODULE_DESCRIPTION("Device driver for simulated temperature sensor.");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1-draft");


// --------------------------------
// --------------------------------
// Macros
// --------------------------------
#define DEVICE_NAME "simtemp"

//#define DEBUG_SIMTEMP_EXECOPEN
//#define DEBUG_SIMTEMP_EXECRELEASE
#define DEBUG_SIMTEMP_EXECREAD
#define DEBUG_SIMTEMP_EXECWRITE
//#define DEBUG_SIMTEMP_EXECPOLL

//#define DEBUG_SIMTEMP_SAMPLING    1


// --------------------------------
// --------------------------------
// Structure definition
// --------------------------------
/*struct nxp_simtemp_sample_t {
    __u64 timestamp_ns;    // monotonic timestamp
    __s32 temp_mc;         // milli-degree Celsius
    __u32 flags;           // bit0=NEW_SAMPLE, bit1=THRESHOLD_CROSSED
} __attribute__((packed));*/


// --------------------------------
// --------------------------------
// Variables
// --------------------------------
static struct class *nxp_simtemp_class;

static unsigned int nxp_simtemp_sampling_ms;
static int nxp_simtemp_threshold_mc;
static unsigned char nxp_simtemp_mode;

static struct hrtimer nxp_simtemp_hrtimer;
static struct nxp_simtemp_sample_t nxp_simtemp_sample;

static wait_queue_head_t nxp_simtemp_wq;
//DECLARE_WAIT_QUEUE_HEAD(nxp_simtemp_wq);

// --------------------------------
// --------------------------------
// Functions for the character device interface.
// --------------------------------

static int nxp_simtemp_open(struct inode *inodep, struct file *filep)
{
    #ifdef DEBUG_SIMTEMP_EXECOPEN
        pr_info("[%s] OPENED \n", DEVICE_NAME);
    #endif
    return 0;
}

static int nxp_simtemp_release(struct inode *inodep, struct file *filep)
{
    #ifdef DEBUG_SIMTEMP_EXECRELEASE
        pr_info("[%s] RELEASED \n", DEVICE_NAME);
    #endif
    return 0;
}

static ssize_t nxp_simtemp_read(struct file *filep, char *bufferp, size_t length, loff_t * offsetp)
{
    if(copy_to_user(bufferp, &nxp_simtemp_sample, sizeof(nxp_simtemp_sample)))
    {
        return -EFAULT;
    }

nxp_simtemp_sample.flags = 0;

    #ifdef DEBUG_SIMTEMP_EXECREAD
        pr_info("[%s] %u\n", DEVICE_NAME, nxp_simtemp_sample.temp_mc);
    #endif
    return 0;
}

static ssize_t nxp_simtemp_write(struct file *filep, const char *bufferp, size_t length, loff_t * offsetp)
{
    #ifdef DEBUG_SIMTEMP_EXECWRITE
        pr_info("[%s] Write callback not implemented.\n", DEVICE_NAME);
    #endif
    return 0;
}

static unsigned int nxp_simtemp_poll(struct file *filep, struct poll_table_struct *waitp)
{
    __poll_t retMask = 0;

    #ifdef DEBUG_SIMTEMP_EXECPOLL
        pr_info("[%s] POLL WAIT.\n", DEVICE_NAME);
    #endif

    // Wait for new data.
    poll_wait(filep, &nxp_simtemp_wq, waitp);

    // Notify user application that new data is available.
    if(nxp_simtemp_sample.flags == 1){
        retMask |= (POLLIN | POLLRDNORM);
    }

    #ifdef DEBUG_SIMTEMP_EXECPOLL
        pr_info("[%s] POLL RET.\n", DEVICE_NAME);
    #endif
    return retMask;
}

static long nxp_simtemp_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    long retVal = 0;
    unsigned int fromUserSampling;
    int fromUserThreshold;
    unsigned char fromUserMode;

    switch(cmd) {
        case NXP_SIMTEMP_GET_SAMPLINGMS:
            if(copy_to_user((unsigned int __user *)arg, &nxp_simtemp_sampling_ms, sizeof(nxp_simtemp_sampling_ms)))
            {
                retVal = -EFAULT;
            }
            break;
        case NXP_SIMTEMP_SET_SAMPLINGMS:
            if(copy_from_user(&fromUserSampling, (unsigned int __user *)arg, sizeof(nxp_simtemp_sampling_ms)))
            {
                retVal = -EFAULT;
            }
            else
            {
                nxp_simtemp_sampling_ms = fromUserSampling;
            }
            break;
        case NXP_SIMTEMP_GET_THRESHOLDMC:
            if(copy_to_user((int __user *)arg, &nxp_simtemp_threshold_mc, sizeof(nxp_simtemp_threshold_mc)))
            {
                retVal = -EFAULT;
            }
            break;
        case NXP_SIMTEMP_SET_THRESHOLDMC:
            if(copy_from_user(&fromUserThreshold, (int __user *)arg, sizeof(nxp_simtemp_threshold_mc)))
            {
                retVal = -EFAULT;
            }
            else
            {
                nxp_simtemp_threshold_mc = fromUserThreshold;
            }
            break;
        case NXP_SIMTEMP_GET_MODE:
            if(copy_to_user((unsigned char __user *)arg, &nxp_simtemp_mode, sizeof(nxp_simtemp_mode)))
            {
                retVal = -EFAULT;
            }
            break;
        case NXP_SIMTEMP_SET_MODE:
            if(copy_from_user(&fromUserMode, (unsigned char __user *)arg, sizeof(nxp_simtemp_mode)))
            {
                retVal = -EFAULT;
            }
            else
            {
                nxp_simtemp_mode = fromUserMode;
            }
            break;
        case NXP_SIMTEMP_GET_STATS:
            if(copy_to_user((unsigned int __user *)arg, &nxp_simtemp_sampling_ms, sizeof(nxp_simtemp_sampling_ms)))
            {
                retVal = -EFAULT;
            }
            break;

    };
    return retVal;
}


// --------------------------------
// --------------------------------
// Class attributes callbacks, show/store by attribute.
// --------------------------------
static ssize_t sampling_ms_show(const struct class *classp, const struct class_attribute *attrp, char *bufp)
{
    return sysfs_emit(bufp, "%u\n", nxp_simtemp_sampling_ms);
}
static ssize_t sampling_ms_store(const struct class *classp, const struct class_attribute *attrp, const char *buf, size_t count)
{
    return count;
}

static ssize_t threshold_mc_show(const struct class *classp, const struct class_attribute *attrp, char *bufp)
{
    return sysfs_emit(bufp, "%u\n", nxp_simtemp_threshold_mc);
}
static ssize_t threshold_mc_store(const struct class *classp, const struct class_attribute *attrp, const char *buf, size_t count)
{
    return count;
}


// --------------------------------
// --------------------------------
// HRTimer callback
// --------------------------------
static enum hrtimer_restart nxp_simtemp_hrtimer_callback(struct hrtimer *timerp)
{
    // Get new temperature value.
    //nxp_simtemp_sample.timestamp_ns += nxp_simtemp_sampling_ms * 1000000UL;
    nxp_simtemp_sample.timestamp_ns = ktime_get_ns();
    //nxp_simtemp_sample.temp_mc = 25600;
    nxp_simtemp_sample.temp_mc += 100;
    nxp_simtemp_sample.flags = 1;

    // Reload timer to provide periodic sampling.
    #ifndef DEBUG_SIMTEMP_SAMPLING
        hrtimer_forward_now(timerp, ktime_set(0, nxp_simtemp_sampling_ms * 1000000UL));
    #else
        hrtimer_forward_now(timerp, ktime_set(DEBUG_SIMTEMP_SAMPLING, 0));
    #endif

    // Notify that new value is available (for poll callback).
    wake_up(&nxp_simtemp_wq);

    return HRTIMER_RESTART;
}


// --------------------------------
// --------------------------------
// Module basic data structures.
// --------------------------------

static const struct file_operations nxp_simtemp_fops = {
    .owner = THIS_MODULE,
    .open = nxp_simtemp_open,
    .release = nxp_simtemp_release,
    .read = nxp_simtemp_read,
    .write = nxp_simtemp_write,
    .poll = nxp_simtemp_poll,
    .unlocked_ioctl = nxp_simtemp_ioctl,
};

static struct miscdevice nxp_simtemp_miscdevice = {
    .minor = MISC_DYNAMIC_MINOR, // Request dynamic minor number.
    .name = DEVICE_NAME,
    .fops = &nxp_simtemp_fops,
    .mode = 0666, // Permissions for device.
};

//static struct class_attribute nxp_simtemp_attr_samplingms = __CLASS_ATTR(nxp_simtemp_sampling_ms, 0666, nxp_simtemp_attr_samplingms_show, nxp_simtemp_attr_samplingms_store);
static CLASS_ATTR_RW(sampling_ms);
static CLASS_ATTR_RW(threshold_mc);

static struct class_attribute *nxp_simtemp_attrs[] = {
    &class_attr_sampling_ms,
    &class_attr_threshold_mc,
    NULL,
};

// --------------------------------
// --------------------------------
// Module basic functions.
// --------------------------------

static int __init nxp_simtemp_init(void)
{
    int retVal;

    // Read initial configuration from DT
    // Not available values from DT, set default values.
    nxp_simtemp_sampling_ms = 100;
    nxp_simtemp_threshold_mc = 45000;
    nxp_simtemp_mode = 0; // normal

    nxp_simtemp_sample.timestamp_ns = 0;
    nxp_simtemp_sample.temp_mc = 25600; //0;
    nxp_simtemp_sample.flags = 0;

    // Create device.
    retVal = misc_register(&nxp_simtemp_miscdevice);
    if(retVal)
    {
        pr_err("[%s] Failed to register device.\n", DEVICE_NAME);
    }
    else
    {
        pr_info("[%s] Device registered with minor=%d.\n", DEVICE_NAME, nxp_simtemp_miscdevice.minor);
    }

    // Create sysfs with class object for our device and its attributes.
    nxp_simtemp_class = class_create(DEVICE_NAME);
    if(IS_ERR(nxp_simtemp_class))
    {
        misc_deregister(&nxp_simtemp_miscdevice);
        return PTR_ERR(nxp_simtemp_class);
    }

    retVal = class_create_file(nxp_simtemp_class, &class_attr_sampling_ms);
    retVal = class_create_file(nxp_simtemp_class, &class_attr_threshold_mc);
//    retVal = class_create_files(nxp_simtemp_class, &nxp_simtemp_attrs);
    if(retVal)
    {
        misc_deregister(&nxp_simtemp_miscdevice);
        class_destroy(nxp_simtemp_class);
        return retVal;
    }
//    nxp_simtemp_class->class_attrs = nxp_simtemp_attrs;

    // Start mechanism to provide periodic temeperature.
    init_waitqueue_head(&nxp_simtemp_wq);

    hrtimer_init(&nxp_simtemp_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    nxp_simtemp_hrtimer.function = &nxp_simtemp_hrtimer_callback;
    #ifndef DEBUG_SIMTEMP_SAMPLING
        hrtimer_start(&nxp_simtemp_hrtimer, ktime_set(0, nxp_simtemp_sampling_ms * 1000000UL), HRTIMER_MODE_REL);
    #else
        hrtimer_start(&nxp_simtemp_hrtimer, ktime_set(DEBUG_SIMTEMP_SAMPLING, 0), HRTIMER_MODE_REL);
    #endif

    return retVal;
}

static void __exit nxp_simtemp_exit(void)
{
    hrtimer_cancel(&nxp_simtemp_hrtimer);

    class_remove_file(nxp_simtemp_class, &class_attr_sampling_ms);
    class_remove_file(nxp_simtemp_class, &class_attr_threshold_mc);
//    class_remove_file(nxp_simtemp_class, &nxp_simtemp_attrs);
    class_destroy(nxp_simtemp_class);

    misc_deregister(&nxp_simtemp_miscdevice);
    pr_info("[%s] Device unregistered.\n", DEVICE_NAME);
}

module_init(nxp_simtemp_init);
module_exit(nxp_simtemp_exit);
