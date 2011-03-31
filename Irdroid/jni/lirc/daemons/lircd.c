/*      $Id: lircd.c,v 5.97 2010/08/27 05:08:09 jarodwilson Exp $      */

/****************************************************************************
 ** lircd.c *****************************************************************
 ****************************************************************************
 *
 * lircd - LIRC Decoder Daemon
 * 
 * Copyright (C) 1996,97 Ralph Metzler <rjkm@thp.uni-koeln.de>
 * Copyright (C) 1998,99 Christoph Bartelmus <lirc@bartelmus.de>
 *
 *  =======
 *  HISTORY
 *  =======
 *
 * 0.1:  03/27/96  decode SONY infra-red signals
 *                 create mousesystems mouse signals on pipe /dev/lircm
 *       04/07/96  send ir-codes to clients via socket (see irpty)
 *       05/16/96  now using ir_remotes for decoding
 *                 much easier now to describe new remotes
 *
 * 0.5:  09/02/98 finished (nearly) complete rewrite (Christoph)
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* disable daemonise if maintainer mode SIM_REC / SIM_SEND defined */
#if defined(SIM_REC) || defined (SIM_SEND)
# undef DAEMONIZE
#endif

#define _GNU_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/file.h>

#if defined(__linux__)
#include <linux/input.h>
#include <linux/uinput.h>
#include "input_map.h"
#endif

#if defined __APPLE__  || defined __FreeBSD__
#include <sys/ioctl.h>
#endif

#ifndef timersub
#define timersub(a, b, result)                                            \
  do {                                                                    \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                         \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                      \
    if ((result)->tv_usec < 0) {                                          \
      --(result)->tv_sec;                                                 \
      (result)->tv_usec += 1000000;                                       \
    }                                                                     \
  } while (0)
#endif

#include "lircd.h"
#include "ir_remote.h"
#include "config_file.h"
#include "hardware.h"
#include "hw-types.h"
#include "release.h"

struct ir_remote *remotes;
struct ir_remote *free_remotes=NULL;

extern struct ir_remote *decoding;
extern struct ir_remote *last_remote;
extern struct ir_remote *repeat_remote;
extern struct ir_ncode *repeat_code;

static int repeat_fd=-1;
static char *repeat_message=NULL;
static unsigned int repeat_max = REPEAT_MAX_DEFAULT;

extern struct hardware hw;

char *progname="lircd";
const char *configfile = NULL;
#ifndef USE_SYSLOG
char *logfile=LOGFILE;
#else
static const char *syslogident = "lircd-" VERSION;
#endif
FILE *pidf;
char *pidfile = PIDFILE;
char *lircdfile = LIRCD;

struct protocol_directive directives[] =
{
	{"LIST",list},
	{"SEND_ONCE",send_once},
	{"SEND_START",send_start},
	{"SEND_STOP",send_stop},
	{"VERSION",version},
	{"SET_TRANSMITTERS",set_transmitters},
	{"SIMULATE",simulate},
	{NULL,NULL}
	/*
	{"DEBUG",debug},
	{"DEBUG_LEVEL",debug_level},
	*/
};

enum protocol_string_num {
	P_BEGIN=0,
	P_DATA,
	P_END,
	P_ERROR,
	P_SUCCESS,
	P_SIGHUP
};

char *protocol_string[] = 
{
	"BEGIN\n",
	"DATA\n",
	"END\n",
	"ERROR\n",
	"SUCCESS\n",
	"SIGHUP\n"
};

static void log_enable(int enabled);
static int log_enabled = 1;

#ifndef USE_SYSLOG
#define HOSTNAME_LEN 128
char hostname[HOSTNAME_LEN+1];

FILE *lf=NULL;
#endif

/* substract one for lirc, sockfd, sockinet, logfile, pidfile, uinput */
#define MAX_PEERS	((FD_SETSIZE-6)/2)
#define MAX_CLIENTS     ((FD_SETSIZE-6)/2)

int sockfd, sockinet;
static int uinputfd = -1;
int clis[MAX_CLIENTS];

#define CT_LOCAL  1
#define CT_REMOTE 2

static int cli_type[MAX_CLIENTS];
static int clin=0;

int listen_tcpip=0;
unsigned short int port=LIRC_INET_PORT;
struct in_addr address;

struct	peer_connection *peers[MAX_PEERS];
int	peern = 0;

int debug=0;
static int daemonized=0;
static int allow_simulate=0;
static int userelease=0;
static int useuinput=0;

static sig_atomic_t term=0,hup=0,alrm=0;
static int termsig;

static unsigned int setup_min_freq=0, setup_max_freq=0;
static lirc_t setup_max_gap=0;
static lirc_t setup_min_pulse=0, setup_min_space=0;
static lirc_t setup_max_pulse=0, setup_max_space=0;

inline int use_hw()
{
	return (clin>0 || (useuinput && uinputfd != -1) || repeat_remote != NULL);
}

/* set_transmitters only supports 32 bit int */
#define MAX_TX (CHAR_BIT*sizeof(unsigned long))

inline int max(int a,int b)
{
	return(a>b ? a:b);
}

/* cut'n'paste from fileutils-3.16: */

#define isodigit(c) ((c) >= '0' && (c) <= '7')

/* Return a positive integer containing the value of the ASCII
   octal number S.  If S is not an octal number, return -1.  */

static int
oatoi (s)
     char *s;
{
  register int i;

  if (*s == 0)
    return -1;
  for (i = 0; isodigit (*s); ++s)
    i = i * 8 + *s - '0';
  if (*s)
    return -1;
  return i;
}

/* A safer write(), since sockets might not write all but only some of the
   bytes requested */

inline int write_socket(int fd, const char *buf, int len)
{
	int done,todo=len;

	while(todo)
	{
#ifdef SIM_REC
		do{
			done=write(fd,buf,todo);
		}
		while(done<0 && errno == EAGAIN);
#else
		done=write(fd,buf,todo);
#endif
		if(done<=0) return(done);
		buf+=done;
		todo-=done;
	}
	return(len);
}

inline int write_socket_len(int fd, const char *buf)
{
	int len;

	len=strlen(buf);
	if(write_socket(fd,buf,len)<len) return(0);
	return(1);
}

inline int read_timeout(int fd,char *buf,int len,int timeout)
{
	fd_set fds;
	struct timeval tv;
	int ret,n;
	
	FD_ZERO(&fds);
	FD_SET(fd,&fds);
	tv.tv_sec=timeout;
	tv.tv_usec=0;
	
	/* CAVEAT: (from libc documentation)
     Any signal will cause `select' to return immediately.  So if your
     program uses signals, you can't rely on `select' to keep waiting
     for the full time specified.  If you want to be sure of waiting
     for a particular amount of time, you must check for `EINTR' and
     repeat the `select' with a newly calculated timeout based on the
     current time.  See the example below.

     Obviously the timeout is not recalculated in the example because
     this is done automatically on Linux systems...
	*/
     
	do
	{
		ret=select(fd+1,&fds,NULL,NULL,&tv);
	}
	while(ret==-1 && errno==EINTR);
	if(ret==-1)
	{
		logprintf(LOG_ERR,"select() failed");
		logperror(LOG_ERR,NULL);
		return(-1);
	}
	else if(ret==0) return(0); /* timeout */
	n=read(fd,buf,len);
	if(n==-1)
	{
		logprintf(LOG_ERR,"read() failed");
		logperror(LOG_ERR,NULL);
		return(-1);
	}
	return(n);
}

void sigterm(int sig)
{
	/* all signals are blocked now */
	if(term) return;
	term=1;
	termsig=sig;
}

void dosigterm(int sig)
{
	int i;
	
	signal(SIGALRM,SIG_IGN);
	
	if(free_remotes!=NULL)
	{
		free_config(free_remotes);
	}
	free_config(remotes);
	repeat_remote = NULL;
	logprintf(LOG_NOTICE,"caught signal");
	for (i=0; i<clin; i++)
	{
		shutdown(clis[i],2);
		close(clis[i]);
	};
	shutdown(sockfd,2);
	close(sockfd);

#if defined(__linux__)
	if(uinputfd != -1)
	{
		ioctl(uinputfd, UI_DEV_DESTROY);
		close(uinputfd);
		uinputfd = -1;
	}
#endif
	if(listen_tcpip)
	{
		shutdown(sockinet,2);
		close(sockinet);
	}
	fclose(pidf);
	(void) unlink(pidfile);
	if(use_hw() && hw.deinit_func) hw.deinit_func();
#ifdef USE_SYSLOG
	closelog();
#else
	if(lf) fclose(lf);
#endif
	signal(sig,SIG_DFL);
	raise(sig);
}

