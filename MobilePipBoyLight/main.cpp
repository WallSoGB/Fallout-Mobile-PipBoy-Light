#include "nvse/PluginAPI.h"
#include <SafeWrite.h>
#include <GameData.hpp>

NVSEInterface* g_nvseInterface{};

NiUpdateData kSSNUpdateData = NiUpdateData(0.f, false, false, false, false, true);

UInt32* (__cdecl* GetPipboyManager)() = (UInt32 * (__cdecl*)())0x705990;
bool(__thiscall* IsLightActive)(UInt32* pPipBoyManager) = (bool(__thiscall*)(UInt32*))0x967700;
void(__thiscall* InitScreenLight)(UInt32* pPipBoyManager) = (void(__thiscall*)(UInt32*))0x7F9840;
bool(__stdcall* IsPipBoyOpen)() = (bool(__stdcall*)())0x7079B0;

static NiPointer<NiNode>		spScreenNode		= nullptr;
static NiPointer<NiAVObject>	spPipBoyLight		= nullptr;
static Actor*					pEffectActor		= nullptr;
static bool						bPrevIsThirdPerson	= false;
static bool						bPrevWeapState		= false;
static UInt32					uiLastArmorID		= 0;
static NiPoint3					kPipBoyLightPos		= NiPoint3(0.0f, 0.0f, 0.0f);

// Gets the player's pipboy node
NiNode* PlayerCharacter::GetPipBoyNode(const bool abFirstPerson) const {
	NiNode* pPlayerNode = GetNode(abFirstPerson);
	NiGeometry* pLightGeometry = static_cast<NiGeometry*>(BSUtilities::GetObjectByName(pPlayerNode, "PipboyLightEffect:0"));
	if (pLightGeometry) {
		spScreenNode = pLightGeometry->GetParent();
		NiGeometryData* pGeomData = pLightGeometry->GetModelData();
		if (pGeomData)
			kPipBoyLightPos = pGeomData->m_kBound.m_kCenter;
	}
	else {
		NiAVObject* pHand = BSUtilities::GetObjectByName(pPlayerNode, "Bip01 Weapon2");
		if (!pHand)
			pHand = BSUtilities::GetObjectByName(pPlayerNode, "Bip01 L Hand");

		if (pHand) {
			spScreenNode = pHand->IsNode();
			kPipBoyLightPos = NiPoint3(0.0f, 0.0f, 0.0f);
		}
	}

	NiNode* pOutputNode = spScreenNode.m_pObject ? spScreenNode.m_pObject : pPlayerNode;

	return pOutputNode;
}

static NiNode* __fastcall GetPipBoyNodeHook(PlayerCharacter* apThis, void*, bool abFirstPerson) {
	return apThis->GetPipBoyNode(!apThis->bThirdPerson);;
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

static bool HandleArmorChange(const PlayerCharacter* apPlayer) {
	ItemChange* pArmor = apPlayer->GetEquippedArmor(false);
	UInt32 uiArmorID = 0;
	if (pArmor && pArmor->pObject)
		uiArmorID = pArmor->pObject->GetFormID();

	bool bArmorChanged = uiArmorID != uiLastArmorID;

	uiLastArmorID = uiArmorID;

	return bArmorChanged;
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
	if (pParentNode == nullptr)
		return;

	pParentNode->AttachChild(spPipBoyLight, true);
	spPipBoyLight->m_kLocal.m_Translate = kPipBoyLightPos;
	pParentNode->UpdateTransformAndBounds(kSSNUpdateData);
	spPipBoyLight->UpdateTransformAndBounds(kSSNUpdateData);
}

// Updates the light based on the player's and menu state
static void UpdateLightSwitch() {
	if (spPipBoyLight.m_pObject == nullptr)
		return;

	if (!IsLightActive(GetPipboyManager()) || (spPipBoyLight.m_pObject && spPipBoyLight->m_uiRefCount < 2)) {
		spPipBoyLight = nullptr;
		return;
	}

	PlayerCharacter* pPlayer = PlayerCharacter::GetSingleton();
	bool bArmorChanged = HandleArmorChange(pPlayer);
	bool bIsThirdPerson = HandlePipBoy(pPlayer);
	bool bWeaponOut = pPlayer->GetIsWeaponOut();

	if (!bArmorChanged) {
		if (bIsThirdPerson == bPrevIsThirdPerson && bPrevWeapState == bWeaponOut || (bIsThirdPerson && bPrevIsThirdPerson))
			return;
	}

	bPrevIsThirdPerson	= bIsThirdPerson;
	bPrevWeapState		= bWeaponOut;

	SetPipBoyParent(pPlayer, bIsThirdPerson, bWeaponOut);
}

static void MessageHandler(NVSEMessagingInterface::Message* msg) {
	switch (msg->type){
	case NVSEMessagingInterface::kMessage_PostLoad:
		{
			const PluginInfo* pInfo = ((NVSECommandTableInterface*)g_nvseInterface->QueryInterface(kInterface_CommandTable))->GetPluginInfoByName("ViewmodelShadingFix");
			if (pInfo && pInfo->version < 220) {
				MessageBoxA(nullptr, "\"Viewmodel Shading Fix\" version 2.2.0 or higher is required for this plugin to work.", "Mobile Pip-Boy Light", MB_OK | MB_ICONERROR);
			}
			else {
				WriteRelCall(0x80E9B2, MagicTarget_GetParent);
				WriteRelCall(0x80EC4F, SetLocalTranslate);
				WriteRelCall(0x80EC67, GetPipBoyNodeHook);
			}
		}
		break;
	case NVSEMessagingInterface::kMessage_PreLoadGame:
		spPipBoyLight = nullptr;
		break;
	case NVSEMessagingInterface::kMessage_PostLoadGame:
		HandleArmorChange(PlayerCharacter::GetSingleton());
		break;
	case NVSEMessagingInterface::kMessage_MainGameLoop:
		UpdateLightSwitch();
		break;
	default:
		break;
	}
}

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "MobilePipBoyLight";
	info->version = 220;
	return !nvse->isEditor;
}

bool NVSEPlugin_Load(NVSEInterface* nvse) {
	HMODULE hModule = GetModuleHandle("Viewmodel Shading Fix.dll");
	if (!hModule) {
		MessageBoxA(nullptr, "\"Viewmodel Shading Fix\" is required for this plugin to work.", "Mobile Pip-Boy Light", MB_OK | MB_ICONERROR);
		return false;
	}

	if (!nvse->isEditor) {
		g_nvseInterface = nvse;

		((NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging))->RegisterListener(nvse->GetPluginHandle(), "NVSE", MessageHandler);
	}

	return true;
}

BOOL WINAPI DllMain(
	HANDLE  hDllHandle,
	DWORD   dwReason,
	LPVOID  lpreserved
)
{
	return TRUE;
}
