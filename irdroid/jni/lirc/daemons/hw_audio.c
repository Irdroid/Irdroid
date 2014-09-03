/*      $Id: hw_audio.c,v 5.9 2010/04/11 18:50:38 lirc Exp $      */

/****************************************************************************
 ** hw_audio.c **************************************************************
 ****************************************************************************
 *
 * routines for using a IR receiver in microphone input using portaudio library
 * 
 * Copyright (C) 1999 Christoph Bartelmus <lirc@bartelmus.de>
 * Copyright (C) 2001, 2002 Pavel Machek <pavel@ucw.cz>
 * Copyright (C) 2002 Matthias Ringwald <ringwald@inf.ethz.ch>
 *
 * Distribute under GPL version 2 or later.
 *
 * Using ... hardware ... 
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <termios.h>
#ifdef __APPLE__
#include <util.h>
#else
/* #include <pty.h> */
#endif

#include "hardware.h"
#include "ir_remote.h"
#include "lircd.h"
#include "receive.h"
#include "transmit.h"
#include "hw_default.h"

/* static int ptyfd;*/        /* the pty */

/* PortAudio Includes */
#include <portaudio.h>

#define DEFAULT_SAMPLERATE (48000)
#define NUM_CHANNELS           (2)
#define PI           (3.141592654)

/* Select sample format. */
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;

typedef struct
{
	int				lastFrames[3];
	int				lastSign;
	int				pulseSign;
	unsigned int			lastCount;
	lirc_t				carrierFreq;
	/* position the sine generator is in */
	double				carrierPos;
	/* length of the remaining signal is stored here when the
	   callback exits */
	double				remainingSignal;
	/* 1 = pulse, 0 = space */
	int				signalPhase;
	int				signaledDone;
	int				samplesToIgnore;
	int				samplerate;
}
paTestData;

static PaStream *stream;


extern struct ir_remote *repeat_remote;
extern struct rbuf rec_buffer;

/* static char ptyName[256]; */
static int master;
static int sendPipe[2];      /* signals are written from audio_send
				and read from the callback */
static int completedPipe[2]; /* a byte is written here when the
				callback has processed all signals */
static int outputLatency;
static int inDevicesPrinted = 0;
static int outDevicesPrinted = 0;

