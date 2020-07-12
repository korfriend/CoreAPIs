#include "ResourceManager.h"

#define NUM_SAFE_VOBJECTS 4096

#define __vmprintf //printf
#define VMERRORMESSAGE(a) ::MessageBoxA(NULL, a, NULL, MB_OK);

VmResourceManager::VmResourceManager()
{
	vobjvol_next_count_id = 0;
	vobjprim_next_count_id = 0;
	vobjvol_next_count_id_safe = 0;
	vobjprim_next_count_id_safe = 0;
	tobj_next_count_id = 0;
	iobj_next_count_id = 0;
	lobj_next_count_id = 0;
}

VmResourceManager::~VmResourceManager()
{
	EraseAllObjects();
}

VmObject* VmResourceManager::GetObject(int obj_id)
{
	EvmObjectType eObjectType = VmObject::GetObjectTypeFromID(obj_id);
	switch(eObjectType)
	{
	case ObjectTypeIMAGEPLANE:
		return GetIObject(obj_id);
	case ObjectTypePRIMITIVE:
	case ObjectTypeVOLUME:
		return GetVObject(obj_id);
	case ObjectTypeBUFFER:
		return GetLObject(obj_id);
	case ObjectTypeTMAP:
		return GetTObject(obj_id);
	case ObjectTypeNONE:
	default:
		break;
	}
	return NULL;
}

void VmResourceManager::EraseAllObjects()
{
	EraseAllIObjects();
	EraseAllVObjects();
	EraseAllTObjects();
	EraseAllLObjects();
}

bool VmResourceManager::EraseObject(const int obj_id)
{
	EvmObjectType eObjectType = VmObject::GetObjectTypeFromID(obj_id);
	switch(eObjectType)
	{
	case ObjectTypeIMAGEPLANE:
		return EraseIObject(obj_id);
	case ObjectTypePRIMITIVE:
	case ObjectTypeVOLUME:
		return EraseVObject(obj_id);
	case ObjectTypeBUFFER:
		return EraseLObject(obj_id);
	case ObjectTypeTMAP:
		return EraseTObject(obj_id);
	case ObjectTypeNONE:
	default:
		break;
	}
	return false;
}

int VmResourceManager::RegisterIObject(VmIObject* iobj, const int forced_obj_id)
{
	if(mapIObjects.size() == 65536)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterIObject - NO MORE POSSIBLE IOBJECT!!");
		return 0;
	}
	if(forced_obj_id != 0 && VmObject::GetObjectTypeFromID(forced_obj_id) != ObjectTypeIMAGEPLANE)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterVObject - forced_obj_id is NOT IObject!!");
		return 0;
	}

	iobj_next_count_id %= 65536;
	int iobj_id = (((int)ObjectTypeIMAGEPLANE) << 24) | (iobj_next_count_id & 0xFFFF);
	if(forced_obj_id == 0)
	{
		while(mapIObjects.find(iobj_id) != mapIObjects.end())
		{
			iobj_next_count_id = (iobj_next_count_id + 1)%65536;
			iobj_id = (((int)ObjectTypeIMAGEPLANE) << 24) | (iobj_next_count_id & 0xFFFF);
		}
		iobj_next_count_id++;
	}
	else
	{
		if(mapIObjects.find(forced_obj_id) != mapIObjects.end())
		{
			VMERRORMESSAGE("VmResourceManager::RegisterIObject - forced_obj_id is already Registered!!");
			return 0;
		}
		iobj_id = forced_obj_id;
	}

	iobj->SetObjectID(iobj_id);
	mapIObjects[iobj_id] = iobj;

	//__vmprintf("Image Plane Register Success : %x (Count-based ID %x)\n", iobj_id, iobj_next_count_id - 1);
	__vmprintf("itype_obj Register Success : %x \n", iobj_id);

	return iobj_id;
}

bool VmResourceManager::EraseIObject(const int iobj_id)
{
	map<int, VmIObject*>::iterator itrIObject = mapIObjects.find(iobj_id);
	if(itrIObject != mapIObjects.end())
	{
		__vmprintf("Removed %x Object\n", itrIObject->second->GetObjectID());
		VMSAFE_DELETE(itrIObject->second);
		mapIObjects.erase(itrIObject);
		return true;
	}
	return false;
}

