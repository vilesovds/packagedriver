/*
 * packagedriver.c
 *
 *  Created on: Jul 27, 2020
 *      Author: d.vilesov
 */

#include "packagedriver.h"
#include "crc16.h"

//#define START_PACKAGE (0xD0)
#define END (0xC0)
#define END_REPLACE (0xDC)

#define ESC (0xDB)
#define ESC_REPLACE (0xDD)

static data_transfer_t * pTransfer = NULL;

static osMutexId_t Mutex = NULL;
static uint32_t inited = 0;



/**
 * @brief Sending data but replace special chars by ESC sequence
 * */
static void sendStuffing(char * buffer, size_t len)
{
	char c;
	while(len--){
		c = *buffer++;
		if(ESC==c){
			pTransfer->write(ESC);
			c = ESC_REPLACE;
		}else if(END==c){
			pTransfer->write(ESC);
			c = END_REPLACE;
		}
		pTransfer->write(c);
	}
}

/**
 * @brief Get data until END mark and unpack ESC sequence
 * */
static int readStuffing(char *buffer, size_t max_len)
{
	char c;
	size_t len = 0;
	int esc = 0;
	int err = 0;
	while((len<max_len)&&(0 == err)){
		c = pTransfer->read();
		if(esc){
			if(END_REPLACE==c)
				*buffer++=END;
			else if(ESC_REPLACE==c)
				*buffer++=ESC;
			else
				err = -1; // wrong ESC sequence
			esc = 0;
			len++;
		}else{
			if(ESC==c){
				esc = 1; // got ESC byte
			}else if(END){
				err = len; // got frame border
			}else{
				*buffer++ = c;
				len++;
			}
		}
	}
	return err;
}


int packageDriverInit(data_transfer_t * psender, osMutexId_t mutex)
{
	if((NULL == mutex)||(NULL == psender))
		return PAKAGE_DRIVER_NOT_INITED;
	if((NULL == psender->write)||(NULL == psender->read))
		return PAKAGE_DRIVER_NOT_INITED;
	pTransfer = psender;
	Mutex = mutex;
	inited = 1;
	return PAKAGE_DRIVER_OK;
}

int packageDriverSend(char * buffer, size_t len)
{
	unsigned short crc;
	if(!inited)
		return PAKAGE_DRIVER_NOT_INITED;
	if(len>PAKAGE_DRIVER_MAX_PACKAGE_PAYLOAD_LEN)
		return PAKAGE_DRIVER_WRONG_SIZE;
	if(0==len)
		return PAKAGE_DRIVER_ZERO_BUFFER; // zero buffer not allowed

	osMutexAcquire(Mutex, osWaitForever);
	sendStuffing(buffer, len);
	// calc and send CRC
	crc = crc16((unsigned char *)buffer, len);
	pTransfer->write(crc>>8);
	pTransfer->write(crc);
	// send stop mark
	pTransfer->write(END);
	osMutexRelease(Mutex);
	return PAKAGE_DRIVER_OK;
}

int packageDriverGet(char * buffer, size_t max_len)
{

	int res;
	if(!inited)
		return PAKAGE_DRIVER_NOT_INITED;
	if(max_len<PAKAGE_DRIVER_MIN_RX_BUFFER)
		return PAKAGE_DRIVER_WRONG_SIZE; // too short buffer

	osMutexAcquire(Mutex, osWaitForever);
	res = readStuffing(buffer, max_len);
	if(res>2){
		unsigned short crc;
		unsigned short readed_crc = buffer[res-1];
		readed_crc|=((unsigned short)buffer[res-2]<<8);
		res -= sizeof(crc);

		// calc and check
		crc = crc16((unsigned char *)buffer, res);
		if(crc!=readed_crc)
			res = PAKAGE_DRIVER_WRONG_CRC;
	}
	osMutexRelease(Mutex);
	return res;
}

