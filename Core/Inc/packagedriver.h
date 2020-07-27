#ifndef _PACKAGE_DRIVER_H_
#define _PACKAGE_DRIVER_H_

#include "cmsis_os.h"


#define PAKAGE_DRIVER_MAX_PACKAGE_PAYLOAD_LEN (4095)

#define PAKAGE_DRIVER_MIN_RX_BUFFER (20)

#define	PAKAGE_DRIVER_OK (0)
#define PAKAGE_DRIVER_NOT_INITED (-1)
#define PAKAGE_DRIVER_WRONG_SIZE (-2)
#define PAKAGE_DRIVER_WRONG_CRC (-2)
#define PAKAGE_DRIVER_ZERO_BUFFER (-3)

};

typedef struct{
	void (*write)(char c);
	int (*read)(void);
}data_transfer_t;

int packageDriverInit(data_transfer_t * psender, osMutexId_t mutex);
int packageDriverGet(char * buffer, size_t max_len);
int packageDriverSend(char * buffer, size_t len);


#endif
