#include "GpuManager.h"

/////////////////////////// new ver.
using namespace vmgpuinterface;
struct __GpuInterfaces
{
	typedef bool(*LPDLLInitializeDevice)(void);
	typedef bool(*LPDLLDeinitializeDevice)(void);
	typedef bool(*LPDLLGetGpuCurrentMemoryBytes)(ullong* dedicated_gpu_mem_bytes/*out*/, ullong* free_gpu_mem/*out*/);
	typedef bool(*LPDLLGetDeviceInformation)(void* dev_info_ptr, const string& dev_specifier);
	typedef ullong(*LPDLLGetUsedGpuMemorySizeBytes)(void);
	typedef bool(*LPDLLUpdateGpuResource)(GpuRes& gres);
	typedef int(*LPDLLUpdateGpuResourcesBySrcID)(const int src_id, vector<GpuRes>& gres_list);

	typedef bool(*LPDLLGenerateGpuResource)(GpuRes& gres, LocalProgress* progress);

	typedef bool(*LPDLLReleaseGpuResource)(GpuRes& gres, const bool call_clearstate);
	typedef bool(*LPDLLReleaseGpuResourcesBySrcID)(const int src_id, const bool call_clearstate);
	typedef bool(*LPDLLReleaseAllGpuResources)();
	typedef bool(*LPDLLSetGpuManagerCustomParameters)(const string& param_name, const data_type& dtype, const void* v_ptr, const int num_elements);
	typedef bool(*LPDLLGetGpuManagerCustomParameters)(const string& param_name, const data_type& dtype, void* v_ptr, int* num_elements);

	LPDLLInitializeDevice lpdllInitializeDevice;
	LPDLLDeinitializeDevice lpdllDenitializeDevice;
	LPDLLGetGpuCurrentMemoryBytes lpdllGetGpuCurrentMemoryBytes;
	LPDLLGetDeviceInformation lpdllGetDeviceInformation;
	LPDLLGetUsedGpuMemorySizeBytes lpdllGetUsedGpuMemorySizeBytes;
	LPDLLUpdateGpuResource lpdllUpdateGpuResource;
	LPDLLUpdateGpuResourcesBySrcID lpdllUpdateGpuResourcesBySrcID;
	LPDLLGenerateGpuResource lpdllGenerateGpuResource;
	LPDLLReleaseGpuResource lpdllReleaseGpuResource;
	LPDLLReleaseGpuResourcesBySrcID lpdllReleaseGpuResourcesBySrcID;
	LPDLLReleaseAllGpuResources lpdllReleaseAllGpuResources;
	LPDLLSetGpuManagerCustomParameters lpdllSetGpuManagerCustomParameters;
	LPDLLGetGpuManagerCustomParameters lpdllGetGpuManagerCustomParameters;

	VmHMODULE hMouleLib;
	EvmGpuSdkType sdk_type;

	__GpuInterfaces()
	{
		sdk_type = GpuSdkTypeUNDEFINED;

		// Module
		hMouleLib = NULL;

		lpdllInitializeDevice = NULL;
		lpdllDenitializeDevice = NULL;
		lpdllGetDeviceInformation = NULL;
		lpdllGetUsedGpuMemorySizeBytes = NULL;
		lpdllUpdateGpuResource = NULL;
		lpdllUpdateGpuResourcesBySrcID = NULL;
		lpdllGenerateGpuResource = NULL;
		lpdllReleaseGpuResource = NULL;
		lpdllReleaseGpuResourcesBySrcID = NULL;
		lpdllReleaseAllGpuResources = NULL;
		lpdllSetGpuManagerCustomParameters = NULL;
		lpdllGetGpuManagerCustomParameters = NULL;
	}
	~__GpuInterfaces()
	{
		if (hMouleLib)
		{
			if (lpdllDenitializeDevice)
				lpdllDenitializeDevice();
			VMFREELIBRARY(hMouleLib);
			hMouleLib = NULL;
		}
	}

