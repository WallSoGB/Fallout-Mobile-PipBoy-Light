#pragma once
#include <NiObjects.h>
#include <NiTypes.h>
#include <NiNodes.h>
#include <GameData.h>

class NiUpdateData
{
public:
	NiUpdateData() { memset(this, 0, sizeof(NiUpdateData)); }
	~NiUpdateData() { }
	float fTime;
	bool bUpdateControllers;
	bool bIsMT;
	bool byte6;
	bool bUpdateTask6;
	bool bUpdateShadowSceneNode;
};