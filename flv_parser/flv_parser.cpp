// flv_parser.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define  FLV_TEST_FILE1  "d:\\video\\1.flv"


#define PRT   printf



#define FLV_TAG_DATA_OFFSET  9
#define FLV_TAG_TYPE_AUDIO   8 
#define FLV_TAG_TYPE_VIDEO	 9
#define FLV_TAG_TYPE_DATA    18
#define FLV_TAG_TYPE_OTHER	 -1

union DATAOFFSET_T
{    
    unsigned int  DataOffSetSize;
	unsigned char DataOffset[4];
};

union DATAO2BYTE_T
{    
    unsigned int  value;
	unsigned char Data[2];
};

union DATAO3BYTE_T
{    
    unsigned int  value;
	unsigned char Data[3];
};

union DATAO4BYTE_T
{    
    unsigned int  value;
	unsigned char Data[4];
};

typedef struct
{
	unsigned char fieldName[3];
	unsigned char version;
	unsigned char cPayLoad;
	DATAOFFSET_T Offset;
	
}FLVHEADER_T;

typedef struct
{
	FILE* flvHandler;
	FILE* flvVideoHandler;
	FILE* flvAudioHandler;
	char  Path[1024];
	FLVHEADER_T  flvHeader;

	int hasVideo;
	int hasAudio;
	int nVideoCodecType;
	int nVideoFs;
	int nVideoWidth;
	int nVideoHeight;
	int nVideoBitrate;

	int nAudioCodecType;
	int nAudioSample;
	int nAudioBit;

	long long fileSeek;
	
	int hasReadTagData;
	int nMetaInfo;
}FLVOBJECT_T;


typedef struct 
{
	unsigned char ncPreviousTagSize[4];
	unsigned char tagType;
	unsigned char ncDataSize[3];
	unsigned char ncTimeStamp[3];
	unsigned char nTimeStampExt;
	unsigned char ncStreamId[3];
	
}FLVTAGS_RAW_T;


typedef struct 
{
	unsigned int nPreviousTagSize;
	unsigned int tagType;
	unsigned int nDataSize;
	unsigned int nTimeStamp;
	unsigned int nTimeStampExt;
	unsigned int nStreamId;
	unsigned char* pData;		// need dynamic malloc from system	
}FLVTAGS_T;

typedef struct 
{

}H263VIDEOPACKET_T;


typedef struct 
{

}SCREENVIDEOPACKET_T;

typedef struct 
{

}VP6FLVVIDEOPACKET_T;

typedef struct 
{

}VP6FLVALPHAVIDEOPACKET_T;

typedef struct 
{

}SCREENV2VIDEOPACKET_T;

typedef struct 
{

}AVCVIDEOPACKET_T;


union VIDEODATA_BODY_T
{
	H263VIDEOPACKET_T			h263VideoPacket;
	SCREENVIDEOPACKET_T			screenVideoPacket;
	VP6FLVVIDEOPACKET_T			vp6FlvVideoPacket;
	VP6FLVALPHAVIDEOPACKET_T	vp6FlvValPhaVideoPacket;
	SCREENV2VIDEOPACKET_T		screen2VideoPacket;
	AVCVIDEOPACKET_T			avcVideoPacket;
};

typedef struct
{
	unsigned char nFrameType;
	unsigned char nCodecID;
	VIDEODATA_BODY_T   tVideoData;
}VIDEODATA_T;

///////////////////////////////////////////
typedef struct 
{

}AACVIDDATA_T;

typedef struct 
{

}GENAUDIODATA_T;

union AUDIODATA_BODY_T
{
	AACVIDDATA_T			aacPacket;
	GENAUDIODATA_T			genAudioPacket;	
};

typedef struct
{
	unsigned char nSoundFormat;
	unsigned char nSoundRate;
	unsigned char nSoundSize;
	unsigned char nSoundType;
	unsigned char nSoundData;
	AUDIODATA_BODY_T   tAudioData;
}AUDIODATA_T;

unsigned int GetDataFrom3Byte(unsigned char* pData)
{
	DATAO3BYTE_T nUData;
	nUData.value = 0;
	nUData.Data[2] = pData[0];
	nUData.Data[1] = pData[1];
	nUData.Data[0] = pData[2];
	return nUData.value;
}