static void addCode( lirc_t data)
{
	write( master, &data, sizeof( lirc_t ) );
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/

static int recordCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* outTime,
                           PaStreamCallbackFlags status,
                           void *userData )
{
	paTestData *data = (paTestData*)userData;
	SAMPLE *rptr = (SAMPLE*)inputBuffer;
	long i;

	SAMPLE	*myPtr = rptr;

	unsigned int time;
	int	diff;

	SAMPLE* outptr = (SAMPLE*)outputBuffer;
	int     out;
	double  currentSignal = data->remainingSignal;
	lirc_t  signal;

	/* Prevent unused variable warnings. */
	(void) outTime;

	if (status & paOutputUnderflow)
		logprintf(LOG_WARNING, "Output underflow %s", hw.device);
	if (status & paInputOverflow)
		logprintf(LOG_WARNING, "Input overflow %s", hw.device);


	for ( i=0; i < framesPerBuffer; i++, myPtr++)
	{
		/* check if we have to ignore this sample */
		if (data->samplesToIgnore)
		{
			*myPtr = 128;
			data->samplesToIgnore--;
		}

		/* New Algo */
		diff = abs(data->lastFrames[0] - *myPtr);
		if ( diff > 100)
		{
			if (data->pulseSign == 0)
			{
				/* we got the first signal, this is a PULSE */
				if ( *myPtr > data->lastFrames[0] )
				{
					data->pulseSign = 1;
				} 
				else
				{
					data->pulseSign = -1;
				}
			}

			if (data->lastCount > 0)
			{
				if ( *myPtr > data->lastFrames[0] && data->lastSign <= 0)
				{
					/* printf("CHANGE ++ "); */
					data->lastSign = 1;
	
					time = data->lastCount * 1000000 / data->samplerate;
					if (data->lastSign == data->pulseSign)
					{
						addCode( time );
						/* printf("Pause: %d us, %d \n", time, data->lastCount); */
					}
					else
					{
						addCode( time | PULSE_BIT );
						/* printf("Pulse: %d us, %d \n", time, data->lastCount); */
					}
					data->lastCount = 0;
				}
				
				else if (  *myPtr < data->lastFrames[0] && data->lastSign >= 0)
				{
					/* printf("CHANGE -- "); */
					data->lastSign = -1;
	
					time = data->lastCount * 1000000 / data->samplerate;
					if (data->lastSign == data->pulseSign)
					{
						/* printf("Pause: %d us, %d \n", time, data->lastCount); */
						addCode( time );
					}
					else
					{
						/* printf("Pulse: %d us, %d \n", time, data->lastCount); */
						addCode( time | PULSE_BIT);
					}data->lastCount = 0;
				}
			}
		}

		if ( data->lastCount < 100000 )
		{
			data->lastCount++;
		}

		data->lastFrames[0] = data->lastFrames[1];
		data->lastFrames[1] = *myPtr;
		
		/* skip 2. channel */
		if (NUM_CHANNELS == 2)
			myPtr++;
	}

	/* generate output */
	for (i = 0; i < framesPerBuffer; i++)
	{
		if (currentSignal <= 0.0) /* last signal we sent went out */
		{
			/* try to read a new signal, non blocking */
			if (read(sendPipe[0], &signal, sizeof(signal)) > 0)
			{
				if (data->signaledDone)
				{
					/* first one sent is the
					   carrier frequency */
					data->carrierFreq = signal;
					data->signaledDone = 0;
				}
				else
				{
					/* when a new signal is read,
					   add it */
					currentSignal += signal;
					/* invert the phase */
					data->signalPhase =
						data->signalPhase ? 0 : 1;
				}

				/* when transmitting, ignore input
				   samples for one second */
				data->samplesToIgnore = data->samplerate;
			}
			else
			{
				/* no more signals, reset phase */
				data->signalPhase = 0;
				/* signal that we have written all
				   signals */
				if (!data->signaledDone)
				{
					char done = 0;
					data->signaledDone = 1;
					(void) write(completedPipe[1], &done,
						     sizeof(done));
				}
			}
		}

		if (currentSignal > 0.0)
		{
			if (data->signalPhase)
			{
				/* write carrier */
				out = rint(sin(data->carrierPos / 
					       (180.0 / PI)) * 127.0 + 128.0);
			}
			else
			{
				out = 128;
			}

			/* one channel is inverted, so both channels
			   can be used to double the voltage */
			*outptr++ = out; 
			if (NUM_CHANNELS == 2)
				*outptr++ = 256 - out;

			/* subtract how much of the current signal was sent */
			currentSignal -= 1000000.0 / data->samplerate;
		}
		else
		{
			*outptr++ = 128;
			if (NUM_CHANNELS == 2)
				*outptr++ = 128;
		}

		/* increase carrier position */
		/* carrier frequency is halved */
		data->carrierPos += (double)data->carrierFreq /
			data->samplerate * 360.0 / 2.0;
		
		if (data->carrierPos >= 360.0)
			data->carrierPos -= 360.0;
	}

	/* save how much we still have to write */
	data->remainingSignal = currentSignal;

	return 0;
}

/*
  decoding stuff
*/

#define BUFSIZE 20
#define SAMPLE 47999


lirc_t audio_readdata(lirc_t timeout)
{
	lirc_t data;
	int ret;

	if (!waitfordata((long) timeout))
		return 0;

	ret=read(hw.fd,&data,sizeof(data));
	if(ret!=sizeof(data))
	{
		LOGPRINTF(1,"error reading from lirc");
		LOGPERROR(1,NULL);
		raise(SIGTERM);
		return 0;
	}
	return(data);
}

