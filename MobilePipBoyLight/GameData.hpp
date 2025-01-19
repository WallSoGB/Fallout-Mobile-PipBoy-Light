#pragma once

#include "SafeWrite.h"
#include "Utilities.h"

#define ASSERT_SIZE(a, b) static_assert(sizeof(a) == b, "Wrong structure size!");
#define ASSERT_OFFSET(a, b, c) static_assert(offsetof(a, b) == c, "Wrong member offset!");
#define CREATE_OBJECT(CLASS, ADDRESS) static CLASS* CreateObject() { return StdCall<CLASS*>(ADDRESS); };

class BGSDistantObjectBlock;
class bhkBlendCollisionObject;
class bhkCollisionObject;
class bhkLimitedHingeConstraint;
class bhkRigidBody;
class BSFadeNode;
class BSMultiBoundNode;
class BSMultiBound;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiCloningProcess;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiStream;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;
class NiControllerManager;
class NiObjectGroup;
class NiObjectNET;
class NiBound;
class NiViewerStringsArray;
class NiUpdateData;
class NiMatrix3;
class NiCullingProcess;
class NiFixedString;
class NiGeometryData;
class NiSkinInstance;
class bhkNiCollisionObject;
class NiDX9Renderer;
class BSShader;
class BGSTextureUseMap;
class NiSourceTexture;
class RenderPassArray;
class RenderPass;
class BSShaderAccumulator;
class NiAdditionalGeometryData;
class NiGeometryBufferData;
class NiD3DPass;
class NiD3DShaderDeclaration;
class NiD3DRenderStateGroup;
class NiD3DShaderConstantMap;
class NiDX9ShaderDeclaration;
class NiDynamicEffectState;
class BGSDistantTreeBlock;
class BGSTerrainChunk;
class NiProperty;
class NiNode;
class BSMultiBoundRoom;
class TESObjectREFR;
class NiPropertyState;
class NiTimeController;

class NiFixedString {
public:
	NiFixedString() { m_kHandle = nullptr; }
	NiFixedString(const char* apString) : m_kHandle(0) {
		ThisStdCall(0x438170, this, apString); 
	}
	~NiFixedString() { CdeclCall(0x4381D0, this); };

	char* m_kHandle;

	NiFixedString& operator=(const char* apString) {
		return ThisStdCall<NiFixedString&>(0xA2E750, this, apString);
	}

	NiFixedString& operator=(NiFixedString& arString) {
		return ThisStdCall<NiFixedString&>(0x48AF40, this, &arString);
	}
};

class NiUpdateData {
public:
	NiUpdateData(float afTime = 0.f, bool abUpdateControllers = false, bool abIsMultiThreaded = false, bool abDeferUpdate = false, bool abUpdateGeomorphs = false, bool abUpdateShadowSceneNode = false)
		: fTime(afTime), bUpdateControllers(abUpdateControllers), bIsMultiThreaded(abIsMultiThreaded), bDeferUpdate(abDeferUpdate), bUpdateGeomorphs(abUpdateGeomorphs), bUpdateShadowSceneNode(abUpdateShadowSceneNode)
	{}
	~NiUpdateData() {};

	float fTime;
	bool bUpdateControllers;
	bool bIsMultiThreaded;
	bool bDeferUpdate;
	bool bUpdateGeomorphs;
	bool bUpdateShadowSceneNode;
};

class NiMatrix3 {
public:
	float m_pEntry[3][3];
};

class NiPoint2 {
public:
	float x, y;
};

class NiPoint3 {
public:
	float x, y, z;

	NiPoint3 operator-(const NiPoint3& akPoint) const {
		return NiPoint3(x - akPoint.x, y - akPoint.y, z - akPoint.z);
	}
};

class NiColorA {
public:
	float r, g, b, a;
};

class NiBound {
public:
	NiPoint3	m_kCenter;
	float		m_fRadius;
};

class NiTransform {
public:
	NiMatrix3	m_Rotate;
	NiPoint3	m_Translate;
	float		m_fScale;
};

class NiRTTI {
public:
	const char*		m_pcName;
	const NiRTTI*	m_pkBaseRTTI;
};

template <typename T_Data>
class NiTArray {
public:
	virtual ~NiTArray();

	T_Data* m_pBase;
	UInt16 m_usMaxSize;
	UInt16 m_usSize;
	UInt16 m_usESize;
	UInt16 m_usGrowBy;
};