unsigned int GetDataFrom4Byte(unsigned char* pData)
{
	DATAO4BYTE_T nUData;
	nUData.value = 0;
	nUData.Data[3] = pData[0];
	nUData.Data[2] = pData[1];
	nUData.Data[1] = pData[2];
	nUData.Data[0] = pData[3];
	return nUData.value;
}


unsigned int HGetDataFrom4Byte(unsigned char* pData)
{
	DATAO4BYTE_T nUData;
	nUData.value = 0;
	nUData.Data[0] = pData[0];
	nUData.Data[1] = pData[1];
	nUData.Data[2] = pData[2];
	nUData.Data[3] = pData[3];
	return nUData.value;
}


unsigned int GetDataFrom2Byte(unsigned char* pData)
{
	DATAO2BYTE_T nUData;
	nUData.value = 0;	
	nUData.Data[1] = pData[0];
	nUData.Data[0] = pData[1];
	return nUData.value;
}

FLVOBJECT_T* CreateFlvInstace(char* flv_file)
{
	FLVOBJECT_T*  pFlvInstance = NULL; 
	unsigned char cFlvHeader[9];
	FILE* file;
	file = fopen ( flv_file, "rb");


	if( NULL == file )
		return NULL;

	pFlvInstance = (FLVOBJECT_T*)malloc( sizeof(FLVOBJECT_T) );
	pFlvInstance->flvAudioHandler = NULL;
	pFlvInstance->flvVideoHandler = NULL;
	pFlvInstance->flvHandler = file;
	pFlvInstance->hasReadTagData = 0;
	strncpy( pFlvInstance->Path,flv_file,sizeof(pFlvInstance->Path) );

	return pFlvInstance;
}

int GetBasicFlvInfo(FLVOBJECT_T*  pHandle)
{
	if( NULL == pHandle )
		return -1;

	fread( (void*)&pHandle->flvHeader,sizeof(pHandle->flvHeader),1,pHandle->flvHandler);
	
	//check video support
	if( pHandle->flvHeader.cPayLoad & 0x20  )
	{
		pHandle->hasAudio = 1;
		PRT("Has Audio  \n");
	}

	if( pHandle->flvHeader.cPayLoad & 0x02  )
	{
		pHandle->hasVideo = 1;
		PRT("Has Video  \n");
	}
	
	return 0;
}

int OnAudioData( FLVOBJECT_T*  pHandle, FLVTAGS_T*  pTagBody)
{
	AUDIODATA_T  stAudioData;
	unsigned char* ptrData =  pTagBody->pData + 1; 


	stAudioData.nSoundFormat =  (pTagBody->pData[0]& 0xf0)>>4;
	stAudioData.nSoundRate = (pTagBody->pData[0]& 0x0c)>>2;
	stAudioData.nSoundSize = (pTagBody->pData[0]& 0x02)>>1;
	stAudioData.nSoundType = (pTagBody->pData[0]& 0x01);
	

	if( NULL == pHandle || NULL == pTagBody )
		return -1;

	if(  NULL == pHandle->flvAudioHandler)
	{
		pHandle->flvAudioHandler = fopen ( "d:\\flv.mp3", "wb");		
	}

	fwrite( (void*)ptrData,pTagBody->nDataSize-1,1 ,pHandle->flvAudioHandler);

	return 0;

}

int OnVideoData( FLVOBJECT_T*  pHandle, FLVTAGS_T*  pTagBody)
{
	VIDEODATA_T  stVideoData;
	unsigned char* ptrData =  pTagBody->pData + 1; 
	
	
	stVideoData.nFrameType = (pTagBody->pData[0]& 0xf0)>>4;
	stVideoData.nCodecID = (pTagBody->pData[0]&0x0f) ;	

	if( NULL == pHandle || NULL == pTagBody )
		return -1;

	if(  NULL == pHandle->flvVideoHandler)
	{
		pHandle->flvVideoHandler = fopen ( "d:\\flv.263", "wb");		
	}

	fwrite( (void*)ptrData,pTagBody->nDataSize-1,1 ,pHandle->flvVideoHandler);

	return 0;

}

