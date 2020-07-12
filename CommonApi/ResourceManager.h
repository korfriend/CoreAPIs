#pragma once

#include "../CommonUnits/VimCommon.h"

using namespace std;
using namespace vmobjects;
/**
 * @class VmResourceManager
 * @brief Framework 에서 VmObject 단위로 resource를 관리하는 class
 */
class VmResourceManager
{
private:
protected:
	/*!
	 * @cond
	 */
	int vobjvol_next_count_id;	// count-based ID for VmVObject, 0 to 65535
	int vobjprim_next_count_id;	// count-based ID for VmVObject, 0 to 65535
	int vobjvol_next_count_id_safe;	// count-based ID for VmVObject, 0 to NUM_SAFE_VOBJECTS
	int vobjprim_next_count_id_safe;	// count-based ID for VmVObject, 0 to NUM_SAFE_VOBJECTS
	int tobj_next_count_id;	// count-based ID for VmTObject, 0 to 65535
	int lobj_next_count_id;	// count-based ID for VmLObject, 0 to 65535
	int iobj_next_count_id;	// count-based ID for VmIObject, 0 to 65535
	// int : Object ID [8but : Object Type][8bit : Magic][16bit : Count-based ID]
	map<int, VmVObjectVolume*> mapVObjectVolumes;	// VmVObjectVolume, container
	map<int, VmVObjectPrimitive*> mapVObjectPrimitives;	// VmVObjectPrimitive, container
	map<int, VmIObject*> mapIObjects;	// VmTObject, container
	map<int, VmTObject*> mapTObjects;	// VmIObject, container
	map<int, VmLObject*> mapLObjects;	// VmLObject, container
	/*!
	 * @endcond
	 */
	
public:
	VmResourceManager();
	~VmResourceManager();

	// Object //
	/*!
	 * @brief 해당 ID의 등록된 VmObject를 얻는 함수
	 * @param obj_id [in] \n int \n module ID
	 * @return VmObject \n 등록된 module의 VmObject
	 * @remarks 최상위 object인 VmObject 가 반환
	 */
	VmObject* GetObject(const int obj_id);
	/*!
	 * @brief 등록된 모든 VmObject 를 삭제
	 */
	void EraseAllObjects();
	/*!
	 * @brief 해당 ID의 VmObject 를 삭제
	 * @param obj_id [in] \n int \n module ID
	 * @return bool \n 해당 ID의 VmObject가 등록되어 있고, 삭제가 성공하면 true, 그렇지 않으면 false 반환
	 */
	bool EraseObject(const int obj_id);

	// IObject //
	/*!
	 * @brief VmIObject를 등록하는 함수
	 * @param iobj [in] \n VmIObject \n VmIObject 를 정의하는 포인터
	 * @param forced_obj_id [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
	 * @return int \n 등록된 ID를 반환, 실패하면 0 반환
	 */
	int RegisterIObject(VmIObject* iobj, const int forced_obj_id);
	/*!
	 * @brief 해당 ID의 VmIObject를 삭제
	 * @param iobj_id [in] \n int \n VmIObject ID
	 * @return bool \n 해당 ID의 VmIObject가 등록되어 있고, 삭제에 성공하면 true, 그렇지 않으면 false 반환
	 */
	bool EraseIObject(const int iobj_id);
	/*!
	 * @brief 모든 VmIObject를 삭제
	 */
	void EraseAllIObjects();
	/*!
	 * @brief 해당 ID의 VmIObject를 얻는 함수
	 * @param iobj_id [in] \n int \n VmIObject ID
	 * @return VmIObject \n 해당 ID의 VmIObject가 정의되어 있는 CVXIObject 의 포인터
	 */
	VmIObject* GetIObject(const int iobj_id);
	