	void _DLLProcLink()
	{
		if (hMouleLib)
		{
			lpdllInitializeDevice = (LPDLLInitializeDevice)VMGETPROCADDRESS(hMouleLib, "__InitializeDevice");
			lpdllDenitializeDevice = (LPDLLDeinitializeDevice)VMGETPROCADDRESS(hMouleLib, "__DeinitializeDevice");
			lpdllGetGpuCurrentMemoryBytes = (LPDLLGetGpuCurrentMemoryBytes)VMGETPROCADDRESS(hMouleLib, "__GetGpuMemoryBytes");
			lpdllGetDeviceInformation = (LPDLLGetDeviceInformation)VMGETPROCADDRESS(hMouleLib, "__GetDeviceInformation");
			lpdllGetUsedGpuMemorySizeBytes = (LPDLLGetUsedGpuMemorySizeBytes)VMGETPROCADDRESS(hMouleLib, "__GetUsedGpuMemorySizeBytes");
			lpdllUpdateGpuResource = (LPDLLUpdateGpuResource)VMGETPROCADDRESS(hMouleLib, "__UpdateGpuResource");
			lpdllUpdateGpuResourcesBySrcID = (LPDLLUpdateGpuResourcesBySrcID)VMGETPROCADDRESS(hMouleLib, "__UpdateGpuResourcesBySrcID");
			lpdllGenerateGpuResource = (LPDLLGenerateGpuResource)VMGETPROCADDRESS(hMouleLib, "__GenerateGpuResource");
			lpdllReleaseGpuResource = (LPDLLReleaseGpuResource)VMGETPROCADDRESS(hMouleLib, "__ReleaseGpuResource");
			lpdllReleaseGpuResourcesBySrcID = (LPDLLReleaseGpuResourcesBySrcID)VMGETPROCADDRESS(hMouleLib, "__ReleaseGpuResourcesBySrcID");
			lpdllReleaseAllGpuResources = (LPDLLReleaseAllGpuResources)VMGETPROCADDRESS(hMouleLib, "__ReleaseAllGpuResources");
			lpdllSetGpuManagerCustomParameters = (LPDLLSetGpuManagerCustomParameters)VMGETPROCADDRESS(hMouleLib, "__SetGpuManagerParameters");
			lpdllGetGpuManagerCustomParameters = (LPDLLGetGpuManagerCustomParameters)VMGETPROCADDRESS(hMouleLib, "__GetGpuManagerParameters");

			if ((lpdllInitializeDevice && lpdllDenitializeDevice && lpdllGetGpuCurrentMemoryBytes && lpdllGetDeviceInformation
				&& lpdllGetUsedGpuMemorySizeBytes && lpdllUpdateGpuResource && lpdllGenerateGpuResource
				&& lpdllUpdateGpuResourcesBySrcID && lpdllReleaseGpuResource
				&& lpdllReleaseGpuResourcesBySrcID && lpdllReleaseAllGpuResources
				&& lpdllSetGpuManagerCustomParameters
				&& lpdllGetGpuManagerCustomParameters) == NULL)
				printf("_DLLProcLink - There is unlinked procedure! ");
		}
	}
};

__GpuInterfaces* __gpu_interface = NULL;
VmGpuManager::VmGpuManager(const EvmGpuSdkType sdk_type, const string& module_file)
{
	__gpu_interface = new __GpuInterfaces();
	VMLOADLIBRARY(__gpu_interface->hMouleLib, module_file.c_str());
	if (__gpu_interface->hMouleLib)
	{
		__gpu_interface->_DLLProcLink();
		__gpu_interface->sdk_type = sdk_type;
		if (__gpu_interface->lpdllInitializeDevice)
		{
			__gpu_interface->lpdllInitializeDevice();
		}
	}
}

VmGpuManager::~VmGpuManager()
{
	VMSAFE_DELETE(__gpu_interface);
}

EvmGpuSdkType VmGpuManager::GetGpuManagerSDK()
{
	return __gpu_interface->sdk_type;
}

