#ifndef CMD_H
#define CMD_H

#include <sys/types.h>
#include <stdint.h>

#define CMD_SET_OP 0
#define CMD_GET_OP 1

#define RESP_SUCCESS 0
#define RESP_FAILED  1

struct cmd {
	uint16_t cmd_no;
	uint16_t cmd_data;
};

struct resp {
	uint16_t resp_code;
	uint16_t resp_data;
};

#endif
