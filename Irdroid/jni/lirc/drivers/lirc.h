/*      $Id: lirc.h,v 5.27 2010/05/13 15:45:48 lirc Exp $      */

#ifndef _LINUX_LIRC_H
#define _LINUX_LIRC_H

#if defined(__linux__)
#include <linux/ioctl.h>
#elif defined(_NetBSD_)
#include <sys/ioctl.h> 
#elif defined(_CYGWIN_)
#define __USE_LINUX_IOCTL_DEFS
#include <sys/ioctl.h>
#endif

/* <obsolete> */
#define PULSE_BIT       0x01000000
#define PULSE_MASK      0x00FFFFFF
/* </obsolete> */

#define LIRC_MODE2_SPACE     0x00000000
#define LIRC_MODE2_PULSE     0x01000000
#define LIRC_MODE2_FREQUENCY 0x02000000
#define LIRC_MODE2_TIMEOUT   0x03000000

#define LIRC_VALUE_MASK      0x00FFFFFF
#define LIRC_MODE2_MASK      0xFF000000

#define LIRC_SPACE(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_SPACE)
#define LIRC_PULSE(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_PULSE)
#define LIRC_FREQUENCY(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_FREQUENCY)
#define LIRC_TIMEOUT(val) (((val)&LIRC_VALUE_MASK) | LIRC_MODE2_TIMEOUT)

#define LIRC_VALUE(val) ((val)&LIRC_VALUE_MASK)
#define LIRC_MODE2(val) ((val)&LIRC_MODE2_MASK)

#define LIRC_IS_SPACE(val) (LIRC_MODE2(val) == LIRC_MODE2_SPACE)
#define LIRC_IS_PULSE(val) (LIRC_MODE2(val) == LIRC_MODE2_PULSE)
#define LIRC_IS_FREQUENCY(val) (LIRC_MODE2(val) == LIRC_MODE2_FREQUENCY)
#define LIRC_IS_TIMEOUT(val) (LIRC_MODE2(val) == LIRC_MODE2_TIMEOUT)

typedef int lirc_t;

/*** lirc compatible hardware features ***/

#define LIRC_MODE2SEND(x) (x)
#define LIRC_SEND2MODE(x) (x)
#define LIRC_MODE2REC(x) ((x) << 16)
#define LIRC_REC2MODE(x) ((x) >> 16)

#define LIRC_MODE_RAW                  0x00000001
#define LIRC_MODE_PULSE                0x00000002
#define LIRC_MODE_MODE2                0x00000004
/* obsolete: #define LIRC_MODE_CODE                 0x00000008 */
#define LIRC_MODE_LIRCCODE             0x00000010
/* obsolete: #define LIRC_MODE_STRING               0x00000020 */


#define LIRC_CAN_SEND_RAW              LIRC_MODE2SEND(LIRC_MODE_RAW)
#define LIRC_CAN_SEND_PULSE            LIRC_MODE2SEND(LIRC_MODE_PULSE)
#define LIRC_CAN_SEND_MODE2            LIRC_MODE2SEND(LIRC_MODE_MODE2)
/* obsolete: #define LIRC_CAN_SEND_CODE             LIRC_MODE2SEND(LIRC_MODE_CODE) */
#define LIRC_CAN_SEND_LIRCCODE         LIRC_MODE2SEND(LIRC_MODE_LIRCCODE)
/* obsolete: #define LIRC_CAN_SEND_STRING           LIRC_MODE2SEND(LIRC_MODE_STRING) */

#define LIRC_CAN_SEND_MASK             0x0000003f

#define LIRC_CAN_SET_SEND_CARRIER      0x00000100
#define LIRC_CAN_SET_SEND_DUTY_CYCLE   0x00000200
#define LIRC_CAN_SET_TRANSMITTER_MASK  0x00000400

#define LIRC_CAN_REC_RAW               LIRC_MODE2REC(LIRC_MODE_RAW)
#define LIRC_CAN_REC_PULSE             LIRC_MODE2REC(LIRC_MODE_PULSE)
#define LIRC_CAN_REC_MODE2             LIRC_MODE2REC(LIRC_MODE_MODE2)
/* obsolete: #define LIRC_CAN_REC_CODE              LIRC_MODE2REC(LIRC_MODE_CODE) */
#define LIRC_CAN_REC_LIRCCODE          LIRC_MODE2REC(LIRC_MODE_LIRCCODE)
/* obsolete: #define LIRC_CAN_REC_STRING            LIRC_MODE2REC(LIRC_MODE_STRING) */

#define LIRC_CAN_REC_MASK              LIRC_MODE2REC(LIRC_CAN_SEND_MASK)

#define LIRC_CAN_SET_REC_CARRIER       (LIRC_CAN_SET_SEND_CARRIER << 16)
#define LIRC_CAN_SET_REC_DUTY_CYCLE    (LIRC_CAN_SET_SEND_DUTY_CYCLE << 16)

#define LIRC_CAN_SET_REC_DUTY_CYCLE_RANGE 0x40000000
#define LIRC_CAN_SET_REC_CARRIER_RANGE    0x80000000
#define LIRC_CAN_GET_REC_RESOLUTION       0x20000000
#define LIRC_CAN_SET_REC_TIMEOUT          0x10000000
#define LIRC_CAN_SET_REC_FILTER           0x08000000