int audio_send(struct ir_remote *remote,struct ir_ncode *code)
{
	int     length;
	lirc_t* signals;
	int     flags;
	char    completed;
	lirc_t  freq;
	static  lirc_t prevfreq = 0;

	if(!init_send(remote, code)) 
		return 0;

	length = send_buffer.wptr;
	signals = send_buffer.data;

	if (length <= 0 || signals == NULL)
	{
		LOGPRINTF(1, "nothing to send");
		return 0;
	}

	/* set completed pipe to non blocking */
	flags = fcntl(completedPipe[0], F_GETFL, 0);
	fcntl(completedPipe[0], F_SETFL, flags | O_NONBLOCK);

	/* remove any unwanted completed bytes */
	while (read(completedPipe[0], &completed, sizeof(completed)) == 1);

	/* set completed pipe to blocking */
	fcntl(completedPipe[0], F_SETFL, flags & ~O_NONBLOCK);

	/* write carrier frequency */
	freq = remote->freq ? remote->freq : DEFAULT_FREQ;
	write(sendPipe[1], &freq, sizeof(freq));
	if (freq != prevfreq)
	{
		prevfreq = freq;
		logprintf(LOG_INFO, "Using carrier frequency %i", freq);
	}

	/* write signals to sendpipe */
	if (write(sendPipe[1], signals, length * sizeof(lirc_t)) == -1)
	{
		logprintf(LOG_ERR,"write failed");
		logperror(LOG_ERR,"write()");
		return 0;
	}

	/* wait for the callback to signal us that all signals are written */
	read(completedPipe[0], &completed, sizeof(completed));

	return 1;
}

static void audio_parsedevicestr(char* api, char* device, int* rate, double* latency)
{
	int ret;

	/* empty device string means default */
	if (strlen(hw.device))
	{
		/* device string is api:device[@rate] or @rate */
		ret = sscanf(hw.device, "%1023[^:]:%1023[^@]@%i:%lf",
			     api, device, rate, latency);

		if (ret == 2 || *rate <= 0)
			*rate = DEFAULT_SAMPLERATE;

		if (ret <= 3)
			*latency = -1.0;

		if (ret >= 2)
			return;

		/* check for @rate:latency */
		ret = sscanf(hw.device, "@%i:%lf", rate, latency);
		if (ret >= 1)
		{
			api[0] = 0;
			device[0] = 0;
			if (*rate <= 0)
				*rate = DEFAULT_SAMPLERATE;

			if (ret == 1)
				*latency = -1.0;

			return;
		}

		logprintf(LOG_ERR, "malformed device string %s, "
			  "syntax is api:device[@rate[:latency]] or @rate[:latency]", hw.device);
	}

	api[0] = 0;
	device[0] = 0;
	*rate = DEFAULT_SAMPLERATE;
	*latency = -1.0;
}