void VmResourceManager::EraseAllIObjects()
{
	map<int, VmIObject*>::iterator itrIObject;
	for(itrIObject = mapIObjects.begin(); itrIObject != mapIObjects.end(); itrIObject++)
	{
		__vmprintf("Removed %x Object\n", itrIObject->second->GetObjectID());
		VMSAFE_DELETE(itrIObject->second);
	}
	mapIObjects.clear();
}

VmIObject* VmResourceManager::GetIObject(const int iobj_id)
{
	map<int, VmIObject*>::iterator itrIObject = mapIObjects.find(iobj_id);
	if(itrIObject != mapIObjects.end())
	{
		return itrIObject->second;
	}
	return NULL;
}

int VmResourceManager::RegisterTObject(VmTObject* tobj, const int forced_obj_id)
{
	if(mapTObjects.size() == 65536)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterTObject - NO MORE POSSIBLE TRANSFERFUNCTIONS!!");
		return 0;
	}
	if(forced_obj_id != 0 && VmObject::GetObjectTypeFromID(forced_obj_id) != ObjectTypeTMAP)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterTObject - forced_obj_id is NOT TObject!!");
		return 0;
	}

	tobj_next_count_id %= 65536;
	int tobj_id = (((int)ObjectTypeTMAP)<<24) | (tobj_next_count_id & 0xFFFF);
	if(forced_obj_id == 0)
	{
		while(mapTObjects.find(tobj_id) != mapTObjects.end())
		{
			tobj_next_count_id = (tobj_next_count_id + 1)%65536;
			tobj_id = (((int)ObjectTypeTMAP)<<24) | (tobj_next_count_id & 0xFFFF);
		}
		tobj_next_count_id++;
	}
	else
	{
		if(mapTObjects.find(forced_obj_id) != mapTObjects.end())
		{
			VMERRORMESSAGE("VmResourceManager::RegisterTObject - forced_obj_id is already Registered!!");
			return 0;
		}
		tobj_id = forced_obj_id;
	}

	tobj->SetObjectID(tobj_id);
	mapTObjects[tobj_id] = tobj;

	//__vmprintf("Transfer Function Register Success : %x (Count-based ID %x)\n", tobj_id, tobj_next_count_id - 1);
	__vmprintf("ttype_obj Register Success : %x \n", tobj_id);

	return tobj_id;
}

bool VmResourceManager::EraseTObject(const int tobj_id)
{
	map<int, VmTObject*>::iterator itrTObject = mapTObjects.find(tobj_id);
	if(itrTObject != mapTObjects.end())
	{
		__vmprintf("Removed %x Object\n", itrTObject->second->GetObjectID());

		for (auto itrVolume = mapVObjectVolumes.begin(); itrVolume != mapVObjectVolumes.end(); itrVolume++)
		{
			if (itrVolume->second->GetVolumeBlock(0))
				itrVolume->second->GetVolumeBlock(0)->DeleteTaggedActivatedBlocks(tobj_id);
			if (itrVolume->second->GetVolumeBlock(1))
				itrVolume->second->GetVolumeBlock(1)->DeleteTaggedActivatedBlocks(tobj_id);
		}

		VMSAFE_DELETE(itrTObject->second);
		mapTObjects.erase(itrTObject);
		return true;
	}
	return false;
}

void VmResourceManager::EraseAllTObjects()
{
	map<int, VmTObject*>::iterator itrTObject;
	for(itrTObject = mapTObjects.begin(); itrTObject != mapTObjects.end(); itrTObject++)
	{
		int tobj_id = itrTObject->second->GetObjectID();
		__vmprintf("Removed %x Object\n", tobj_id);

		for (auto itrVolume = mapVObjectVolumes.begin(); itrVolume != mapVObjectVolumes.end(); itrVolume++)
		{
			if (itrVolume->second->GetVolumeBlock(0))
				itrVolume->second->GetVolumeBlock(0)->DeleteTaggedActivatedBlocks(tobj_id);
			if (itrVolume->second->GetVolumeBlock(1))
				itrVolume->second->GetVolumeBlock(1)->DeleteTaggedActivatedBlocks(tobj_id);
		}

		VMSAFE_DELETE(itrTObject->second);
	}
	mapTObjects.clear();
}

VmTObject* VmResourceManager::GetTObject(const int tobj_id)
{
	map<int, VmTObject*>::iterator itrTObject = mapTObjects.find(tobj_id);
	if(itrTObject != mapTObjects.end())
	{
		return itrTObject->second;
	}
	return NULL;
}

