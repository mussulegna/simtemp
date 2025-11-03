
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

#define NXP_SIMTEMP_MODE_STR_LEN    16

//#define DEBUG_SIMTEMP_EXECOPEN
//#define DEBUG_SIMTEMP_EXECRELEASE
#define DEBUG_SIMTEMP_EXECREAD
#define DEBUG_SIMTEMP_EXECWRITE
//#define DEBUG_SIMTEMP_EXECPOLL
#define DEBU_SIMTEMP_SYSFSMODESET

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
    return -EPERM;
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
    unsigned int lsampling;
    int lthreshold;
    unsigned char lmode;
    unsigned int lstats = 0;

    switch(cmd) {
        case NXP_SIMTEMP_GET_SAMPLINGMS:
            lsampling = nxp_simtemp_sampling_ms;
            if(copy_to_user((unsigned int __user *)arg, &lsampling, sizeof(nxp_simtemp_sampling_ms)))
            {
                retVal = -EFAULT;
            }
            break;
        case NXP_SIMTEMP_SET_SAMPLINGMS:
            if(copy_from_user(&lsampling, (unsigned int __user *)arg, sizeof(nxp_simtemp_sampling_ms)))
            {
                retVal = -EFAULT;
            }
            else
            {
                nxp_simtemp_sampling_ms = lsampling;
            }
            break;
        case NXP_SIMTEMP_GET_THRESHOLDMC:
            lthreshold = nxp_simtemp_threshold_mc;
            if(copy_to_user((int __user *)arg, &lthreshold, sizeof(nxp_simtemp_threshold_mc)))
            {
                retVal = -EFAULT;
            }
            break;
        case NXP_SIMTEMP_SET_THRESHOLDMC:
            if(copy_from_user(&lthreshold, (int __user *)arg, sizeof(nxp_simtemp_threshold_mc)))
            {
                retVal = -EFAULT;
            }
            else
            {
                nxp_simtemp_threshold_mc = lthreshold;
            }
            break;
        case NXP_SIMTEMP_GET_MODE:
            lmode = nxp_simtemp_mode;
            if(copy_to_user((unsigned char __user *)arg, &lmode, sizeof(nxp_simtemp_mode)))
            {
                retVal = -EFAULT;
            }
            break;
        case NXP_SIMTEMP_SET_MODE:
            if(copy_from_user(&lmode, (unsigned char __user *)arg, sizeof(nxp_simtemp_mode)))
            {
                retVal = -EFAULT;
            }
            else
            {
                if((NXP_SIMTEMP_MODE_NORMAL == lmode) || (NXP_SIMTEMP_MODE_NOISY == lmode) || (NXP_SIMTEMP_MODE_RAMP == lmode))
                {
                    nxp_simtemp_mode = lmode;
                }
                else
                {
                    retVal = -EINVAL;
                }
            }
            break;
        case NXP_SIMTEMP_GET_STATS:
            if(copy_to_user((unsigned int __user *)arg, &lstats, sizeof(nxp_simtemp_sampling_ms)))
            {
                retVal = -EFAULT;
            }
            break;
        default:
            retVal = -EINVAL;
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
    unsigned int lsampling_ms;

    lsampling_ms = nxp_simtemp_sampling_ms;
    return sysfs_emit(bufp, "%u\n", lsampling_ms);
}
static ssize_t sampling_ms_store(const struct class *classp, const struct class_attribute *attrp, const char *bufp, size_t count)
{
    unsigned int lsampling;

    if(kstrtouint(bufp, 10, &lsampling) < 0)
    {
        return -EINVAL;
    }
    nxp_simtemp_sampling_ms = lsampling;

    return count;
}

static ssize_t threshold_mc_show(const struct class *classp, const struct class_attribute *attrp, char *bufp)
{
    int lthreshold_mc;

    lthreshold_mc = nxp_simtemp_threshold_mc;
    return sysfs_emit(bufp, "%i\n",  lthreshold_mc);
}
static ssize_t threshold_mc_store(const struct class *classp, const struct class_attribute *attrp, const char *bufp, size_t count)
{
    int lthreshold;

    if(kstrtoint(bufp, 10, &lthreshold) < 0)
    {
        return -EINVAL;
    }
    nxp_simtemp_threshold_mc = lthreshold;

    return count;
}

static ssize_t mode_show(const struct class *classp, const struct class_attribute *attrp, char *bufp)
{
    unsigned char lmode;

    lmode = nxp_simtemp_mode;
    if(NXP_SIMTEMP_MODE_NORMAL == lmode) {
        return sysfs_emit(bufp, "normal\n");
    } else if(NXP_SIMTEMP_MODE_NOISY == lmode) {
        return sysfs_emit(bufp, "noisy\n");
    } else if(NXP_SIMTEMP_MODE_RAMP == lmode) {
        return sysfs_emit(bufp, "ramp\n");
    } else {
        return sysfs_emit(bufp, "error!\n");
    }
}
static ssize_t mode_store(const struct class *classp, const struct class_attribute *attrp, const char *bufp, size_t count)
{
    char lmodeStr[NXP_SIMTEMP_MODE_STR_LEN] = {'\0'};
    unsigned char lmode = 0;

    #ifdef DEBU_SIMTEMP_SYSFSMODESET
        pr_info("[%s] mode_store start .\n", DEVICE_NAME);
    #endif

    snprintf(lmodeStr, sizeof(lmodeStr), "%.*s", (int)count, bufp);
    lmodeStr[count-1] = '\0';

    if(strncmp(lmodeStr, "normal", NXP_SIMTEMP_MODE_STR_LEN) == 0) {
        lmode = NXP_SIMTEMP_MODE_NORMAL;
    } else if(strncmp(lmodeStr, "noisy", NXP_SIMTEMP_MODE_STR_LEN) == 0) {
        lmode = NXP_SIMTEMP_MODE_NOISY;
    } else if(strncmp(lmodeStr, "ramp", NXP_SIMTEMP_MODE_STR_LEN) == 0) {
        lmode = NXP_SIMTEMP_MODE_RAMP;
    } else {
        lmode = 0;
    }

    if(lmode != 0)
    {
        nxp_simtemp_mode = lmode;
    }

    return count;
}

static ssize_t stats_show(const struct class *classp, const struct class_attribute *attrp, char *bufp)
{
    return sysfs_emit(bufp, "%u\n",  0);
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
static CLASS_ATTR_RW(mode);
static CLASS_ATTR_RO(stats);

/*static struct class_attribute *nxp_simtemp_attrs[] = {
    &class_attr_sampling_ms,
    &class_attr_threshold_mc,
    NULL,
};*/

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
    nxp_simtemp_mode = NXP_SIMTEMP_MODE_NORMAL;

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
    retVal = class_create_file(nxp_simtemp_class, &class_attr_mode);
    retVal = class_create_file(nxp_simtemp_class, &class_attr_stats);
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
    class_remove_file(nxp_simtemp_class, &class_attr_mode);
    class_remove_file(nxp_simtemp_class, &class_attr_stats);
//    class_remove_file(nxp_simtemp_class, &nxp_simtemp_attrs);
    class_destroy(nxp_simtemp_class);

    misc_deregister(&nxp_simtemp_miscdevice);
    pr_info("[%s] Device unregistered.\n", DEVICE_NAME);
}

module_init(nxp_simtemp_init);
module_exit(nxp_simtemp_exit);