void sighup(int sig)
{
	hup=1;
}

void dosighup(int sig)
{
#ifndef USE_SYSLOG
	struct stat s;
#endif
	int i;

	/* reopen logfile first */
#ifdef USE_SYSLOG
	/* we don't need to do anyting as this is syslogd's task */
#else
	logprintf(LOG_INFO,"closing logfile");
	if(-1==fstat(fileno(lf),&s))		
	{
		dosigterm(SIGTERM); /* shouldn't ever happen */
	}
	fclose(lf);
	lf=fopen(logfile,"a");
	if(lf==NULL)
	{
		/* can't print any error messagees */
		dosigterm(SIGTERM);
	}
	logprintf(LOG_INFO,"reopened logfile");
	if(-1==fchmod(fileno(lf),s.st_mode))
	{
		logprintf(LOG_WARNING,"could not set file permissions");
		logperror(LOG_WARNING,NULL);
	}
#endif

	config();
	
	for (i=0; i<clin; i++)
	{
		if(!(write_socket_len(clis[i],protocol_string[P_BEGIN]) &&
		     write_socket_len(clis[i],protocol_string[P_SIGHUP]) &&
		     write_socket_len(clis[i],protocol_string[P_END])))
		{
			remove_client(clis[i]);
			i--;
		}
	}
      /* restart all connection timers */
      for (i=0; i<peern; i++)
      {
              if (peers[i]->socket == -1)
              {
                      gettimeofday(&peers[i]->reconnect, NULL);
                      peers[i]->connection_failure = 0;
              }
      }
}

int setup_uinputfd(const char *name)
{
#if defined(__linux__)
	int fd;
	int key;
	struct uinput_user_dev dev;
	
	fd = open("/dev/input/uinput", O_RDWR);
	if(fd == -1)
	{
		fd = open("/dev/uinput", O_RDWR);
		if(fd == -1)
		{
			fd = open("/dev/misc/uinput", O_RDWR);
			if(fd == -1)
			{
				fprintf(stderr, "could not open %s\n",
					"uinput");
				perror(NULL);
				return -1;
			}
		}
	}
	memset(&dev, 0, sizeof(dev));
	strncpy(dev.name, name, sizeof(dev.name));
	dev.name[sizeof(dev.name)-1] = 0;
	if(write(fd, &dev, sizeof(dev)) != sizeof(dev) ||
	   ioctl(fd, UI_SET_EVBIT, EV_KEY) != 0 ||
	   ioctl(fd, UI_SET_EVBIT, EV_REP) != 0)
	{
		goto setup_error;
	}

        for(key = KEY_RESERVED; key <= KEY_UNKNOWN; key++)
	{
                if(ioctl(fd, UI_SET_KEYBIT, key) != 0)
		{
			goto setup_error;
                }
	}

	if(ioctl(fd, UI_DEV_CREATE) != 0)
	{
		goto setup_error;
	}
	return fd;
	
 setup_error:
	fprintf(stderr, "could not setup %s\n", "uinput");
	perror(NULL);
	close(fd);
#endif
	return -1;
}

static int setup_frequency()
{
	unsigned int freq;
	
	if(!(hw.features&LIRC_CAN_SET_REC_CARRIER))
	{
		return(1);
	}
	if(setup_min_freq == 0 || setup_max_freq == 0)
	{
		setup_min_freq = DEFAULT_FREQ;
		setup_max_freq = DEFAULT_FREQ;
	}
	if(hw.features&LIRC_CAN_SET_REC_CARRIER_RANGE &&
	   setup_min_freq!=setup_max_freq)
	{
		if(hw.ioctl_func(LIRC_SET_REC_CARRIER_RANGE, &setup_min_freq)==-1)
		{
			logprintf(LOG_ERR,"could not set receive carrier");
			logperror(LOG_ERR, __FUNCTION__);
			return(0);
		}
		freq=setup_max_freq;
	}
	else
	{
		freq=(setup_min_freq+setup_max_freq)/2;
	}
	if(hw.ioctl_func(LIRC_SET_REC_CARRIER, &freq)==-1)
	{
		logprintf(LOG_ERR, "could not set receive carrier");
		logperror(LOG_ERR, __FUNCTION__);
		return(0);
	}
	return(1);
}

static int setup_timeout()
{
	lirc_t val, min_timeout, max_timeout;
	
	if(!(hw.features&LIRC_CAN_SET_REC_TIMEOUT))
	{
		return 1;
	}
	
	if(setup_max_space == 0)
	{
		return 1;
	}
	if(hw.ioctl_func(LIRC_GET_MIN_TIMEOUT, &min_timeout) == -1 ||
	   hw.ioctl_func(LIRC_GET_MAX_TIMEOUT, &max_timeout) == -1)
	{
		return 0;
	}
	if(setup_max_gap >= min_timeout && setup_max_gap <= max_timeout)
	{
		/* may help to detect end of signal faster */
		val = setup_max_gap;
	}
	else
	{
		/* keep timeout to a minimum */
		val = setup_max_space + 1;
		if(val < min_timeout)
		{
			val = min_timeout;
		}
		else if(val > max_timeout)
		{
			/* maximum timeout smaller than maximum possible
			   space, hmm */
			val = max_timeout;
		}
	}
	
	if(hw.ioctl_func(LIRC_SET_REC_TIMEOUT, &val) == -1)
	{
		logprintf(LOG_ERR, "could not set timeout");
		logperror(LOG_ERR, __FUNCTION__);
		return 0;
	}
	else
	{
		unsigned int enable = 1;
		hw.ioctl_func(LIRC_SET_REC_TIMEOUT_REPORTS, &enable);
	}
	return 1;
}

static int setup_filter()
{
	int ret1, ret2;
	lirc_t min_pulse_supported, max_pulse_supported;
	lirc_t min_space_supported, max_space_supported;
	
	if(!(hw.features&LIRC_CAN_SET_REC_FILTER))
	{
		return 1;
	}
	if(hw.ioctl_func(LIRC_GET_MIN_FILTER_PULSE,
			 &min_pulse_supported) == -1 ||
	   hw.ioctl_func(LIRC_GET_MAX_FILTER_PULSE,
			 &max_pulse_supported) == -1 ||
	   hw.ioctl_func(LIRC_GET_MIN_FILTER_SPACE,
			 &min_space_supported) == -1 ||
	   hw.ioctl_func(LIRC_GET_MAX_FILTER_SPACE,
			 &max_space_supported) == -1)
	{
		logprintf(LOG_ERR, "could not get filter range");
		logperror(LOG_ERR, __FUNCTION__);
	}
	
	if(setup_min_pulse > max_pulse_supported)
	{
		setup_min_pulse = max_pulse_supported;
	}
	else if(setup_min_pulse < min_pulse_supported)
	{
		setup_min_pulse = 0; /* disable filtering */
	}
	
	if(setup_min_space > max_space_supported)
	{
		setup_min_space = max_space_supported;
	}
	else if(setup_min_space < min_space_supported)
	{
		setup_min_space = 0; /* disable filtering */
	}
	
	ret1 = hw.ioctl_func(LIRC_SET_REC_FILTER_PULSE, &setup_min_pulse);
	ret2 = hw.ioctl_func(LIRC_SET_REC_FILTER_SPACE, &setup_min_space);
	if(ret1 == -1 || ret2 == -1)
	{
		if(hw.ioctl_func(LIRC_SET_REC_FILTER,
				 setup_min_pulse < setup_min_space ?
				 &setup_min_pulse:&setup_min_space) == -1)
		{
			logprintf(LOG_ERR, "could not set filter");
			logperror(LOG_ERR, __FUNCTION__);
			return 0;
		}
	}
	return 1;
}