int VmResourceManager::RegisterLObject(VmLObject* lobj, const int forced_obj_id)
{
	if(mapLObjects.size() == 65536)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterLObject - NO MORE POSSIBLE CUSTOMLIST!!");
		return 0;
	}
	if(forced_obj_id != 0 && VmObject::GetObjectTypeFromID(forced_obj_id) != ObjectTypeBUFFER)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterLObject - forced_obj_id is NOT LObject!!");
		return 0;
	}

	lobj_next_count_id %= 65536;
	int lobj_id = (((int)ObjectTypeBUFFER)<<24) | (lobj_next_count_id & 0xFFFF);
	if(forced_obj_id == 0)
	{
		while(mapLObjects.find(lobj_id) != mapLObjects.end())
		{
			lobj_next_count_id = (lobj_next_count_id + 1)%65536;
			lobj_id = (((int)ObjectTypeBUFFER)<<24) | (lobj_next_count_id & 0xFFFF);
		}
		lobj_next_count_id++;
	}
	else
	{
		if(mapLObjects.find(forced_obj_id) != mapLObjects.end())
		{
			VMERRORMESSAGE("VmResourceManager::RegisterLObject - forced_obj_id is already Registered!!");
			return 0;
		}
		lobj_id = forced_obj_id;
	}

	lobj->SetObjectID(lobj_id);
	mapLObjects[lobj_id] = lobj;

	//__vmprintf("Custom List Register Success : %x (Count-based ID %x)\n", lobj_id, lobj_next_count_id - 1);
	__vmprintf("ltype_obj Register Success : %x \n", lobj_id);

	return lobj_id;
}

bool VmResourceManager::EraseLObject(const int lobj_id)
{
	map<int, VmLObject*>::iterator itrLObject = mapLObjects.find(lobj_id);
	if(itrLObject != mapLObjects.end())
	{
		__vmprintf("Removed %x Object\n", itrLObject->second->GetObjectID());
		VMSAFE_DELETE(itrLObject->second);
		mapLObjects.erase(itrLObject);
		return true;
	}
	return false;
}

void VmResourceManager::EraseAllLObjects()
{
	map<int, VmLObject*>::iterator itrLObject;
	for(itrLObject = mapLObjects.begin(); itrLObject != mapLObjects.end(); itrLObject++)
	{
		__vmprintf("Removed %x Object\n", itrLObject->second->GetObjectID());
		VMSAFE_DELETE(itrLObject->second);
	}
	mapLObjects.clear();
}

VmLObject* VmResourceManager::GetLObject(const int lobj_id)
{
	map<int, VmLObject*>::iterator itrLObject = mapLObjects.find(lobj_id);
	if(itrLObject != mapLObjects.end())
	{
		return itrLObject->second;
	}
	return NULL;
}

template <typename T>
int __RegisterVObject(map<int, T*>* mapVObjects_ptr, int* vobj_cnt_id_ptr, bool is_safe_id, 
	VmVObject* vobj, EvmObjectType otype, int forced_obj_id)
{
	if(forced_obj_id != 0 && VmObject::GetObjectTypeFromID(forced_obj_id) != otype)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterVObject - forced_obj_id is NOT VObject!!");
		return 0;
	}

	int max_count = 65536 - NUM_SAFE_VOBJECTS;
	int id_offset = 0;
	if(is_safe_id)
	{
		max_count = NUM_SAFE_VOBJECTS;
		id_offset = 65536 - NUM_SAFE_VOBJECTS;
	}

	*vobj_cnt_id_ptr %= max_count;

	int vobj_id = (((int)otype)<<24) | ((*vobj_cnt_id_ptr + id_offset) & 0xFFFF);
	if(forced_obj_id == 0)
	{
		int iCount = 0;
		while(mapVObjects_ptr->find(vobj_id) != mapVObjects_ptr->end())
		{
			*vobj_cnt_id_ptr = ((*vobj_cnt_id_ptr) + 1)%max_count;
			vobj_id = (((int)otype)<<24) | ((*vobj_cnt_id_ptr + id_offset) & 0xFFFF);
			iCount++;
			if(iCount >= max_count)
			{
				VMERRORMESSAGE("VmResourceManager::RegisterVObject - NO MORE OBJECT!!");
				return 0;
			}
		}
		(*vobj_cnt_id_ptr)++;
	}
	else
	{
		if(mapVObjects_ptr->find(forced_obj_id) != mapVObjects_ptr->end())
		{
			VMERRORMESSAGE("VmResourceManager::RegisterVObject - forced_obj_id is already Registered!!");
			return 0;
		}
		vobj_id = forced_obj_id;
	}

	vobj->SetObjectID(vobj_id);
	(*mapVObjects_ptr)[vobj_id] = (T*)vobj;

	switch(otype)
	{
	case ObjectTypeVOLUME:
		//__vmprintf("Volume Register Success : %x (Count-based ID %x)\n", vobj_id, *vobj_cnt_id_ptr - 1);
		__vmprintf("model_vtype_obj Register Success : %x \n", vobj_id);
		break;
	case ObjectTypePRIMITIVE:
		//__vmprintf("Primitive Register Success : %x (Count-based ID %x)\n", vobj_id, *vobj_cnt_id_ptr - 1);
		__vmprintf("model_ptype_obj Register Success : %x \n", vobj_id);
		break;
	}

	return vobj_id;
}

