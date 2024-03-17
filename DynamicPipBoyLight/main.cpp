#include "nvse/PluginAPI.h"
#include <SafeWrite.h>
#include <GameData.hpp>
#include <fstream>
#include <string>

NVSEInterface* g_nvseInterface{};
IDebugLog	   gLog("logs\\MobilePipBoyLight.log");

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "MobilePipBoyLight";
	info->version = 200;
	return true;
}

NiUpdateData kSSNUpdateData = NiUpdateData(0.f, false, false, false, false, true);

UInt32* (__cdecl* GetPipboyManager)() = (UInt32 * (__cdecl*)())0x705990;
bool(__thiscall* IsLightActive)(UInt32* pPipBoyManager) = (bool(__thiscall*)(UInt32*))0x967700;
bool(__stdcall* IsPipBoyOpen)() = (bool(__stdcall*)())0x7079B0;
bool(__stdcall* IsMenuMode)() = (bool(__stdcall*)())0x702360;
bool(__stdcall* IsContainerMode)() = (bool(__stdcall*)())0x705050;
bool* const bIsConsoleOpen = (bool*)0x11DEA2E;

static NiPointer<NiNode>		spScreenNode		= nullptr;
static NiPointer<NiAVObject>	spPipBoyLight		= nullptr;
static Actor*					pEffectActor		= nullptr;
static bool						bPrevIsThirdPerson	= false;
static bool						bPrevWeapState		= false;
static bool						bLastFrameMenuMode	= false;
static NiPoint3					kPipBoyLightPos		= NiPoint3(0.0f, 0.0f, 0.0f);
static NiPoint3					kLastWorldPos		= NiPoint3(0.0f, 0.0f, 0.0f);

static bool IsInMenu() {
	return IsContainerMode() || IsMenuMode() || *bIsConsoleOpen;
}

// Gets the player's pipboy node
NiNode* PlayerCharacter::GetPipBoyNode(const bool abFirstPerson) const {
	static NiFixedString kScreenName = NiFixedString("PipboyLightEffect:0");

	NiNode* pPlayerNode = GetNode(abFirstPerson);
	NiGeometry* pLightGeometry = static_cast<NiGeometry*>(pPlayerNode->GetObjectByName(kScreenName));
	if (pLightGeometry) {
		spScreenNode = pLightGeometry->GetParent();
		NiGeometryData* pGeomData = pLightGeometry->GetModelData();
		if (pGeomData)
			kPipBoyLightPos = pGeomData->m_kBound.m_kCenter;
	}

	return spScreenNode.m_pObject ? spScreenNode.m_pObject : pPlayerNode;
}

static NiNode* __fastcall GetPipBoyNodeHook(PlayerCharacter* apThis, void*, bool abFirstPerson) {
	return apThis->GetPipBoyNode(!apThis->bThirdPerson);
}

// Gets the parent actor of the magic target
static Actor* __fastcall MagicTarget_GetParent(UInt32* apThis) {
	pEffectActor = ThisStdCall<Actor*>(0x822B40, apThis); // MagicTarget::GetParent itself
	return pEffectActor;
}

// Attaches pipboy light to the player's pipboy node, and sets the light's position to screen's center
static void __fastcall SetLocalTranslate(NiNode* apThis, void*, float x, float y, float z) {
	if (pEffectActor == PlayerCharacter::GetSingleton()) {
		static NiFixedString kLightName = NiFixedString("PipboyLight");
		apThis->m_kLocal.m_Translate = kPipBoyLightPos;
		spPipBoyLight = apThis;
		spPipBoyLight->m_kName = kLightName;
		pEffectActor = nullptr;
	}
	else {
		apThis->m_kLocal.m_Translate.x = x;
		apThis->m_kLocal.m_Translate.y = y;
		apThis->m_kLocal.m_Translate.z = z;
	}
}

// Handles the pipboy state
// If pipboy is open, returns true in order to use 3rd person animation to avoid sudden light movement
static bool HandlePipBoy(const PlayerCharacter* apPlayer) {
	return IsPipBoyOpen() ? true : apPlayer->bThirdPerson;
}

// Attaches the light to the player's pipboy node
static void SetPipBoyParent(const PlayerCharacter* apPlayer, const bool abThirdPerson, const bool abWeaponOut) {
	// Determine parent node for light
	NiNode* pParentNode = !abWeaponOut ? apPlayer->GetPipBoyNode(false) : apPlayer->GetPipBoyNode(!abThirdPerson);
	pParentNode->AttachChild(spPipBoyLight, true);
	spPipBoyLight->m_kLocal.m_Translate = kPipBoyLightPos;
	pParentNode->Update(kSSNUpdateData);
	spPipBoyLight->Update(kSSNUpdateData);
}

// Handles menu mode transition when player enters/exits menu mode
// This is only needed for the first person mode, because the light is not getting correct world transform in menu mode,
// leading to jarring light movement or it missing completely
static void HandleMenuMode(const PlayerCharacter* apPlayer, const bool abThirdPerson, const bool abWeaponOut) {
	if (spPipBoyLight.m_pObject == nullptr)
		return;

	bool bInMenu = IsInMenu();

	if (!bInMenu && !abThirdPerson)
		kLastWorldPos = spPipBoyLight->m_kWorld.m_Translate;

	// Detach the light if player enters menu mode
	if (bInMenu && !abThirdPerson && !bLastFrameMenuMode) {
		NiNode* pParentNode = spPipBoyLight->GetParent();
		if (pParentNode)
			pParentNode->DetachChildAlt(spPipBoyLight);

		spPipBoyLight->m_kLocal.m_Translate = kLastWorldPos;
		spPipBoyLight->Update(kSSNUpdateData);

		ShadowSceneNode* pSSN = BSShaderManager::GetShadowSceneNode(0);
		ShadowSceneLight* pSSL = pSSN->GetLight(spPipBoyLight);
		if (pSSL) {
			pSSL->kPointPosition = kLastWorldPos;
			pSSN->UpdateLightGeometryList(pSSL);
		}

		bLastFrameMenuMode = true;
		return;
	}

	// Reattach the light if player exits menu mode
	if (!bInMenu && bLastFrameMenuMode && spPipBoyLight->GetParent() == nullptr) {
		SetPipBoyParent(apPlayer, abThirdPerson, abWeaponOut);

		bLastFrameMenuMode = false;
	}
}

// Updates the light based on the player's and menu state
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

	HandleMenuMode(pPlayer, bIsThirdPerson, bWeaponOut);

	if (bIsThirdPerson == bPrevIsThirdPerson && bPrevWeapState == bWeaponOut || (bIsThirdPerson && bPrevIsThirdPerson))
		return;

	bPrevIsThirdPerson = bIsThirdPerson;
	bPrevWeapState = bWeaponOut;

	SetPipBoyParent(pPlayer, bIsThirdPerson, bWeaponOut);
}

static void MessageHandler(NVSEMessagingInterface::Message* msg) {
	if (msg->type == NVSEMessagingInterface::kMessage_PreLoadGame) {
		spPipBoyLight = nullptr;
	}
	else if (msg->type == NVSEMessagingInterface::kMessage_MainGameLoop) {
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