int OnData( FLVOBJECT_T*  pHandle, FLVTAGS_T*  pTagBody)
{
	if( NULL == pHandle || NULL == pTagBody )
		return -1;

	// dump Metadta tag



	return 0;
}

int GetMediaData(FLVOBJECT_T*  pHandle)
{
	FLVTAGS_T*  pTagBody = NULL;
	FLVTAGS_RAW_T	tagRawData;
	DATAOFFSET_T  nData;
	DATAO3BYTE_T	nUData;
	int ret = 0;
	int len = 0;
	if( NULL == pHandle )
		return -1;

	if( 0 == pHandle->hasReadTagData)
	{
		fseek(pHandle->flvHandler, FLV_TAG_DATA_OFFSET ,SEEK_SET);
	}
	pHandle->hasReadTagData = 1;
	pTagBody = (FLVTAGS_T*)malloc( sizeof(FLVTAGS_T) );	
	fread( (void*)&tagRawData,15,1,pHandle->flvHandler);
	
	pTagBody->nDataSize = GetDataFrom3Byte(  tagRawData.ncDataSize);
	pTagBody->nPreviousTagSize = GetDataFrom4Byte(  tagRawData.ncPreviousTagSize);
	pTagBody->nTimeStamp = GetDataFrom4Byte(  tagRawData.ncTimeStamp);
	pTagBody->nStreamId = GetDataFrom3Byte(  tagRawData.ncStreamId);
	pTagBody->tagType = tagRawData.tagType;
	pTagBody->nTimeStampExt = tagRawData.nTimeStampExt;

	pTagBody->pData = (unsigned char*)malloc( pTagBody->nDataSize );
	len = fread( (void*)pTagBody->pData,pTagBody->nDataSize,1,pHandle->flvHandler);

	if( len <= 0 )
	{
		free(pTagBody->pData);
		return -1;
	}

	switch( pTagBody->tagType )
	{
	case FLV_TAG_TYPE_AUDIO:
		 ret = OnAudioData(pHandle,pTagBody);
		break;
	case FLV_TAG_TYPE_VIDEO:
		 ret = OnVideoData(pHandle,pTagBody);
		break;
	case FLV_TAG_TYPE_DATA:
		 ret = OnData(pHandle,pTagBody);
		break;
	default:
		ret = -1;
		break;	
	}

	free(pTagBody->pData);
	return ret;
}




int GetMetaInfo(FLVOBJECT_T*  pHandle)
{
	if( NULL == pHandle )
		return -1;

	return 0;
}

int DumpFlvInfo(FLVOBJECT_T*  pHandle)
{
	if( NULL == pHandle )
		return -1;

	return 0;
}


int ReleaseFlvTag(FLVTAGS_T*  pTagBody)
{
	if( NULL == pTagBody )
	{
		return -1;
	}

	if( NULL != pTagBody->pData )
	{
		free( pTagBody->pData);
		pTagBody->pData = NULL;
	}

	if( NULL != pTagBody )
	{
		free( pTagBody);
		pTagBody = NULL;
	}

	return 0;
}

int ReleaseFlvInstance(FLVOBJECT_T*  pHandle)
{
	if( NULL == pHandle )
	{
		return -1;
	}

	if(  NULL != pHandle->flvHandler )
	{
		fclose(pHandle->flvHandler);
		pHandle->flvHandler = NULL;
	}		

	if(  NULL != pHandle->flvVideoHandler )
	{
		fclose(pHandle->flvVideoHandler);
		pHandle->flvVideoHandler = NULL;
	}	

	if(  NULL != pHandle->flvAudioHandler )
	{
		fclose(pHandle->flvAudioHandler);
		pHandle->flvAudioHandler = NULL;
	}	
	
	free(pHandle);
	pHandle = NULL;
	return 0;
}

void test_case0(void)
{
	FLVOBJECT_T*  pFlvInstace = NULL;
	pFlvInstace = CreateFlvInstace(FLV_TEST_FILE1);
	GetBasicFlvInfo(pFlvInstace);
	DumpFlvInfo(pFlvInstace);
	while(  GetMediaData(pFlvInstace) >= 0 );	
	ReleaseFlvInstance(pFlvInstace);
}

int main(int argc,char** argv)
{
	test_case0();
	return 0;
}

