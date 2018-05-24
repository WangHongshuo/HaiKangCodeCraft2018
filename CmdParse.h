/**	@file       CmdParse.h
 *	@note       Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *	@brief		
 *
 *	@author     lipengfei
 *	@date       2018/05/10
 *	@note       ��ʷ��¼��
 *	@note       V1.0.0  
 *	@warning	
 */
#ifndef __CMDPARSE_H__
#define __CMDPARSE_H__

#include "JsonParse.h"
#include "Point3.h"
#include <vector>

using std::vector;
#define SOCKET_HEAD_LEN          8                      ///< 8���ֽڵ�ͷ������


struct CONNECT_NOTICE
{
    char    szNotice[64];
    char    szMsg[128];
};

struct TOKEN_INFO
{
    char    szToken[64];
    char    szAction[64];
};


struct TOKEN_RESULT
{
    char    szToken[64];
    char    szNotice[64];
    char    szRoundId[64];
    char    szPalyerId[64];
    int     nResult;
};

struct READY_PARAM
{
    char    szToken[64];
    char    szAction[64];
};

struct BUILDING
{
    int     nX;
    int     nY;
    int     nL;
    int     nW;
    int     nH;
};

struct FOG
{
    int     nX;
    int     nY;
    int     nL;
    int     nW;
    int     nB;
    int     nT;
};


#define MAX_BUILDING_NUM        100      
#define MAX_FOG_NUM        100
#define MAX_UAV_NUM         512
#define MAX_UAV_PRICE_NUM    64
#define MAX_GOODS_NUM       256
#define MAX_PATH_LENGTH     600


enum UAV_STATUS
{
    UAV_NOMAL = 0,
    UAV_CRASH,
    UAV_FOG
};

enum UAV_ACTION
{ 
	UAV_STANDBY = 0, 
	UAV_MOVING, 
	UAV_CATCHING,
	UAV_DELIVERYING
};

struct UAV
{
    int     nNO;
    char    szType[8];
	Point3 nPos;
    int     nLoadWeight;            ///< ��type��Ӧ�����˻�������һ����
	UAV_STATUS  nStatus;
    int     nGoodsNo;
	// �Զ�������
	Point3 nTarget;
	UAV_ACTION nAction = UAV_ACTION::UAV_STANDBY;
	int nPathLength = 0;
	int nCurrentPathIndex = 0;
	vector<Point3> nPath;
	bool nIsCrash = false;
	bool nIsMoved = false;
	bool nIsGetPath = false;
	int nGoodsTarget = -1;
	int nLastPosMapMark = -1;
	Point3 nLastPos;
};

struct UAV_PRICE
{
    char    szType[8];
    int     nLoadWeight;
    int     nValue;
};

/** @struct
 * 	@brief	
 *	@note
 */
struct MAP_INFO
{
    int     nMapX;
    int     nMapY;
    int     nMapZ;

	Point3 nParkingPos;

    int     nHLow;
    int     nHHigh;

    int     nBuildingNum;
    BUILDING    astBuilding[MAX_BUILDING_NUM];
    int     nFogNum;
    FOG         astFog[MAX_FOG_NUM];
    int     nUavNum;
    UAV     astUav[MAX_UAV_NUM];
    int     nUavPriceNum;
    UAV_PRICE   astUavPrice[MAX_UAV_PRICE_NUM];
};


struct FLAY_PLANE
{
    int     nUavNum;
    UAV     astUav[MAX_UAV_NUM];

    int     nPurchaseNum;
    char    szPurchaseType[MAX_UAV_PRICE_NUM][8];
};

struct GOODS
{
    int     nNO;

	Point3 nStartPos;
	Point3 nEndPos;

    int     nWeight;
    int     nValue;
    int     nStartTime;
    int     nRemainTime;
	int     nLeftTime;
    int     nState;
};

struct GOODSSTATUS
{
	int     nCatchedUavNo = -1;
};

struct MATCH_STATUS
{
    int     nTime;
    int     nMacthStatus;
    int     nUavWeNum;
    UAV     astWeUav[MAX_UAV_NUM];
    int     nWeValue;
    int     nUavEnemyNum;
    UAV     astEnemyUav[MAX_UAV_NUM];
    int     nEnemyValue;
    int     nGoodsNum;
    GOODS   astGoods[MAX_GOODS_NUM];
};

/** @fn     int ParserConnect(char *pBuffer, CONNECT_NOTICE *pstNotice)
 *  @brief	
 *	@param  -I   - char * pBuffer
 *	@param  -I   - CONNECT_NOTICE * pstNotice
 *	@return int
 */
int ParserConnect(char *pBuffer, CONNECT_NOTICE *pstNotice);


/** @fn     int ParserTokenResult(char *pBuffer, TOKEN_RESULT *pResult)
 *  @brief	
 *	@param  -I   - char * pBuffer
 *	@param  -I   - TOKEN_RESULT * pResult
 *	@return int
 */
int ParserTokenResult(char *pBuffer, TOKEN_RESULT *pResult);


/** @fn     int ParserMapInfo(char *pBuffer, MAP_INFO *pstMap)
 *  @brief	
 *	@param  -I   - char * pBuffer
 *	@param  -I   - MAP_INFO * pstMap
 *	@return int
 */
int ParserMapInfo(char *pBuffer, MAP_INFO *pstMap);


/** @fn     int ParserUav(cJSON *pUavArray, UAV *astUav, int *pNum)
 *  @brief	
 *	@param  -I   - cJSON * pUavArray
 *	@param  -I   - UAV * astUav
 *	@param  -I   - int * pNum
 *	@return int
 */
int ParserUav(cJSON *pUavArray, UAV *astUav, int *pNum);

/** @fn     int ParserMatchStatus(char *pBuffer, MATCH_STATUS *pstStatus)
 *  @brief	
 *	@param  -I   - char * pBuffer
 *	@param  -I   - MATCH_STATUS * pstStatus
 *	@return int
 */
int ParserMatchStatus(char *pBuffer, MATCH_STATUS *pstStatus);


/** @fn     int CreateTokenInfo(TOKEN_INFO *pstInfo, char *pBuffer)
 *  @brief	
 *	@param  -I   - TOKEN_INFO * pstInfo
 *	@param  -I   - char * pBuffer
 *	@return int
 */
int CreateTokenInfo(TOKEN_INFO *pstInfo, char *pBuffer, int *pLen);

/** @fn     int CreateReadyParam(READY_PARAM *pstParam, char *pBuffer, int *pLen)
 *  @brief	
 *	@param  -I   - READY_PARAM * pstParam
 *	@param  -I   - char * pBuffer
 *	@param  -I   - int * pLen
 *	@return int
 */
int CreateReadyParam(READY_PARAM *pstParam, char *pBuffer, int *pLen);


/** @fn     int CreateFlayPlane(FLAY_PLANE *pstPlane, char *pBuffer, int *pLen)
 *  @brief	
 *	@param  -I   - FLAY_PLANE * pstPlane
 *	@param  -I   - char * pBuffer
 *	@param  -I   - int * pLen
 *	@return int
 */
int CreateFlayPlane(FLAY_PLANE *pstPlane, char *szToken, char *pBuffer, int *pLen);

#endif

