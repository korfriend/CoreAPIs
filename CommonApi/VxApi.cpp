#include "VxApi.h"

#include "../GpuManager/GpuManager.h"
#include "ModuleArbiter.h"
#include "ResourceManager.h"

using namespace vmmath;
using namespace vmhelpers;
using namespace vmgpuinterface;

#define VMERRORMESSAGE printf

namespace vxengineapi {

	string dtype_bool_name = typeid(bool).name();
	string dtype_char_name = typeid(char).name();
	string dtype_byte_name = typeid(byte).name();
	string dtype_byte4_name = typeid(vmbyte4).name();
	string dtype_short_name = typeid(short).name();
	string dtype_ushort_name = typeid(ushort).name();
	string dtype_uint_name = typeid(uint).name();
	string dtype_int_name = typeid(int).name();
	string dtype_float_name = typeid(float).name();
	string dtype_double_name = typeid(double).name();
	string dtype_string_name = typeid(string).name();
	string dtype_vector_byte_name = typeid(vector<byte>).name();
	string dtype_vector_int_name = typeid(vector<int>).name();
	string dtype_vector_double_name = typeid(vector<double>).name();

	VmModuleArbiter* module_arbitor = NULL;
	VmResourceManager* res_manager = NULL;

	map<int, VmGpuManager*> gpu_manager;
	double g_dProgress = 0;

	// System Helper
	void VXEGetSystemMemoryInfo(ullong* pullFreeMemoryBytes, ullong* pullAvailablePhysicalMemoryBytes)
	{
		double free_bytes, valid_sysmem_bytes;
		GetSystemMemoryInfo(&free_bytes, &valid_sysmem_bytes);
		if (pullFreeMemoryBytes) *pullFreeMemoryBytes = (ullong)free_bytes;
		if (pullAvailablePhysicalMemoryBytes) *pullAvailablePhysicalMemoryBytes = (ullong)valid_sysmem_bytes;
	}

	void VXEGetAllocatedResources(uint* puiSizeOfVolumesKB /*out*/, uint* puiSizeOfMeshesKB /*out*/, uint* puiSizeOfIObjectsKB /*out*/, uint* puiSizeOfEtcKB /*out*/)
	{
		res_manager->GetAllocatedResources(puiSizeOfVolumesKB, puiSizeOfMeshesKB, puiSizeOfIObjectsKB, puiSizeOfEtcKB);
	}

	void VXEGetCPUInstructionInfo(int* piCPUInstructionInfo /*out*/)
	{
		GetCPUInstructionInfo(piCPUInstructionInfo);
	}

	// Pair
	bool VXEBeginEngineLib()
	{
		if (module_arbitor || res_manager || gpu_manager.size() != 0)
		{
			VMERRORMESSAGE("ERROR - ALREADY CREATED!!");
			return false;
		}

		res_manager = new VmResourceManager();
		module_arbitor = new VmModuleArbiter("Framework 3.0");

#if defined (_WIN64)
		//#if _MSC_VER >= 1800
		_set_FMA3_enable(0);
		//#endif
#endif
		return true;
	}

	bool VXEEndEngineLib()
	{
		if (res_manager)
		{
			// This is for Module DLL Static function's DELEGATION
			res_manager->EraseAllLObjects();
		}
		// module_arbitor must be deleted BEFORE g_pCResourceManager and g_pCGpuManager
		VMSAFE_DELETE(module_arbitor);
		VMSAFE_DELETE(res_manager);
		gpu_manager.clear();
		return true;
	}