int VmResourceManager::RegisterVObject(VmVObject* vobj, const EvmObjectType otype, const int forced_obj_id, const bool is_safe_id)
{
	if(otype != ObjectTypeVOLUME && otype != ObjectTypePRIMITIVE)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterVObject - NOT VObject!!");
		return 0;
	}
	if(otype == ObjectTypeVOLUME && mapVObjectVolumes.size() == 65536)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterVObject - NOT VObject!!");
		return 0;
	}
	if(otype == ObjectTypePRIMITIVE && mapVObjectPrimitives.size() == 65536)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterVObject - NOT VObject!!");
		return 0;
	}
	if(forced_obj_id != 0 && VmObject::GetObjectTypeFromID(forced_obj_id) != otype)
	{
		VMERRORMESSAGE("VmResourceManager::RegisterVObject - forced_obj_id is NOT VObject!!");
		return 0;
	}

	int vobj_id = 0;
	if(otype == ObjectTypeVOLUME)
	{
		int* cnt_id_ptr = &vobjvol_next_count_id;
		if(is_safe_id)
			cnt_id_ptr = &vobjvol_next_count_id_safe;
		vobj_id = __RegisterVObject<VmVObjectVolume>((map<int, VmVObjectVolume*>*)&mapVObjectVolumes, cnt_id_ptr, is_safe_id, vobj, otype, forced_obj_id);
	}
	else
	{
		int* cnt_id_ptr = &vobjprim_next_count_id;
		if(is_safe_id)
			cnt_id_ptr = &vobjprim_next_count_id_safe;
		vobj_id = __RegisterVObject<VmVObjectPrimitive>((map<int, VmVObjectPrimitive*>*)&mapVObjectPrimitives, cnt_id_ptr, is_safe_id, vobj, otype, forced_obj_id);
	}

	return vobj_id;
}

bool VmResourceManager::EraseVObject(const int vobj_id)
{
	if(VmObject::GetObjectTypeFromID(vobj_id) != ObjectTypePRIMITIVE &&
		VmObject::GetObjectTypeFromID(vobj_id) != ObjectTypeVOLUME)
	{
		VMERRORMESSAGE("VmResourceManager::EraseVObject - Not VOBJECT!");
		return false;
	}
	map<int, VmVObjectVolume*>::iterator itrVObjectVolume = mapVObjectVolumes.find(vobj_id);
	if(itrVObjectVolume != mapVObjectVolumes.end())
	{
		__vmprintf("Removed %x Object\n", itrVObjectVolume->second->GetObjectID());
		VmVObjectVolume* pCVObjectVolume = (VmVObjectVolume*)itrVObjectVolume->second;
		VMSAFE_DELETE(pCVObjectVolume);
		mapVObjectVolumes.erase(itrVObjectVolume);
		return true;
	}
	map<int, VmVObjectPrimitive*>::iterator itrVObjectPrimitive = mapVObjectPrimitives.find(vobj_id);
	if(itrVObjectPrimitive != mapVObjectPrimitives.end())
	{
		__vmprintf("Removed %x Object\n", itrVObjectPrimitive->second->GetObjectID());
		VmVObjectPrimitive* pCVObjectPrimitive = (VmVObjectPrimitive*)itrVObjectPrimitive->second;
		VMSAFE_DELETE(pCVObjectPrimitive);
		mapVObjectPrimitives.erase(itrVObjectPrimitive);
		return true;
	}
	return false;
}

