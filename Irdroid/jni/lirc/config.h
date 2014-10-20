/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* [z] For android */
#define ZLOG_INFO(info) __android_log_write(ANDROID_LOG_INFO,"JNI",info)
#include <android/log.h>

/* device file names - beneath DEVDIR (default /dev) */
#define DEV_LIRC	"lirc"
#define DEV_LIRCD	"lircd"
#define DEV_LIRCM	"lircm"

/* config file names - beneath SYSCONFDIR (default /etc) */
#define CFG_LIRCD	"lircd.conf"
#define CFG_LIRCM	"lircmd.conf"

/* config file names - beneath $HOME or SYSCONFDIR */
#define CFG_LIRCRC	"lircrc"

/* log files */
#define LOG_LIRCD	"lircd"
#define LOG_LIRMAND	"lirmand"

/* pid file */
#define PID_LIRCD       "lircd.pid"

/* default port number */
#define	LIRC_INET_PORT	8765


/* Define to run daemons as daemons */
#define DAEMONIZE 1

/* Define to enable debugging output */
/* #undef DEBUG */

/* device files directory */
#define DEVDIR "/dev"

/* Define to use dynamic IR codes */
/* #undef DYNCODES */

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
/* #undef HAVE_ALSA_ASOUNDLIB_H */

/* Define if the ALSA library with SB RC support is installed */
/* #undef HAVE_ALSA_SB_RC */

/* Define to 1 if you have the `daemon' function. */
#define HAVE_DAEMON 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define if forkpty is available */
#define HAVE_FORKPTY 1

/* Define if the libftdi library is installed */
/* #undef HAVE_FTDI */

/* Define to 1 if you have the `gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define if the iguanaIR library is installed */
/* #undef HAVE_IGUANAIR */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if the ALSA library is installed */
/* #undef HAVE_LIBALSA */

/* Define if the caraca library is installed */
/* #undef HAVE_LIBCARACA */

/* Define if the libirman library is installed */
/* #undef HAVE_LIBIRMAN */

/* Define if the software only test version of libirman is installed */
/* #undef HAVE_LIBIRMAN_SW */

/* Define if the portaudio library is installed */
#define HAVE_LIBPORTAUDIO 1

/* Define if libusb is installed */
#define HAVE_LIBUSB 1

/* Define if the complete vga libraries (vga, vgagl) are installed */
/* #undef HAVE_LIBVGA */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* defined if Linux input interface is available */
#define HAVE_LINUX_DEVINPUT 1

/* defined if Linux hiddev HIDDEV_FLAG_UREF flag is available */
#define HAVE_LINUX_HIDDEV_FLAG_UREF 1

/* Define to 1 if you have the <linux/hiddev.h> header file. */
#define HAVE_LINUX_HIDDEV_H 1

/* Define to 1 if you have the <linux/i2c-dev.h> header file. */
#define HAVE_LINUX_I2C_DEV_H 1

/* Define to 1 if you have the <linux/input.h> header file. */
#define HAVE_LINUX_INPUT_H 1

/* Define to 1 if you have the <linux/types.h> header file. */
#define HAVE_LINUX_TYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkfifo' function. */
#define HAVE_MKFIFO 1

/* Define to 1 if you have the <portaudio.h> header file. */
#define HAVE_PORTAUDIO_H 1

/* defined if SCSI API is available */
#define HAVE_SCSI 1

/* Define to 1 if you have the <scsi/sg.h> header file. */
#define HAVE_SCSI_SG_H 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* defined if soundcard API is available */
#define HAVE_SOUNDCARD 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strsep' function. */
#define HAVE_STRSEP 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#define HAVE_SYS_SOUNDCARD_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* define if you have vsyslog( prio, fmt, va_arg ) */
#define HAVE_VSYSLOG 1

/* The name of the hw_* structure to use by default */
#define HW_DEFAULT hw_audio

/* Text string signifying which driver is configured */
#define LIRC_DRIVER "audio"

/* Define to include most drivers */
/* #undef LIRC_DRIVER_ANY */

/* Define if devfs support is present in current kernel */
/* #undef LIRC_HAVE_DEVFS */

/* Define if your iMON is an LCD and not a VFD. */
/* #undef LIRC_IMON_LCD */

