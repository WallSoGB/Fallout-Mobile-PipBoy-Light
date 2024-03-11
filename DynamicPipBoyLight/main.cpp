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
	info->name = "MobilePipBoyLight";
	info->version = 200;
	return true;
}

NiUpdateData kSSNUpdateData = NiUpdateData(0.f, false, false, false, false, true);

UInt32* (__cdecl* GetPipboyManager)() = (UInt32 * (__cdecl*)())0x705990;
bool(__thiscall* IsLightActive)(UInt32* pPipBoyManager) = (bool(__thiscall*)(UInt32*))0x967700;
bool(__stdcall* IsPipBoyOpen)() = (bool(__stdcall*)())0x7079B0;

static NiPointer<NiNode>		spScreenNode		= nullptr;
static NiPointer<NiAVObject>	spPipBoyLight		= nullptr;
static Actor*					pEffectActor		= nullptr;
static bool						bPrevIsThirdPerson	= false;
static bool						bPrevWeapState		= false;
static NiPoint3					kPipBoyLightPos		= NiPoint3(0.0f, 0.0f, 0.0f);

NiNode* PlayerCharacter::GetPipBoyNode(const bool abFirstPerson) const {
	static NiFixedString string = NiFixedString("PipboyLightEffect:0");

	NiNode* pPlayerNode = GetNode(abFirstPerson);
	if (!spScreenNode || spScreenNode->m_uiRefCount < 2) {
		spScreenNode = nullptr;
		NiGeometry* pLightGeometry = static_cast<NiGeometry*>(pPlayerNode->GetObjectByName(string));
		if (pLightGeometry) {
			spScreenNode = pLightGeometry->GetParent();
			NiGeometryData* pGeomData = pLightGeometry->GetModelData();
			if (pGeomData)
				kPipBoyLightPos = pGeomData->m_kBound.m_kCenter;
		}
	}

	return spScreenNode.m_pObject ? spScreenNode.m_pObject : pPlayerNode;
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
		static NiFixedString lightName = NiFixedString("PipboyLight");
		apThis->m_kLocal.m_Translate = kPipBoyLightPos;
		spPipBoyLight = apThis;
		spPipBoyLight->m_kName = lightName;
		pEffectActor = nullptr;
	}
	else {
		apThis->m_kLocal.m_Translate.x = x;
		apThis->m_kLocal.m_Translate.y = y;
		apThis->m_kLocal.m_Translate.z = z;
	}
}

static bool HandlePipBoy(PlayerCharacter* apPlayer) {
	bool bIsThirdPerson = apPlayer->bThirdPerson;
	if (!IsPipBoyOpen())
		return bIsThirdPerson;

	return true;
}

static void UpdateLightSwitch() {
	if (!IsLightActive(GetPipboyManager()) || spPipBoyLight.m_pObject == nullptr)
		return;

	if (spPipBoyLight->m_uiRefCount < 2) {
		spPipBoyLight = nullptr;
		return;
	}

	PlayerCharacter* pPlayer = PlayerCharacter::GetSingleton();
	bool bIsThirdPerson = HandlePipBoy(pPlayer);
	bool bWeaponOut = pPlayer->GetIsWeaponOut();

	if (bIsThirdPerson == bPrevIsThirdPerson && bPrevWeapState == bWeaponOut || (bIsThirdPerson && bPrevIsThirdPerson))
		return;

	bPrevIsThirdPerson = bIsThirdPerson;
	bPrevWeapState = bWeaponOut;

	// Determine parent node for light
	NiNode* pParentNode = !bWeaponOut ? pPlayer->GetPipBoyNode(false) : pPlayer->GetPipBoyNode(!bIsThirdPerson);
	pParentNode->AttachChild(spPipBoyLight, 1);
	spPipBoyLight->m_kLocal.m_Translate = kPipBoyLightPos;
	pParentNode->Update(kSSNUpdateData);
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