static int setup_hardware()
{
	int ret = 1;
	
	if(hw.fd != -1 && hw.ioctl_func)
	{
		if((hw.features&LIRC_CAN_SET_REC_CARRIER) ||
		   (hw.features&LIRC_CAN_SET_REC_TIMEOUT) ||
		   (hw.features&LIRC_CAN_SET_REC_FILTER))
		{
			(void) hw.ioctl_func(LIRC_SETUP_START, NULL);
			ret = setup_frequency() &&
				setup_timeout() &&
				setup_filter();
			(void) hw.ioctl_func(LIRC_SETUP_END, NULL);
		}
	}
	return ret;
}

void config(void)
{
	FILE *fd;
	struct ir_remote *config_remotes;
	const char *filename = configfile;
	if(filename == NULL) filename = LIRCDCFGFILE;
	
	if(free_remotes!=NULL)
	{
		logprintf(LOG_ERR,"cannot read config file");
		logprintf(LOG_ERR,"old config is still in use");
		return;
	}
	fd=fopen(filename, "r");
	if(fd == NULL && errno == ENOENT && configfile == NULL)
	{
		/* try old lircd.conf location */
		int save_errno = errno;
		fd = fopen(LIRCDOLDCFGFILE, "r");
		if(fd != NULL)
		{
			filename = LIRCDOLDCFGFILE;
		}
		else
		{
			errno = save_errno;
		}
	}
	if(fd==NULL)
	{
		logprintf(LOG_ERR, "could not open config file '%s'",
			  filename);
		logperror(LOG_ERR, NULL);
		return;
	}
	configfile = filename;
	config_remotes=read_config(fd, configfile);
	fclose(fd);
	if(config_remotes==(void *) -1)
	{
		logprintf(LOG_ERR,"reading of config file failed");
	}
	else
	{
		LOGPRINTF(1,"config file read");
		if(config_remotes==NULL)
		{
			logprintf(LOG_WARNING,"config file contains no "
				  "valid remote control definition");
		}
		/* I cannot free the data structure
		   as they could still be in use */
		free_remotes=remotes;
		remotes=config_remotes;
		
		
		get_frequency_range(remotes,&setup_min_freq,&setup_max_freq);
		get_filter_parameters(remotes,&setup_max_gap,
				      &setup_min_pulse,&setup_min_space,
				      &setup_max_pulse,&setup_max_space);
		
		setup_hardware();
	}
}

void nolinger(int sock)
{
	static struct linger  linger = {0, 0};
	int lsize  = sizeof(struct linger);
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&linger, lsize);
}

void remove_client(int fd)
{
	int i;

	for(i=0;i<clin;i++)
	{
		if(clis[i]==fd)
		{
			shutdown(clis[i],2);
			close(clis[i]);
			logprintf(LOG_INFO,"removed client");
			
			clin--;
			if(!use_hw() && hw.deinit_func)
			{
				hw.deinit_func();
			}
			for(;i<clin;i++)
			{
				clis[i]=clis[i+1];
			}
			return;
		}
	}
	LOGPRINTF(1,"internal error in remove_client: no such fd");
}

void add_client(int sock)
{
	int fd;
	socklen_t clilen;
	struct sockaddr client_addr;
	int flags;

	clilen=sizeof(client_addr);
	fd=accept(sock,(struct sockaddr *)&client_addr,&clilen);
	if(fd==-1) 
	{
		logprintf(LOG_ERR,"accept() failed for new client");
		logperror(LOG_ERR,NULL);
		dosigterm(SIGTERM);
	};

	if(fd>=FD_SETSIZE || clin>=MAX_CLIENTS)
	{
		logprintf(LOG_ERR,"connection rejected");
		shutdown(fd,2);
		close(fd);
		return;
	}
	nolinger(fd);
	flags=fcntl(fd,F_GETFL,0);
	if(flags!=-1)
	{
		fcntl(fd,F_SETFL,flags|O_NONBLOCK);
	}
	if(client_addr.sa_family==AF_UNIX)
	{
		cli_type[clin]=CT_LOCAL;
		logprintf(LOG_NOTICE,"accepted new client on %s",lircdfile);
	}
	else if(client_addr.sa_family==AF_INET)
	{
		cli_type[clin]=CT_REMOTE;
		logprintf(LOG_NOTICE,"accepted new client from %s",
			  inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr));
	}
	else
	{
		cli_type[clin]=0; /* what? */
	}
	clis[clin]=fd;

	if(!use_hw())
	{
		if(hw.init_func)
		{
			if(!hw.init_func())
			{
				logprintf(LOG_WARNING,
					  "Failed to initialize hardware");
				/* Don't exit here, otherwise lirc
				 * bails out, and lircd exits, making
				 * it impossible to connect to when we
				 * have a device actually plugged
				 * in. */
			}
			else
			{
				setup_hardware();
			}
		}
	}
	clin++;
}

int add_peer_connection(char *server)
{
	char *sep;
	struct servent *service;
	
	if(peern<MAX_PEERS)
	{
		peers[peern]=malloc(sizeof(struct peer_connection));
		if(peers[peern]!=NULL)
		{
			gettimeofday(&peers[peern]->reconnect,NULL);
			peers[peern]->connection_failure = 0;
			sep=strchr(server,':');
			if(sep!=NULL)
			{
				*sep=0;sep++;
				peers[peern]->host=strdup(server);
				service=getservbyname(sep,"tcp");
				if(service)
				{
					peers[peern]->port=
						ntohs(service->s_port);
				}
				else
				{
					long p;
					char *endptr;
				
					p=strtol(sep,&endptr,10);
					if(!*sep || *endptr ||
					   p<1 || p>USHRT_MAX)
					{
						fprintf(stderr,
							"%s: bad port number \"%s\"\n",
							progname,sep);
						return(0);
					}
					
					peers[peern]->port=
						(unsigned short int) p;
				}
			}
			else
			{
				peers[peern]->host=strdup(server);
				peers[peern]->port=LIRC_INET_PORT;
			}
			if(peers[peern]->host==NULL)
			{
				fprintf(stderr, "%s: out of memory\n",progname);
			}
		}
		else
		{
			fprintf(stderr, "%s: out of memory\n",progname);
			return(0);
		}
		peers[peern]->socket=-1;
		peern++;
		return(1);
	}
	else
	{
		fprintf(stderr,"%s: too many client connections\n",
			progname);
	}
	return(0);
}

void connect_to_peers()
{
	int	i;
	struct	hostent *host;
	struct	sockaddr_in	addr;
	struct timeval now;
	int enable=1;
	
	gettimeofday(&now,NULL);
	for(i=0;i<peern;i++)
	{
		if(peers[i]->socket!=-1)
			continue;
		/* some timercmp() definitions don't work with <= */
		if(timercmp(&peers[i]->reconnect,&now,<))
		{
			peers[i]->socket=socket(AF_INET, SOCK_STREAM,0);
			host=gethostbyname(peers[i]->host);
			if(host==NULL)
			{
				logprintf(LOG_ERR,"name lookup failure "
					  "connecting to %s",peers[i]->host);
				peers[i]->connection_failure++;
				gettimeofday(&peers[i]->reconnect,NULL);
				peers[i]->reconnect.tv_sec+=
					5*peers[i]->connection_failure;
				close(peers[i]->socket);
				peers[i]->socket=-1;
				continue;
			}
			
			(void) setsockopt(peers[i]->socket,SOL_SOCKET,
					  SO_KEEPALIVE,&enable,sizeof(enable));
			
			addr.sin_family=host->h_addrtype;;
			addr.sin_addr=*((struct in_addr *)host->h_addr);
			addr.sin_port=htons(peers[i]->port);
			if(connect(peers[i]->socket,(struct sockaddr *) &addr,
				   sizeof(addr))==-1)
			{
				logprintf(LOG_ERR, "failure connecting to %s",
					  peers[i]->host);
				logperror(LOG_ERR, NULL);
				peers[i]->connection_failure++;
				gettimeofday(&peers[i]->reconnect,NULL);
				peers[i]->reconnect.tv_sec+=
					5*peers[i]->connection_failure;
				close(peers[i]->socket);
				peers[i]->socket=-1;
				continue;
			}
			logprintf(LOG_NOTICE, "connected to %s",
				  peers[i]->host);
			peers[i]->connection_failure=0;
		}
	}
}