	// Common Object
	bool VXEObjectIsGenerated(int iObjectID)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		return true;
	}

	bool VXEObjectRemove(int iObjectID)
	{
		if (VmObject::IsVObject(iObjectID))
		{
			// VObject Related Scenario //
			const VmVObject* pCVObject = res_manager->GetVObject(iObjectID);
			if (pCVObject)
			{
				//g_pCResourceManager->EraseReferencingObjectsByReferenceID(iObjectID);
				//g_pCGpuManager->ReleaseGpuResourcesBySrcID(iObjectID);

				res_manager->EraseObject(iObjectID);
			}
		}

		map<int, VmGpuManager*>::iterator itrGpuManager;
		for (itrGpuManager = gpu_manager.begin(); itrGpuManager != gpu_manager.end(); itrGpuManager++)
		{
			VmGpuManager* pCGM = itrGpuManager->second;
			pCGM->ReleaseGpuResourcesBySrcID(iObjectID);
		}

		return res_manager->EraseObject(iObjectID);
	}

	bool VXEObjectSetMent(int iObjectID, string strHelpMent)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		pCObject->SetDescriptor(strHelpMent);

		return true;
	}

	bool VXEObjectGetMent(int iObjectID, string* pstrHelpMent)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		if (pstrHelpMent == NULL)
			return true;

		*pstrHelpMent = pCObject->GetDescriptor();

		return true;
	}

	int VXEObjectGetRelatedObjectID(int iObjectID)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return 0;

		return pCObject->GetReferenceObjectID();
	}

	bool VXEObjectIsDefined(int iObjectID)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		return pCObject->IsDefined();
	}

	EvmObjectType VXEObjectGetTypeFromID(int iObjectID)
	{
		return VmObject::GetObjectTypeFromID(iObjectID);
	}

	bool VXEObjectSetCustomParameter(int iObjectID, string strParameterName, EnumVXRDataType eDataType, void* pvValue)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		switch (eDataType)
		{
		case vxrDataTypeSTRING:
			pCObject->RegisterCustomParameter(strParameterName, *(string*)pvValue); break;
		case vxrDataTypeBOOLEAN:
			pCObject->RegisterCustomParameter(strParameterName, *(bool*)pvValue); break;
		case vxrDataTypeINT:
			pCObject->RegisterCustomParameter(strParameterName, *(int*)pvValue); break;
		case vxrDataTypeINT2:
			pCObject->RegisterCustomParameter(strParameterName, *(vmint2*)pvValue); break;
		case vxrDataTypeINT3:
			pCObject->RegisterCustomParameter(strParameterName, *(vmint3*)pvValue); break;
		case vxrDataTypeINT4:
			pCObject->RegisterCustomParameter(strParameterName, *(vmint4*)pvValue); break;
		case vxrDataTypeDOUBLE:
			pCObject->RegisterCustomParameter(strParameterName, *(double*)pvValue); break;
		case vxrDataTypeDOUBLE2:
			pCObject->RegisterCustomParameter(strParameterName, *(vmdouble2*)pvValue); break;
		case vxrDataTypeDOUBLE3:
			pCObject->RegisterCustomParameter(strParameterName, *(vmdouble3*)pvValue); break;
		case vxrDataTypeDOUBLE4:
			pCObject->RegisterCustomParameter(strParameterName, *(vmdouble4*)pvValue); break;
		case vxrDataTypeMATRIX:
			pCObject->RegisterCustomParameter(strParameterName, *(vmmat44*)pvValue); break;
		default: return false;
		}
		return true;
	}

	bool VXEObjectGetCustomParameter(int iObjectID, string strParameterName, EnumVXRDataType eDataType, void* pvValue)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		data_type dtype;
		switch (eDataType)
		{
		case vxrDataTypeSTRING:
			dtype = data_type::dtype<string>(); break;
		case vxrDataTypeBOOLEAN:
			dtype = data_type::dtype<bool>(); break;
		case vxrDataTypeINT:
			dtype = data_type::dtype<int>(); break;
		case vxrDataTypeINT2:
			dtype = data_type::dtype<vmint2>(); break;
		case vxrDataTypeINT3:
			dtype = data_type::dtype<vmint3>(); break;
		case vxrDataTypeINT4:
			dtype = data_type::dtype<vmint4>(); break;
		case vxrDataTypeDOUBLE:
			dtype = data_type::dtype<double>(); break;
		case vxrDataTypeDOUBLE2:
			dtype = data_type::dtype<vmdouble2>(); break;
		case vxrDataTypeDOUBLE3:
			dtype = data_type::dtype<vmdouble3>(); break;
		case vxrDataTypeDOUBLE4:
			dtype = data_type::dtype<vmdouble4>(); break;
		case vxrDataTypeMATRIX:
			dtype = data_type::dtype<vmmat44>(); break;
		default: return false;
		}
		return pCObject->GetCustomParameter(strParameterName, dtype, pvValue);
	}

	double VXEFrameWorkGetProgress()
	{
		return g_dProgress;
	}

	// GPU Interface
	void VXEGpuCleanAllResources()
	{
		map<int, VmGpuManager*>::iterator itrGpuManager;
		for (itrGpuManager = gpu_manager.begin(); itrGpuManager != gpu_manager.end(); itrGpuManager++)
		{
			VmGpuManager* pCGM = itrGpuManager->second;
			pCGM->ReleaseAllGpuResources();
		}
	}

	void VXEGpuRemoveResource(int iObectID)
	{
		map<int, VmGpuManager*>::iterator itrGpuManager;
		for (itrGpuManager = gpu_manager.begin(); itrGpuManager != gpu_manager.end(); itrGpuManager++)
		{
			VmGpuManager* pCGM = itrGpuManager->second;
			pCGM->ReleaseGpuResourcesBySrcID(iObectID);
		}
	}

	bool VXEGpuGetMemoryBytes(uint* puiDedicatedGpuMemory/*out*/, uint* puiFreeMemory/*out*/)
	{
		if (gpu_manager.size() == 0)
			return false;

		ullong dedicated_bytes, free_bytes;
		map<int, VmGpuManager*>::iterator itrGpuManager;
		for (itrGpuManager = gpu_manager.begin(); itrGpuManager != gpu_manager.end(); itrGpuManager++)
		{
			VmGpuManager* pCGM = itrGpuManager->second;
			// Consider Priority
			if (pCGM->GetGpuManagerSDK() == GpuSdkTypeCUDA)
			{
				if (pCGM->GetGpuCurrentMemoryBytes(&dedicated_bytes, &free_bytes))
				{
					if (puiDedicatedGpuMemory) *puiDedicatedGpuMemory = (uint)dedicated_bytes;
					if (puiFreeMemory) *puiFreeMemory = (uint)free_bytes;
					return true;
				}
			}
			else if (pCGM->GetGpuManagerSDK() == GpuSdkTypeDX11)
			{
				if (pCGM->GetGpuCurrentMemoryBytes(&dedicated_bytes, &free_bytes))
				{
					if (puiDedicatedGpuMemory) *puiDedicatedGpuMemory = (uint)dedicated_bytes;
					if (puiFreeMemory) *puiFreeMemory = (uint)free_bytes;
					return true;
				}
			}
			else if (pCGM->GetGpuManagerSDK() == GpuSdkTypeOPENGL)
			{
				if (pCGM->GetGpuCurrentMemoryBytes(&dedicated_bytes, &free_bytes))
				{
					if (puiDedicatedGpuMemory) *puiDedicatedGpuMemory = (uint)dedicated_bytes;
					if (puiFreeMemory) *puiFreeMemory = (uint)free_bytes;
					return true;
				}
			}
			else if (pCGM->GetGpuManagerSDK() == GpuSdkTypeOPENCL)
			{
				if (pCGM->GetGpuCurrentMemoryBytes(&dedicated_bytes, &free_bytes))
				{
					if (puiDedicatedGpuMemory) *puiDedicatedGpuMemory = (uint)dedicated_bytes;
					if (puiFreeMemory) *puiFreeMemory = (uint)free_bytes;
					return true;
				}
			}
		}
		return false;
	}

	bool VXEGpuGetMemoryBytesUsedInVXFramework(uint* puiGpuAllMemoryBytes/*out*/, uint* puiGpuVObjectMemoryBytes/*optional out*/)
	{
		if (gpu_manager.size() == 0)
			return false;
		*puiGpuAllMemoryBytes = 0;
		map<int, VmGpuManager*>::iterator itrGpuManager;
		for (itrGpuManager = gpu_manager.begin(); itrGpuManager != gpu_manager.end(); itrGpuManager++)
		{
			VmGpuManager* pCGM = itrGpuManager->second;

			*puiGpuAllMemoryBytes += (uint)pCGM->GetUsedGpuMemorySizeBytes() * 1024;
		}
		return true;
	}

	// Common VObject
	bool VXEVObjectGetBoundingOrthoBox(int iObjectID, bool bIsBoxDefinedInOS /*false : WS*/, vmdouble3* pd3PosOrthoBoxMin/*out*/, vmdouble3* pd3PosOrthoBoxMax/*out*/)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		if (VmObject::IsVObject(iObjectID) == false)
			return false;

		VmVObject* pCVObject = (VmVObject*)pCObject;
		if (!pCVObject->IsGeometryDefined())
			return false;

		AaBbMinMax stOrthoBoundingBox;
		pCVObject->GetOrthoBoundingBox(stOrthoBoundingBox);
		if (bIsBoxDefinedInOS)
		{
			vmmat44* pmatOS2WS = &pCVObject->GetMatrixOS2WS();
			TransformPoint(&stOrthoBoundingBox.pos_min, &stOrthoBoundingBox.pos_min, pmatOS2WS);
			TransformPoint(&stOrthoBoundingBox.pos_max, &stOrthoBoundingBox.pos_max, pmatOS2WS);

			pd3PosOrthoBoxMin->x = (double)stOrthoBoundingBox.pos_min.x;
			pd3PosOrthoBoxMin->y = (double)stOrthoBoundingBox.pos_min.y;
			pd3PosOrthoBoxMin->z = (double)stOrthoBoundingBox.pos_min.z;
			pd3PosOrthoBoxMax->x = (double)stOrthoBoundingBox.pos_max.x;
			pd3PosOrthoBoxMax->y = (double)stOrthoBoundingBox.pos_max.y;
			pd3PosOrthoBoxMax->z = (double)stOrthoBoundingBox.pos_max.z;
		}
		else
		{
			vmdouble3 pos_minOS = stOrthoBoundingBox.pos_min;
			vmdouble3 pos_maxOS = stOrthoBoundingBox.pos_max;
			vmdouble3 f3PosOrthoPointsOS[8];

			f3PosOrthoPointsOS[0] = pos_minOS;

			f3PosOrthoPointsOS[1].x = pos_maxOS.x;
			f3PosOrthoPointsOS[1].y = pos_minOS.y;
			f3PosOrthoPointsOS[1].z = pos_minOS.z;

			f3PosOrthoPointsOS[2].x = pos_maxOS.x;
			f3PosOrthoPointsOS[2].y = pos_maxOS.y;
			f3PosOrthoPointsOS[2].z = pos_minOS.z;

			f3PosOrthoPointsOS[3].x = pos_minOS.x;
			f3PosOrthoPointsOS[3].y = pos_maxOS.y;
			f3PosOrthoPointsOS[3].z = pos_minOS.z;

			f3PosOrthoPointsOS[4].x = pos_minOS.x;
			f3PosOrthoPointsOS[4].y = pos_minOS.y;
			f3PosOrthoPointsOS[4].z = pos_maxOS.z;

			f3PosOrthoPointsOS[5].x = pos_maxOS.x;
			f3PosOrthoPointsOS[5].y = pos_minOS.y;
			f3PosOrthoPointsOS[5].z = pos_maxOS.z;

			f3PosOrthoPointsOS[6] = pos_maxOS;

			f3PosOrthoPointsOS[7].x = pos_minOS.x;
			f3PosOrthoPointsOS[7].y = pos_maxOS.y;
			f3PosOrthoPointsOS[7].z = pos_maxOS.z;

			pd3PosOrthoBoxMin->x = DBL_MAX;
			pd3PosOrthoBoxMin->y = DBL_MAX;
			pd3PosOrthoBoxMin->z = DBL_MAX;
			pd3PosOrthoBoxMax->x = -DBL_MAX;
			pd3PosOrthoBoxMax->y = -DBL_MAX;
			pd3PosOrthoBoxMax->z = -DBL_MAX;

			vmmat44* pmatOS2WS = &pCVObject->GetMatrixOS2WS();

			for (int i = 0; i < 8; i++)
			{
				vmdouble3 f3PosOrthoPointWS;
				TransformPoint(&f3PosOrthoPointWS, &f3PosOrthoPointsOS[i], pmatOS2WS);

				pd3PosOrthoBoxMin->x = (double)min(pd3PosOrthoBoxMin->x, f3PosOrthoPointWS.x);
				pd3PosOrthoBoxMin->y = (double)min(pd3PosOrthoBoxMin->y, f3PosOrthoPointWS.y);
				pd3PosOrthoBoxMin->z = (double)min(pd3PosOrthoBoxMin->z, f3PosOrthoPointWS.z);

				pd3PosOrthoBoxMax->x = (double)max(pd3PosOrthoBoxMax->x, f3PosOrthoPointWS.x);
				pd3PosOrthoBoxMax->y = (double)max(pd3PosOrthoBoxMax->y, f3PosOrthoPointWS.y);
				pd3PosOrthoBoxMax->z = (double)max(pd3PosOrthoBoxMax->z, f3PosOrthoPointWS.z);
			}
		}

		return true;
	}

	bool VXEVObjectGetMatrixBetweenOSandWS(int iObjectID, vmmat44* pmatOS2WS/*out*/, vmmat44* pmatWS2OS/*out*/)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		if (VmObject::IsVObject(iObjectID) == false)
			return false;

		VmVObject* pCVObject = (VmVObject*)pCObject;
		AaBbMinMax svxOrthoBoundingBox;
		vmmat44* pmatLoadOS2WS = &pCVObject->GetMatrixOS2WS();
		vmmat44* pmatLoadWS2OS = &pCVObject->GetMatrixWS2OS();
		if (pmatLoadOS2WS == NULL || pmatLoadWS2OS == NULL)
			return false;

		if (pmatOS2WS)
		{
			memcpy(pmatOS2WS, pmatLoadOS2WS, sizeof(vmmat44));
		}

		if (pmatWS2OS)
		{
			memcpy(pmatWS2OS, pmatLoadWS2OS, sizeof(vmmat44));
		}

		return true;
	}

	bool VXEVObjectSetMatrixOS2WS(int iObjectID, const vmmat44* pmatOS2WS)
	{
		VmObject* pCObject = (VmObject*)res_manager->GetObject(iObjectID);
		if (pCObject == NULL)
			return false;

		if (VmObject::IsVObject(iObjectID) == false)
			return false;

		VmVObject* pCVObject = (VmVObject*)pCObject;

		pCVObject->SetTransformMatrixOS2WS(*pmatOS2WS);

		return true;
	}

	bool VXEVObjectRemoveCustomObjects(int iObjectID)
	{
		VmVObject* pCVObject = (VmVObject*)res_manager->GetObject(iObjectID);
		if (pCVObject == NULL)
			return false;

		pCVObject->RemoveCustomDataStructures();
		return true;
	}

	// Volume Objects
	bool VXEVolumeGenerateNew(int* piObjectID/*out*/, int iForcedObjectID)
	{
		VmVObjectVolume* pCVObjectVolume = new VmVObjectVolume();

		int iObjectID = res_manager->RegisterVObject(pCVObjectVolume, ObjectTypeVOLUME, iForcedObjectID);
		if (iObjectID == 0)
		{
			VMSAFE_DELETE(pCVObjectVolume);
			return false;
		}
		if (piObjectID)
			*piObjectID = iObjectID;

		return true;
	}

	bool VXEVolumeCopy(int iObjectDestID, int iObjectSrcID)
	{
		if (VmVObject::GetObjectTypeFromID(iObjectDestID) != ObjectTypeVOLUME ||
			VmVObject::GetObjectTypeFromID(iObjectSrcID) != ObjectTypeVOLUME)
			return false;

		VmVObjectVolume* pCVObjectVolumeSrc = (VmVObjectVolume*)res_manager->GetVObject(iObjectSrcID);
		if (pCVObjectVolumeSrc == NULL)
			return false;
		//victory : source에 볼륨이 할당되지 않았을 때 로직 추가
		if ((VolumeData*)pCVObjectVolumeSrc->GetVolumeData() == NULL)
			return false;
		VmVObjectVolume* pCVObjectVolumeDest = (VmVObjectVolume*)res_manager->GetVObject(iObjectDestID);
		if (pCVObjectVolumeDest == NULL)
			return false;

		//victory : (VolumeData*)pCVObjectVolumeDest->GetVolumeData()가 null일때 delete시 뻑난다. 안전장치 추가.
		if ((VolumeData*)pCVObjectVolumeDest->GetVolumeData() != NULL)
			((VolumeData*)pCVObjectVolumeDest->GetVolumeData())->Delete();

		VolumeData* pstVolumeArchiveSrc = (VolumeData*)pCVObjectVolumeSrc->GetVolumeData();
		VolumeData svxVolumeArchiveDest = *pstVolumeArchiveSrc;

		svxVolumeArchiveDest.vol_slices = NULL;
		svxVolumeArchiveDest.histo_values = NULL;

		int iSizeOfSlice = (svxVolumeArchiveDest.vol_size.x + svxVolumeArchiveDest.bnd_size.x * 2)*(svxVolumeArchiveDest.vol_size.y + svxVolumeArchiveDest.bnd_size.y * 2);
		AllocateVoidPointer2D(&svxVolumeArchiveDest.vol_slices,
			svxVolumeArchiveDest.vol_size.z + svxVolumeArchiveDest.bnd_size.z * 2, iSizeOfSlice * svxVolumeArchiveDest.store_dtype.type_bytes);

		int iSizeOfType = svxVolumeArchiveDest.store_dtype.type_bytes;
		for (int i = 0; i < svxVolumeArchiveDest.vol_size.z + svxVolumeArchiveDest.bnd_size.z * 2; i++)
		{
			memcpy(svxVolumeArchiveDest.vol_slices[i], pstVolumeArchiveSrc->vol_slices[i], iSizeOfSlice*iSizeOfType);
		}

		uint uiRangeHistogram = svxVolumeArchiveDest.GetHistogramSize();
		svxVolumeArchiveDest.histo_values = new ullong[uiRangeHistogram];

		memcpy(svxVolumeArchiveDest.histo_values, pstVolumeArchiveSrc->histo_values, uiRangeHistogram * sizeof(ullong));

		if (pCVObjectVolumeSrc->GetVolumeBlock(0) == NULL)
		{
			pCVObjectVolumeDest->RegisterVolumeData(svxVolumeArchiveDest, NULL, 0);
		}
		else
		{
			vmint3 i3BlockSizes[2] = { pCVObjectVolumeSrc->GetVolumeBlock(0)->blk_vol_size, pCVObjectVolumeSrc->GetVolumeBlock(1)->blk_vol_size };
			pCVObjectVolumeDest->RegisterVolumeData(svxVolumeArchiveDest, i3BlockSizes, 0);
		}

		pCVObjectVolumeDest->SetTransformMatrixOS2WS(pCVObjectVolumeSrc->GetMatrixOS2WS());

		return true;
	}

	bool VXEVolumeGetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo/*out*/)
	{
		if (VmVObject::GetObjectTypeFromID(iObjectID) != ObjectTypeVOLUME)
			return false;

		VmVObjectVolume* pCVObjectVolume = (VmVObjectVolume*)res_manager->GetVObject(iObjectID);
		if (pCVObjectVolume == NULL)
			return false;

		const VolumeData* pstVolumeArchive = pCVObjectVolume->GetVolumeData();
		if (pstVolumeArchive == NULL)
		{
			psivxVolumeInfo->pullHistogram= NULL;
			memset(&psivxVolumeInfo->d2MinMaxValue, 0, sizeof(vmdouble2));
			memset(&psivxVolumeInfo->d2ActualMinMaxValue, 0, sizeof(vmdouble2));
			memset(&psivxVolumeInfo->i3VolumeSize, 0, sizeof(vmint3));
			memset(&psivxVolumeInfo->d3VoxelPitch, 0, sizeof(vmdouble3));
			psivxVolumeInfo->ppvVolumeSlices = NULL;
			psivxVolumeInfo->i3SizeExtraBoundary = vmint3(0, 0, 0);
		}
		else
		{
			psivxVolumeInfo->pullHistogram = pstVolumeArchive->histo_values;
			psivxVolumeInfo->d2MinMaxValue = pstVolumeArchive->store_Mm_values;
			psivxVolumeInfo->d2ActualMinMaxValue = pstVolumeArchive->actual_Mm_values;
			psivxVolumeInfo->i3VolumeSize = pstVolumeArchive->vol_size;
			psivxVolumeInfo->d3VoxelPitch.x = pstVolumeArchive->vox_pitch.x;
			psivxVolumeInfo->d3VoxelPitch.y = pstVolumeArchive->vox_pitch.y;
			psivxVolumeInfo->d3VoxelPitch.z = pstVolumeArchive->vox_pitch.z;
			psivxVolumeInfo->ppvVolumeSlices = pstVolumeArchive->vol_slices;
			psivxVolumeInfo->stored_dtype = pstVolumeArchive->store_dtype;
			psivxVolumeInfo->origin_dtype = pstVolumeArchive->origin_dtype;
			psivxVolumeInfo->i3SizeExtraBoundary = pstVolumeArchive->bnd_size;
			psivxVolumeInfo->svxAlignAxisOS2WS = pstVolumeArchive->axis_info;
		}

		return true;
	}

	bool VXEVolumeSetVolumeInfo(int iVObjectVolumeID, SIVXVolumeInfo* psivxVolumeInfo)
	{
		if (VmVObject::GetObjectTypeFromID(iVObjectVolumeID) != ObjectTypeVOLUME)
			return false;

		VmVObjectVolume* pCVObjectVolume = (VmVObjectVolume*)res_manager->GetVObject(iVObjectVolumeID);
		if (pCVObjectVolume == NULL)
			return false;

		VolumeData svxVolumeArchive;
		svxVolumeArchive.actual_Mm_values = psivxVolumeInfo->d2ActualMinMaxValue;
		svxVolumeArchive.store_Mm_values = psivxVolumeInfo->d2MinMaxValue;
		svxVolumeArchive.store_dtype = psivxVolumeInfo->stored_dtype;
		svxVolumeArchive.origin_dtype = psivxVolumeInfo->origin_dtype;
		svxVolumeArchive.vox_pitch = psivxVolumeInfo->d3VoxelPitch;
		svxVolumeArchive.vol_size = psivxVolumeInfo->i3VolumeSize;
		svxVolumeArchive.bnd_size = psivxVolumeInfo->i3SizeExtraBoundary;
		svxVolumeArchive.vol_slices = psivxVolumeInfo->ppvVolumeSlices;
		svxVolumeArchive.histo_values = psivxVolumeInfo->pullHistogram;

		g_dProgress = 0;
		LocalProgress svxProgress;
		svxProgress.range = 100;
		svxProgress.start = 0;
		svxProgress.progress_ptr = &g_dProgress;

		if (svxVolumeArchive.histo_values == NULL)
		{
			svxProgress.range = 80;
			svxProgress.Init();
			VmVObjectVolume::FillHistogram(svxVolumeArchive, &svxProgress);
			svxProgress.Deinit();

			svxProgress.range = 20;
		}

		svxProgress.Init();
		pCVObjectVolume->RegisterVolumeData(svxVolumeArchive, NULL, 0, &svxProgress);
		svxProgress.Deinit();

		return true;
	}

	template<typename T> void _VXEVolumeGetVolumeSliceInfo(SIVXVolumeSliceInfo* psivxVolumeSliceInfo/*out*/, const VolumeData* pstVolumeArchive, int iSliceIndex)
	{
		vmint3 i3SizeExtraBound = pstVolumeArchive->bnd_size;
		int iAddrWidth = pstVolumeArchive->vol_size.x + pstVolumeArchive->bnd_size.x * 2;
		T* ptSlice = (T*)pstVolumeArchive->vol_slices[iSliceIndex + pstVolumeArchive->bnd_size.z];

		double* pdValues = psivxVolumeSliceInfo->pdValues;

		psivxVolumeSliceInfo->d2MinMaxValue.x = DBL_MAX;
		psivxVolumeSliceInfo->d2MinMaxValue.y = -DBL_MAX;

		for (int i = 0; i < pstVolumeArchive->vol_size.y; i++)
		{
			for (int j = 0; j < pstVolumeArchive->vol_size.x; j++)
			{
				double dValue = (double)ptSlice[(i + i3SizeExtraBound.y)*iAddrWidth + j + i3SizeExtraBound.x];
				pdValues[i*pstVolumeArchive->vol_size.x + j] = dValue;

				psivxVolumeSliceInfo->d2MinMaxValue.x = min(psivxVolumeSliceInfo->d2MinMaxValue.x, dValue);
				psivxVolumeSliceInfo->d2MinMaxValue.y = max(psivxVolumeSliceInfo->d2MinMaxValue.y, dValue);
			}
		}

		psivxVolumeSliceInfo->i2DimSize.x = pstVolumeArchive->vol_size.x;
		psivxVolumeSliceInfo->i2DimSize.y = pstVolumeArchive->vol_size.y;
	}

	bool VXEVolumeGetVolumeZaxisSliceInfo(int iObjectID, int iSliceIndex, SIVXVolumeSliceInfo* psivxVolumeSliceInfo/*out*/)
	{
		VmVObjectVolume* pCVObjectVolume = (VmVObjectVolume*)res_manager->GetVObject(iObjectID);
		if (pCVObjectVolume == NULL)
			return false;

		const VolumeData* pstVolumeArchive = pCVObjectVolume->GetVolumeData();
		if (pstVolumeArchive == NULL)
			return false;

		if (iSliceIndex < 0 || iSliceIndex >= pstVolumeArchive->vol_size.z)
			return false;

		int iSizeArray = pstVolumeArchive->vol_size.x*pstVolumeArchive->vol_size.y;
		psivxVolumeSliceInfo->Delete();
		psivxVolumeSliceInfo->pdValues = new double[iSizeArray];

		if (pstVolumeArchive->store_dtype.type_name == dtype_byte_name)
			_VXEVolumeGetVolumeSliceInfo<byte>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_char_name)
			_VXEVolumeGetVolumeSliceInfo<char>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_ushort_name)
			_VXEVolumeGetVolumeSliceInfo<ushort>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_short_name)
			_VXEVolumeGetVolumeSliceInfo<short>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_uint_name)
			_VXEVolumeGetVolumeSliceInfo<uint>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_int_name)
			_VXEVolumeGetVolumeSliceInfo<int>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_float_name)
			_VXEVolumeGetVolumeSliceInfo<float>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_double_name)
			_VXEVolumeGetVolumeSliceInfo<double>(psivxVolumeSliceInfo, pstVolumeArchive, iSliceIndex);
		else 
		{
			psivxVolumeSliceInfo->Delete();
			return false;
		}
		return true;
	}

	template <typename T> void TrilinearSamples(const VolumeData* pstVolumeArchive, vmfloat3 f3PosStartVS, vmfloat3 f3PosEndVS, int iNumSamples, vector<double>* pvtrSampleValues)
	{
		T** pptVolume = (T**)pstVolumeArchive->vol_slices;

		vmint3 bnd_size = pstVolumeArchive->bnd_size;
		vmint3 vol_size = pstVolumeArchive->vol_size;
		int iSizeSampleWidth = vol_size.x + bnd_size.x * 2;

		double dDiffMaxMin = pstVolumeArchive->store_Mm_values.y - pstVolumeArchive->store_Mm_values.x;
		double dDiffActualMaxMin = pstVolumeArchive->actual_Mm_values.y - pstVolumeArchive->actual_Mm_values.x;

		if (dDiffMaxMin == 0)
		{
			for (int i = 0; i < iNumSamples; i++)
			{
				pvtrSampleValues->push_back(pstVolumeArchive->actual_Mm_values.x);
			}
			return;
		}

		double dValueRatio = dDiffActualMaxMin / dDiffMaxMin;
		double dValueOffset = pstVolumeArchive->actual_Mm_values.y - pstVolumeArchive->store_Mm_values.y*dValueRatio;

		for (int i = 0; i < iNumSamples; i++)
		{
			float fRatio = 1.f;
			if (iNumSamples > 1)
				fRatio = (float)i / (float)(iNumSamples - 1);
			vmfloat3 f3PosSampleVS;
			f3PosSampleVS.x = (1.f - fRatio)*f3PosStartVS.x + fRatio * f3PosEndVS.x;
			f3PosSampleVS.y = (1.f - fRatio)*f3PosStartVS.y + fRatio * f3PosEndVS.y;
			f3PosSampleVS.z = (1.f - fRatio)*f3PosStartVS.z + fRatio * f3PosEndVS.z;

			if (f3PosSampleVS.x >= -0.5f && f3PosSampleVS.x < (float)vol_size.x - 0.5f
				&& f3PosSampleVS.y >= -0.5f && f3PosSampleVS.y < (float)vol_size.y - 0.5f
				&& f3PosSampleVS.z >= -0.5f && f3PosSampleVS.z < (float)vol_size.z - 0.5f)
			{
#pragma region // Sampling
				vmint3 i3PosSample;
				i3PosSample.x = (int)floor(f3PosSampleVS.x);
				i3PosSample.y = (int)floor(f3PosSampleVS.y);
				i3PosSample.z = (int)floor(f3PosSampleVS.z);

				int iMinMaxAddrX = min(max(i3PosSample.x, 0), vol_size.x - 1) + bnd_size.x;
				int iMinMaxAddrNextX = min(max(i3PosSample.x + 1, 0), vol_size.x - 1) + bnd_size.x;
				int iMinMaxAddrY = (min(max(i3PosSample.y, 0), vol_size.y - 1) + bnd_size.y)*iSizeSampleWidth;
				int iMinMaxAddrNextY = (min(max(i3PosSample.y + 1, 0), vol_size.y - 1) + bnd_size.y)*iSizeSampleWidth;

				int iSampleAddr0 = iMinMaxAddrX + iMinMaxAddrY;
				int iSampleAddr1 = iMinMaxAddrNextX + iMinMaxAddrY;
				int iSampleAddr2 = iMinMaxAddrX + iMinMaxAddrNextY;
				int iSampleAddr3 = iMinMaxAddrNextX + iMinMaxAddrNextY;
				int iSampleAddrZ0 = i3PosSample.z + bnd_size.z;
				int iSampleAddrZ1 = i3PosSample.z + bnd_size.z + 1;

				if (i3PosSample.z < 0)
					iSampleAddrZ0 = iSampleAddrZ1;
				else if (i3PosSample.z >= vol_size.z - 1)
					iSampleAddrZ1 = iSampleAddrZ0;

				vmfloat3 f3InterpolateRatio;
				f3InterpolateRatio.x = f3PosSampleVS.x - i3PosSample.x;
				f3InterpolateRatio.y = f3PosSampleVS.y - i3PosSample.y;
				f3InterpolateRatio.z = f3PosSampleVS.z - i3PosSample.z;

				float fInterpolateWeights[8];
				fInterpolateWeights[0] = (1.f - f3InterpolateRatio.z)*(1.f - f3InterpolateRatio.y)*(1.f - f3InterpolateRatio.x);
				fInterpolateWeights[1] = (1.f - f3InterpolateRatio.z)*(1.f - f3InterpolateRatio.y)*f3InterpolateRatio.x;
				fInterpolateWeights[2] = (1.f - f3InterpolateRatio.z)*f3InterpolateRatio.y*(1.f - f3InterpolateRatio.x);
				fInterpolateWeights[3] = (1.f - f3InterpolateRatio.z)*f3InterpolateRatio.y*f3InterpolateRatio.x;
				fInterpolateWeights[4] = f3InterpolateRatio.z*(1.f - f3InterpolateRatio.y)*(1.f - f3InterpolateRatio.x);
				fInterpolateWeights[5] = f3InterpolateRatio.z*(1.f - f3InterpolateRatio.y)*f3InterpolateRatio.x;
				fInterpolateWeights[6] = f3InterpolateRatio.z*f3InterpolateRatio.y*(1.f - f3InterpolateRatio.x);
				fInterpolateWeights[7] = f3InterpolateRatio.z*f3InterpolateRatio.y*f3InterpolateRatio.x;

				float fSampleTrilinear = 0;
				fSampleTrilinear += (float)pptVolume[iSampleAddrZ0][iSampleAddr0] * fInterpolateWeights[0];
				fSampleTrilinear += (float)pptVolume[iSampleAddrZ0][iSampleAddr1] * fInterpolateWeights[1];
				fSampleTrilinear += (float)pptVolume[iSampleAddrZ0][iSampleAddr2] * fInterpolateWeights[2];
				fSampleTrilinear += (float)pptVolume[iSampleAddrZ0][iSampleAddr3] * fInterpolateWeights[3];

				fSampleTrilinear += (float)pptVolume[iSampleAddrZ1][iSampleAddr0] * fInterpolateWeights[4];
				fSampleTrilinear += (float)pptVolume[iSampleAddrZ1][iSampleAddr1] * fInterpolateWeights[5];
				fSampleTrilinear += (float)pptVolume[iSampleAddrZ1][iSampleAddr2] * fInterpolateWeights[6];
				fSampleTrilinear += (float)pptVolume[iSampleAddrZ1][iSampleAddr3] * fInterpolateWeights[7];

				double dActualSampleValue = (double)fSampleTrilinear*dValueRatio + dValueOffset;
				pvtrSampleValues->push_back(dActualSampleValue);
#pragma endregion
			}
			else
			{
				pvtrSampleValues->push_back(pstVolumeArchive->actual_Mm_values.x);
			}
		}
	}

	bool VXEVolumeGetActualValuesAlongLine(int iObjectID, vmfloat3 f3PosStartWS, vmfloat3 f3PosEndWS, int iNumSamples, vector<double>* pvtrSampleValues/*out*/)
	{
		VmVObjectVolume* pCVObjectVolume = (VmVObjectVolume*)res_manager->GetVObject(iObjectID);
		if (pCVObjectVolume == NULL)
			return false;

		vmdouble3 f3PosStartVS, f3PosEndVS;
		vmmat44* pmatWS2VS = &pCVObjectVolume->GetMatrixWS2OS();
		TransformPoint(&f3PosStartVS, &vmdouble3(f3PosStartWS), pmatWS2VS);
		TransformPoint(&f3PosEndVS, &vmdouble3(f3PosEndWS), pmatWS2VS);

		const VolumeData* pstVolumeArchive = pCVObjectVolume->GetVolumeData();

		if (pstVolumeArchive->store_dtype.type_name == dtype_byte_name)
			TrilinearSamples<byte>(pstVolumeArchive, f3PosStartVS, f3PosEndVS, iNumSamples, pvtrSampleValues);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_char_name)
			TrilinearSamples<char>(pstVolumeArchive, f3PosStartVS, f3PosEndVS, iNumSamples, pvtrSampleValues);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_ushort_name)
			TrilinearSamples<ushort>(pstVolumeArchive, f3PosStartVS, f3PosEndVS, iNumSamples, pvtrSampleValues);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_short_name)
			TrilinearSamples<short>(pstVolumeArchive, f3PosStartVS, f3PosEndVS, iNumSamples, pvtrSampleValues);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_float_name)
			TrilinearSamples<float>(pstVolumeArchive, f3PosStartVS, f3PosEndVS, iNumSamples, pvtrSampleValues);
		else if (pstVolumeArchive->store_dtype.type_name == dtype_double_name)
			TrilinearSamples<double>(pstVolumeArchive, f3PosStartVS, f3PosEndVS, iNumSamples, pvtrSampleValues);
		else
		{
			VMERRORMESSAGE("NOT YET SUPPORT! - VXEVolumeGetValues");
			return false;
		}

		return true;
	}

	bool VXEVolumeGetActualValue(int iObjectID, vmfloat3 f3PosSample, bool bIsVolumeSpacePosition, double* pdSampleValue/*out*/)
	{
		VmVObjectVolume* pCVObjectVolume = (VmVObjectVolume*)res_manager->GetVObject(iObjectID);
		if (pCVObjectVolume == NULL)
			return false;

		vmdouble3 d3PosSampleVS = vmdouble3(f3PosSample);
		if (!bIsVolumeSpacePosition)
		{
			vmmat44* pmatWS2VS = &pCVObjectVolume->GetMatrixWS2OS();
			TransformPoint(&d3PosSampleVS, &d3PosSampleVS, pmatWS2VS);
		}
		vmfloat3 f3PosSampleVS(d3PosSampleVS);

		VolumeData* pstVolArchive = pCVObjectVolume->GetVolumeData();
		*pdSampleValue = pstVolArchive->actual_Mm_values.x;
		vmint3 vol_size = pstVolArchive->vol_size;

		if (f3PosSampleVS.x >= -0.5f && f3PosSampleVS.x < (float)vol_size.x - 0.5f
			&& f3PosSampleVS.y >= -0.5f && f3PosSampleVS.y < (float)vol_size.y - 0.5f
			&& f3PosSampleVS.z >= -0.5f && f3PosSampleVS.z < (float)vol_size.z - 0.5f)
		{
#pragma region // Sampling
			vmint3 i3PosSample;
			i3PosSample.x = (int)floor(f3PosSampleVS.x);
			i3PosSample.y = (int)floor(f3PosSampleVS.y);
			i3PosSample.z = (int)floor(f3PosSampleVS.z);

			int iSizeSampleWidth = pstVolArchive->vol_size.x + pstVolArchive->bnd_size.x * 2;
			int iMinMaxAddrX = min(max(i3PosSample.x, 0), vol_size.x - 1) + pstVolArchive->bnd_size.x;
			int iMinMaxAddrNextX = min(max(i3PosSample.x + 1, 0), vol_size.x - 1) + pstVolArchive->bnd_size.x;
			int iMinMaxAddrY = (min(max(i3PosSample.y, 0), vol_size.y - 1) + pstVolArchive->bnd_size.y)*iSizeSampleWidth;
			int iMinMaxAddrNextY = (min(max(i3PosSample.y + 1, 0), vol_size.y - 1) + pstVolArchive->bnd_size.y)*iSizeSampleWidth;

			int iSampleAddr0 = iMinMaxAddrX + iMinMaxAddrY;
			int iSampleAddr1 = iMinMaxAddrNextX + iMinMaxAddrY;
			int iSampleAddr2 = iMinMaxAddrX + iMinMaxAddrNextY;
			int iSampleAddr3 = iMinMaxAddrNextX + iMinMaxAddrNextY;
			int iSampleAddrZ0 = i3PosSample.z + pstVolArchive->bnd_size.z;
			int iSampleAddrZ1 = i3PosSample.z + pstVolArchive->bnd_size.z + 1;

			if (i3PosSample.z < 0)
				iSampleAddrZ0 = iSampleAddrZ1;
			else if (i3PosSample.z >= vol_size.z - 1)
				iSampleAddrZ1 = iSampleAddrZ0;

			vmfloat3 f3InterpolateRatio;
			f3InterpolateRatio.x = f3PosSampleVS.x - i3PosSample.x;
			f3InterpolateRatio.y = f3PosSampleVS.y - i3PosSample.y;
			f3InterpolateRatio.z = f3PosSampleVS.z - i3PosSample.z;

			float fInterpolateWeights[8];
			fInterpolateWeights[0] = (1.f - f3InterpolateRatio.z)*(1.f - f3InterpolateRatio.y)*(1.f - f3InterpolateRatio.x);
			fInterpolateWeights[1] = (1.f - f3InterpolateRatio.z)*(1.f - f3InterpolateRatio.y)*f3InterpolateRatio.x;
			fInterpolateWeights[2] = (1.f - f3InterpolateRatio.z)*f3InterpolateRatio.y*(1.f - f3InterpolateRatio.x);
			fInterpolateWeights[3] = (1.f - f3InterpolateRatio.z)*f3InterpolateRatio.y*f3InterpolateRatio.x;
			fInterpolateWeights[4] = f3InterpolateRatio.z*(1.f - f3InterpolateRatio.y)*(1.f - f3InterpolateRatio.x);
			fInterpolateWeights[5] = f3InterpolateRatio.z*(1.f - f3InterpolateRatio.y)*f3InterpolateRatio.x;
			fInterpolateWeights[6] = f3InterpolateRatio.z*f3InterpolateRatio.y*(1.f - f3InterpolateRatio.x);
			fInterpolateWeights[7] = f3InterpolateRatio.z*f3InterpolateRatio.y*f3InterpolateRatio.x;

			float fSampleTrilinear = 0;
			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ0][iSampleAddr0] * fInterpolateWeights[0];
			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ0][iSampleAddr1] * fInterpolateWeights[1];
			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ0][iSampleAddr2] * fInterpolateWeights[2];
			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ0][iSampleAddr3] * fInterpolateWeights[3];

			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ1][iSampleAddr0] * fInterpolateWeights[4];
			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ1][iSampleAddr1] * fInterpolateWeights[5];
			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ1][iSampleAddr2] * fInterpolateWeights[6];
			fSampleTrilinear += (float)((ushort**)pstVolArchive->vol_slices)[iSampleAddrZ1][iSampleAddr3] * fInterpolateWeights[7];

			double dDiffMaxMin = pstVolArchive->store_Mm_values.y - pstVolArchive->store_Mm_values.x;
			double dDiffActualMaxMin = pstVolArchive->actual_Mm_values.y - pstVolArchive->actual_Mm_values.x;

			if (dDiffMaxMin != 0)
			{
				double dValueRatio = dDiffActualMaxMin / dDiffMaxMin;
				double dValueOffset = pstVolArchive->actual_Mm_values.y - pstVolArchive->actual_Mm_values.y*dValueRatio;

				double dActualSampleValue = (double)fSampleTrilinear*dValueRatio + dValueOffset;
				*pdSampleValue = dActualSampleValue;
			}