ASSERT_SIZE(NiTArray<void*>, 0x10);

typedef void* NiTListIterator;

template <typename T_Data>
class NiTListItem {
public:
	NiTListItem*	m_pkNext;
	NiTListItem*	m_pkPrev;
	T_Data			m_element;
};

template <typename T_Data>
class NiTListBase {
public:
	NiTListItem<T_Data>*	m_pkHead;
	NiTListItem<T_Data>*	m_pkTail;
	UInt32					m_uiCount;

	inline UInt32 GetSize() const { return m_uiCount; };
	bool IsEmpty() const { return m_uiCount == 0; };

	NiTListIterator GetHeadPos() const { return m_pkHead; };
	NiTListIterator GetTailPos() const { return m_pkTail; };
};


template <typename T_Data>
class BSSimpleArray {
public:
	virtual			~BSSimpleArray();
	virtual T_Data* Allocate(UInt32 auiCount);
	virtual void    Deallocate(T_Data* apData);
	virtual T_Data* Reallocate(T_Data* apData, UInt32 auiCount);

	T_Data* pBuffer;
	UInt32	uiSize;
	UInt32	uiAllocSize;

	T_Data* GetAt(UInt32 idx) { return &pBuffer[idx]; }
};

template <class T_Data>
class NiPointer {
public:
	NiPointer(T_Data* apObject = (T_Data*)0) {
		m_pObject = apObject;
		if (m_pObject)
			m_pObject->IncRefCount();
	}
	NiPointer(const NiPointer& arPointer) {
		m_pObject = arPointer.m_pObject;
		if (m_pObject)
			m_pObject->IncRefCount();
	}
	~NiPointer() {
		if (m_pObject)
			m_pObject->DecRefCount();
	}

	T_Data* m_pObject;

	__forceinline NiPointer<T_Data>& operator =(const NiPointer& arPointer) {
		if (m_pObject != arPointer.m_pObject) {
			if (m_pObject)
				m_pObject->DecRefCount();
			m_pObject = arPointer.m_pObject;
			if (m_pObject)
				m_pObject->IncRefCount();
		}
		return *this;
	}

	__forceinline NiPointer<T_Data>& operator =(T_Data* apObject) {
		if (m_pObject != apObject) {
			if (m_pObject)
				m_pObject->DecRefCount();
			m_pObject = apObject;
			if (m_pObject)
				m_pObject->IncRefCount();
		}
		return *this;
	}

	__forceinline bool operator==(T_Data* apObject) const { return (m_pObject == apObject); }
	__forceinline bool operator==(const NiPointer& arPointer) const { return (m_pObject == arPointer.m_pObject); }
	__forceinline operator bool() const { return m_pObject != nullptr; }
	__forceinline operator T_Data* () const { return m_pObject; }
	__forceinline T_Data& operator*() const { return *m_pObject; }
	__forceinline T_Data* operator->() const { return m_pObject; }
};

class NiRefObject {
public:
    virtual			~NiRefObject();
    virtual void	DeleteThis();

    UInt32 m_uiRefCount;

    // 0x40F6E0
    inline void IncRefCount() {
        InterlockedIncrement(&m_uiRefCount);
    }

    // 0x401970
    inline void DecRefCount() {
        if (!InterlockedDecrement(&m_uiRefCount))
            DeleteThis();
    }
};