static void audio_choosedevice(PaStreamParameters* streamparameters, int input,
			char* api, char* device, double latency)
{
	char* direction = input ? "input" : "output";
	int   chosendevice = -1;
	int   i;
	int   nrdevices = Pa_GetDeviceCount();
	const PaDeviceInfo*  deviceinfo;
	const PaHostApiInfo* hostapiinfo;
	const char* devicetype = "custom";
	const char* latencytype = "custom";

	for (i = 0; i < nrdevices; i++)
	{
		deviceinfo = Pa_GetDeviceInfo(i);

		/* check if device can do input or output if
			 we need it */
		if ((deviceinfo->maxOutputChannels >= NUM_CHANNELS &&
				 !input) ||
				(deviceinfo->maxInputChannels >= NUM_CHANNELS &&
				 input))
		{
			hostapiinfo = Pa_GetHostApiInfo
				(deviceinfo->hostApi);
			/*check if this matches the custom device*/
			if (strlen(api) && strlen(device))
			{
				if (strcmp(api, hostapiinfo->name) == 0 &&
						strcmp(device, deviceinfo->name) == 0)
					chosendevice = i;
			}

			/*allow devices to be printed to the log twice*/
			/*once for input, once for output*/
			if ((!inDevicesPrinted && input) || (!outDevicesPrinted && !input))
			{
				logprintf(LOG_INFO, "Found %s device %i %s:%s",
						direction, i, hostapiinfo->name, deviceinfo->name);
			}
		}
	}

	if (input)
		inDevicesPrinted = 1;
	else
		outDevicesPrinted = 1;

	if (chosendevice == -1)
	{
		devicetype = "default";

		if (strlen(api) && strlen(device))
			logprintf(LOG_ERR, "Device %s %s:%s not found",
					direction, api, device);

		if (input)
			chosendevice = Pa_GetDefaultInputDevice();
		else
			chosendevice = Pa_GetDefaultOutputDevice();
	}

	streamparameters->device = chosendevice;
	if (latency < 0.0)
	{
		if (input)
		{
			streamparameters->suggestedLatency =
				Pa_GetDeviceInfo(chosendevice)->defaultHighInputLatency;
			latencytype = "default high input";
		}
		else
		{
			streamparameters->suggestedLatency =
				Pa_GetDeviceInfo(chosendevice)->defaultHighOutputLatency;
			latencytype = "default high output";
		}
	}
	else
	{
		streamparameters->suggestedLatency = latency;
	}


	deviceinfo = Pa_GetDeviceInfo(chosendevice);
	hostapiinfo = Pa_GetHostApiInfo(deviceinfo->hostApi);
	logprintf(LOG_INFO, "Using %s %s device "
			"%i: %s:%s with %s latency %f",
			devicetype,
			direction, chosendevice,
			hostapiinfo->name,
			deviceinfo->name,
			latencytype,
			streamparameters->suggestedLatency);
}

/*
  interface functions
*/
static paTestData data;

int audio_init()
{

	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;
	PaError    err;
	int 		flags;
	struct termios	t;
	char api[1024];
	char device[1024];
	double latency;

	LOGPRINTF(1,"hw_audio_init()");
	
	// 
	logprintf(LOG_INFO,"Initializing %s...",hw.device);
	init_rec_buffer();
	rewind_rec_buffer();

	/* new */
	data.lastFrames[0] = 128;
	data.lastFrames[1] = 128;
	data.lastFrames[2] = 128;
	data.lastSign = 0;
	data.lastCount = 0;
	data.pulseSign = 0;
	data.carrierPos = 0.0;
	data.remainingSignal = 0.0;
	data.signalPhase = 0;
	data.signaledDone = 1;
	data.samplesToIgnore = 0;
	data.carrierFreq = DEFAULT_FREQ;
	err = Pa_Initialize();
	if( err != paNoError ) goto error;

	audio_parsedevicestr(api, device, &data.samplerate, &latency);
	logprintf(LOG_INFO, "Using samplerate %i", data.samplerate);

	/* choose input device */
	audio_choosedevice(&inputParameters, 1, api, device, latency);
	if (inputParameters.device == paNoDevice) {
		logprintf(LOG_ERR, "No input device found");
		goto error;
	}
	inputParameters.channelCount = NUM_CHANNELS;	/* stereo input */
	inputParameters.sampleFormat = PA_SAMPLE_TYPE;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	/* choose output device */
	audio_choosedevice(&outputParameters, 0, api, device, latency);
	if (outputParameters.device == paNoDevice) {
		logprintf(LOG_ERR, "No output device found");
		goto error;
	}
	outputParameters.channelCount = NUM_CHANNELS;	/* stereo output */
	outputParameters.sampleFormat = PA_SAMPLE_TYPE;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	outputLatency = outputParameters.suggestedLatency * 1000000;

	/* Record some audio. -------------------------------------------- */
	err = Pa_OpenStream
		(
		 &stream,
		 &inputParameters,
		 &outputParameters,
		 data.samplerate,
		 512,             /* frames per buffer */
		 paPrimeOutputBuffersUsingStreamCallback,
		 recordCallback,
		 &data );

	if( err != paNoError ) goto error;

	/* open pty */
/*
	if ( openpty( &master, &ptyfd, ptyName, 0, 0) == -1)
	{
		logprintf(LOG_ERR,"openpty failed");
		logperror(LOG_ERR,"openpty()");
		goto error;
	}
*/
	/* regular device file */
	if( tcgetattr( master, &t ) < 0 ) {
		logprintf(LOG_ERR,"tcgetattr failed");
		logperror(LOG_ERR,"tcgetattr()");
	}
	
	cfmakeraw( &t );
	
	/* apply file descriptor options */
	if( tcsetattr( master, TCSANOW, &t ) < 0) {  
		logprintf(LOG_ERR,"tcsetattr failed");
		logperror(LOG_ERR,"tcsetattr()");
	}

/*	flags=fcntl(ptyfd,F_GETFL,0);
	if(flags!=-1)
	{
		fcntl(ptyfd,F_SETFL,flags|O_NONBLOCK);
	}

	LOGPRINTF(LOG_INFO,"PTY name: %s", ptyName);

	hw.fd=ptyfd;
*/
	/* make a pipe for sending signals to the callback */
	/* make a pipe for signaling from the callback that everything
	   was sent */
	if (pipe(sendPipe) == -1 || pipe(completedPipe) == -1)
	{
		logprintf(LOG_ERR,"pipe failed");
		logperror(LOG_ERR,"pipe()");
	}

	/* make the readable end non-blocking */
	flags = fcntl(sendPipe[0], F_GETFL, 0);
	if(flags != -1)
	{
		fcntl(sendPipe[0], F_SETFL, flags | O_NONBLOCK);
	}
	else
	{
		logprintf(LOG_ERR,"fcntl failed");
		logperror(LOG_ERR,"fcntl()");
	}

	err = Pa_StartStream( stream );
	if( err != paNoError ) goto error;

	/* wait for portaudio to settle */
	usleep(50000);

	return(1);

 error:
	Pa_Terminate();
	logprintf(LOG_ERR,
		  "an error occured while using the portaudio stream" );
	logprintf(LOG_ERR, "error number: %d", err );
	logprintf(LOG_ERR, "error message: %s", Pa_GetErrorText( err ) );

	return (0);
}

