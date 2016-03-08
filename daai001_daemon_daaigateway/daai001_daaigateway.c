/////////////////////////////////////////////////////////////////////////////
//                           Headers                                       //
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "cwmp.h"
#include "daemon_core.h"

#include "libctools/common.h"
#include "libctools/string_utils.h"

#include "gwInc/gateway_api.h"
#include "daai001_daaigateway.h"

/////////////////////////////////////////////////////////////////////////////
//                      Constant Declarations                              //
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// XML TAG DEFINE

#define ROOT_NAME                      "Device"

#define X_DAAIGATEWAY                  "X_DAAIGateway"
// ---
#define X_PERIODICSCANINTERVAL         "X_PeriodicScanInterval"

#define X_GWSENSORINFO                 "X_GWSensorInfo"

#define X_GWINDICATION                 "X_GWIndication"

#define X_SCANINVERTER                 "X_ScanInverter"

#define X_INVERTERIDLIST               "X_InverterIDList"

#define X_INVERTERNUMBEROFENTRIES      "X_InverterNumberOfEntries"

#define X_INVERTER                     "X_Inverter"
// ---
#define X_INVERTERID                   "X_InverterID"
#define X_INVERTERENERGYINFO           "X_InverterEnergyInfo"

// XXX : need to cut prefix "Device" for next version code refine
#define PATH_X_PERIODICSCANINTERVAL 	"Device.X_DAAIGateway.X_PeriodicScanInterval"
#define PATH_X_SCANINVERTER 			"Device.X_DAAIGateway.X_ScanInverter"

/////////////////////////////////////////////////////////////////////////////
//                      Variables Declarations                             //
/////////////////////////////////////////////////////////////////////////////

//char m_remoteIP[] = "127.0.0.1";
//int m_remotePort = 10690;

//char m_profileName[] = "DAAIGW";

//int m_dcID = -1;

#define PROFILE_NAME    "DAAIGW"

pthread_t m_daai_ScanDeviceThread;

pthread_cond_t m_daai_sdThreadWakeupCondition;

pthread_mutex_t m_daai_sdThreadMutex;

bool m_daai_isSDThreadRunning;

/////////////////////////////////////////////////////////////////////////////
//                      Functions Declarations                             //
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
//                           Implementation                                //
/////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// main or init and dispose function



int DAAIGW_procRegularCallback() {

	DPRINTF("[DAAIGW_procRegularCallback] start ....\n");

	int ret = -1;
	char strParamName[STRING_MAX_LENGTH] = "";
	char strParamValue[STRING_MAX_LENGTH] = "";

	// get GWSensorInfo
	memset(strParamName, 0x00, sizeof(strParamName));
	sprintf(strParamName, "%s.%s.%s", ROOT_NAME, X_DAAIGATEWAY, X_GWSENSORINFO);

	ret = daai_getSensorInfo(strParamValue, STRING_MAX_LENGTH);
	if (ret >= 0) {
		DaemonCore_notify(PROFILE_NAME, strParamName, strParamValue);
	}

	// InvIDs format : <num>,<id1>,<id2>,...
	char strIDs[8192] = "";
	char tmpIDs[8192] = "";

	ret = daai_getSysInv(strIDs, 8192);

	if (ret < 0) {
		return ret;
	}
	// set to parameter "X_InverterIDList"

	memset(strParamName, 0x00, sizeof(strParamName));
	sprintf(strParamName, "%s.%s.%s", ROOT_NAME, X_DAAIGATEWAY, X_INVERTERIDLIST);
	//DaemonCore_notify(m_dcID, strParamName, strIDs);

	strcpy(tmpIDs, strIDs);

	// printf("Splitting string \"%s\" into tokens:\n", tmpIDs);

	// The first is number of Inverter id.
	char * pch;
	char* tmptok_ptr01 = NULL;
	pch = strtok_r(tmpIDs, ",", &tmptok_ptr01);

	int num = 0;
	char strNum[16] = "";
	strcpy(strNum, pch);

	if (String_isNumeric(strNum) == false) {
		return -1;
	}

	num = atoi(strNum);

	memset(strParamName, 0x00, sizeof(strParamName));
	memset(strParamValue, 0x00, sizeof(strParamValue));
	sprintf(strParamName, "%s.%s.%s", ROOT_NAME, X_DAAIGATEWAY, X_INVERTERNUMBEROFENTRIES);
	sprintf(strParamValue, "%d", num);

	if(DaemonCore_notify(PROFILE_NAME, strParamName, strParamValue)<0) {
		return -1;
	}

	int k = 0;

	while (pch != NULL) {
		pch = strtok_r(NULL, ",", &tmptok_ptr01);
		if (pch != NULL) {
			k++;

			memset(strParamName, 0x00, sizeof(strParamName));
			memset(strParamValue, 0x00, sizeof(strParamValue));
			sprintf(strParamName, "%s.%s.%s.%d.%s", ROOT_NAME, X_DAAIGATEWAY, X_INVERTER, k, X_INVERTERID);
			strcpy(strParamValue, pch);

			DaemonCore_notify(PROFILE_NAME, strParamName, strParamValue);
			if (String_isNumeric(strParamValue) == false) {
				continue;
			}
			int iid = atoi(strParamValue);
			// get Inverter Energy Info
			memset(strParamName, 0x00, sizeof(strParamName));
			memset(strParamValue, 0x00, sizeof(strParamValue));
			sprintf(strParamName, "%s.%s.%s.%d.%s", ROOT_NAME, X_DAAIGATEWAY, X_INVERTER, k, X_INVERTERENERGYINFO);
			int ret = daai_getEnergy(iid, strParamValue, STRING_MAX_LENGTH);
			if (ret >= 0) {
				DaemonCore_notify(PROFILE_NAME, strParamName, strParamValue);
			}
		}
	}

	DPRINTF("[DAAIGW_procRegularCallback] finish ....\n");
	return 0;
}

