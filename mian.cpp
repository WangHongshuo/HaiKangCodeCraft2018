/**	@file       mian.cpp
 *	@note       Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *	@brief
 *
 *	@author     lipengfei
 *	@date       2018/05/10
 *	@note       ��ʷ��¼��
 *	@note       V1.0.0
 *	@warning
 */

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include "OSSocket.h"
#include "JsonParse.h"
#include "CmdParse.h"
#include "UAVAI.h"
#include "Point3.h"

const int MAX_SOCKET_BUFFER = (1024 * 1024 * 4);       /// ���ͽ����������4M
using std::cout;
using std::endl;

/** @fn     int RecvJuderData(OS_SOCKET hSocket, char *pBuffer)
 *  @brief	��������
 *	@param  -I   - OS_SOCKET hSocket
 *	@param  -I   - char * pBuffer
 *	@return int
 */
int RecvJuderData(OS_SOCKET hSocket, char *pBuffer)
{
	int         nRecvLen = 0;
	int         nLen = 0;

	while (1)
	{
		// ����ͷ������
		nLen = OSRecv(hSocket, pBuffer + nRecvLen, MAX_SOCKET_BUFFER);
		if (nLen <= 0)
		{
			printf("recv error\n");
			return nLen;
		}

		nRecvLen += nLen;

		if (nRecvLen >= SOCKET_HEAD_LEN)
		{
			break;
		}
	}

	int         nJsonLen = 0;
	char        szLen[10] = { 0 };

	memcpy(szLen, pBuffer, SOCKET_HEAD_LEN);

	nJsonLen = atoi(szLen);

	while (nRecvLen < (SOCKET_HEAD_LEN + nJsonLen))
	{
		// ˵�����ݻ�û������
		nLen = OSRecv(hSocket, pBuffer + nRecvLen, MAX_SOCKET_BUFFER);
		if (nLen <= 0)
		{
			printf("recv error\n");
			return nLen;
		}

		nRecvLen += nLen;
	}

	return 0;
}

/** @fn     int SendJuderData(OS_SOCKET hSocket, char *pBuffer, int nLen)
 *  @brief	��������
 *	@param  -I   - OS_SOCKET hSocket
 *	@param  -I   - char * pBuffer
 *	@param  -I   - int nLen
 *	@return int
 */
int SendJuderData(OS_SOCKET hSocket, char *pBuffer, int nLen)
{
	int     nSendLen = 0;
	int     nLenTmp = 0;

	while (nSendLen < nLen)
	{
		nLenTmp = OSSend(hSocket, pBuffer + nSendLen, nLen - nSendLen);
		if (nLenTmp < 0)
		{
			return -1;
		}

		nSendLen += nLenTmp;
	}

	return 0;
}

/** @fn     void AlgorithmCalculationFun()
 *  @brief	ѧ�����㷨���㣬 ����ʲô�Ķ��Լ�д��
 *	@return void
 */
void  AlgorithmCalculationFun(UAVAI *pstAI)
{
	pstAI->getNextAction();
}

int main(int argc, char *argv[])
{
	UAVAI *pstAI = new UAVAI;
#ifdef OS_WINDOWS
	// windows�£���Ҫ���г�ʼ������
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("WSAStartup failed\n");
		return false;
	}