int get_peer_message(struct peer_connection *peer)
{
	int length;
	char buffer[PACKET_SIZE+1];
	char *end;
	int	i;

	length=read_timeout(peer->socket,buffer,PACKET_SIZE,0);
	if(length)
	{
		buffer[length]=0;
		end=strrchr(buffer,'\n');
		if(end == NULL || end[1] != 0)
		{
			logprintf(LOG_ERR,"bad send packet: \"%s\"",buffer);
			/* remove clients that behave badly */
			return(0);
		}
		end++;	/* include the \n */
		end[0]=0;
		length=strlen(buffer);
		LOGPRINTF(1,"received peer message: \"%s\"",buffer);
		for(i=0;i<clin;i++)
		{
			/* don't relay messages to remote clients */
			if(cli_type[i]==CT_REMOTE)
				continue;
			LOGPRINTF(1,"writing to client %d",i);
			if(write_socket(clis[i],buffer,length)<length)
			{
				remove_client(clis[i]);
				i--;
			}			
		}
	}

	if(length==0) /* EOF: connection closed by client */
	{
		return(0);
	}
	return(1);
}

int start_server(mode_t permission,int nodaemon)
{
	struct sockaddr_un serv_addr;
	struct sockaddr_in serv_addr_in;
	struct stat s;
	int ret;
	int new=1;
	int fd;
	char err_mesg[50];

	/* create pid lockfile in /var/run */
	if((fd=open(pidfile,O_RDWR|O_CREAT,0644))==-1 ||
	   (pidf=fdopen(fd,"r+"))==NULL)
	{

		sprintf(err_mesg, "%s : can't open or create %s(%d)\n", pidfile, progname, fd);
		ZLOG_INFO(err_mesg);
		perror(progname);
//		exit(EXIT_FAILURE);
		return 0;
	}
	if(flock(fd,LOCK_EX|LOCK_NB)==-1)
	{
		pid_t otherpid;

		if(fscanf(pidf,"%d\n",&otherpid)>0)
		{
			sprintf(err_mesg,"%s: there seems to already be "
				"a lircd process with pid %d\n",
				progname,otherpid);
			ZLOG_INFO(err_mesg);
			sprintf(err_mesg, "%s: otherwise delete stale "
				"lockfile %s\n",progname,pidfile);
			ZLOG_INFO(err_mesg);
		}
		else
		{
			sprintf(err_mesg, "%s: invalid %s encountered\n",
				progname,pidfile);
			ZLOG_INFO(err_mesg);
		}
//		exit(EXIT_FAILURE);
		return 0;
	}
	(void) fcntl(fd,F_SETFD,FD_CLOEXEC);
	rewind(pidf);
	(void) fprintf(pidf,"%d\n",getpid());
	(void) fflush(pidf);
	(void) ftruncate(fileno(pidf),ftell(pidf));

	/* create socket*/
	sockfd=socket(AF_UNIX,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		sprintf(err_mesg,"%s: could not create socket\n",progname);
		ZLOG_INFO(err_mesg);
		perror(progname);
		goto start_server_failed0;
	}

	/*
	   get owner, permissions, etc.
	   so new socket can be the same since we
	   have to delete the old socket.
	*/
	ret=stat(lircdfile,&s);
	if(ret==-1 && errno!=ENOENT)
	{
		sprintf(err_mesg,"%s: could not get file information for %s\n",
			progname,lircdfile);
		ZLOG_INFO(err_mesg);
		perror(progname);
		goto start_server_failed1;
	}
	if(ret!=-1)
	{
		new=0;
		ret=unlink(lircdfile);
		if(ret==-1)
		{
			sprintf(err_mesg,"%s: could not delete %s\n",
				progname,lircdfile);
			ZLOG_INFO(err_mesg);
			perror(NULL);
			goto start_server_failed1;
		}
	}

	serv_addr.sun_family=AF_UNIX;
	strcpy(serv_addr.sun_path,lircdfile);
	ZLOG_INFO(lircdfile);
	if(bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))==-1)
	{
		sprintf(err_mesg,"%s: could not assign address to socket\n",
			progname);
		ZLOG_INFO(err_mesg);
		ZLOG_INFO(strerror(errno));
		perror(progname);
		goto start_server_failed1;
	}

	if(new ?
	   chmod(lircdfile,permission):
	   (chmod(lircdfile,s.st_mode)==-1 ||
	    chown(lircdfile,s.st_uid,s.st_gid)==-1)
	   )
	{
		sprintf(err_mesg,"%s: could not set file permissions\n",
			progname);
		ZLOG_INFO(err_mesg);
		perror(progname);
		goto start_server_failed1;
	}

	listen(sockfd,3);
	nolinger(sockfd);

//	if(useuinput)
//	{
//		uinputfd = setup_uinputfd(progname);
//	}
//	if(listen_tcpip)
//	{
//		int enable=1;

		/* create socket*/
//		sockinet=socket(PF_INET,SOCK_STREAM,IPPROTO_IP);
//		if(sockinet==-1)
//		{
//			fprintf(stderr,"%s: could not create TCP/IP socket\n",
//				progname);
//			perror(progname);
//			goto start_server_failed1;
//		}
//		(void) setsockopt(sockinet,SOL_SOCKET,SO_REUSEADDR,
//				  &enable,sizeof(enable));
//		serv_addr_in.sin_family=AF_INET;
//		serv_addr_in.sin_addr=address;
//		serv_addr_in.sin_port=htons(port);

//		if(bind(sockinet,(struct sockaddr *) &serv_addr_in,
//			sizeof(serv_addr_in))==-1)
//		{
//			fprintf(stderr,
//				"%s: could not assign address to socket\n",
//				progname);
//			perror(progname);
//			goto start_server_failed2;
//		}

//		listen(sockinet,3);
//		nolinger(sockinet);
//	}

//#ifdef USE_SYSLOG
//#ifdef DAEMONIZE
//	if(nodaemon)
//	{
//		openlog(syslogident,LOG_CONS|LOG_PID|LOG_PERROR,LIRC_SYSLOG);
//	}
//	else
//	{
//		openlog(syslogident,LOG_CONS|LOG_PID,LIRC_SYSLOG);
//	}
//#else
//	openlog(syslogident,LOG_CONS|LOG_PID|LOG_PERROR,LIRC_SYSLOG);
//#endif
//#else
	lf=fopen(logfile,"a");
	if(lf==NULL)
	{
		sprintf(err_mesg,"%s: could not open logfile %s\n",progname, logfile);
		ZLOG_INFO(err_mesg);
		free(lf);
		perror(progname);
		goto start_server_failed2;
	}
	gethostname(hostname,HOSTNAME_LEN);
//#endif
	ZLOG_INFO("Server socket started");
	LOGPRINTF(1,"started server socket");
	return 1;

 start_server_failed2:

	if(listen_tcpip)
	{
		close(sockinet);
	}
 start_server_failed1:

	close(sockfd);

 start_server_failed0:
	fclose(pidf);
	(void) unlink(pidfile);
	//	exit(EXIT_FAILURE);
	return(0);

}

void log_enable(int enabled)
{
	log_enabled = enabled;
}

#ifdef USE_SYSLOG
void logprintf(int prio, const char *format_str, ...)
{
	int save_errno = errno;
	va_list ap;
	
	if(!log_enabled) return;
	
	va_start(ap,format_str);
	vsyslog(prio, format_str, ap);
	va_end(ap);

	errno = save_errno;
}

void logperror(int prio,const char *s)
{
	if(!log_enabled) return;
	
	if((s)!=NULL) syslog(prio,"%s: %m\n",(char *) s);
	else syslog(prio,"%m\n");
}
#else
void logprintf(int prio,const char *format_str, ...)
{
	int save_errno = errno;
	va_list ap;  
	
	if(!log_enabled) return;
	
	if(lf)
	{
		time_t current;
		char *currents;
		
		current=time(&current);
		currents=ctime(&current);
		
		fprintf(lf,"%15.15s %s %s: ",currents+4,hostname,progname);
		va_start(ap,format_str);
		if(prio==LOG_WARNING) fprintf(lf,"WARNING: ");
		vfprintf(lf,format_str,ap);
		fputc('\n',lf);fflush(lf);
		va_end(ap);
	}
	if(!daemonized)
	{
		fprintf(stderr,"%s: ",progname);
		va_start(ap,format_str);
		if(prio==LOG_WARNING) fprintf(stderr,"WARNING: ");
		vfprintf(stderr,format_str,ap);
		fputc('\n',stderr);fflush(stderr);
		va_end(ap);
	}
	errno = save_errno;
}

