#ifndef _LED_IOCTL_H
#define _LED_IOCTL_H

#include <linux/ioctl.h>

#define DRV_SET_OUTPUT _IOW('l', 1, int)
#define DRV_GET_OUTPUT _IOR('l', 1, int)

#endif /* _LED_IOCTL_H */