/* Set the IRQ for the lirc driver */
/* #undef LIRC_IRQ */

/* Set the default tty used by the drivers accessing /dev/ttyX */
/* #undef LIRC_IRTTY */

/* Define if you want to use lirc_it87 with an Asus Digimatrix */
/* #undef LIRC_IT87_DIGIMATRIX */

/* Set the device major for the lirc driver */
#define LIRC_MAJOR 61

/* Define if you want to cross-compile for the SA1100 */
/* #undef LIRC_ON_SA1100 */

/* Set the port address for the lirc driver */
/* #undef LIRC_PORT */

/* Define if you have an animax serial port receiver */
/* #undef LIRC_SERIAL_ANIMAX */

/* Define if you have an Igor Cesko receiver */
/* #undef LIRC_SERIAL_IGOR */

/* Define if you have an IRdeo serial port receiver */
/* #undef LIRC_SERIAL_IRDEO */

/* Define if you have an IRdeo remote transmitter */
/* #undef LIRC_SERIAL_IRDEO_REMOTE */

/* Define if you have a Linksys NSLU2 and use CTS2+GreenLED */
/* #undef LIRC_SERIAL_NSLU2 */

/* Define if the software needs to generate the carrier frequency */
#define LIRC_SERIAL_SOFTCARRIER 1

/* Define if you have a IR diode connected to the serial port */
/* #undef LIRC_SERIAL_TRANSMITTER */

/* Define if you want to use a Actisys Act200L */
/* #undef LIRC_SIR_ACTISYS_ACT200L */

/* Define if you want to use a Actisys Act220L */
/* #undef LIRC_SIR_ACTISYS_ACT220L */

/* Define if you want to use a Tekram Irmate 210 */
/* #undef LIRC_SIR_TEKRAM */

/* syslog facility to use */
#define LIRC_SYSLOG 

/* Set the timer for the parallel port driver */
/* #undef LIRC_TIMER */

/* modifiable single-machine data */
/* #define LOCALSTATEDIR "/var" */
#define LOCALSTATEDIR 		VARRUNDIR "/" PACKAGE

/* Define to use long long IR codes */
#define LONG_IR_CODE 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* define in maintainer mode */
/* #undef MAINTAINER_MODE */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "com.zokama.androlirc"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* system configuration directory */
/* #define SYSCONFDIR "/etc" */
#define SYSCONFDIR "/data/data"

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* define if you want to log to syslog instead of logfile */
/* #undef USE_SYSLOG */

/* /var/run */
#define VARRUNDIR "/data/data"

/* Version number of package */
#define VERSION "0.8.7"

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */


#ifdef LIRC_HAVE_DEVFS
#define LIRC_DRIVER_DEVICE	DEVDIR "/" DEV_LIRC "/0"
#else
#define LIRC_DRIVER_DEVICE      DEVDIR "/" DEV_LIRC
#endif /* LIRC_HAVE_DEVFS */

#define LIRCD			VARRUNDIR "/" PACKAGE "/" DEV_LIRCD
#define LIRCM			VARRUNDIR "/" PACKAGE "/" DEV_LIRCM

#define LIRCDCFGFILE		SYSCONFDIR "/" PACKAGE "/" CFG_LIRCD
#define LIRCMDCFGFILE		SYSCONFDIR "/" PACKAGE "/" CFG_LIRCM

#define LIRCDOLDCFGFILE		SYSCONFDIR "/" CFG_LIRCD
#define LIRCMDOLDCFGFILE	SYSCONFDIR "/" CFG_LIRCM

#define LIRCRC_USER_FILE	"." CFG_LIRCRC
#define LIRCRC_ROOT_FILE	SYSCONFDIR "/" PACKAGE "/" CFG_LIRCRC
#define LIRCRC_OLD_ROOT_FILE	SYSCONFDIR "/" CFG_LIRCRC

#define LOGFILE			LOCALSTATEDIR "/log/" LOG_LIRCD
#define LIRMAND_LOGFILE		LOCALSTATEDIR "/log/" LOG_LIRMAND

#define PIDFILE                 VARRUNDIR "/" PACKAGE "/" PID_LIRCD

#define LIRC_RELEASE_SUFFIX     "_UP"