void logperror(int prio,const char *s)
{
	if(!log_enabled) return;
	
	if(s!=NULL)
	{
		logprintf(prio,"%s: %s",s,strerror(errno));
	}
	else
	{
		logprintf(prio,"%s",strerror(errno));
	}
}
#endif

#ifdef DAEMONIZE

void daemonize(void)
{
	if(daemon(0,0)==-1)
	{
		logprintf(LOG_ERR,"daemon() failed");
		logperror(LOG_ERR,NULL);
		dosigterm(SIGTERM);
	}
	umask(0);
	rewind(pidf);
	(void) fprintf(pidf,"%d\n",getpid());
	(void) fflush(pidf);
	(void) ftruncate(fileno(pidf),ftell(pidf));
	daemonized=1;
}

#endif /* DAEMONIZE */

void sigalrm(int sig)
{
	alrm=1;
}

void dosigalrm(int sig)
{
	struct itimerval repeat_timer;
	
	if(repeat_remote->last_code!=repeat_code)
	{
		/* we received a different code from the original
		   remote control we could repeat the wrong code so
		   better stop repeating */
		if(repeat_fd != -1)
		{
			send_error(repeat_fd, repeat_message, "repeating interrupted\n");
		}
		
		repeat_remote=NULL;
		repeat_code=NULL;
		repeat_fd=-1;
		if(repeat_message!=NULL)
		{
			free(repeat_message);
			repeat_message=NULL;
		}
		if(!use_hw() && hw.deinit_func)
		{
			hw.deinit_func();
		}
		return;
	}
	if(repeat_code->next == NULL || (repeat_code->transmit_state != NULL && repeat_code->transmit_state->next == NULL))
	{
		repeat_remote->repeat_countdown--;
	}
	if(send_ir_ncode(repeat_remote,repeat_code) &&
	   repeat_remote->repeat_countdown>0)
	{
		repeat_timer.it_value.tv_sec=0;
		repeat_timer.it_value.tv_usec=repeat_remote->min_remaining_gap;
		repeat_timer.it_interval.tv_sec=0;
		repeat_timer.it_interval.tv_usec=0;
		
		setitimer(ITIMER_REAL,&repeat_timer,NULL);
		return;
	}
	repeat_remote=NULL;
	repeat_code=NULL;
	if(repeat_fd!=-1)
	{
		send_success(repeat_fd,repeat_message);
		free(repeat_message);
		repeat_message=NULL;
		repeat_fd=-1;
	}
	if(!use_hw() && hw.deinit_func)
	{
		hw.deinit_func();
	}
}

int parse_rc(int fd,char *message,char *arguments,struct ir_remote **remote,
	     struct ir_ncode **code, int *reps, int n, int *err)
{
	char *name=NULL,*command=NULL,*repeats,*end_ptr=NULL;

	*remote=NULL;
	*code=NULL;
	*err = 1;
	if(arguments==NULL) goto arg_check;

	name=strtok(arguments,WHITE_SPACE);
	if(name == NULL) goto arg_check;
	*remote=get_ir_remote(remotes,name);
	if(*remote==NULL)
	{
		return(send_error(fd,message,"unknown remote: \"%s\"\n",
				  name));
	}
	command=strtok(NULL,WHITE_SPACE);
	if(command == NULL) goto arg_check;
	*code=get_code_by_name(*remote,command);
	if(*code==NULL)
	{
		return(send_error(fd,message, "unknown command: \"%s\"\n",
				  command));
	}
	if(reps!=NULL)
	{
		repeats=strtok(NULL,WHITE_SPACE);
		if (repeats!=NULL)
		{
			*reps=strtol(repeats,&end_ptr,10);
			if (*end_ptr || *reps<0 )
			{
				return(send_error(fd,message,
						  "bad send packet\n"));
			}
			if (*reps > repeat_max)
			{
				return(send_error
				       (fd,message,
					"too many repeats: \"%d\" > \"%u\"\n",
					*reps, repeat_max));
			}
		}
		else
		{
			*reps=-1;
		}
	}
	if(strtok(NULL,WHITE_SPACE)!=NULL)
	{
		return(send_error(fd,message,"bad send packet\n"));
	}
 arg_check:
	if(n>0 && *remote==NULL)
	{
		return(send_error(fd,message,"remote missing\n"));
	}
	if(n>1 && *code==NULL)
	{
		return(send_error(fd,message,"code missing\n"));
	}
	*err = 0;
	return(1);
}

int send_success(int fd,char *message)
{
	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]) &&
	     write_socket_len(fd,protocol_string[P_END]))) return(0);
	return(1);
}

int send_error(int fd,char *message,char *format_str, ...)
{
	char lines[4],buffer[PACKET_SIZE+1];
	int i,n,len;
	va_list ap;  
	char *s1,*s2;
	
	va_start(ap,format_str);
	vsprintf(buffer,format_str,ap);
	va_end(ap);
	
	s1=strrchr(message,'\n');
	s2=strrchr(buffer,'\n');
	if(s1!=NULL) s1[0]=0;
	if(s2!=NULL) s2[0]=0;
	logprintf(LOG_ERR,"error processing command: %s",message);
	logprintf(LOG_ERR,"%s",buffer);
	if(s1!=NULL) s1[0]='\n';
	if(s2!=NULL) s2[0]='\n';

	n=0;
	len=strlen(buffer);
	for(i=0;i<len;i++) if(buffer[i]=='\n') n++;
	sprintf(lines,"%d\n",n);
	
	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_ERROR]) &&
	     write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,lines) &&
	     write_socket_len(fd,buffer) &&
	     write_socket_len(fd,protocol_string[P_END]))) return(0);
	return(1);
}

int send_remote_list(int fd,char *message)
{
	char buffer[PACKET_SIZE+1];
	struct ir_remote *all;
	int n,len;
	
	n=0;
	all=remotes;
	while(all)
	{
		n++;
		all=all->next;
	}

	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]))) return(0);
	
	if(n==0)
	{
		return(write_socket_len(fd,protocol_string[P_END]));
	}
	sprintf(buffer,"%d\n",n);
	len=strlen(buffer);
	if(!(write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,buffer))) return(0);

	all=remotes;
	while(all)
	{
		len=snprintf(buffer,PACKET_SIZE+1,"%s\n",all->name);
		if(len>=PACKET_SIZE+1)
		{
			len=sprintf(buffer,"name_too_long\n");
		}
		if(write_socket(fd,buffer,len)<len) return(0);
		all=all->next;
	}
	return(write_socket_len(fd,protocol_string[P_END]));
}

int send_remote(int fd,char *message,struct ir_remote *remote)
{
	struct ir_ncode *codes;
	char buffer[PACKET_SIZE+1];
	int n,len;

	n=0;
	codes=remote->codes;
	if(codes!=NULL)
	{
		while(codes->name!=NULL)
		{
			n++;
			codes++;
		}
	}

	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]))) return(0);
	if(n==0)
	{
		return(write_socket_len(fd,protocol_string[P_END]));
	}
	sprintf(buffer,"%d\n",n);
	if(!(write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,buffer))) return(0);

	codes=remote->codes;
	while(codes->name!=NULL)
	{
#ifdef __GLIBC__
		/* It seems you can't print 64-bit longs on glibc */
		
		len=snprintf(buffer,PACKET_SIZE+1,"%08lx%08lx %s\n",
			     (unsigned long) (codes->code>>32),
			     (unsigned long) (codes->code&0xFFFFFFFF),
			     codes->name);
#else
		len=snprintf(buffer,PACKET_SIZE,"%016llx %s\n",
			     codes->code,
			     codes->name);
#endif
		if(len>=PACKET_SIZE+1)
		{
			len=sprintf(buffer,"code_too_long\n");
		}
		if(write_socket(fd,buffer,len)<len) return(0);
		codes++;
	}
	return(write_socket_len(fd,protocol_string[P_END]));
}

int send_name(int fd,char *message,struct ir_ncode *code)
{
	char buffer[PACKET_SIZE+1];
	int len;

	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]) && 
	     write_socket_len(fd,protocol_string[P_DATA]))) return(0);
#ifdef __GLIBC__
	/* It seems you can't print 64-bit longs on glibc */
	
	len=snprintf(buffer,PACKET_SIZE+1,"1\n%08lx%08lx %s\n",
		     (unsigned long) (code->code>>32),
		     (unsigned long) (code->code&0xFFFFFFFF),
		     code->name);