#endif

	char        szIp[64] = { 0 };
	int         nPort = 0;
	char        szToken[128] = { 0 };
	int         nRet = 0;
	OS_SOCKET   hSocket;
	char        *pRecvBuffer = NULL;
	char        *pSendBuffer = NULL;
	int         nLen = 0;

	// ���ص���ȥ�����
	if (argc != 4)
	{
		printf("error arg num\n");
		return -1;
	}

	// ��������
	strcpy(szIp, argv[1]);
	nPort = atoi(argv[2]);
	strcpy(szToken, argv[3]);

	//strcpy(szIp, "39.106.111.130");
	//nPort = 4010;
	//strcpy(szToken, "36d0a20b-7fab-4a93-b7e4-3247533b903a");

	printf("server ip %s, prot %d, token %s\n", szIp, nPort, szToken);

	// test

	// ��ʼ���ӷ�����
	nRet = OSCreateSocket(szIp, (unsigned int)nPort, &hSocket);
	if (nRet != 0)
	{
		printf("connect server error\n");
		return nRet;
	}

	// ������ܷ����ڴ�
	pRecvBuffer = (char*)malloc(MAX_SOCKET_BUFFER);
	if (pRecvBuffer == NULL)
	{
		return -1;
	}

	pSendBuffer = (char*)malloc(MAX_SOCKET_BUFFER);
	if (pSendBuffer == NULL)
	{
		free(pRecvBuffer);
		return -1;
	}

	memset(pRecvBuffer, 0, MAX_SOCKET_BUFFER);

	// ��������  ���ӳɹ���Judger�᷵��һ����Ϣ��
	nRet = RecvJuderData(hSocket, pRecvBuffer);
	if (nRet != 0)
	{
		return nRet;
	}
	cout << pRecvBuffer << endl;
	// json ����
	// ��ȡͷ��
	CONNECT_NOTICE  stNotice;

	nRet = ParserConnect(pRecvBuffer + SOCKET_HEAD_LEN, &stNotice);
	if (nRet != 0)
	{
		return nRet;
	}

	// ���ɱ�����ݵ�json
	TOKEN_INFO  stToken;

	strcpy(stToken.szToken, szToken);  // ����ǵ��Խ׶Σ�����������Ե�token�����ҵĶ�ս�л�ȡ��
									   // ʵ�ʱ�������Ҫ������Եģ�����demoд�ģ��з��������ô��롣
	strcpy(stToken.szAction, "sendtoken");

	memset(pSendBuffer, 0, MAX_SOCKET_BUFFER);
	nRet = CreateTokenInfo(&stToken, pSendBuffer, &nLen);
	if (nRet != 0)
	{
		return nRet;
	}
	// ѡ������з������������(Player -> Judger)
	nRet = SendJuderData(hSocket, pSendBuffer, nLen);
	if (nRet != 0)
	{
		return nRet;
	}
	cout << pSendBuffer << endl;

	//�����֤���(Judger -> Player)��
	memset(pRecvBuffer, 0, MAX_SOCKET_BUFFER);
	nRet = RecvJuderData(hSocket, pRecvBuffer);
	if (nRet != 0)
	{
		return nRet;
	}

	// ������֤�����json
	TOKEN_RESULT      stResult;
	nRet = ParserTokenResult(pRecvBuffer + SOCKET_HEAD_LEN, &stResult);
	if (nRet != 0)
	{
		return 0;
	}
	cout << pRecvBuffer << endl;

	// �Ƿ���֤�ɹ�
	if (stResult.nResult != 0)
	{
		printf("token check error\n");
		return -1;
	}
	// ѡ������з����������Լ���׼������(Player -> Judger)
	READY_PARAM     stReady;

	strcpy(stReady.szToken, szToken);
	strcpy(stReady.szAction, "ready");

	memset(pSendBuffer, 0, MAX_SOCKET_BUFFER);
	nRet = CreateReadyParam(&stReady, pSendBuffer, &nLen);
	if (nRet != 0)
	{
		return nRet;
	}
	nRet = SendJuderData(hSocket, pSendBuffer, nLen);
	if (nRet != 0)
	{
		return nRet;
	}
	cout << pSendBuffer << endl;

	//��ս��ʼ֪ͨ(Judger -> Player)��
	memset(pRecvBuffer, 0, MAX_SOCKET_BUFFER);
	nRet = RecvJuderData(hSocket, pRecvBuffer);
	if (nRet != 0)
	{
		return nRet;
	}
	cout << pRecvBuffer << endl;

	// ��������
	//Mapinfo �ṹ����ܴܺ󣬲�̫�ʺϷ���ջ�У����Զ���Ϊȫ�ֻ����ڴ����
	MAP_INFO            *pstMapInfo;
	MATCH_STATUS        *pstMatchStatus;
	FLAY_PLANE          *pstFlayPlane;

	pstMapInfo = (MAP_INFO *)malloc(sizeof(MAP_INFO));
	if (pstMapInfo == NULL)
	{
		return -1;
	}

	pstMatchStatus = (MATCH_STATUS *)malloc(sizeof(MATCH_STATUS));
	if (pstMapInfo == NULL)
	{
		return -1;
	}

	pstFlayPlane = (FLAY_PLANE *)malloc(sizeof(FLAY_PLANE));
	if (pstFlayPlane == NULL)
	{
		return -1;
	}

	memset(pstMapInfo, 0, sizeof(MAP_INFO));
	memset(pstMatchStatus, 0, sizeof(MATCH_STATUS));
	memset(pstFlayPlane, 0, sizeof(FLAY_PLANE));

	nRet = ParserMapInfo(pRecvBuffer + SOCKET_HEAD_LEN, pstMapInfo);
	if (nRet != 0)
	{
		return nRet;
	}

	// ��һ�ΰ����˻��ĳ�ʼ��ֵ��flayplane
	pstFlayPlane->nPurchaseNum = 0;
	pstFlayPlane->nUavNum = pstMapInfo->nUavNum;
	pstMatchStatus->nUavWeNum = pstMapInfo->nUavNum;
	for (int i = 0; i < pstMapInfo->nUavNum; i++)
	{
		pstMatchStatus->astWeUav[i] = pstMapInfo->astUav[i];
		pstFlayPlane->astUav[i] = pstMapInfo->astUav[i];
	}

	pstAI->initPtr(pstMapInfo, pstMatchStatus, pstFlayPlane);
	pstAI->initMap();
	pstAI->setInitUavTarget();

	clock_t clockBegin, clockEnd;
	clock_t timeCount = 0;
	// ���ݷ�����ָ���ͣ�Ľ��ܷ�������
	while (1)
	{
		cout << "Current step: " << pstMatchStatus->nTime << endl;
		// ���е�ǰʱ�̵����ݼ���, �����мƻ��ṹ�壬ע�⣺0ʱ�̲��ܽ����ƶ�������һ�ν����ѭ��ʱ
		if (pstMatchStatus->nTime != 0)
		{
			clockBegin = clock();
			AlgorithmCalculationFun(pstAI);
			clockEnd = clock();
			cout << "Cost Time: " << clockEnd - clockBegin << endl;
			timeCount += clockEnd - clockBegin;
		}

		//���ͷ��мƻ��ṹ��
		memset(pSendBuffer, 0, MAX_SOCKET_BUFFER);
		nRet = CreateFlayPlane(pstFlayPlane, szToken, pSendBuffer, &nLen);
		if (nRet != 0)
		{
			return nRet;
		}
		nRet = SendJuderData(hSocket, pSendBuffer, nLen);
		if (nRet != 0)
		{
			return nRet;
		}

		//printf("%s\n", pSendBuffer);

		// ���ܵ�ǰ����״̬
		memset(pRecvBuffer, 0, MAX_SOCKET_BUFFER);
		nRet = RecvJuderData(hSocket, pRecvBuffer);
		if (nRet != 0)
		{
			return nRet;
		}

		// ����
		nRet = ParserMatchStatus(pRecvBuffer + SOCKET_HEAD_LEN, pstMatchStatus);
		if (nRet != 0)
		{
			return nRet;
		}

		if (pstMatchStatus->nMacthStatus == 1)
		{
			// ��������
			printf("game over, we value %d, enemy value %d\n", pstMatchStatus->nWeValue, pstMatchStatus->nEnemyValue);
			cout << "Total time: " << timeCount << endl;
			return 0;
		}
	}

	delete pstAI;
	// �ر�socket
	OSCloseSocket(hSocket);
	// ��Դ����
	free(pRecvBuffer);
	free(pSendBuffer);
	free(pstMapInfo);
	free(pstMatchStatus);
	free(pstFlayPlane);

	return 0;
}