void* DAAIGW_scanDeviceThread(void* pParameter) {
	(void) pParameter;
	List didList;
	List_init(&didList);

	while (m_daai_isSDThreadRunning) {
		pthread_mutex_lock(&m_daai_sdThreadMutex);

		//DEL printf("[SPGW][scanDeviceThread] enter block... \n");
		pthread_cond_wait(&m_daai_sdThreadWakeupCondition, &m_daai_sdThreadMutex);
		//DEL printf("[SPGW][scanDeviceThread] leave block... \n");

		pthread_mutex_unlock(&m_daai_sdThreadMutex);

		if (m_daai_isSDThreadRunning == false)
			break;

//		char strParameterName[257] = "";
//		char strValue[16] = "";
//
//		// "Device.X_ShanePuGateway.X_SCANINVERTER"
//		sprintf(strParameterName, "%s.%s.%s", ROOT_NAME, X_SPGATEWAY, X_SCANINVERTER);
//		strcpy(strValue, "0");
//
//		if(m_sp_notify_func != NULL)
//			m_sp_notify_func(strParameterName, strValue);
//
//		//	scan_routine();
//
//		sleep(1);
//
//		char InvIDs[1024] = "";
//		char tmpIDs[1024] = "";
//		int ret = getSysInv(InvIDs, 1024);
//
//		// InvIDs format : <num>,<id1>,<id2>,...
//
//		if(ret < 0)
//			continue;
//
//		char strParamName[STRING_MAX_LENGTH] = "";
//		char strParamValue[STRING_MAX_LENGTH] = "";
//
//		// set to parameter "X_InverterIDList"
//		sprintf(strParamName, "%s.%s.%s", ROOT_NAME, X_SPGATEWAY, X_INVERTERIDLIST);
//		strcpy(strParamValue, InvIDs);
//		if(m_sp_notify_func != NULL)
//			m_sp_notify_func(strParamName, strParamValue);
//
//		// first count
//		char * pch;
//		strcpy(tmpIDs, InvIDs);
//
//		// printf("Splitting string \"%s\" into tokens:\n", tmpIDs);
//
//                char* tmptok_ptr01 = NULL;
//		pch = strtok_r(tmpIDs, ",", &tmptok_ptr01);
//
//		char strNum[16] = "";
//		strcpy(strNum, pch);
//		if(String_isNumeric(strNum) == false)
//			continue;
////		int num = atoi(strNum);
//
//		while (pch != NULL ) {
//			pch = strtok_r(NULL, ",", &tmptok_ptr01);
//			if(pch != NULL)
//			{
//				// printf("%s\n", pch);
//				char* did = malloc(strlen(pch)*sizeof(char) + 1);
//				strcpy(did, pch);
//				List_push(&didList,did);
//			}
//		}
//
//		memset(strParamName, 0x00, sizeof(strParamName));
//		memset(strParamValue, 0x00, sizeof(strParamValue));
//		sprintf(strParamName,"%s.%s.%s",ROOT_NAME, X_SPGATEWAY, X_INVERTERSETNUMBEROFENTRIES);
//		sprintf(strParamValue,"%d", didList.size);
//
//		//DEL printf("[SPGW][scanDeviceThread][set number of entries] strParamName : %s ,  strParamValue = %s \n", strParamName, strParamValue);
//
//	    if(m_sp_notify_func(strParamName, strParamValue) < 0 )
//	    {
//	    	List_clear(&didList);
//	   	   continue;
//	    }
//		// second get id
//		int k = 0;
//
//		while(didList.size > 0){
//			char* did = List_pop(&didList);
//			k++;
//			memset(strParamName, 0x00, sizeof(strParamName));
//			memset(strParamValue, 0x00, sizeof(strParamValue));
//			sprintf(strParamName,"%s.%s.%s.%d.%s",ROOT_NAME, X_SPGATEWAY, X_INVERTERSET, k, X_INVERTERID);
//			strcpy(strParamValue, did);
//			// printf("%s\n", did);
//
//			if(m_sp_notify_func != NULL)
//				m_sp_notify_func(strParamName, strParamValue);
//
//			// get Inverter Energy Info
//			memset(strParamName, 0x00, sizeof(strParamName));
//			memset(strParamValue, 0x00, sizeof(strParamValue));
//			sprintf(strParamName, "%s.%s.%s.%d.%s", ROOT_NAME, X_SPGATEWAY, X_INVERTERSET, k, X_INVERTERENERGY);
//
//
//			int nIndex = 0;
//			if(String_isInteger(did) == false){
//				continue;
//			}
//
//			nIndex = atoi(did);
//
//			if(nIndex <=0)
//				continue;
//
//            int ret = getEnergy(nIndex, strParamValue, STRING_MAX_LENGTH);
//			printf("debug >>> [scanDeviceThread] ret = %d, did = %s, index=%d\n", ret, did, nIndex);
//			printf("debug >>> strParamValue --%s--\n", strParamValue);
//			printf("debug >>> strParamName --%s--\n", strParamName);
//			if(ret >= 0 && m_sp_notify_func != NULL)
//				m_sp_notify_func(strParamName, strParamValue);
//
//			SAFE_FREE(did);
//		}
//		List_clear(&didList);
	}
	return 0;
}