#else
	len=snprintf(buffer,PACKET_SIZE,"1\n%016llx %s\n",
		     code->code,
		     code->name);
#endif
	if(len>=PACKET_SIZE+1)
	{
		len=sprintf(buffer,"1\ncode_too_long\n");
	}
	if(write_socket(fd,buffer,len)<len) return(0);
	return(write_socket_len(fd,protocol_string[P_END]));
}

int list(int fd,char *message,char *arguments)
{
	struct ir_remote *remote;
	struct ir_ncode *code;
	int err;

	if(parse_rc(fd, message, arguments,
		    &remote, &code, NULL, 0, &err) == 0) return 0;
	if(err) return 1;
	
	if(remote==NULL)
	{
		return(send_remote_list(fd,message));
	}
	if(code==NULL)
	{
		return(send_remote(fd,message,remote));
	}
	return(send_name(fd,message,code));
}

int set_transmitters(int fd,char *message,char *arguments)
{
	char *next_arg=NULL,*end_ptr;
	unsigned long next_tx_int;
	unsigned long next_tx_hex;
	unsigned int channels=0;
	int retval=0;
	int i;
	
	if(arguments==NULL) goto string_error;
	if(hw.send_mode==0) return(send_error(fd,message,"hardware does not "
					      "support sending\n"));
	if(hw.ioctl_func == NULL ||
	   !(hw.features & LIRC_CAN_SET_TRANSMITTER_MASK))
	{
		return(send_error(fd,message,"hardware does not support "
				  "multiple transmitters\n"));
	}
	
	next_arg=strtok(arguments,WHITE_SPACE);
	if (next_arg==NULL) goto string_error;
	do
	{
		next_tx_int=-1;
		next_tx_int = strtoul(next_arg,&end_ptr,10);
		if(*end_ptr || next_tx_int == 0 || (next_tx_int == ULONG_MAX && errno == ERANGE))
		{
			return(send_error(fd,message, "invalid argument\n"));
		}
		if(next_tx_int > MAX_TX)
		{
			return(send_error(fd, message, "cannot support more "
					  "than %d transmitters\n", MAX_TX));
		}
		next_tx_hex=1;
		for(i=1; i<next_tx_int; i++) next_tx_hex = next_tx_hex << 1;
		channels |= next_tx_hex;
	}while ((next_arg=strtok(NULL,WHITE_SPACE))!=NULL);
	
	retval = hw.ioctl_func(LIRC_SET_TRANSMITTER_MASK, &channels);
	if(retval<0)
	{
		return(send_error(fd, message, "error - could not set "
				  "transmitters\n"));
	}
	if (retval>0)
	{
		return(send_error(fd, message, "error - maximum of %d "
				  "transmitters\n", retval));
	}
	return(send_success(fd,message));
	
string_error:
	return(send_error(fd,message,"no arguments given\n"));
}

int simulate(int fd,char *message,char *arguments)
{
	int i;
	char *sim, *s, *space;
	
	if(!allow_simulate)
	{
		return send_error(fd, message,
				  "SIMULATE command is disabled\n");
	}
	if(arguments==NULL)
	{
		return send_error(fd, message, "no arguments given\n");
	}
	
	s=arguments;
	for(i=0; i<16; i++, s++)
	{
		if(!isxdigit(*s)) goto simulate_invalid_event;
	}
	if(*s != ' ')
	{
		goto simulate_invalid_event;
	}
	s++;
	if(*s == ' ')
	{
		goto simulate_invalid_event;
	}
	for(; *s != ' '; s++)
	{
		if(!isxdigit(*s)) goto simulate_invalid_event;
	}
	s++;
	space = strchr(s, ' ');
	if(space == NULL || space == s)
	{
		goto simulate_invalid_event;
	}
	s = space + 1;
	space = strchr(s, ' ');
	if(strlen(s) == 0 || space != NULL)
	{
		goto simulate_invalid_event;
	}
	
	sim = malloc(strlen(arguments) + 1 + 1);
	if(sim == NULL)
	{
		return send_error(fd, message, "out of memory\n");
	}
	strcpy(sim, arguments);
	strcat(sim, "\n");
	broadcast_message(sim);
	free(sim);
	
	return(send_success(fd,message));
 simulate_invalid_event:
	return send_error(fd, message, "invalid event\n");
	
}

int send_once(int fd,char *message,char *arguments)
{
	return(send_core(fd,message,arguments,1));
}

int send_start(int fd,char *message,char *arguments)
{
	return(send_core(fd,message,arguments,0));
}

int send_core(int fd,char *message,char *arguments,int once)
{
	struct ir_remote *remote;
	struct ir_ncode *code;
	struct itimerval repeat_timer;
	int reps;
	int err;
	
	if(hw.send_mode==0) return(send_error(fd,message,"hardware does not "
					      "support sending\n"));
	
	if(parse_rc(fd, message, arguments, &remote, &code,
		    once ? &reps:NULL, 2, &err) == 0) return(0);
	if(err) return 1;
	
	if(once)
	{
		if(repeat_remote!=NULL)
		{
			return(send_error(fd,message,"busy: repeating\n"));
		}
	}
	else
	{
		if(repeat_remote!=NULL)
		{
			return(send_error(fd,message,"already repeating\n"));
		}
	}
	if(has_toggle_mask(remote))
	{
		remote->toggle_mask_state=0;
	}
	if(has_toggle_bit_mask(remote))
	{
		remote->toggle_bit_mask_state =
			(remote->toggle_bit_mask_state^remote->toggle_bit_mask);
	}
	code->transmit_state = NULL;
	if(!send_ir_ncode(remote,code))
	{
		return(send_error(fd,message,"transmission failed\n"));
	}
	gettimeofday(&remote->last_send,NULL);
	remote->last_code = code;
	if(once)
	{
		remote->repeat_countdown=max(remote->repeat_countdown,reps);
	}
	else
	{
		/* you've been warned, now we have a limit */
		remote->repeat_countdown = repeat_max;
	}
	if(remote->repeat_countdown>0 || code->next != NULL)
	{
		repeat_remote=remote;
		repeat_code=code;
		repeat_timer.it_value.tv_sec=0;
		repeat_timer.it_value.tv_usec=
			remote->min_remaining_gap;
		repeat_timer.it_interval.tv_sec=0;
		repeat_timer.it_interval.tv_usec=0;
		if(once)
		{
			repeat_message=strdup(message);
			if(repeat_message==NULL)
			{
				repeat_remote=NULL;
				repeat_code=NULL;
				return(send_error(fd,message,
						  "out of memory\n"));
			}
			repeat_fd=fd;
		}
		else if(!send_success(fd,message))
		{
			repeat_remote=NULL;
			repeat_code=NULL;
			return(0);
		}
		setitimer(ITIMER_REAL,&repeat_timer,NULL);
		return(1);
	}
	else
	{
		return(send_success(fd,message));
	}
}

int send_stop(int fd,char *message,char *arguments)
{
	struct ir_remote *remote;
	struct ir_ncode *code;
	struct itimerval repeat_timer;
	int err;
	
	if(parse_rc(fd, message, arguments,
		    &remote, &code, NULL, 0, &err) == 0) return 0;
	if(err) return 1;
	
	if(repeat_remote && repeat_code)
	{
		int done;
		if(remote &&
		   strcasecmp(remote->name, repeat_remote->name) != 0)
		{
			return(send_error(fd, message, "specified remote "
					  "does not match\n"));
		}
		if(code && strcasecmp(code->name, repeat_code->name) != 0)
		{
			return(send_error(fd, message, "specified code "
					  "does not match\n"));
		}

		done = repeat_max - repeat_remote->repeat_countdown;
		if(done < repeat_remote->min_repeat)
		{
			/* we still have some repeats to do */
			repeat_remote->repeat_countdown =
				repeat_remote->min_repeat - done;
			return(send_success(fd,message));
		}
		repeat_timer.it_value.tv_sec=0;
		repeat_timer.it_value.tv_usec=0;
		repeat_timer.it_interval.tv_sec=0;
		repeat_timer.it_interval.tv_usec=0;
		
		setitimer(ITIMER_REAL,&repeat_timer,NULL);
		
		repeat_remote->toggle_mask_state=0;
		repeat_remote=NULL;
		repeat_code=NULL;
		/* clin!=0, so we don't have to deinit hardware */
		alrm=0;
		return(send_success(fd,message));
	}
	else
	{
		return(send_error(fd,message,"not repeating\n"));
	}
}