bool VmGpuManager::GetDeviceInformation(void* dev_info_ptr, const string& dev_specifier)
{
	if (__gpu_interface->lpdllGetDeviceInformation == NULL)
		return false;
	return __gpu_interface->lpdllGetDeviceInformation(dev_info_ptr, dev_specifier);
}

bool VmGpuManager::GetGpuCurrentMemoryBytes(ullong* dedicated_gpu_mem_bytes/*out*/, ullong* free_gpu_mem/*out*/)
{
	ullong _dedicatedGpuMemory = 0, freeMemory = 0;
	bool bRet = false;

	if (__gpu_interface->lpdllGetGpuCurrentMemoryBytes == NULL)
		return false;
	bRet = __gpu_interface->lpdllGetGpuCurrentMemoryBytes(&_dedicatedGpuMemory, &freeMemory);

	if (dedicated_gpu_mem_bytes) *dedicated_gpu_mem_bytes = _dedicatedGpuMemory;
	if (free_gpu_mem) *free_gpu_mem = freeMemory;
	return bRet;
}

ullong VmGpuManager::GetUsedGpuMemorySizeBytes()
{
	ullong uiSizeKiloBytes = 0;
	if (__gpu_interface->lpdllGetUsedGpuMemorySizeBytes)
	{
		uiSizeKiloBytes += __gpu_interface->lpdllGetUsedGpuMemorySizeBytes();
	}
	return uiSizeKiloBytes;
}

bool VmGpuManager::UpdateGpuResource(GpuRes& gres/*in-out*/)
{
	if (__gpu_interface->lpdllUpdateGpuResource == NULL)
		return false;
	return __gpu_interface->lpdllUpdateGpuResource(gres);
}

int VmGpuManager::UpdateGpuResourcesBySrcID(const int src_id, vector<GpuRes>& gres_list/*out*/)
{
	if (__gpu_interface->lpdllUpdateGpuResourcesBySrcID == NULL)
		return 0;
	return __gpu_interface->lpdllUpdateGpuResourcesBySrcID(src_id, gres_list);
}

bool VmGpuManager::GenerateGpuResource(GpuRes& gres/*in-out*/, LocalProgress* progress)
{
	if (__gpu_interface->lpdllGenerateGpuResource == NULL || 
		__gpu_interface->lpdllUpdateGpuResource == NULL)
		return false;
	return __gpu_interface->lpdllGenerateGpuResource(gres, progress);
}

bool VmGpuManager::ReleaseGpuResource(GpuRes& gres, const bool call_clearstate)
{
	if (__gpu_interface->lpdllReleaseGpuResource == NULL)
		return false;
	return __gpu_interface->lpdllReleaseGpuResource(gres, call_clearstate);
}

bool VmGpuManager::ReleaseGpuResourcesBySrcID(const int src_id, const bool call_clearstate)
{
	if (__gpu_interface->lpdllReleaseGpuResourcesBySrcID)
	{
		__gpu_interface->lpdllReleaseGpuResourcesBySrcID(src_id, call_clearstate);
	}
	return true;
}

bool VmGpuManager::ReleaseAllGpuResources()
{
	if (__gpu_interface->lpdllReleaseAllGpuResources)
	{
		__gpu_interface->lpdllReleaseAllGpuResources();
	}
	return true;
}

bool VmGpuManager::SetGpuManagerParameters(const string& param_name, const data_type& dtype, const void* v_ptr, const int num_elements)
{
	if (__gpu_interface->lpdllSetGpuManagerCustomParameters)
	{
		return __gpu_interface->lpdllSetGpuManagerCustomParameters(param_name, dtype, v_ptr, num_elements);
	}
	return false;
}

bool VmGpuManager::GetGpuManagerParameters(const string& param_name, const data_type& dtype, void* v_ptr, int* num_elements)
{
	if (__gpu_interface->lpdllGetGpuManagerCustomParameters)
	{
		return __gpu_interface->lpdllGetGpuManagerCustomParameters(param_name, dtype, v_ptr, num_elements);
	}
	return false;
}