///////////////////////////////////////////////////////////
// process parameter function

int DAAIGW_procPerdiodicScanInterval(int cmd, char* ijk, char* resultValue) {

	DPRINTF("[DAAIGW][procPerdiodicScanInterval] start... cmd : %d , ijk = %s \n", cmd, ijk);

	if (resultValue == NULL)
		return -1;

	char strValue[256] = "";

	int ret = -1;

	switch (cmd) {
	case DATACMD_GET:
		ret = daai_getSysCtrl(strValue, 256);
		if (ret >= 0)
			strcpy(resultValue, strValue);
		break;
	case DATACMD_SET:
		ret = daai_setSysCtrl((char*) resultValue);
		break;
	}

	DPRINTF("[DAAIGW][procPerdiodicScanInterval] finish... resultValue : %s\n", resultValue);

	if (ret < 0)
		return -1;

	return 0;
}

int DAAIGW_procScanInverter(int cmd, char* ijk, char* resultValue) {

	DPRINTF("[DAAIGW][DAAIGW_procScanInverter] start... cmd : %d , ijk = %s \n", cmd, ijk);

	if (resultValue == NULL)
		return -1;

	int ret = -1;
	switch (cmd) {
	case DATACMD_GET:
		break;
	case DATACMD_SET:

		if (String_StrCaseStr(resultValue, "false") != NULL || strcmp(resultValue, "0") == 0)
			break;

		pthread_cond_signal(&(m_daai_sdThreadWakeupCondition));
		ret = 1;
		break;
	}

	DPRINTF("[SPGW][DAAIGW_procScanInverter] finish... \n");

	if (ret < 0)
		return -1;

	return 0;
}

void TR_registerCallbacks() {

	DaemonCore_register(PROFILE_NAME, PATH_X_PERIODICSCANINTERVAL, DAAIGW_procPerdiodicScanInterval);
	DaemonCore_register(PROFILE_NAME, PATH_X_SCANINVERTER, DAAIGW_procScanInverter);

}

bool Daemon_init(int argc, char** argv) {
   (void) argc;
   (void) argv;
    // ------------------------------------------------------------------------
    // Must have
    DaemonCore_init(PROFILE_NAME);
    TR_registerCallbacks();
    // ------------------------------------------------------------------------
    // Customerize start here.


    m_daai_isSDThreadRunning = true;
    pthread_mutex_init(&(m_daai_sdThreadMutex), NULL);
    pthread_cond_init(&(m_daai_sdThreadWakeupCondition), NULL);

    daai_setSysInit(DAAIGW_procRegularCallback);
    
    int intResult = -1;
    intResult = pthread_create(&m_daai_ScanDeviceThread, NULL, (void *) DAAIGW_scanDeviceThread, NULL);
    if (intResult < 0)
        DPRINTF("[DAAIGW_init] failed to create DAAIGW_scanDeviceThread...\n");

    sleep(2);

    return true;
}


void Daemon_start() {

}


void Daemon_dispose() {

	daai_setSysDispose();

	m_daai_isSDThreadRunning = false;
	// notify all workerThread wakeup and stop
	pthread_cond_broadcast(&(m_daai_sdThreadWakeupCondition));

	// stop workerThread
	//Pthread_cancel(m_daai_ScanDeviceThread);

	// release mutex & cond
	pthread_mutex_destroy(&(m_daai_sdThreadMutex));
	pthread_cond_destroy(&(m_daai_sdThreadWakeupCondition));

}