class NiObject : public NiRefObject {
public:
    virtual const NiRTTI*				GetRTTI() const;												// 02 | Returns NiRTTI of the object
	virtual NiNode*						IsNode() const;												// 03 | Returns this if it's a NiNode, otherwise nullptr
	virtual BSFadeNode*					IsFadeNode() const;												// 04 | Returns this if it's a BSFadeNode, otherwise nullptr
	virtual BSMultiBoundNode*			IsMultiBoundNode() const;										// 05 | Returns this if it's a BSMultiBoundNode, otherwise nullptr
	virtual NiGeometry*					IsGeometry() const;												// 06 | Returns this if it's a NiGeometry, otherwise nullptr
	virtual NiTriBasedGeom*				IsTriBasedGeometry() const;										// 07 | Returns this if it's a NiTriBasedGeom, otherwise nullptr
	virtual NiTriStrips*				IsTriStrips() const;											// 08 | Returns this if it's a NiTriStrips, otherwise nullptr
	virtual NiTriShape*					IsTriShape() const;												// 09 | Returns this if it's a NiTriShape, otherwise nullptr
	virtual BSSegmentedTriShape*		IsSegmentedTriShape() const;									// 10 | Returns this if it's a BSSegmentedTriShape, otherwise nullptr
	virtual BSResizableTriShape*		IsResizableTriShape() const;									// 11 | Returns this if it's a BSResizableTriShape, otherwise nullptr
	virtual NiParticles*				IsParticlesGeom() const;										// 12 | Returns this if it's a NiParticles, otherwise nullptr
	virtual NiLines*					IsLinesGeom() const;											// 13 | Returns this if it's a NiLines, otherwise nullptr
	virtual bhkCollisionObject*			IsBhkNiCollisionObject() const;									// 14 | Returns this if it's a bhkCollisionObject, otherwise nullptr
	virtual bhkBlendCollisionObject*	IsBhkBlendCollisionObject() const;								// 15 | Returns this if it's a bhkBlendCollisionObject, otherwise nullptr
	virtual bhkRigidBody*				IsBhkRigidBody() const;											// 16 | Returns this if it's a bhkRigidBody, otherwise nullptr
	virtual bhkLimitedHingeConstraint*	IsBhkLimitedHingeConstraint() const;							// 17 | Returns this if it's a bhkLimitedHingeConstraint, otherwise nullptr
	virtual NiObject*					CreateClone(NiCloningProcess* apCloning);						// 18 | Creates a clone of this object
	virtual void						LoadBinary(NiStream* apStream);									// 19 | Loads objects from disk
	virtual void						LinkObject(NiStream* apStream);									// 20 | Called by the streaming system to resolve links to other objects once it can be guaranteed that all objects have been loaded
	virtual void						RegisterStreamables(NiStream* apStream);						// 21 | When an object is inserted into a stream, it calls register streamables to make sure that any contained objects or objects linked in a scene graph are streamed as well
	virtual void						SaveBinary(NiStream* apStream);									// 22 | Saves objects to disk
	virtual bool						IsEqual(NiObject* apObject) const;								// 23 | Compares this object with another
	virtual void						GetViewerStrings(NiViewerStringsArray* apStrings);				// 24 | Gets strings containing information about the object
	virtual void						AddViewerStrings(NiViewerStringsArray* apStrings);				// 25 | Adds additional strings containing information about the object
	virtual void						ProcessClone(NiCloningProcess* apCloning);						// 26 | Post process for CreateClone
	virtual void						PostLinkObject(NiStream* apStream);								// 27 | Called by the streaming system to resolve any tasks that require other objects to be correctly linked. It is called by the streaming system after LinkObject has been called on all streamed objects
	virtual bool						StreamCanSkip();												// 28
	virtual const NiRTTI*				GetStreamableRTTI();											// 29
	virtual void						SetBound(NiBound* apNewBound);									// 30 | Replaces the bound of the object
	virtual void						GetBlockAllocationSize();										// 31 | Used by geometry data
	virtual NiObjectGroup*				GetGroup();														// 32 | Used by geometry data
	virtual void						SetGroup(NiObjectGroup* apGroup);								// 33 | Used by geometry data
	virtual NiControllerManager*		IsControllerManager() const;									// 34 | Returns this if it's a NiControllerManager, otherwise nullptr

	// 0x6532C0
	bool IsKindOf(const NiRTTI& apRTTI) const {
		for (const NiRTTI* i = GetRTTI(); i; i = i->m_pkBaseRTTI) {
			if (i == &apRTTI)
				return true;
		}
		return false;
	}
};

class NiObjectNET : public NiObject {
public:
	NiFixedString					m_kName;
	NiPointer<NiTimeController>		m_spControllers;
	void**							m_ppkExtra;
	UInt16							m_usExtraDataSize;
	UInt16							m_usMaxSize;
};