#pragma endregion
		}

		return true;
	}

	// Primitive Object //
	bool VXEPrimitiveGenerateNew(int* piObjectID/*out*/, int iForcedObjectID, bool bIsSafeID)
	{
		VmVObjectPrimitive* pCVObjectPrimitive = new VmVObjectPrimitive();

		int iObjectID = res_manager->RegisterVObject(pCVObjectPrimitive, ObjectTypePRIMITIVE, iForcedObjectID, bIsSafeID);
		if (iObjectID == 0)
		{
			VMSAFE_DELETE(pCVObjectPrimitive);
			return false;
		}
		if (piObjectID)
			*piObjectID = iObjectID;

		return true;
	}

	bool VXEPrimitiveCopy(int iObjectDestID, int iObjectSrcID)
	{
		if (VmVObject::GetObjectTypeFromID(iObjectDestID) != ObjectTypePRIMITIVE ||
			VmVObject::GetObjectTypeFromID(iObjectSrcID) != ObjectTypePRIMITIVE)
			return false;

		VmVObjectPrimitive* pCVObjectPrimitiveSrc = (VmVObjectPrimitive*)res_manager->GetVObject(iObjectSrcID);
		if (pCVObjectPrimitiveSrc == NULL)
			return false;

		if ((PrimitiveData*)pCVObjectPrimitiveSrc->GetPrimitiveData() == NULL)
			return false;

		VmVObjectPrimitive* pCVObjectPrimitiveDest = (VmVObjectPrimitive*)res_manager->GetVObject(iObjectDestID);
		if (pCVObjectPrimitiveDest == NULL)
			return false;

		//victory : (VolumeData*)pCVObjectVolumeDest->GetVolumeData()가 null일때 delete시 뻑난다. 안전장치 추가.
		if ((PrimitiveData*)pCVObjectPrimitiveDest->GetPrimitiveData() != NULL)
			((PrimitiveData*)pCVObjectPrimitiveDest->GetPrimitiveData())->Delete();

		PrimitiveData* pstPrimitiveArchiveSrc = (PrimitiveData*)pCVObjectPrimitiveSrc->GetPrimitiveData();
		PrimitiveData svxPrimitiveArchiveDest = *pstPrimitiveArchiveSrc;

		svxPrimitiveArchiveDest.ClearVertexDefinitionContainer();
		svxPrimitiveArchiveDest.vidx_buffer = NULL;

		string vtypes[7] = {
			"POSITION",
			"NORMAL",
			"TEXCOORD0",
			"TEXCOORD1",
			"TEXCOORD2",
			"TEXCOORD3",
			"TEXCOORD4"
		};

		for (int i = 0; i < (int)7; i++)
		{
			vmfloat3* pf3Vertex = pstPrimitiveArchiveSrc->GetVerticeDefinition(vtypes[i]);
			if (pf3Vertex)
			{
				vmfloat3* pf3VertexNew = new vmfloat3[pstPrimitiveArchiveSrc->num_vtx];
				memcpy(pf3VertexNew, pf3Vertex, sizeof(vmfloat3) * pstPrimitiveArchiveSrc->num_vtx);
				svxPrimitiveArchiveDest.ReplaceOrAddVerticeDefinition(vtypes[i], pf3VertexNew);
			}
		}

		svxPrimitiveArchiveDest.vidx_buffer = new uint[pstPrimitiveArchiveSrc->num_vidx];
		memcpy(svxPrimitiveArchiveDest.vidx_buffer, pstPrimitiveArchiveSrc->vidx_buffer, sizeof(uint) * pstPrimitiveArchiveSrc->num_vidx);

		if (pstPrimitiveArchiveSrc->texture_res_info.size() > 0)
		{
			// Always Byte Type
			svxPrimitiveArchiveDest.texture_res_info = pstPrimitiveArchiveSrc->texture_res_info;
			for (auto it = pstPrimitiveArchiveSrc->texture_res_info.begin(); it != pstPrimitiveArchiveSrc->texture_res_info.end(); it++)
			{
				int w = get<0>(it->second);
				int h = get<1>(it->second);
				int bypp = get<2>(it->second);

				auto& dst = svxPrimitiveArchiveDest.texture_res_info[it->first];
				get<3>(dst) = new byte[w * h * bypp];
				memcpy(get<3>(dst), get<3>(it->second), w * h * bypp);
			}
		}

		pCVObjectPrimitiveDest->RegisterPrimitiveData(svxPrimitiveArchiveDest);

		bool _visibility = true;
		pCVObjectPrimitiveSrc->GetCustomParameter("_bool_visibility", data_type::dtype<bool>(), &_visibility);
		vmdouble4 _color(1.);
		pCVObjectPrimitiveSrc->GetCustomParameter("_double4_color", data_type::dtype<vmdouble4>(), &_color);
		pCVObjectPrimitiveDest->RegisterCustomParameter("_bool_visibility", _visibility);
		pCVObjectPrimitiveDest->RegisterCustomParameter("_double4_color", _color);
		pCVObjectPrimitiveDest->SetTransformMatrixOS2WS(pCVObjectPrimitiveSrc->GetMatrixOS2WS());
		bool bVisibleWireSrc;
		vmdouble4 f4WireframeColorSrc;
		pCVObjectPrimitiveSrc->GetPrimitiveWireframeVisibilityColor(&bVisibleWireSrc, &f4WireframeColorSrc);
		pCVObjectPrimitiveDest->SetPrimitiveWireframeVisibilityColor(bVisibleWireSrc, f4WireframeColorSrc);

		pCVObjectPrimitiveDest->RemoveCustomDataStructures();

		return true;
	}

	bool VXEPrimitiveGetPrimitiveInfo(int iObjectID, SIVXPrimitiveInfo* psivxPrimitiveInfo/*out*/)
	{
		VmVObjectPrimitive* pCVObjectPrimitive = (VmVObjectPrimitive*)res_manager->GetVObject(iObjectID);
		if (pCVObjectPrimitive == NULL)
			return false;

		const PrimitiveData* pstPrimitiveArchive = pCVObjectPrimitive->GetPrimitiveData();
		if (pstPrimitiveArchive == NULL)
			return false;

		psivxPrimitiveInfo->uiNumVertice = pstPrimitiveArchive->num_vtx;
		psivxPrimitiveInfo->uiNumPrimitives = pstPrimitiveArchive->num_prims;
		psivxPrimitiveInfo->f3PosMaxBox = pstPrimitiveArchive->aabb_os.pos_max;
		psivxPrimitiveInfo->f3PosMinBox = pstPrimitiveArchive->aabb_os.pos_min;
		psivxPrimitiveInfo->bIsTopologyConnected = pstPrimitiveArchive->check_redundancy;
		return true;
	}

	bool VXEPrimitiveSetFrontFace(int iObjectID, bool bIsFrontCCW)
	{
		VmVObjectPrimitive* pCVObjectPrimitive = (VmVObjectPrimitive*)res_manager->GetVObject(iObjectID);
		if (pCVObjectPrimitive == NULL)
			return false;

		PrimitiveData* pstPrimitiveArchive = (PrimitiveData*)pCVObjectPrimitive->GetPrimitiveData();
		if (pstPrimitiveArchive == NULL)
			return false;

		pstPrimitiveArchive->is_ccw = bIsFrontCCW;
		return true;
	}

	bool VXPrimitiveSetColorVisibility(int iObjectID, vmfloat4 f4ColorMesh, bool bIsVisibleMesh, vmfloat4 f4ColorWire, bool bVisibleWire)
	{
		VmVObjectPrimitive* pCVObjectPrimitive = (VmVObjectPrimitive*)res_manager->GetVObject(iObjectID);
		if (pCVObjectPrimitive == NULL)
			return false;

		pCVObjectPrimitive->RegisterCustomParameter("_bool_visibility", bIsVisibleMesh);
		pCVObjectPrimitive->RegisterCustomParameter("_double4_color", vmdouble4(f4ColorMesh));
		pCVObjectPrimitive->SetPrimitiveWireframeVisibilityColor(bVisibleWire, vmdouble4(f4ColorWire));

		return true;
	}

	bool VXPrimitiveGetColorVisibility(int iObjectID, vmfloat4* pf4ColorMesh, bool* pbIsVisibleMesh, vmfloat4* pf4ColorWire, bool* pbVisibleWire)
	{
		VmVObjectPrimitive* pCVObjectPrimitive = (VmVObjectPrimitive*)res_manager->GetVObject(iObjectID);
		if (pCVObjectPrimitive == NULL)
			return false;

		bool _visibility = true;
		pCVObjectPrimitive->GetCustomParameter("_bool_visibility", data_type::dtype<bool>(), &_visibility);
		vmdouble4 _color(1.);
		pCVObjectPrimitive->GetCustomParameter("_double4_color", data_type::dtype<vmdouble4>(), &_color);

		if (pf4ColorMesh != NULL)
			*pf4ColorMesh = _color;
		if (pbIsVisibleMesh != NULL)
			*pbIsVisibleMesh = _visibility;

		vmdouble4 d4ColorTemp;
		bool bVisibleWireTemp;
		pCVObjectPrimitive->GetPrimitiveWireframeVisibilityColor(&bVisibleWireTemp, &d4ColorTemp);
		if (pf4ColorWire != NULL)
			*pf4ColorWire = d4ColorTemp;
		if (pbVisibleWire != NULL)
			*pbVisibleWire = bVisibleWireTemp;

		return true;
	}

	// CustomList Object //
	bool VXECustomListGenerateNew(int* piObjectID/*out*/, int iForcedObjectID)
	{
		VmLObject* pCLObject = new VmLObject();

		int iObjectID = res_manager->RegisterLObject(pCLObject, iForcedObjectID);
		if (iObjectID == 0)
		{
			VMSAFE_DELETE(pCLObject);
			return false;
		}
		if (piObjectID)
			*piObjectID = iObjectID;

		return true;
	}

	bool VXECustomListGetList(int iObjectID, string* pstrListName, void** ppvOutput/*out*/, size_t& size_bytes)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		return pCLObject->LoadBufferPtr(*pstrListName, ppvOutput, size_bytes);
	}

	bool VXECustomListGetStringObjs(int iObjectID, string* pstrListName, string** ppstrOutput/*out*/, int& num_stringobjs)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		return pCLObject->GetStringBuffer(*pstrListName, ppstrOutput, num_stringobjs);
	}

	bool VXECustomListSetList(int iObjectID, string* pstrListName, void* pvInput, int ele_num, int type_bytes, void* dstInput)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		pCLObject->ReplaceOrAddBufferPtr(*pstrListName, pvInput, ele_num, type_bytes, dstInput);
		return true;
	}

	bool VXECustomListSetStringObjs(int iObjectID, string* pstrListName, string* pvInput, int ele_num)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		pCLObject->ReplaceOrAddStringBuffer(*pstrListName, pvInput, ele_num);
		return true;
	}

	bool VXECustomListSetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, void* pvValue, size_t bytes_obj, void* pvValueDst)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		pCLObject->ReplaceOrAddDstObjValue(iTargetObjectID, *pstrListName, pvValue, (int)bytes_obj, pvValueDst);
		return true;
	}

	bool VXECustomListGetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, void* pvValue)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		return pCLObject->GetDstObjValue(iTargetObjectID, *pstrListName, pvValue);
	}

	bool VXECustomListCopyCustomValues(int iObjectSrcID, int iObjectDstID)
	{
		VmLObject* pCLObjectSrc = (VmLObject*)res_manager->GetLObject(iObjectSrcID);
		VmLObject* pCLObjectDst = (VmLObject*)res_manager->GetLObject(iObjectDstID);
		if (pCLObjectSrc == NULL || pCLObjectDst == NULL)
			return false;

		pCLObjectDst->CopyFrom(*pCLObjectSrc);

		return true;
	}

	bool VXECustomListRemoveCustomValueGroup(int iObjectID, int iTargetObjectID)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		return pCLObject->RemoveDstObjValues(iTargetObjectID);
	}

	bool VXECustomListRemoveAll(int iObjectID)
	{
		VmLObject* pCLObject = (VmLObject*)res_manager->GetLObject(iObjectID);
		if (pCLObject == NULL)
			return false;

		pCLObject->ClearAllBuffers();
		pCLObject->ClearAllDstObjValues();

		return true;
	}

	// Transfer Function Object //
	bool VXETransferfunctionGenerateNew(int* piObjectID/*out*/, int iForcedObjectID)
	{
		VmTObject* pCTObject = new VmTObject();

		int iObjectID = res_manager->RegisterTObject(pCTObject, iForcedObjectID);
		if (iObjectID == 0)
		{
			VMSAFE_DELETE(pCTObject);
			return false;
		}
		if (piObjectID)
			*piObjectID = iObjectID;


		return true;
	}

	bool VXETransferfunctionGetArchiveInfo(int iObjectID, SIVXTransferfunctionArchiveInfo* psivxTransferfunctionArchiveInfo/*out*/)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeTMAP)
		{
			VMERRORMESSAGE("VXETransferfunctionGetSingleDimState - UNAVAILABLE TYPE");
			return false;
		}

		VmTObject* pCTObject = res_manager->GetTObject(iObjectID);
		if (pCTObject == NULL)
			return false;
		const TMapData* pCTfArchive = pCTObject->GetTMapData();
		if (pCTfArchive == NULL)
			return false;

		psivxTransferfunctionArchiveInfo->iRefObjectID = pCTObject->GetReferenceObjectID();
		psivxTransferfunctionArchiveInfo->i3DimSizes = pCTfArchive->array_lengths;
		psivxTransferfunctionArchiveInfo->iNumDims = pCTfArchive->num_dim;
		psivxTransferfunctionArchiveInfo->i3ValidMinIndex = pCTfArchive->valid_min_idx;
		psivxTransferfunctionArchiveInfo->i3ValidMaxIndex = pCTfArchive->valid_max_idx;

		return true;
	}

	bool VXETransferfunctionGetOpticalValue(int iObjectID, SIVXTransferfunctionOpticalValue* psivxTransferfunctionOpticalValue/*out*/, int iTfIndexZ)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeTMAP)
		{
			VMERRORMESSAGE("VXETransferfunctionGetSingleDimManipulator - UNAVAILABLE TYPE");
			return false;
		}

		VmTObject* pCTObject = res_manager->GetTObject(iObjectID);
		if (pCTObject == NULL)
			return false;

		const TMapData* pstTfArchive = pCTObject->GetTMapData();
		if (pstTfArchive == NULL || pCTObject->IsDefined() == false)
			return false;

		double* pdValues = NULL;
		vmdouble3* pd3Colors = NULL;

		switch (pstTfArchive->num_dim)
		{
		case 1:
			psivxTransferfunctionOpticalValue->iSizeArray = pstTfArchive->array_lengths.x;
			iTfIndexZ = 0;
			break;
		case 2:
			if (pCTObject->GetDescriptor() == "Cpu_TransferfunctionGenerator_Preintegrated1D_Simple_By_Dojo_2010.11.19")
				// special case //
				psivxTransferfunctionOpticalValue->iSizeArray = pstTfArchive->array_lengths.x;
			else
				psivxTransferfunctionOpticalValue->iSizeArray = pstTfArchive->array_lengths.x*pstTfArchive->array_lengths.y;
			iTfIndexZ = 0;
			break;
		case 3:
			psivxTransferfunctionOpticalValue->iSizeArray = pstTfArchive->array_lengths.x*pstTfArchive->array_lengths.y*pstTfArchive->array_lengths.z;
			break;
		default:
			return false;
		}

		if (pstTfArchive->dtype.type_name != dtype_byte4_name)
			return false;

		psivxTransferfunctionOpticalValue->py4OpticalValue = (vmbyte4*)pstTfArchive->tmap_buffers[iTfIndexZ];

		return true;
	}

	// Image Plane Object //
	bool VXEImageplaneGenerateNew(vmint2 i2WindowSizePix, int* piObjectID/*out*/, int iForcedObjectID)
	{
		VmIObject* pCIObject = new VmIObject(i2WindowSizePix.x, i2WindowSizePix.y);
		int iObjectID = res_manager->RegisterIObject(pCIObject, iForcedObjectID);
		if (piObjectID)
			*piObjectID = iObjectID;

		if (iObjectID == 0)
		{
			VMSAFE_DELETE(pCIObject);
			return false;
		}

		return true;
	}

	bool VXEImageplaneGenerateBmpBindBuffer(int iObjectID)
	{
		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		if (!pCIObject->ReplaceFrameBuffer(FrameBufferUsageRENDEROUT, 0, data_type::dtype<vmbyte4>(), "Bitmap binding buffer generated in VXEImageplaneGenerateBMPBindBuffer"))
			pCIObject->InsertFrameBuffer(data_type::dtype<vmbyte4>(), FrameBufferUsageRENDEROUT, "Bitmap binding buffer generated in VXEImageplaneGenerateBMPBindBuffer");

		return true;
	}

	bool VXEImageplaneResize(int iObjectID, vmint2 i2WindowSizePix)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneResize - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		pCIObject->ResizeFrameBuffer(i2WindowSizePix.x, i2WindowSizePix.y);

		return true;
	}

	bool VXEImageplaneGetOutBufferSize(int iObjectID, vmint2* pi2BufferSize/*out*/)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetOutBufferInfo - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		pCIObject->GetFrameBufferInfo(pi2BufferSize);
		return true;
	}

	bool VXEImageplaneGetOutBufferInfo(int iObjectID, EvmFrameBufferUsage eFrameBufferUsage, int iBufferIndex, SIVXOutBufferInfo* pstBufferInfo/*out*/)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetOutBufferInfo - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		FrameBuffer* pstFrameBuffer = pCIObject->GetFrameBuffer(eFrameBufferUsage, iBufferIndex);
		if (pstFrameBuffer)
		{
#ifdef __WINDOWS
			pstBufferInfo->hFileMap = pstFrameBuffer->hFileMap;
#endif
			pstBufferInfo->i2FrameBufferSize.x = pstFrameBuffer->w;
			pstBufferInfo->i2FrameBufferSize.y = pstFrameBuffer->h;
			pstBufferInfo->pvBuffer = pstFrameBuffer->fbuffer;
			pstBufferInfo->dtype = pstFrameBuffer->dtype;

			return true;
		}
		return false;
	}

	bool VXEImageplaneSetCameraState(int iObjectID, const SIVXCameraStateDescription* psivxCameraStateDescriptor)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetOutBufferInfo - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		VmCObject* pCCObject = pCIObject->GetCameraObject();
		if (pCCObject == NULL)
		{
			//return false;
			// Dummy Setting
			AaBbMinMax svxViewStage;
			svxViewStage.pos_max.x = 1.f; svxViewStage.pos_max.y = 1.f; svxViewStage.pos_max.z = 1.f;
			pCIObject->AttachCamera(svxViewStage, StageViewORTHOBOXOVERVIEW);
			pCCObject = pCIObject->GetCameraObject();
		}

		SIVXCameraStateDescription* psixvState = (SIVXCameraStateDescription*)psivxCameraStateDescriptor;
		double fp = (double)psixvState->fFarPlaneDistFromCamera;
		double np = (double)psixvState->fNearPlaneDistFromCamera;
		double dfit_fovy = (double)psixvState->fFittingFovY;
		double dfovy = (double)psixvState->fFovY;
		pCCObject->SetCameraExtState(&vmdouble3(psixvState->f3PosCamera), &vmdouble3(psixvState->f3VecView), &vmdouble3(psixvState->f3VecUp));
		pCCObject->SetCameraIntState(&vmdouble2(psixvState->f2ImageplaneSize), &np, &fp, NULL, &vmdouble2(psixvState->f2FittingSize), &dfit_fovy);
		pCCObject->SetPerspectiveViewingState(psivxCameraStateDescriptor->bIsPerspective, &dfovy);
		return true;
	}

	bool VXEImageplaneGetCameraState(int iObjectID, SIVXCameraStateDescription* psivxCameraStateDescriptor/*out*/)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetOutBufferInfo - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		VmCObject* pCCObject = pCIObject->GetCameraObject();
		if (pCCObject == NULL)
			return false;

		pCCObject->GetCameraExtStatef(&psivxCameraStateDescriptor->f3PosCamera, 
			&psivxCameraStateDescriptor->f3VecView, &psivxCameraStateDescriptor->f3VecUp);

		vmdouble2 ipsize, fitsize;
		double np, fp, fov, fitfov;
		pCCObject->GetCameraIntState(&ipsize, &np, &fp, NULL, &fitsize, &fitfov);

		psivxCameraStateDescriptor->f2ImageplaneSize = ipsize;
		psivxCameraStateDescriptor->fNearPlaneDistFromCamera = (float)np;
		psivxCameraStateDescriptor->fFarPlaneDistFromCamera = (float)fp;
		psivxCameraStateDescriptor->f2FittingSize = fitsize;
		psivxCameraStateDescriptor->fFittingFovY = (float)fitfov;
		psivxCameraStateDescriptor->bIsPerspective = pCCObject->GetPerspectiveViewingState(&fov);
		psivxCameraStateDescriptor->fFovY = (float)fov;

		return true;
	}

	bool VXEImageplaneSetOrthoStageFitting(int iObjectID, vmdouble3 d3PosOrthoBoxMinWS, vmdouble3 d3PosOrthoBoxMaxWS, EvmStageViewType eStageViewFlag)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetOutBufferInfo - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		AaBbMinMax svxStage;
		svxStage.pos_min.x = d3PosOrthoBoxMinWS.x;
		svxStage.pos_min.y = d3PosOrthoBoxMinWS.y;
		svxStage.pos_min.z = d3PosOrthoBoxMinWS.z;
		svxStage.pos_max.x = d3PosOrthoBoxMaxWS.x;
		svxStage.pos_max.y = d3PosOrthoBoxMaxWS.y;
		svxStage.pos_max.z = d3PosOrthoBoxMaxWS.z;

		VmCObject* pCCObject = pCIObject->GetCameraObject();
		if (pCCObject == NULL)
			pCIObject->AttachCamera(svxStage, eStageViewFlag);
		else
			pCCObject->SetViewStage(&svxStage, eStageViewFlag);

		return true;
	}

	bool VXEImageplaneGetMatrixBetweenWSandSS(int iObjectID, vmmat44* pstMatrixWS2SS, vmmat44* pstMatrixSS2WS)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetMatrixBetweenWSandSS - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;
		VmCObject* pCCObject = pCIObject->GetCameraObject();
		if (pCCObject == NULL)
			return false;

		if (pstMatrixWS2SS)
		{
			vmmat44 svxMatrixWS2CS, svxMatrixCS2PS, svxMatrixPS2SS;
			pCCObject->GetMatrixWStoSS(&svxMatrixWS2CS, &svxMatrixCS2PS, &svxMatrixPS2SS);
			vmmat44 svxMatrixWS2PS;//, svxMatrixWS2SS;
			MatrixMultiply(&svxMatrixWS2PS, &svxMatrixWS2CS, &svxMatrixCS2PS);
			MatrixMultiply(pstMatrixWS2SS, &svxMatrixWS2PS, &svxMatrixPS2SS);
		}
		if (pstMatrixSS2WS)
		{
			vmmat44 svxMatrixSS2PS, svxMatrixPS2CS, svxMatrixCS2WS;
			pCCObject->GetMatrixSStoWS(&svxMatrixSS2PS, &svxMatrixPS2CS, &svxMatrixCS2WS);
			vmmat44 svxMatrixPS2WS;//, svxMatrixSS2WS;
			MatrixMultiply(&svxMatrixPS2WS, &svxMatrixPS2CS, &svxMatrixCS2WS);
			MatrixMultiply(pstMatrixSS2WS, &svxMatrixSS2PS, &svxMatrixPS2WS);
		}

		return true;
	}

	bool VXEImageplaneGetMatrixThroughWSandSS(int iObjectID, vmmat44* pstMatrixWS2CS, vmmat44* pstMatrixCS2PS, vmmat44* pstMatrixPS2SS)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetMatrixThroughWSandSS - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;
		VmCObject* pCCObject = pCIObject->GetCameraObject();
		if (pCCObject == NULL)
			return false;

		pCCObject->GetMatrixWStoSS(pstMatrixWS2CS, pstMatrixCS2PS, pstMatrixPS2SS);

		return true;
	}

	bool BilinearSamples(const FrameBuffer* pstFrameBuffer, int iNumSamples, vmfloat2 f2PosStartSS, vmfloat2 f2PosEndSS, vector<vmbyte4>* pvtrRGBAValues/*out*/)
	{
		if (pstFrameBuffer->dtype.type_name != dtype_byte4_name)
			return false;

		vmbyte4* py4FBRGBA = (vmbyte4*)pstFrameBuffer->fbuffer;
		vmbyte4 y4RGBAZero = vmbyte4();

		for (int i = 0; i < iNumSamples; i++)
		{
			float fRatio = 1.f;
			if (iNumSamples > 1)
				fRatio = (float)i / (float)(iNumSamples - 1);

			vmfloat2 f2PosSampleSS;
			f2PosSampleSS.x = (1.f - fRatio)*f2PosStartSS.x + fRatio * f2PosEndSS.x;
			f2PosSampleSS.y = (1.f - fRatio)*f2PosStartSS.y + fRatio * f2PosEndSS.y;
			if (f2PosSampleSS.x >= 0 && f2PosSampleSS.x < pstFrameBuffer->w
				&& f2PosSampleSS.y >= 0 && f2PosSampleSS.y < pstFrameBuffer->h)
			{
#pragma region // Sampling
				vmint2 i2PosSample;
				vmfloat2 f2InterpolateRatio;
				i2PosSample.x = (int)f2PosSampleSS.x;
				i2PosSample.y = (int)f2PosSampleSS.y;
				f2InterpolateRatio.x = f2PosSampleSS.x - i2PosSample.x;
				f2InterpolateRatio.y = f2PosSampleSS.y - i2PosSample.y;

				float fInterpolateWeights[4];
				fInterpolateWeights[0] = (1.f - f2InterpolateRatio.y)*(1.f - f2InterpolateRatio.x);
				fInterpolateWeights[1] = (1.f - f2InterpolateRatio.y)*f2InterpolateRatio.x;
				fInterpolateWeights[2] = f2InterpolateRatio.y*(1.f - f2InterpolateRatio.x);
				fInterpolateWeights[3] = f2InterpolateRatio.y*f2InterpolateRatio.x;

				int iSampleAddrXY = i2PosSample.x + i2PosSample.y*pstFrameBuffer->w;

				int iSampleAddr0 = iSampleAddrXY;
				int iSampleAddr1 = iSampleAddrXY + 1;
				int iSampleAddr2 = iSampleAddrXY + pstFrameBuffer->w;
				int iSampleAddr3 = iSampleAddrXY + 1 + pstFrameBuffer->w;

				vmbyte4 y4RGBAValues[4];

				y4RGBAValues[0] = py4FBRGBA[iSampleAddr0];

				if (i2PosSample.x == pstFrameBuffer->w - 1 && i2PosSample.y == pstFrameBuffer->h - 1)
				{
					y4RGBAValues[1] = y4RGBAZero;
					y4RGBAValues[2] = y4RGBAZero;
					y4RGBAValues[3] = y4RGBAZero;
				}
				else if (i2PosSample.x == pstFrameBuffer->w - 1)
				{
					y4RGBAValues[1] = y4RGBAZero;
					y4RGBAValues[2] = py4FBRGBA[iSampleAddr2];
					y4RGBAValues[3] = y4RGBAZero;
				}
				else if (i2PosSample.y == pstFrameBuffer->h - 1)
				{
					y4RGBAValues[1] = py4FBRGBA[iSampleAddr1];
					y4RGBAValues[2] = y4RGBAZero;
					y4RGBAValues[3] = y4RGBAZero;
				}
				else
				{
					y4RGBAValues[1] = py4FBRGBA[iSampleAddr1];
					y4RGBAValues[2] = py4FBRGBA[iSampleAddr2];
					y4RGBAValues[3] = py4FBRGBA[iSampleAddr3];
				}

				vmdouble4 d4SampleBilinear;
				memset(&d4SampleBilinear, 0, sizeof(vmdouble4));
				for (int k = 0; k < 4; k++)
				{
					d4SampleBilinear.x += (double)y4RGBAValues[k].r*fInterpolateWeights[k];
					d4SampleBilinear.y += (double)y4RGBAValues[k].g*fInterpolateWeights[k];
					d4SampleBilinear.z += (double)y4RGBAValues[k].b*fInterpolateWeights[k];
					d4SampleBilinear.w += (double)y4RGBAValues[k].a*fInterpolateWeights[k];
				}

				vmbyte4 y4BiLinearRGBA;
				y4BiLinearRGBA.r = (byte)min(d4SampleBilinear.x + 0.5, 255.);
				y4BiLinearRGBA.g = (byte)min(d4SampleBilinear.y + 0.5, 255.);
				y4BiLinearRGBA.b = (byte)min(d4SampleBilinear.z + 0.5, 255.);
				y4BiLinearRGBA.a = (byte)min(d4SampleBilinear.w + 0.5, 255.);
				y4BiLinearRGBA.r = max(y4BiLinearRGBA.r, (byte)0);
				y4BiLinearRGBA.g = max(y4BiLinearRGBA.g, (byte)0);
				y4BiLinearRGBA.b = max(y4BiLinearRGBA.b, (byte)0);
				y4BiLinearRGBA.a = max(y4BiLinearRGBA.a, (byte)0);
				pvtrRGBAValues->push_back(y4BiLinearRGBA);
#pragma endregion
			}
			else
			{
				pvtrRGBAValues->push_back(y4RGBAZero);
			}
		}
		return true;
	}

	bool VXEImageplaneGetOutRgbaAlongLine(int iObjectID, int iNumSamples, vmint2 i2PosStartSS, vmint2 i2PosEndSS, vector<vmbyte4>* pvtrRGBAValues/*out*/)
	{
		if (VmObject::GetObjectTypeFromID(iObjectID) != ObjectTypeIMAGEPLANE)
		{
			VMERRORMESSAGE("VXEImageplaneGetMatrixBetweenWSandSS - UNAVAILABLE TYPE");
			return false;
		}

		VmIObject* pCIObject = res_manager->GetIObject(iObjectID);
		if (pCIObject == NULL)
			return false;

		const vector<FrameBuffer>* pvtrFrameBuffers = pCIObject->GetBufferPointerList(FrameBufferUsageRENDEROUT);
		if (pvtrFrameBuffers->size() == 0)
			return false;

		vmfloat2 f2PosStartSS, f2PosEndSS;
		f2PosStartSS.x = (float)i2PosStartSS.x;
		f2PosStartSS.y = (float)i2PosStartSS.y;
		f2PosEndSS.x = (float)i2PosEndSS.x;
		f2PosEndSS.y = (float)i2PosEndSS.y;
		if (!BilinearSamples(&((*pvtrFrameBuffers)[0]), iNumSamples, f2PosStartSS, f2PosEndSS, pvtrRGBAValues))
			return false;

		return true;
	}

	/////////////
	// Modules //
	using namespace fncontainer;

	EnumVXMModuleType VXEModuleTypeGet(int iModuleID)
	{
		return (EnumVXMModuleType)VmModuleArbiter::GetModuleType(iModuleID);
	}

	bool VXEModuleRegister(string strModuleName, EnumVXMModuleType eModuleType, string strModuleSpecifier, int* piModuleID/*out*/)
	{
		int iModuleID = 0;

		iModuleID = module_arbitor->RegisterModule(strModuleName, (EvmModuleType)eModuleType, strModuleSpecifier);
		if (piModuleID)
			*piModuleID = iModuleID;

		if (iModuleID == 0)
			return false;

		vector<string> vtrModuleSpec;
		if (module_arbitor->GetModuleRequirements(vtrModuleSpec, iModuleID))
		{
			for (int i = 0; i < (int)vtrModuleSpec.size(); i++)
			{
				if (vtrModuleSpec.at(i).compare("GPUMANAGER") == 0)
				{
					VmFnContainer svxModuleParam;
					VmGpuManager* pCGM = NULL;
					svxModuleParam.rmw_buffers["_outptr_class_GpuManager"] = (void*)&pCGM;
					if (module_arbitor->InteropModuleCustomWork(iModuleID, svxModuleParam))
					{
						if (pCGM != NULL)
							gpu_manager[iModuleID] = pCGM;
					}
					break;
				}
			}
		}

		return true;
	}

	int __StringSplit(vector<string>* pvtrTokens, const string* pstrSrc, const string* pstrDelimiter)
	{
		string::size_type lastPos = pstrSrc->find_first_not_of(*pstrDelimiter, 0);
		string::size_type pos = pstrSrc->find_first_of(*pstrDelimiter, lastPos);

		while (string::npos != pos || string::npos != lastPos)
		{
			pvtrTokens->push_back(pstrSrc->substr(lastPos, pos - lastPos));
			lastPos = pstrSrc->find_first_not_of(*pstrDelimiter, pos);
			pos = pstrSrc->find_first_of(*pstrDelimiter, lastPos);
		}

		return (int)pvtrTokens->size();
	}

	bool _GenerateModuleParameters(VmFnContainer* pstModuleParameters/*out*/, const map<string, int>* pmapObjIDs, const map<string, void*>* pmapCustomParameters, const map<string, void*>* pmapCustomObjects)
	{
		if (pstModuleParameters == NULL)
			return false;
#pragma region // Global Object Setting
		map<int, VmObject*> mapVolumeObjectsIn;
		map<int, VmObject*> mapPrimitiveObjectsIn;
		map<int, VmObject*> mapCListObjectsIn;
		map<int, VmObject*> mapTObjectsIn;
		map<int, VmObject*> mapIObjectsIn;
		map<int, VmObject*> mapVolumeObjectsOut;
		map<int, VmObject*> mapPrimitiveObjectsOut;
		map<int, VmObject*> mapCListObjectsOut;
		map<int, VmObject*> mapTObjectsOut;
		map<int, VmObject*> mapIObjectsOut;

		//auto itrObjects = pmapObjIDs->begin();
		map<string, int>::iterator itrObjects = ((map<string, int>*)pmapObjIDs)->begin();
		string strDelimeter = "_";
		for (; itrObjects != pmapObjIDs->end(); itrObjects++)
		{
			vector<string> vtrTokens;
			__StringSplit(&vtrTokens, &itrObjects->first, &strDelimeter);
			if (vtrTokens[0].compare("in") == 0)
			{
				if (vtrTokens[1].compare(("VOLUME")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeVOLUME)
					{
						printf("Invalid [in] ID : %d\n", itrObjects->second);
						return false;
					}
					mapVolumeObjectsIn.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("PRIMITIVE")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypePRIMITIVE)
					{
						printf("Invalid [in] ID : %d\n", itrObjects->second);
						return false;
					}
					mapPrimitiveObjectsIn.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("CUSTOMLIST")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeBUFFER)
					{
						printf("Invalid [in] ID : %d\n", itrObjects->second);
						return false;
					}
					mapCListObjectsIn.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("TRANSFERFUNCTION")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeTMAP)
					{
						printf("Invalid [in] ID : %d\n", itrObjects->second);
						return false;
					}
					mapTObjectsIn.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("IMAGEPLANE")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeIMAGEPLANE)
					{
						printf("Invalid [in] ID : %d\n", itrObjects->second);
						return false;
					}
					mapIObjectsIn.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else
				{
					VMERRORMESSAGE("VXEExecuteModule - NOT YET supported object type");
				}
			}
			if (vtrTokens[0].compare(("out")) == 0)
			{
				if (vtrTokens[1].compare(("VOLUME")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeVOLUME)
					{
						printf("Invalid [out] ID : %d\n", itrObjects->second);
						return false;
					}
					mapVolumeObjectsOut.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("PRIMITIVE")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypePRIMITIVE)
					{
						printf("Invalid [out] ID : %d\n", itrObjects->second);
						return false;
					}
					mapPrimitiveObjectsOut.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("CUSTOMLIST")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeBUFFER)
					{
						printf("Invalid [out] ID : %d\n", itrObjects->second);
						return false;
					}
					mapCListObjectsOut.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("TRANSFERFUNCTION")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeTMAP)
					{
						printf("Invalid [out] ID : %d\n", itrObjects->second);
						return false;
					}
					mapTObjectsOut.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else if (vtrTokens[1].compare(("IMAGEPLANE")) == 0)
				{
					VmObject* pCObject = res_manager->GetObject(itrObjects->second);
					if (pCObject == NULL || VmObject::GetObjectTypeFromID(itrObjects->second) != ObjectTypeIMAGEPLANE)
					{
						printf("Invalid [out] ID : %d\n", itrObjects->second);
						return false;
					}
					mapIObjectsOut.insert(pair<int, VmObject*>(std::stoi(vtrTokens[2]), pCObject));
				}
				else
				{
					VMERRORMESSAGE("VXEExecuteModule - NOT YET supported object type");
				}
			}
		}