void VmResourceManager::EraseAllVObjects()
{
	map<int, VmVObjectVolume*>::iterator itrVObjectVolume;
	for(itrVObjectVolume = mapVObjectVolumes.begin(); itrVObjectVolume != mapVObjectVolumes.end(); itrVObjectVolume++)
	{
		int vobj_id = itrVObjectVolume->second->GetObjectID();
		__vmprintf("Removed %x Object\n", vobj_id);
		VmVObjectVolume* pCVObjectVolume = (VmVObjectVolume*)itrVObjectVolume->second;
		VMSAFE_DELETE(pCVObjectVolume);
	}
	mapVObjectVolumes.clear();

	map<int, VmVObjectPrimitive*>::iterator itrVObjectPrimitive;
	for(itrVObjectPrimitive = mapVObjectPrimitives.begin(); itrVObjectPrimitive != mapVObjectPrimitives.end(); itrVObjectPrimitive++)
	{
		int vobj_id = itrVObjectPrimitive->second->GetObjectID();
		__vmprintf("Removed %x Object\n", vobj_id);
		VmVObjectPrimitive* pCVObjectPrimitive = (VmVObjectPrimitive*)itrVObjectPrimitive->second;
		VMSAFE_DELETE(pCVObjectPrimitive);
	}
	mapVObjectPrimitives.clear();
}

VmVObject* VmResourceManager::GetVObject(const int vobj_id)
{
	map<int, VmVObjectVolume*>::iterator itrVObjectVolume = mapVObjectVolumes.find(vobj_id);
	if(itrVObjectVolume != mapVObjectVolumes.end())
	{
		return itrVObjectVolume->second;
	}
	map<int, VmVObjectPrimitive*>::iterator itrVObjectPrimitive = mapVObjectPrimitives.find(vobj_id);
	if(itrVObjectPrimitive != mapVObjectPrimitives.end())
	{
		return itrVObjectPrimitive->second;
	}
	return NULL;
}