int audio_deinit(void)
{
	PaError    err;

	LOGPRINTF(1,"hw_audio_deinit()");
	
	logprintf(LOG_INFO,"Deinitializing %s...",hw.device);

	/* make absolutely sure the full output buffer has played out
	   even though portaudio should wait for it, it doesn't always
	   happen */
	sleep(outputLatency / 1000000);
	usleep(outputLatency % 1000000);

	/* close port audio */
	err = Pa_CloseStream( stream );
	if( err != paNoError ) goto error;

	Pa_Terminate();
	
	/* wait for terminaton */
	usleep(20000);

	/* close pty */
	close(master);
/*	close(ptyfd);*/
	
	close(sendPipe[0]);
	close(sendPipe[1]);
	close(completedPipe[0]);
	close(completedPipe[1]);

	return 1;

 error:
	Pa_Terminate();
	logprintf(LOG_ERR,
		  "an error occured while using the portaudio stream");
	logprintf(LOG_ERR,"error number: %d",err);
	logprintf(LOG_ERR,"eError message: %s",Pa_GetErrorText(err));
	return 0;
}

char *audio_rec(struct ir_remote *remotes)
{
	if(!clear_rec_buffer()) return(NULL);
	return(decode_all(remotes));
}

struct hardware hw_audio=
{
	"",	            /* default device */
	-1,                 /* fd */
	LIRC_CAN_REC_MODE2 | LIRC_CAN_SEND_PULSE, /* features */
	LIRC_MODE_PULSE,    /* send_mode */
	LIRC_MODE_MODE2,    /* rec_mode */
	0,                  /* code_length */
	audio_init,         /* init_func */
	audio_deinit,       /* deinit_func */
	audio_send,         /* send_func */
	audio_rec,          /* rec_func */
	receive_decode,     /* decode_func */
	NULL,               /* ioctl_func */
	audio_readdata,
	"audio"
};