#define __MAP_TO_VEC(A, B){\
	for (auto itr = A.begin(); itr != A.end(); itr++)\
	{\
		B.push_back(itr->second);\
	}\
	}\

		vector<VmObject*> vtrVolumeObjectsIn; __MAP_TO_VEC(mapVolumeObjectsIn, vtrVolumeObjectsIn);
		vector<VmObject*> vtrPrimitiveObjectsIn; __MAP_TO_VEC(mapPrimitiveObjectsIn, vtrPrimitiveObjectsIn);
		vector<VmObject*> vtrCListObjectsIn; __MAP_TO_VEC(mapCListObjectsIn, vtrCListObjectsIn);
		vector<VmObject*> vtrTObjectsIn; __MAP_TO_VEC(mapTObjectsIn, vtrTObjectsIn);
		vector<VmObject*> vtrIObjectsIn; __MAP_TO_VEC(mapIObjectsIn, vtrIObjectsIn);
		vector<VmObject*> vtrVolumeObjectsOut; __MAP_TO_VEC(mapVolumeObjectsOut, vtrVolumeObjectsOut);
		vector<VmObject*> vtrPrimitiveObjectsOut; __MAP_TO_VEC(mapPrimitiveObjectsOut, vtrPrimitiveObjectsOut);
		vector<VmObject*> vtrCListObjectsOut; __MAP_TO_VEC(mapCListObjectsOut, vtrCListObjectsOut);
		vector<VmObject*> vtrTObjectsOut; __MAP_TO_VEC(mapTObjectsOut, vtrTObjectsOut);
		vector<VmObject*> vtrIObjectsOut; __MAP_TO_VEC(mapIObjectsOut, vtrIObjectsOut);

		if (vtrVolumeObjectsIn.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeVOLUME, true), vtrVolumeObjectsIn));
		if (vtrTObjectsIn.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeTMAP, true), vtrTObjectsIn));
		if (vtrPrimitiveObjectsIn.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypePRIMITIVE, true), vtrPrimitiveObjectsIn));
		if (vtrCListObjectsIn.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeBUFFER, true), vtrCListObjectsIn));
		if (vtrIObjectsIn.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeIMAGEPLANE, true), vtrIObjectsIn));

		if (vtrVolumeObjectsOut.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeVOLUME, false), vtrVolumeObjectsOut));
		if (vtrTObjectsOut.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeTMAP, false), vtrTObjectsOut));
		if (vtrPrimitiveObjectsOut.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypePRIMITIVE, false), vtrPrimitiveObjectsOut));
		if (vtrCListObjectsOut.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeBUFFER, false), vtrCListObjectsOut));
		if (vtrIObjectsOut.size() > 0)
			pstModuleParameters->vmobjs.insert(pair< VmObjKey, vector<VmObject*> >(VmObjKey(vmenums::ObjectTypeIMAGEPLANE, false), vtrIObjectsOut));
