#pragma once

#include "../CommonUnits/VimCommon.h"

using namespace std;
using namespace vmobjects;
/**
 * @class VmResourceManager
 * @brief Framework ���� VmObject ������ resource�� �����ϴ� class
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
	 * @brief �ش� ID�� ��ϵ� VmObject�� ��� �Լ�
	 * @param obj_id [in] \n int \n module ID
	 * @return VmObject \n ��ϵ� module�� VmObject
	 * @remarks �ֻ��� object�� VmObject �� ��ȯ
	 */
	VmObject* GetObject(const int obj_id);
	/*!
	 * @brief ��ϵ� ��� VmObject �� ����
	 */
	void EraseAllObjects();
	/*!
	 * @brief �ش� ID�� VmObject �� ����
	 * @param obj_id [in] \n int \n module ID
	 * @return bool \n �ش� ID�� VmObject�� ��ϵǾ� �ְ�, ������ �����ϸ� true, �׷��� ������ false ��ȯ
	 */
	bool EraseObject(const int obj_id);

	// IObject //
	/*!
	 * @brief VmIObject�� ����ϴ� �Լ�
	 * @param iobj [in] \n VmIObject \n VmIObject �� �����ϴ� ������
	 * @param forced_obj_id [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
	 * @return int \n ��ϵ� ID�� ��ȯ, �����ϸ� 0 ��ȯ
	 */
	int RegisterIObject(VmIObject* iobj, const int forced_obj_id);
	/*!
	 * @brief �ش� ID�� VmIObject�� ����
	 * @param iobj_id [in] \n int \n VmIObject ID
	 * @return bool \n �ش� ID�� VmIObject�� ��ϵǾ� �ְ�, ������ �����ϸ� true, �׷��� ������ false ��ȯ
	 */
	bool EraseIObject(const int iobj_id);
	/*!
	 * @brief ��� VmIObject�� ����
	 */
	void EraseAllIObjects();
	/*!
	 * @brief �ش� ID�� VmIObject�� ��� �Լ�
	 * @param iobj_id [in] \n int \n VmIObject ID
	 * @return VmIObject \n �ش� ID�� VmIObject�� ���ǵǾ� �ִ� CVXIObject �� ������
	 */
	VmIObject* GetIObject(const int iobj_id);
	
	// TObject //
	/*!
	 * @brief VmTObject�� ����ϴ� �Լ�
	 * @param tobj [in] \n VmTObject \n VmTObject �� �����ϴ� CVXIObject �� ������
	 * @param forced_obj_id [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
	 * @return int \n ��ϵ� ID�� ��ȯ, �����ϸ� 0 ��ȯ
	 */
	int RegisterTObject(VmTObject* tobj, const int forced_obj_id = 0);
	/*!
	 * @brief �ش� ID�� VmTObject�� ����
	 * @param tobj_id [in] \n int \n VmTObject ID
	 * @return bool \n �ش� ID�� VmTObject�� ��ϵǾ� �ְ�, ������ �����ϸ� true, �׷��� ������ false ��ȯ
	 */
	bool EraseTObject(const int tobj_id);
	/*!
	 * @brief ��� VmTObjects�� ����
	 */
	void EraseAllTObjects();
	/*!
	 * @brief �ش� ID�� VmTObjects�� ��� �Լ�
	 * @param tobj_id [in] \n int \n VmTObjects ID
	 * @return VmTObject \n �ش� ID�� VmTObjects�� ���ǵǾ� �ִ� VmTObject �� ������
	 */
	VmTObject* GetTObject(const int tobj_id);

	// LObject //
	/*!
	 * @brief VmLObject�� ����ϴ� �Լ�
	 * @param pCLObject [in] \n VmLObject \n VmLObject �� �����ϴ� VmLObject �� ������
	 * @param forced_obj_id [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
	 * @return int \n ��ϵ� ID�� ��ȯ, �����ϸ� 0 ��ȯ
	 */
	int RegisterLObject(VmLObject* lobj, const int forced_obj_id = 0);
	/*!
	 * @brief �ش� ID�� VmLObject�� ����
	 * @param lobj_id [in] \n int \n VmLObject ID
	 * @return bool \n �ش� ID�� VmLObject�� ��ϵǾ� �ְ�, ������ �����ϸ� true, �׷��� ������ false ��ȯ
	 */
	bool EraseLObject(const int lobj_id);
	/*!
	 * @brief ��� VXLObject�� ����
	 */
	void EraseAllLObjects();
	/*!
	 * @brief �ش� ID�� VmLObject�� ��� �Լ�
	 * @param lobj_id [in] \n int \n VmLObject ID
	 * @return VmLObject \n �ش� ID�� VmLObject�� ���ǵǾ� �ִ� VmLObject �� ������
	 */
	VmLObject* GetLObject(const int lobj_id);

	// VObject //
	/*!
	 * @brief VmVObject�� ����ϴ� �Լ�
	 * @param vobj [in] \n VmVObject \n VmVObject �� �����ϴ� ������
	 * @param otype [in] \n EvmObjectType \n VmVObject type, ObjectTypeVOLUME or ObjectTypePRIMITIVE, �� �ܿ� ����
	 * @param forced_obj_id [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
	 * @param is_safe_id [in] \n bool \n Object ID �Ҵ� ������ �������� Safe Zone ���� ID�� �Ҵ��Ͽ� ID�� �ߺ����� �ʵ��� ��
	 * @return int \n ��ϵ� ID�� ��ȯ, �����ϸ� 0 ��ȯ
	 */
	int RegisterVObject(VmVObject* vobj, const EvmObjectType otype, const int forced_obj_id = 0, const bool is_safe_id = false);
	/*!
	 * @brief �ش� ID�� VmVObject�� ����
	 * @param vobj_id [in] \n int \n VmVObject ID
	 * @return bool \n �ش� ID�� VXVObject�� ��ϵǾ� �ְ�, ������ �����ϸ� true, �׷��� ������ false ��ȯ
	 */
	bool EraseVObject(const int vobj_id);
	/*!
	 * @brief ��� VmVObject�� ����
	 */
	void EraseAllVObjects();
	/*!
	 * @brief �ش� ID�� VmVObject�� ��� �Լ�
	 * @param vobj_id [in] \n int \n VmVObject ID
	 * @return VmVObject \n �ش� ID�� VmVObject�� ���ǵǾ� �ִ� ������
	 */
	VmVObject* GetVObject(const int vobj_id);
	/*!
	 * @brief ���� �Ҵ�� resource size (KBypes) ���
	 * @param volume_size_kb [out] \n uint \n Volume ����� �����ϴ� uint �����ͷ� KB ����
	 * @param prim_size_kb [out] \n uint \n Mesh ����� �����ϴ� uint �����ͷ� KB ����
	 * @param iobj_size_kb [out] \n uint \n Frame buffer ����� �����ϴ� uint �����ͷ� KB ����
	 * @param etc_size_kb [out] \n uint \n TObject �� LObject ����� �����ϴ� uint �����ͷ� KB ����
	 * @remarks �����Ͱ� NULL �̸� ���� �������� ����
	 */
	void GetAllocatedResources(uint* volume_size_kb /*out*/, uint* prim_size_kb /*out*/, uint* iobj_size_kb /*out*/, uint* etc_size_kb /*out*/);
};