class NiAVObject : public NiObjectNET {
public:
	virtual void			UpdateControllers(NiUpdateData& arData);
	virtual void			ApplyTransform(NiMatrix3& arMat, NiPoint3& arTrn, bool abOnLeft);
	virtual void			Unk_39();
	virtual NiAVObject*		GetObject_(const NiFixedString& arName);
	virtual NiAVObject*		GetObjectByName(const NiFixedString& arName);
	virtual void			SetSelectiveUpdateFlags(bool& arSelectiveUpdate, BOOL abSelectiveUpdateTransforms, bool& arRigid);
	virtual void			UpdateDownwardPass(const NiUpdateData& arData, UInt32 auiFlags);
	virtual void			UpdateSelectedDownwardPass(const NiUpdateData& arData, UInt32 auiFlags);
	virtual void			UpdateRigidDownwardPass(const NiUpdateData& arData, UInt32 auiFlags);
	virtual void			UpdatePropertiesDownward(NiPropertyState* apParentState);
	virtual void			Unk_47();
	virtual void			UpdateWorldData(const NiUpdateData& arData);
	virtual void			UpdateWorldBound();
	virtual void			UpdateTransformAndBounds(const NiUpdateData& arData);
	virtual void			PreAttachUpdate(NiNode* apEventualParent, const NiUpdateData& arData);
	virtual void			PreAttachUpdateProperties(NiNode* apEventualParent);
	virtual void			DetachParent();
	virtual void			UpdateUpwardPassParent(void* arg);
	virtual void			OnVisible(NiCullingProcess* apCuller);
	virtual void			PurgeRendererData(NiDX9Renderer* apRenderer);

	NiNode*							m_pkParent;
	bhkNiCollisionObject*			m_spCollisionObject;
	NiBound*						m_pWorldBound;
	NiTListBase<NiProperty*>		m_kPropertyList;
	Bitfield32						m_uiFlags;
	NiTransform						m_kLocal;
	NiTransform						m_kWorld;

	// 0xA59C60
	void Update(NiUpdateData& arData) {
		ThisStdCall(0xA59C60, this, &arData);
	}

	NiNode* GetParent() const {
		return m_pkParent;
	}

};

class NiNode : public NiAVObject {
public:
	virtual void			AttachChild(NiAVObject* apChild, bool abFirstAvail);
	virtual void			InsertChildAt(UInt32 i, NiAVObject* apChild);
	virtual void			DetachChild(NiAVObject* apChild, NiAVObject*& aspAVObject);
	virtual void			DetachChildAlt(NiAVObject* apChild);
	virtual void			DetachChildAt(UInt32 i, NiAVObject*& aspAVObject);
	virtual NiAVObject*		DetachChildAtAlt(UInt32 i);
	virtual void			SetAt(UInt32 i, NiAVObject* apChild, NiAVObject*& aspAVObject);
	virtual void			SetAtAlt(UInt32 i, NiAVObject* apChild);
	virtual void			UpdateUpwardPass();

	NiTArray<NiAVObject*> m_kChildren;

    UInt32 GetChildCount() const {
		return m_kChildren.m_usESize;
    }

	UInt32 GetArrayCount() const {
		return m_kChildren.m_usSize;
	}

	NiAVObject* GetAt(UInt32 index) const {
		if (index >= GetArrayCount())
			return nullptr;

		return m_kChildren.m_pBase[index];
	}
};

class NiPropertyState {
public:
	enum PropertyID {
		ALPHA = 0,
		CULLING = 1,
		MATERIAL = 2,
		SHADE = 3,
		STENCIL = 4,
		TEXTURING = 5,
		UNK = 6,
		MAX,
	};

	NiProperty* m_aspProps[MAX];
};

class NiGeometry : public NiAVObject {
public:
	NiPropertyState	m_kProperties;
	NiGeometryData*	m_spModelData;
	NiSkinInstance*	m_spSkinInstance;
	BSShader*		m_pShader;

	NiGeometryData* GetModelData() const { return m_spModelData; };
};

class NiGeometryData : public NiObject {
public:
	enum Consistency {
		MUTABLE = 0x0000,
		STATIC = 0x4000,
		VOLATILE = 0x8000,
		CONSISTENCY_MASK = 0xF000,
	};

	enum KeepFlags {
		KEEP_NONE = 0x0,
		KEEP_XYZ = 0x1,
		KEEP_NORM = 0x2,
		KEEP_COLOR = 0x4,
		KEEP_UV = 0x8,
		KEEP_INDICES = 0x10,
		KEEP_BONEDATA = 0x20,
		KEEP_ALL = 0x3F,
	};

	enum Compression {
		COMPRESS_NORM = 0x1,
		COMPRESS_COLOR = 0x2,
		COMPRESS_UV = 0x4,
		COMPRESS_WEIGHT = 0x8,
		COMPRESS_POSITION = 0x10,
		COMPRESS_ALL = 0x1F,
	};