#pragma endregion

#pragma region // Custom Parameter Setting
		pstModuleParameters->vmparams = *pmapCustomParameters;
#pragma endregion

#pragma region // Custom Object Setting
		pstModuleParameters->rmw_buffers = *pmapCustomObjects;
#pragma endregion
		return true;
	}

	bool VXEModuleExecute(int iModuleID, const map<string, int>* pmapObjIDs, const map<string, void*>* pmapCustomParameters, const map<string, void*>* pmapCustomObjects, bool bIsOnlyParameterCheck)
	{
		if (VmModuleArbiter::GetModuleType(iModuleID) == ModuleTypeNONE)
			return false;

		map<string, int> mapObjIDs;
		if (pmapObjIDs)
			mapObjIDs = *pmapObjIDs;
		map<string, void*> mapCustomParameters;
		if (pmapCustomParameters)
			mapCustomParameters = *pmapCustomParameters;
		map<string, void*> mapCustomObjects;
		if (pmapCustomObjects)
			mapCustomObjects = *pmapCustomObjects;

		VmFnContainer svxModuleParameters;
		if (!_GenerateModuleParameters(&svxModuleParameters, &mapObjIDs, &mapCustomParameters, &mapCustomObjects))
		{
			VMERRORMESSAGE("Invalid Module Parameters!");
			return false;
		}

		bool bRet = false;

		if (bIsOnlyParameterCheck)
		{
			bRet = module_arbitor->CheckModuleParameters(iModuleID, svxModuleParameters);
		}
		else
		{
			bool bCalledInitializer = false;
			bRet = module_arbitor->ExecuteModule(iModuleID, svxModuleParameters, &bCalledInitializer);
			if (bCalledInitializer)
			{
				vector<string> vtrModuleSpec;
				if (module_arbitor->GetModuleRequirements(vtrModuleSpec, iModuleID))
				{
					for (int i = 0; i < (int)vtrModuleSpec.size(); i++)
					{
						if (vtrModuleSpec.at(i).compare("GPUMANAGER") == 0)
						{
							VmFnContainer svxModuleParam;
							VmGpuManager* pCGM = NULL;
							svxModuleParam.rmw_buffers["_outptr_class_GpuManager"] = (void*)&pCGM;
							if (module_arbitor->InteropModuleCustomWork(iModuleID, svxModuleParam))
							{
								if (pCGM != NULL)
									gpu_manager[iModuleID] = pCGM;
							}
							break;
						}
					}
				}
			}
		}

		return bRet;
	}

	bool VXEModuleClear(int iModuleID, map<string, void*>* pmapCustomParamters)
	{
		VmFnContainer fncon;
		if (pmapCustomParamters)
			fncon.vmparams = *pmapCustomParamters;
		return module_arbitor->ClearModule(iModuleID, fncon);
	}

	double VXEModuleGetProgress(int iModuleID)
	{
		return module_arbitor->GetProgress(iModuleID);
	}

	bool VXEModuleInteropCustomWork(int iModuleID, const map<string, int>* pmapObjIDs,
		const map<string /*_[DataType]_[CustomName]*/, void*>* pmapCustomParameters,
		const map<string /*_[in/out]_[double/bool/int]_[CustomName]*/, void*>* pmapCustomInformation)
	{
		map<string, int> mapObjIDs;
		if (pmapObjIDs)
			mapObjIDs = *pmapObjIDs;
		map<string, void*> mapCustomParameters;
		if (pmapCustomParameters)
			mapCustomParameters = *pmapCustomParameters;
		map<string, void*> mapCustomInformation;
		if (pmapCustomInformation)
			mapCustomInformation = *pmapCustomInformation;

		VmFnContainer svxModuleParameters;
		if (!_GenerateModuleParameters(&svxModuleParameters, &mapObjIDs, &mapCustomParameters, &mapCustomInformation))
			return false;

		return module_arbitor->InteropModuleCustomWork(iModuleID, svxModuleParameters);
	}
}