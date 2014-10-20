/*      $Id: config_file.h,v 5.12 2009/05/24 10:46:52 lirc Exp $      */

/****************************************************************************
 ** config_file.h ***********************************************************
 ****************************************************************************
 *
 * config_file.h - parses the config file of lircd
 *
 * Copyright (C) 1998 Pablo d'Angelo (pablo@ag-trek.allgaeu.org)
 *
 */

/* #ifndef  _CONFIG_FILE_H */
/* #define  _CONFIG_FILE_H */

#include <sys/types.h>
#include <unistd.h>

#include "ir_remote.h"

struct flaglist {
	char *name;
	int flag;
};

static struct flaglist all_flags[]=
{
        {"RAW_CODES",       RAW_CODES},
	{"RC5",             RC5},
	{"SHIFT_ENC",       SHIFT_ENC}, /* obsolete */
	{"RC6",             RC6},
	{"RCMM",            RCMM},
	{"SPACE_ENC",       SPACE_ENC},
	{"SPACE_FIRST",     SPACE_FIRST},
        {"GOLDSTAR",        GOLDSTAR},
	{"GRUNDIG",         GRUNDIG},
	{"BO",              BO},
	{"SERIAL",          SERIAL},
	{"XMP",             XMP},
	
	{"REVERSE",         REVERSE},
	{"NO_HEAD_REP",     NO_HEAD_REP},
        {"NO_FOOT_REP",     NO_FOOT_REP},
	{"CONST_LENGTH",    CONST_LENGTH}, /* remember to adapt warning
					      message when changing this */
        {"REPEAT_HEADER",   REPEAT_HEADER},
	{NULL,0},
};

/*
  config stuff
*/

enum directive {ID_none,ID_remote,ID_codes,ID_raw_codes,ID_raw_name};

struct ptr_array
{
        void **ptr;
        size_t nr_items;
        size_t chunk_size;
};

struct void_array
{
        void *ptr;
        size_t item_size;
        size_t nr_items;
        size_t chunk_size;
};

void **init_void_array(struct void_array *ar,size_t chunk_size, size_t item_size);
int add_void_array(struct void_array *ar, void * data);
inline void * get_void_array(struct void_array *ar);

/* some safer functions */
void * s_malloc(size_t size);
char * s_strdup(char * string);
ir_code s_strtocode(const char *val);
unsigned long  s_strtoul(char *val);
int s_strtoi(char *val);
unsigned int s_strtoui(char *val);
lirc_t s_strtolirc_t(char *val);

int checkMode(int is_mode, int c_mode, char *error);
int parseFlags(char *val);
int addSignal(struct void_array *signals, char *val);
struct ir_ncode * defineCode(char *key, char *val, struct ir_ncode *code);
struct ir_code_node *defineNode(struct ir_ncode *code, const char *val);
int defineRemote(char * key, char * val, char *val2, struct ir_remote *rem);
struct ir_remote *read_config(FILE *f, const char *name);
void free_config(struct ir_remote *remotes);

/* #endif */
