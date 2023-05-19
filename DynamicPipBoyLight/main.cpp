#include "nvse/PluginAPI.h"
#include <SafeWrite.h>
#include <GameData.h>
#include <fstream>
#include <string>
#include <decoded.h>

NVSEInterface* g_nvseInterface{};
IDebugLog	   gLog("logs\\DynamicPipBoyLight.log");

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "PipBoyTest";
	info->version = 100;
	return true;
}

bool(__thiscall* Actor__GetIsWeaponOut)(Actor* pActor) = (bool(__thiscall*)(Actor*))0x8A16D0;
UInt32* (__cdecl* GetPipboyManager)() = (UInt32 * (__cdecl*)())0x705990;
bool(__thiscall* IsLightActive)(UInt32* pPipBoyManager) = (bool(__thiscall*)(UInt32*))0x967700;
void(__thiscall* AttachParent)(NiAVObject* pObject, NiAVObject* pNewParent) = (void(__thiscall*)(NiAVObject*, NiAVObject*))0xA59D00;
void(__thiscall* AttachChild)(NiNode* apThis, NiAVObject* pkChild, int bUseFirst) = (void(__thiscall*)(NiNode*, NiAVObject*, int))0xA5ED10;

static NiNode* pScreenNode;
static NiNode* pPipBoyLight; // Should be NiPointLight but too lazy, doesn't matter
static Actor* pEffectActor;
static bool bPrevIsThirdPerson = 0;
static bool bPrevWeapState = 0;
static BOOL bIsThirdPersonNode = -1;

NiNode* __fastcall GetPipBoyNode(PlayerCharacter* apThis, void*, bool bForceThirdPerson) {
	NiNode* playerNode;
	BSFixedString string = BSFixedString();
	BSFixedString* string2 = ThisStdCall<BSFixedString*>(0x438170, &string, "PipboyLightEffect");
	if (bForceThirdPerson) {
		playerNode = apThis->renderState->niNode;
	}
	else {
		playerNode = apThis->bThirdPerson ? apThis->renderState->niNode : apThis->playerNode;
	}
	pScreenNode = (NiNode*)playerNode->GetObjectByName(string2);
	ThisStdCall(0x4381B0, &string);
	return pScreenNode ? pScreenNode : playerNode;
}

Actor* __fastcall MagicTarget_GetParent(UInt32* apThis) {
	pEffectActor = ThisStdCall<Actor*>(0x822B40, apThis); // MagicTarget::GetParent itself
	return pEffectActor;
}

void __fastcall SetLocalTranslateXYZ(NiNode* apThis, void*, float x, float y, float z) {
	if (pEffectActor == PlayerCharacter::GetSingleton()) {
		apThis->m_kLocal.translate.x = 0.0f;
		apThis->m_kLocal.translate.y = 0.0f;
		apThis->m_kLocal.translate.z = 10.0f;
		pPipBoyLight = apThis;
		pEffectActor = nullptr;
	}
	else {
		apThis->m_kLocal.translate.x = x;
		apThis->m_kLocal.translate.y = y;
		apThis->m_kLocal.translate.z = z;
	}
}

static NiUpdateData updateData = NiUpdateData();

void __fastcall UpdateLightSwitch() {
	PlayerCharacter* pPlayer = PlayerCharacter::GetSingleton();
	if (!IsLightActive(GetPipboyManager())) {
		return;
	}

	if (pPipBoyLight == nullptr) {
		return;
	}

	bool bIsThirdPerson = pPlayer->bThirdPerson;
	bool bWeaponOut = Actor__GetIsWeaponOut(pPlayer);

	if (bIsThirdPerson == bPrevIsThirdPerson && bPrevWeapState == bWeaponOut || (bIsThirdPerson && bPrevIsThirdPerson)) {
		return;
	}

	bPrevIsThirdPerson = bIsThirdPerson;
	bPrevWeapState = bWeaponOut;

	// Determine parent node for light
	NiNode* parentNode;
	if (!bWeaponOut) {
		parentNode = GetPipBoyNode(pPlayer, 0, 1);
#if _DEBUG
		Console_Print_Str("Using third person node");
#endif
	}
	else {
		if (!bIsThirdPerson) {
			parentNode = GetPipBoyNode(pPlayer, 0, 0);
#if _DEBUG

			Console_Print_Str("Using first person node");
#endif
		}
		else {
			parentNode = GetPipBoyNode(pPlayer, 0, 1);
#if _DEBUG
			Console_Print_Str("Using third person node");
#endif
		}
	}

	AttachChild(parentNode, pPipBoyLight, 0);
	updateData.bUpdateShadowSceneNode = true;
	parentNode->UpdateDownwardPass(&updateData, 0);
}

void MessageHandler(NVSEMessagingInterface::Message* msg)
{
	if (msg->type == NVSEMessagingInterface::kMessage_PreLoadGame) {
		pPipBoyLight = nullptr;
	}
	if (msg->type == NVSEMessagingInterface::kMessage_MainGameLoop) {
		UpdateLightSwitch();
	}
}

bool NVSEPlugin_Load(NVSEInterface* nvse) {
	if (!nvse->isEditor) {
		WriteRelCall(0x80E9B2, MagicTarget_GetParent);
		WriteRelCall(0x80EC4F, SetLocalTranslateXYZ);
		WriteRelCall(0x80EC67, GetPipBoyNode);

		((NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging))->RegisterListener(nvse->GetPluginHandle(), "NVSE", MessageHandler);
	}

	return true;
}