int version(int fd,char *message,char *arguments)
{
	char buffer[PACKET_SIZE+1];

	if(arguments!=NULL)
	{
		return(send_error(fd,message,"bad send packet\n"));
	}
	sprintf(buffer,"1\n%s\n",VERSION);
	if(!(write_socket_len(fd,protocol_string[P_BEGIN]) &&
	     write_socket_len(fd,message) &&
	     write_socket_len(fd,protocol_string[P_SUCCESS]) &&
	     write_socket_len(fd,protocol_string[P_DATA]) &&
	     write_socket_len(fd,buffer) &&
	     write_socket_len(fd,protocol_string[P_END]))) return(0);
	return(1);
}

int get_command(int fd)
{
	int length;
	char buffer[PACKET_SIZE+1],backup[PACKET_SIZE+1];
	char *end;
	int packet_length,i;
	char *directive;

	length=read_timeout(fd,buffer,PACKET_SIZE,0);
	packet_length=0;
	while(length>packet_length)
	{
		buffer[length]=0;
		end=strchr(buffer,'\n');
		if(end==NULL)
		{
			logprintf(LOG_ERR,"bad send packet: \"%s\"",buffer);
			/* remove clients that behave badly */
			return(0);
		}
		end[0]=0;
		LOGPRINTF(1,"received command: \"%s\"",buffer);
		packet_length=strlen(buffer)+1;

		strcpy(backup,buffer);strcat(backup,"\n");
		
		/* remove DOS line endings */
		end = strrchr(buffer, '\r');
		if(end && end[1] == 0) *end = 0;
		
		directive=strtok(buffer,WHITE_SPACE);
		if(directive==NULL)
		{
			if(!send_error(fd,backup,"bad send packet\n"))
				return(0);
			goto skip;
		}
		for(i=0;directives[i].name!=NULL;i++)
		{
			if(strcasecmp(directive,directives[i].name)==0)
			{
				if(!directives[i].
				   function(fd,backup,strtok(NULL,"")))
					return(0);
				goto skip;
			}
		}
		
		if(!send_error(fd,backup,"unknown directive: \"%s\"\n",
			       directive))
			return(0);
	skip:
		if(length>packet_length)
		{
			int new_length;

			memmove(buffer,buffer+packet_length,
				length-packet_length+1);
			if(strchr(buffer,'\n')==NULL)
			{
				new_length=read_timeout(fd,buffer+length-
							packet_length,
							PACKET_SIZE-
							(length-
							 packet_length),5);
				if(new_length>0)
				{
					length=length-packet_length+new_length;
				}
				else
				{
					length=new_length;
				}
			}
			else
			{
				length-=packet_length;
			}
			packet_length=0;
		}
	}

	if(length==0) /* EOF: connection closed by client */
	{
		return(0);
	}
	return(1);
}

void free_old_remotes()
{
	struct ir_remote *scan_remotes,*found;
	struct ir_ncode *code;
	const char *release_event;
	const char *release_remote_name;
	const char *release_button_name;
	
	if(decoding ==free_remotes) return;
	
	release_event = release_map_remotes(free_remotes, remotes,
					    &release_remote_name,
					    &release_button_name);
	if(release_event != NULL)
	{
		input_message(release_event,
			      release_remote_name,
			      release_button_name,
			      0,
			      1);
	}
	if(last_remote!=NULL)
	{
		if(is_in_remotes(free_remotes, last_remote))
		{
			logprintf(LOG_INFO, "last_remote found");
			found=get_ir_remote(remotes,last_remote->name);
			if(found!=NULL)
			{
				code=get_code_by_name(found,last_remote->last_code->name);
				if(code!=NULL)
				{
					found->reps=last_remote->reps;
					found->toggle_bit_mask_state=last_remote->toggle_bit_mask_state;
					found->min_remaining_gap=last_remote->min_remaining_gap;
					found->max_remaining_gap=last_remote->max_remaining_gap;
					found->last_send=last_remote->last_send;
					last_remote=found;
					last_remote->last_code=code;
					logprintf(LOG_INFO, "mapped last_remote");
				}
			}
		}
		else
		{
			last_remote=NULL;
		}
	}
	/* check if last config is still needed */
	found=NULL;
	if(repeat_remote!=NULL)
	{
		scan_remotes=free_remotes;
		while(scan_remotes!=NULL)
		{
			if(repeat_remote==scan_remotes)
			{
				found=repeat_remote;
				break;
			}
			scan_remotes=scan_remotes->next;
		}
		if(found!=NULL)
		{
			found=get_ir_remote(remotes,repeat_remote->name);
			if(found!=NULL)
			{
				code=get_code_by_name(found,repeat_code->name);
				if(code!=NULL)
				{
					struct itimerval repeat_timer;

					repeat_timer.it_value.tv_sec=0;
					repeat_timer.it_value.tv_usec=0;
					repeat_timer.it_interval.tv_sec=0;
					repeat_timer.it_interval.tv_usec=0;

					found->last_code=code;
					found->last_send=repeat_remote->last_send;
					found->toggle_bit_mask_state=repeat_remote->toggle_bit_mask_state;
					found->min_remaining_gap=repeat_remote->min_remaining_gap;
					found->max_remaining_gap=repeat_remote->max_remaining_gap;

					setitimer(ITIMER_REAL,&repeat_timer,&repeat_timer);
					/* "atomic" (shouldn't be necessary any more) */
					repeat_remote=found;
					repeat_code=code;
					/* end "atomic" */
					setitimer(ITIMER_REAL,&repeat_timer,NULL);
					found=NULL;
				}
			}
			else
			{
				found=repeat_remote;
			}
		}
	}
	if(found==NULL && decoding!=free_remotes)
	{
		free_config(free_remotes);
		free_remotes=NULL;
	}
	else
	{
		LOGPRINTF(1,"free_remotes still in use");
	}
}

void input_message(const char *message, const char *remote_name,
		   const char *button_name, int reps, int release)
{
	const char *release_message;
	const char *release_remote_name;
	const char *release_button_name;

	release_message = check_release_event(&release_remote_name,
					      &release_button_name);
	if(release_message)
	{
		input_message(release_message, release_remote_name,
			      release_button_name, 0, 1);
	}
	
	if(!release || userelease)
	{
		broadcast_message(message);
	}
	
#ifdef __linux__
	if(uinputfd != -1)
	{
		linux_input_code input_code;
		
		if(reps < 2 &&
		   get_input_code(button_name, &input_code) != -1)
		{
			struct input_event event;
			
			memset(&event, 0, sizeof(event));
			event.type = EV_KEY;
			event.code = input_code;
			event.value = release ? 0 : (reps>0 ? 2:1);
			if(write(uinputfd, &event, sizeof(event)) != sizeof(event))
			{
				logprintf(LOG_ERR, "writing to uinput failed");
				logperror(LOG_ERR, NULL);
			}

			/* Need to write sync event */
			memset(&event, 0, sizeof(event));
			event.type = EV_SYN;
			event.code = SYN_REPORT;
			event.value = 0;
			if(write(uinputfd, &event, sizeof(event)) != sizeof(event))
			{
				logprintf(LOG_ERR, "writing EV_SYN to uinput failed");
				logperror(LOG_ERR, NULL);
			}
		}
	}
#endif
}

void broadcast_message(const char *message)
{
	int len,i;
	
	len=strlen(message);
	
	for (i=0; i<clin; i++)
	{
		LOGPRINTF(1,"writing to client %d",i);
		if(write_socket(clis[i],message,len)<len)
		{
			remove_client(clis[i]);
			i--;
		}			
	}
}

