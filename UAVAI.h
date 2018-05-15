#ifndef __UAVAI_H__
#define __UAVAI_H__

#include "CmdParse.h"
#include <iostream>

class UAVAI
{
public:
	UAVAI();
	~UAVAI();
	void initPtr(MAP_INFO * _map, MATCH_STATUS * _match, FLAY_PLANE * _flayPlane);

private:
	MAP_INFO *map = NULL;
	MATCH_STATUS *match = NULL;
	FLAY_PLANE *plan = NULL;

};

#endif // !__UAVAI_H__