	// TObject //
	/*!
	 * @brief VmTObject를 등록하는 함수
	 * @param tobj [in] \n VmTObject \n VmTObject 를 정의하는 CVXIObject 의 포인터
	 * @param forced_obj_id [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
	 * @return int \n 등록된 ID를 반환, 실패하면 0 반환
	 */
	int RegisterTObject(VmTObject* tobj, const int forced_obj_id = 0);
	/*!
	 * @brief 해당 ID의 VmTObject를 삭제
	 * @param tobj_id [in] \n int \n VmTObject ID
	 * @return bool \n 해당 ID의 VmTObject가 등록되어 있고, 삭제에 성공하면 true, 그렇지 않으면 false 반환
	 */
	bool EraseTObject(const int tobj_id);
	/*!
	 * @brief 모든 VmTObjects를 삭제
	 */
	void EraseAllTObjects();
	/*!
	 * @brief 해당 ID의 VmTObjects를 얻는 함수
	 * @param tobj_id [in] \n int \n VmTObjects ID
	 * @return VmTObject \n 해당 ID의 VmTObjects가 정의되어 있는 VmTObject 의 포인터
	 */
	VmTObject* GetTObject(const int tobj_id);

	// LObject //
	/*!
	 * @brief VmLObject를 등록하는 함수
	 * @param pCLObject [in] \n VmLObject \n VmLObject 를 정의하는 VmLObject 의 포인터
	 * @param forced_obj_id [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
	 * @return int \n 등록된 ID를 반환, 실패하면 0 반환
	 */
	int RegisterLObject(VmLObject* lobj, const int forced_obj_id = 0);
	/*!
	 * @brief 해당 ID의 VmLObject를 삭제
	 * @param lobj_id [in] \n int \n VmLObject ID
	 * @return bool \n 해당 ID의 VmLObject가 등록되어 있고, 삭제에 성공하면 true, 그렇지 않으면 false 반환
	 */
	bool EraseLObject(const int lobj_id);
	/*!
	 * @brief 모든 VXLObject를 삭제
	 */
	void EraseAllLObjects();
	/*!
	 * @brief 해당 ID의 VmLObject를 얻는 함수
	 * @param lobj_id [in] \n int \n VmLObject ID
	 * @return VmLObject \n 해당 ID의 VmLObject가 정의되어 있는 VmLObject 의 포인터
	 */
	VmLObject* GetLObject(const int lobj_id);

	// VObject //
	/*!
	 * @brief VmVObject를 등록하는 함수
	 * @param vobj [in] \n VmVObject \n VmVObject 를 정의하는 포인터
	 * @param otype [in] \n EvmObjectType \n VmVObject type, ObjectTypeVOLUME or ObjectTypePRIMITIVE, 그 외엔 실패
	 * @param forced_obj_id [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
	 * @param is_safe_id [in] \n bool \n Object ID 할당 영역의 마지막에 Safe Zone 에서 ID를 할당하여 ID가 중복되지 않도록 함
	 * @return int \n 등록된 ID를 반환, 실패하면 0 반환
	 */
	int RegisterVObject(VmVObject* vobj, const EvmObjectType otype, const int forced_obj_id = 0, const bool is_safe_id = false);
	/*!
	 * @brief 해당 ID의 VmVObject를 삭제
	 * @param vobj_id [in] \n int \n VmVObject ID
	 * @return bool \n 해당 ID의 VXVObject가 등록되어 있고, 삭제에 성공하면 true, 그렇지 않으면 false 반환
	 */
	bool EraseVObject(const int vobj_id);
	/*!
	 * @brief 모든 VmVObject를 삭제
	 */
	void EraseAllVObjects();
	/*!
	 * @brief 해당 ID의 VmVObject를 얻는 함수
	 * @param vobj_id [in] \n int \n VmVObject ID
	 * @return VmVObject \n 해당 ID의 VmVObject가 정의되어 있는 포인터
	 */
	VmVObject* GetVObject(const int vobj_id);
	/*!
	 * @brief 현재 할당된 resource size (KBypes) 얻기
	 * @param volume_size_kb [out] \n uint \n Volume 사이즈를 저장하는 uint 포이터로 KB 단위
	 * @param prim_size_kb [out] \n uint \n Mesh 사이즈를 저장하는 uint 포이터로 KB 단위
	 * @param iobj_size_kb [out] \n uint \n Frame buffer 사이즈를 저장하는 uint 포이터로 KB 단위
	 * @param etc_size_kb [out] \n uint \n TObject 및 LObject 사이즈를 저장하는 uint 포이터로 KB 단위
	 * @remarks 포인터가 NULL 이면 값을 저장하지 않음
	 */
	void GetAllocatedResources(uint* volume_size_kb /*out*/, uint* prim_size_kb /*out*/, uint* iobj_size_kb /*out*/, uint* etc_size_kb /*out*/);
};