void VmResourceManager::GetAllocatedResources(uint* volume_size_kb /*out*/, uint* prim_size_kb /*out*/, uint* iobj_size_kb /*out*/, uint* etc_size_kb /*out*/)
{
	uint _volume_size_kb = 0, _prim_size_kb = 0, _iobj_size_kb = 0, _etc_size_kb = 0;
	for(map<int, VmVObjectVolume*>::iterator itrVObjs = mapVObjectVolumes.begin(); itrVObjs != mapVObjectVolumes.end(); itrVObjs++)
	{
		VmVObjectVolume* vol_obj = (VmVObjectVolume*)itrVObjs->second;
		VolumeData* vol_data = vol_obj->GetVolumeData();

		if(vol_data == NULL)
			continue;

		_volume_size_kb += (uint)(
			((uint)vol_data->vol_size.x) * ((uint)vol_data->vol_size.y)
			* ((uint)vol_data->vol_size.z) * (uint)(vol_data->store_dtype.type_bytes)
			/ 1024
			);

		if(vol_data->histo_values)
		{
			_volume_size_kb += (uint)(
				(vol_data->store_Mm_values.y - vol_data->store_Mm_values.x + 1.5) * sizeof(ullong) / 1024);
		}

		for(int i = 0; i < 2; i++)
		{
			VolumeBlocks* volblks = vol_obj->GetVolumeBlock(i);

			_volume_size_kb += (uint)(
				((ullong)volblks->blk_vol_size.x
				* (ullong)volblks->blk_vol_size.y
				* (ullong)volblks->blk_vol_size.z / 1024) * volblks->tflag_blks_map.size());

			if(volblks->mM_blks)
			{
				_volume_size_kb += (uint)(
					(ullong)volblks->blk_vol_size.x
					* (ullong)volblks->blk_vol_size.y
					* (ullong)volblks->blk_vol_size.z
					* (ullong)(volblks->dtype.type_bytes) * 2 / 1024 );
			}
		}
	}

	for(map<int, VmVObjectPrimitive*>::iterator itrVObjs = mapVObjectPrimitives.begin(); itrVObjs != mapVObjectPrimitives.end(); itrVObjs++)
	{
		VmVObjectPrimitive* prim_obj = (VmVObjectPrimitive*)itrVObjs->second;

		PrimitiveData* prim_data = prim_obj->GetPrimitiveData();

		if(prim_data == NULL)
			continue;

		_prim_size_kb += (uint)((ullong)prim_data->num_vtx * sizeof(vmfloat3) 
			* prim_data->GetNumVertexDefinitions() / 1024);

		if(prim_data->vidx_buffer)
			_prim_size_kb += (uint)((ullong)prim_data->num_vidx * sizeof(uint) / 1024);
	}

	for(map<int, VmIObject*>::iterator itrIObjs = mapIObjects.begin(); itrIObjs != mapIObjects.end(); itrIObjs++)
	{
		VmIObject* pCIObject = itrIObjs->second;

		vector<FrameBuffer>* pvtrRenderBuffers = pCIObject->GetBufferPointerList(FrameBufferUsageRENDEROUT);
		int iNumRenderBuffers = (int)pvtrRenderBuffers->size();
		for(int i = 0; i < iNumRenderBuffers; i++)
		{
			_iobj_size_kb += (uint)( pvtrRenderBuffers->at(i).w * pvtrRenderBuffers->at(i).h 
				* (uint)(pvtrRenderBuffers->at(i).dtype.type_bytes) / 1024 );
		}

		vector<FrameBuffer>* pvtrDepthBuffers = pCIObject->GetBufferPointerList(FrameBufferUsageDEPTH);
		int iNumDepthBuffers = (int)pvtrDepthBuffers->size();
		for(int i = 0; i < iNumDepthBuffers; i++)
		{
			_iobj_size_kb += (uint)( pvtrDepthBuffers->at(i).w * pvtrDepthBuffers->at(i).h 
				* (uint)(pvtrDepthBuffers->at(i).dtype.type_bytes) / 1024 );
		}

		vector<FrameBuffer>* pvtrCustomBuffers = pCIObject->GetBufferPointerList(FrameBufferUsageCUSTOM);
		int iNumCustomBuffers = (int)pvtrCustomBuffers->size();
		for(int i = 0; i < iNumCustomBuffers; i++)
		{
			_iobj_size_kb += (uint)( pvtrCustomBuffers->at(i).w * pvtrCustomBuffers->at(i).h 
				* (uint)(pvtrCustomBuffers->at(i).dtype.type_bytes) / 1024 );
		}

		vector<FrameBuffer>* pvtrAlignedBuffers = pCIObject->GetBufferPointerList(FrameBufferUsageALIGNEDSTURCTURE);
		int iNumAlignemdBuffers = (int)pvtrAlignedBuffers->size();
		for (int i = 0; i < iNumAlignemdBuffers; i++)
		{
			_iobj_size_kb += (uint)(pvtrAlignedBuffers->at(i).w * pvtrCustomBuffers->at(i).h
				* (uint)(pvtrAlignedBuffers->at(i).dtype.type_bytes) / 1024);
		}
	}

	for(map<int, VmTObject*>::iterator itrTObjs = mapTObjects.begin(); itrTObjs != mapTObjects.end(); itrTObjs++)
	{
		VmTObject* tobj = itrTObjs->second;
		TMapData* tmap_data = tobj->GetTMapData();
		if(tmap_data == NULL)
			continue;
		_etc_size_kb += uint(
			max(max(tmap_data->array_lengths.x, (int)1)
			* max(tmap_data->array_lengths.y, (int)1)
			* max(tmap_data->array_lengths.z, (int)1) * tmap_data->dtype.type_bytes / 1024, 1) );
	}

	for(map<int, VmLObject*>::iterator itrLObjs = mapLObjects.begin(); itrLObjs != mapLObjects.end(); itrLObjs++)
	{
		VmLObject* lobj = itrLObjs->second;
		
		_etc_size_kb += uint(
			lobj->GetSizeOfAllLists() / 1024 );
	}

	if(volume_size_kb)
		*volume_size_kb = _volume_size_kb;
	if(prim_size_kb)
		*prim_size_kb = _prim_size_kb;
	if(iobj_size_kb)
		*iobj_size_kb = _iobj_size_kb;
	if(etc_size_kb)
		*etc_size_kb = _etc_size_kb;
}