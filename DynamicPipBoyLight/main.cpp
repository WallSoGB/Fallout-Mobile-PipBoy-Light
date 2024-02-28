#include "nvse/PluginAPI.h"
#include <SafeWrite.h>
#include <GameData.hpp>
#include <fstream>
#include <string>

NVSEInterface* g_nvseInterface{};
IDebugLog	   gLog("logs\\MobilePipBoyLight.log");

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Mobile Pip-Boy Light";
	info->version = 110;
	return true;
}

NiUpdateData kSSNUpdateData = NiUpdateData(0.f, false, false, false, false, true);

UInt32* (__cdecl* GetPipboyManager)() = (UInt32 * (__cdecl*)())0x705990;
bool(__thiscall* IsLightActive)(UInt32* pPipBoyManager) = (bool(__thiscall*)(UInt32*))0x967700;

static NiPointer<NiNode>		spScreenNode	= nullptr;
static NiPointer<NiAVObject>	spPipBoyLight	= nullptr;
static Actor*					pEffectActor	= nullptr;
static bool						bPrevIsThirdPerson = 0;
static bool						bPrevWeapState = 0;

NiNode* PlayerCharacter::GetPipBoyNode(const bool abFirstPerson) const {
	static NiFixedString string = NiFixedString("PipboyLightEffect");
	NiNode* playerNode = GetNode(abFirstPerson);
	spScreenNode = static_cast<NiNode*>(playerNode->GetObjectByName(string));
	return spScreenNode.m_pObject ? spScreenNode.m_pObject : playerNode;
}

static NiNode* __fastcall GetPipBoyNodeHook(PlayerCharacter* apThis, void*, bool abFirstPerson) {
	return apThis->GetPipBoyNode(!apThis->bThirdPerson);
}



static Actor* __fastcall MagicTarget_GetParent(UInt32* apThis) {
	pEffectActor = ThisStdCall<Actor*>(0x822B40, apThis); // MagicTarget::GetParent itself
	return pEffectActor;
}

static void __fastcall SetLocalTranslate(NiNode* apThis, void*, float x, float y, float z) {
	if (pEffectActor == PlayerCharacter::GetSingleton()) {
		apThis->m_kLocal.m_Translate.x = 0.0f;
		apThis->m_kLocal.m_Translate.y = 0.0f;
		apThis->m_kLocal.m_Translate.z = 10.0f;
		spPipBoyLight = apThis;
		pEffectActor = nullptr;
	}
	else {
		apThis->m_kLocal.m_Translate.x = x;
		apThis->m_kLocal.m_Translate.y = y;
		apThis->m_kLocal.m_Translate.z = z;
	}
}

static void UpdateLightSwitch() {
	if (!IsLightActive(GetPipboyManager()) || spPipBoyLight.m_pObject == nullptr)
		return;

	if (spPipBoyLight->m_uiRefCount == 1 || spPipBoyLight->m_uiRefCount == 0) {
		spPipBoyLight = nullptr;
		return;
	}

	PlayerCharacter* pPlayer = PlayerCharacter::GetSingleton();
	bool bIsThirdPerson = pPlayer->bThirdPerson;
	bool bWeaponOut = pPlayer->GetIsWeaponOut();

	if (bIsThirdPerson == bPrevIsThirdPerson && bPrevWeapState == bWeaponOut || (bIsThirdPerson && bPrevIsThirdPerson))
		return;

	bPrevIsThirdPerson = bIsThirdPerson;
	bPrevWeapState = bWeaponOut;

	// Determine parent node for light
	NiNode* parentNode = !bWeaponOut ? pPlayer->GetPipBoyNode(false) : pPlayer->GetPipBoyNode(!bIsThirdPerson);
	parentNode->AttachChild(spPipBoyLight, 1);
	parentNode->Update(kSSNUpdateData);
}

static void MessageHandler(NVSEMessagingInterface::Message* msg)
{
	if (msg->type == NVSEMessagingInterface::kMessage_PreLoadGame) {
		spPipBoyLight = nullptr;
	}
	if (msg->type == NVSEMessagingInterface::kMessage_MainGameLoop) {
		UpdateLightSwitch();
	}
}

bool NVSEPlugin_Load(NVSEInterface* nvse) {
	if (!nvse->isEditor) {
		WriteRelCall(0x80E9B2, MagicTarget_GetParent);
		WriteRelCall(0x80EC4F, SetLocalTranslate);
		WriteRelCall(0x80EC67, GetPipBoyNodeHook);

		((NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging))->RegisterListener(nvse->GetPluginHandle(), "NVSE", MessageHandler);
	}

	return true;
}