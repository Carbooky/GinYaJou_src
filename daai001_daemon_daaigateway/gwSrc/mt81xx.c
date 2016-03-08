#include "mt81xx.h"
#include "lib_crc16.h"

int append_CRC16(raw_485_t* FrameBuffer){
	int cal_size=0xff, i;
	unsigned short crc_result=0xffff;
	
	// determine cal_size
	switch(((raw_485_t*)FrameBuffer)->pkt_type){
		case mt81xx_cmd_read:
		case mt81xx_cmd_write:
		case mt81xx_cmd_6C:
		case mt81xx_cmd_AA:
		case mt81xx_cmd_0B:
			cal_size = 0x06;
			break; 
		case mt81xx_cmd_485:
			cal_size = 0x09;
			break;
		default:
			cal_size = 0xff;
	}
	
	// start from slave address byte, not from start byte(0x0a)
	i=1;
	
	// calculate
	if(cal_size!=0xff){
		while(cal_size){
			crc_result = update_crc_16(crc_result, (char)((uint8_t*)(&((raw_485_t*)FrameBuffer)->uniFrame))[i]);
			cal_size--;
			i++;
		}
		
		// fill the crc 
		((uint8_t*)(&((raw_485_t*)FrameBuffer)->uniFrame))[i] = ((uint8_t*)&crc_result)[0];
		i++;
		((uint8_t*)(&((raw_485_t*)FrameBuffer)->uniFrame))[i] = ((uint8_t*)&crc_result)[1];
		
		return 1;
	}else
		return -1;
}

int check_CRC16(mt81xx_format_rtn_t * FrameBuffer){
	int cal_size=0xff, i;
	unsigned short crc_result=0xffff;

	// get cal_size
	cal_size = FrameBuffer->mt81xx_offset_Len;
	
	// start from slave address byte, not from start byte(0x0a)
	i=1;
	// add 3 bytes, total length is $len plus (addr + fun + len) 
	cal_size +=3;	
	
	// calculate
	if(cal_size!=0xff){
		while(cal_size){
			crc_result = update_crc_16(crc_result, (char)((uint8_t*)(&(FrameBuffer->mt81xx_offset_StartByte)))[i]);
			//printf("byte=0x%.2x\n", ((uint8_t *)(&((raw_485_t*)FrameBuffer)->uniFrame))[i] );
			cal_size--;
			i++;
		}
		
		//printf("crc_check %.2x", ((uint8_t*)(&((raw_485_t*)FrameBuffer)->uniFrame))[i]);
		
		// fill the crc 
		//printf( "CRC=0x%x ,0x%x\n",((uint8_t*)(&(FrameBuffer->mt81xx_offset_StartByte)))[i], ((uint8_t*)(&(FrameBuffer->mt81xx_offset_StartByte)))[i+1] );
		if(((uint8_t*)(&(FrameBuffer->mt81xx_offset_StartByte)))[i] == ((uint8_t*)&crc_result)[0]){
			i++;
			if(((uint8_t*)(&(FrameBuffer->mt81xx_offset_StartByte)))[i] == ((uint8_t*)&crc_result)[1])
				return 1;
		}
		return -1;
	}else
		return -1;
}

// assume Little Endian machine
int create_mt81xx_cmd(raw_485_t* FrameBuffer, uint16_t dev_addr_plusNew, uint16_t reg, uint32_t size){
	
	// decide pkt size
	switch(((raw_485_t*)FrameBuffer)->pkt_type){
		case mt81xx_cmd_read:
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_Function = 0x03;
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_SA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_SA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_NO_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_NO_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format03.mt81xx_offset_StopByte = 0x0d;			   
			break;
		case mt81xx_cmd_write:
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_Function = 0x06;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format06.mt81xx_offset_StopByte = 0x0d;			   
			break;	
		case mt81xx_cmd_6C:
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_Function = 0x6c;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format6C.mt81xx_offset_StopByte = 0x0d;			   
			break;
		case mt81xx_cmd_AA:
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_Function = 0xaa;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_formatAA.mt81xx_offset_StopByte = 0x0d;			   
			break;
		case mt81xx_cmd_0B:
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_Function = 0x0b;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_RA_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_RA_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_Data_HiByte = 0x00;		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_Data_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format0B.mt81xx_offset_StopByte = 0x0d;				
			break; 
		case mt81xx_cmd_485:
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_StartByte = 0x0a;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SlaveAddr = ((uint8_t*)&dev_addr_plusNew)[0];
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_Function = 0x08;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_MO_HiByte = ((uint8_t*)&reg)[1];
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_MO_LoByte = ((uint8_t*)&reg)[0];
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNH_HiByte = ((uint8_t*)&size)[3];		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNH_LoByte = ((uint8_t*)&size)[2];	
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNL_HiByte = ((uint8_t*)&size)[1];		// only 00 allowe
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_SNL_LoByte = ((uint8_t*)&size)[0];	
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_NewAddr = ((uint8_t*)&dev_addr_plusNew)[1];	
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_CS_LoByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_CS_HiByte = 0xff;
			FrameBuffer->uniFrame.im_mt81xx_format08.mt81xx_offset_StopByte = 0x0d;		
			break;
		default:
			return -1;
	}
	
	return 0;
}