	enum MarkAsChangedFlags {
		VERTEX_MASK = 0x1,
		NORMAL_MASK = 0x2,
		COLOR_MASK = 0x4,
		TEXTURE_MASK = 0x8,
		DIRTY_MASK = 0xFFF,
	};

	UInt16						m_usVertices;
	UInt16						m_usID;
	UInt16						m_usDataFlags;
	UInt16						m_usDirtyFlags;
	NiBound						m_kBound;
	NiPoint3*					m_pkVertex;
	NiPoint3*					m_pkNormal;
	NiColorA*					m_pkColor;
	NiPoint2*					m_pkTexture;
	NiAdditionalGeometryData*	m_spAdditionalGeomData;
	NiGeometryBufferData*		m_pkBuffData;
	UInt8						m_ucKeepFlags;
	UInt8						m_ucCompressFlags;
	UInt8						Unk3A;
	UInt8						Unk3B;
	bool						m_bCanSave;
};

class TESWorldSpace;
class BGSTerrainNode;
class TESObjectREFR;

class TESForm {
public:
	void*	vtable;
	UInt8	cFormType;
	UInt8	jipFormFlags1;
	UInt8	jipFormFlags2;
	UInt8	jipFormFlags3;
	UInt32	iFormFlags;
	UInt32	uiFormID;
	UInt32	kMods[2];

	UInt32 GetFormID() const {
		return uiFormID;
	}
};

class TESObjectREFR : public TESForm {
public:
};

class ItemChange {
public:
	void*		pExtraLists;
	SInt32		iCountDelta;
	TESForm*	pObject;
};

class Actor : public TESObjectREFR {
public:
	bool GetIsWeaponOut() const {
		return ThisStdCall<bool>(0x8A16D0, this);
	}

	ItemChange* GetEquippedArmor(bool abHead) const {
		return ThisStdCall<ItemChange*>(0x89D8B0, this, abHead);
	}
};

class PlayerCharacter : public Actor {
public:
	UInt32 padding[0x630 / 4];
	bool padding2[3];
	bool bThirdPerson;

	static PlayerCharacter* GetSingleton() {
		return *(PlayerCharacter**)0x11DEA3C;
	}

	NiNode* GetCurrentNode() const {
		return ThisStdCall<NiNode*>(0x950BE0, this);
	}

	NiNode* GetNode(const bool abFirstPerson) const {
		return ThisStdCall<NiNode*>(0x950BB0, this, abFirstPerson);
	}

	NiNode* GetPipBoyNode(const bool abFirstPerson) const;

	bool HasPipBoyOpen() const {
		return ThisStdCall<bool>(0x967AE0, this);
	}
};
ASSERT_OFFSET(PlayerCharacter, bThirdPerson, 0x64B);

class ShadowSceneLight : public NiRefObject {
public:
	UInt32					filler[62];
	NiPoint3				kPointPosition;
};
ASSERT_OFFSET(ShadowSceneLight, kPointPosition, 0x100);

class ShadowSceneNode : public NiNode {
public:
	UInt32		filler[33];
	bool		bDisableLightUpdate;

	ShadowSceneLight* GetLight(NiAVObject* apLight) const {
		return ThisStdCall<ShadowSceneLight*>(0xB5B4A0, this, apLight);
	}

	void UpdateQueuedLight(ShadowSceneLight* apLight) {
		ThisStdCall(0xB5D300, this, apLight);
	}
};

ASSERT_OFFSET(ShadowSceneNode, bDisableLightUpdate, 0x130);


class BSShaderManager {
public:
	static ShadowSceneNode* GetShadowSceneNode(UInt32 aeType) {
		return ((ShadowSceneNode**)0x11F91C8)[aeType];
	}
};

class TESMain {
public:
	bool bOneMore;
	bool bQuitGame;
	bool bExitToMainMenu;
	bool bGameActive;
	bool bInGame;

	static TESMain* GetSingleton() {
		return *(TESMain**)0x11DEA0C;
	}
};

class BSUtilities {
public:
	static NiAVObject* GetObjectByName(NiAVObject* apScene, const NiFixedString& arName, bool abTestScenegraph = true) {
		return CdeclCall<NiAVObject*>(0xC4B470, apScene, &arName, abTestScenegraph);
	}
};