#define LIRC_CAN_MEASURE_CARRIER          0x02000000

#define LIRC_CAN_SEND(x) ((x)&LIRC_CAN_SEND_MASK)
#define LIRC_CAN_REC(x) ((x)&LIRC_CAN_REC_MASK)

#define LIRC_CAN_NOTIFY_DECODE            0x01000000

/*** IOCTL commands for lirc driver ***/

#define LIRC_GET_FEATURES              _IOR('i', 0x00000000, unsigned long)

#define LIRC_GET_SEND_MODE             _IOR('i', 0x00000001, unsigned long)
#define LIRC_GET_REC_MODE              _IOR('i', 0x00000002, unsigned long)
#define LIRC_GET_SEND_CARRIER          _IOR('i', 0x00000003, unsigned int)
#define LIRC_GET_REC_CARRIER           _IOR('i', 0x00000004, unsigned int)
#define LIRC_GET_SEND_DUTY_CYCLE       _IOR('i', 0x00000005, unsigned int)
#define LIRC_GET_REC_DUTY_CYCLE        _IOR('i', 0x00000006, unsigned int)
#define LIRC_GET_REC_RESOLUTION        _IOR('i', 0x00000007, unsigned int)

#define LIRC_GET_MIN_TIMEOUT           _IOR('i', 0x00000008, lirc_t)
#define LIRC_GET_MAX_TIMEOUT           _IOR('i', 0x00000009, lirc_t)

#define LIRC_GET_MIN_FILTER_PULSE      _IOR('i', 0x0000000a, lirc_t)
#define LIRC_GET_MAX_FILTER_PULSE      _IOR('i', 0x0000000b, lirc_t)
#define LIRC_GET_MIN_FILTER_SPACE      _IOR('i', 0x0000000c, lirc_t)
#define LIRC_GET_MAX_FILTER_SPACE      _IOR('i', 0x0000000d, lirc_t)

/* code length in bits, currently only for LIRC_MODE_LIRCCODE */
#define LIRC_GET_LENGTH                _IOR('i', 0x0000000f, unsigned long)

/* all values set should be reset by the driver when the device is
   reopened */

/* obsolete: drivers only support one mode */
#define LIRC_SET_SEND_MODE             _IOW('i', 0x00000011, unsigned long)
/* obsolete: drivers only support one mode */
#define LIRC_SET_REC_MODE              _IOW('i', 0x00000012, unsigned long)
/* Note: these can reset the according pulse_width */
#define LIRC_SET_SEND_CARRIER          _IOW('i', 0x00000013, unsigned int)
#define LIRC_SET_REC_CARRIER           _IOW('i', 0x00000014, unsigned int)
#define LIRC_SET_SEND_DUTY_CYCLE       _IOW('i', 0x00000015, unsigned int)
#define LIRC_SET_REC_DUTY_CYCLE        _IOW('i', 0x00000016, unsigned int)
#define LIRC_SET_TRANSMITTER_MASK      _IOW('i', 0x00000017, unsigned int)

/* a value of 0 disables all hardware timeouts and data should be
   reported as soon as possible */
#define LIRC_SET_REC_TIMEOUT           _IOW('i', 0x00000018, lirc_t)
/* 1 enables, 0 disables timeout reports in MODE2 */
#define LIRC_SET_REC_TIMEOUT_REPORTS   _IOW('i', 0x00000019, unsigned int)

/* pulses shorter than this are filtered out by hardware (software
   emulation in lirc_dev/lircd?) */
#define LIRC_SET_REC_FILTER_PULSE      _IOW('i', 0x0000001a, lirc_t)
/* spaces shorter than this are filtered out by hardware (software
   emulation in lirc_dev/lircd?) */
#define LIRC_SET_REC_FILTER_SPACE      _IOW('i', 0x0000001b, lirc_t)
/* if filter cannot be set independently for pulse/space, this should
   be used */
#define LIRC_SET_REC_FILTER            _IOW('i', 0x0000001c, lirc_t)

/* if enabled from the next key press on the driver will send
   LIRC_MODE2_FREQUENCY packets */
#define LIRC_SET_MEASURE_CARRIER_MODE  _IOW('i', 0x0000001d, unsigned int)

/*
 * to set a range use
 * LIRC_SET_REC_DUTY_CYCLE_RANGE/LIRC_SET_REC_CARRIER_RANGE with the
 * lower bound first and later
 * LIRC_SET_REC_DUTY_CYCLE/LIRC_SET_REC_CARRIER with the upper bound
 */

#define LIRC_SET_REC_DUTY_CYCLE_RANGE  _IOW('i', 0x0000001e, unsigned int)
#define LIRC_SET_REC_CARRIER_RANGE     _IOW('i', 0x0000001f, unsigned int)

#define LIRC_NOTIFY_DECODE             _IO('i', 0x00000020)

#define LIRC_SETUP_START               _IO('i', 0x00000021)
#define LIRC_SETUP_END                 _IO('i', 0x00000022)

#endif