int waitfordata(long maxusec)
{
	fd_set fds;
	int maxfd,i,ret,reconnect;
	struct timeval tv,start,now,timeout,release_time;

	while(1)
	{
		do{
			/* handle signals */
//			if(term)
//			{
//				dosigterm(termsig);
//				/* never reached */
//			}
//			if(hup)
//			{
//				dosighup(SIGHUP);
//				hup=0;
//			}
//			if(alrm)
//			{
//				dosigalrm(SIGALRM);
//				alrm=0;
//			}
			FD_ZERO(&fds);
			FD_SET(sockfd,&fds);

			maxfd=sockfd;
			if(listen_tcpip)
			{
				FD_SET(sockinet,&fds);
				maxfd=max(maxfd,sockinet);
			}
			if(use_hw() && hw.rec_mode!=0 && hw.fd!=-1)
			{
				FD_SET(hw.fd,&fds);
				maxfd=max(maxfd,hw.fd);
			}
			
			for(i=0;i<clin;i++)
			{
				/* Ignore this client until codes have been
				   sent and it will get an answer. Otherwise
				   we could mix up answer packets and send
				   them back in the wrong order.*/
				if(clis[i]!=repeat_fd)
				{
					FD_SET(clis[i],&fds);
					maxfd=max(maxfd,clis[i]);
				}
			}
			timerclear(&tv);
			reconnect=0;
			for(i=0;i<peern;i++)
			{
				if(peers[i]->socket!=-1)
				{
					FD_SET(peers[i]->socket,&fds);
					maxfd=max(maxfd,peers[i]->socket);
				}
				else if(timerisset(&tv))
				{
					if(timercmp
					   (&tv,&peers[i]->reconnect,>))
					{
						tv=peers[i]->reconnect;
					}
				}
				else
				{
					tv=peers[i]->reconnect;
				}
			}
			if(timerisset(&tv))
			{
				gettimeofday(&now,NULL);
				if(timercmp(&now,&tv,>))
				{
					timerclear(&tv);
				}
				else
				{
					timersub(&tv,&now,&start);
					tv=start;
				}
				reconnect=1;
			}
			gettimeofday(&start,NULL);
			if(maxusec>0)
			{
				tv.tv_sec= maxusec / 1000000;
				tv.tv_usec=maxusec % 1000000;
			}
			if(hw.fd == -1 && use_hw())
			{
				/* try to reconnect */
				timerclear(&timeout);
				timeout.tv_sec = 1;
				
				if(timercmp(&tv, &timeout, >) ||
				   (!reconnect && !timerisset(&tv)))
				{
					tv = timeout;
				}
			}
			get_release_time(&release_time);
			if(timerisset(&release_time))
			{
				gettimeofday(&now,NULL);
				if(timercmp(&now,&release_time,>))
				{
					timerclear(&tv);
				}
				else
				{
					struct timeval gap;
					
					timersub(&release_time, &now, &gap);
					if(!(timerisset(&tv) || reconnect) ||
					   timercmp(&tv, &gap, >))
					{
						tv = gap;
					}
				}
			}
#ifdef SIM_REC
			ret=select(maxfd+1,&fds,NULL,NULL,NULL);
#else
			if(timerisset(&tv) || timerisset(&release_time) ||
			   reconnect)
			{
				ret=select(maxfd+1,&fds,NULL,NULL,&tv);
			}
			else
			{
				ret=select(maxfd+1,&fds,NULL,NULL,NULL);
			}
#endif
			if(ret==-1 && errno!=EINTR)
			{
				logprintf(LOG_ERR,"select() failed");
				logperror(LOG_ERR,NULL);
				raise(SIGTERM);
				continue;
			}
			gettimeofday(&now,NULL);
			if(timerisset(&release_time) &&
			   timercmp(&now, &release_time, >))
			{
				const char *release_message;
				const char *release_remote_name;
				const char *release_button_name;
				
				release_message = trigger_release_event
					(&release_remote_name,
					 &release_button_name);
				if(release_message)
				{
					input_message(release_message,
						      release_remote_name,
						      release_button_name,
						      0,
						      1);
				}
			}
			if(free_remotes!=NULL)
			{
				free_old_remotes();
			}
			if(maxusec>0)
			{
				if(ret==0)
				{
					return(0);
				}
				if(time_elapsed(&start,&now)>=maxusec)
				{
					return(0);
				}
				else
				{
					maxusec-=time_elapsed(&start,&now);
				}
				
			}
			if(reconnect)
			{
				connect_to_peers();
			}
		}
		while(ret==-1 && errno==EINTR);
		
		if(hw.fd == -1 &&
		   use_hw() &&
		   hw.init_func)
		{
			log_enable(0);
			hw.init_func();
			setup_hardware();
			log_enable(1);
		}
		for(i=0;i<clin;i++)
		{
			if(FD_ISSET(clis[i],&fds))
			{
				FD_CLR(clis[i],&fds);
				if(get_command(clis[i])==0)
				{
					remove_client(clis[i]);
					i--;
				}
			}
		}
		for(i=0;i<peern;i++)
		{
			if(peers[i]->socket!=-1 &&
			   FD_ISSET(peers[i]->socket,&fds))
			{
				if(get_peer_message(peers[i])==0)
				{
					shutdown(peers[i]->socket,2);
					close(peers[i]->socket);
					peers[i]->socket=-1;
					peers[i]->connection_failure = 1;
					gettimeofday(&peers[i]->reconnect,NULL);
					peers[i]->reconnect.tv_sec+=5;
				}
			}
		}

		if(FD_ISSET(sockfd,&fds))
		{
			LOGPRINTF(1,"registering local client");
			add_client(sockfd);
		}
		if(listen_tcpip && FD_ISSET(sockinet,&fds))
		{
			LOGPRINTF(1,"registering inet client");
			add_client(sockinet);
		}

                if(use_hw() && hw.rec_mode!=0 && hw.fd!=-1 &&
		   FD_ISSET(hw.fd,&fds))
                {
			register_input();
                        /* we will read later */
			return(1);
                }
	}
}

void loop()
{
	char *message;
	
	logprintf(LOG_NOTICE,"lircd(%s) ready, using %s", hw.name, lircdfile);
	while(1)
	{
		(void) waitfordata(0);
		if(!hw.rec_func) continue;
		message=hw.rec_func(remotes);
		
		if(message!=NULL)
		{
			const char *remote_name;
			const char *button_name;
			int reps;
			
			if(hw.ioctl_func &&
			   (hw.features&LIRC_CAN_NOTIFY_DECODE))
			{
				hw.ioctl_func(LIRC_NOTIFY_DECODE, NULL);
			}
			
			get_release_data(&remote_name, &button_name, &reps);

			input_message(message, remote_name, button_name,
				      reps, 0);
		}
	}
}


int startDaemon(int argc,char **argv)
{

	struct sigaction act;
	int nodaemon=0;
	mode_t permission=S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH;
	char *device=NULL;

	address.s_addr = htonl(INADDR_ANY);
	hw_choose_driver(NULL);


	/* device=optarg; */
	device = argv[2];


	if(device!=NULL)
	{
		hw.device=device;
	}

	if(strcmp(hw.name, "null")==0 && peern==0)
	{
		ZLOG_INFO(progname);
		ZLOG_INFO(": there's no hardware I can use and "
				"no peers are specified\n");
		return(EXIT_FAILURE);
	}

	if(hw.device!=NULL && strcmp(hw.device, lircdfile)==0)
	{
		ZLOG_INFO(progname);
		ZLOG_INFO("%s: refusing to connect to myself\n");

		ZLOG_INFO(progname);
		ZLOG_INFO(lircdfile);
		ZLOG_INFO("%s: device and output must not be the "
			"same file: %s\n");
		return(EXIT_FAILURE);
	}
	
/*	signal(SIGPIPE,SIG_IGN); */



/*	act.sa_handler=sigterm;
	sigfillset(&act.sa_mask);
	act.sa_flags=SA_RESTART;           /* don't fiddle with EINTR */
/*	sigaction(SIGTERM,&act,NULL);
	sigaction(SIGINT,&act,NULL);

	act.sa_handler=sigalrm;
	sigemptyset(&act.sa_mask);
	act.sa_flags=SA_RESTART;           /* don't fiddle with EINTR */
/*	sigaction(SIGALRM,&act,NULL);
*/
	remotes=NULL;
	config();                          /* read config file */
/*
	act.sa_handler=sighup;
	sigemptyset(&act.sa_mask);
	act.sa_flags=SA_RESTART;           /* don't fiddle with EINTR */
/*	sigaction(SIGHUP,&act,NULL);
*/

	ZLOG_INFO("AVANT start_server");
	if (start_server(permission,nodaemon) == 1);
	{
		ZLOG_INFO("APRES start_server");
		loop();
	}

	return(0);
}
