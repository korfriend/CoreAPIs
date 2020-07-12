#include "ModuleArbiter.h"
//#include <iostream>

VmModuleArbiter::VmModuleArbiter(string _version)
{
	version = _version;
	render_module_count_id = 0;
	tmap_module_count_id = 0;
	vgen_module_count_id = 0;
	helper_module_count_id = 0;
}

VmModuleArbiter::~VmModuleArbiter()
{
	map<int, VmModule*>::iterator itrModule;
	for(itrModule = map_modules.begin(); itrModule != map_modules.end(); itrModule++)
	{
		VMSAFE_DELETE(itrModule->second);	// Including "call lpdllDeInit(NULL)"
	}
	map_modules.clear();
	map_module_names.clear();
}

int VmModuleArbiter::RegisterModule(const string& module_name, const EvmModuleType mtype, const string& descriptor)
{
	map<string, int>::iterator itrModuleName = map_module_names.find(module_name);
	if(itrModuleName != map_module_names.end())
	{
		printf("VmModuleArbiter::RegisterModule - Already Registered Module!");
		return 0;
	}
	
	VmHMODULE hMouleLib;
	LPDLLfnCheckModuleParameters lpdllCheckModuleParameters;
	LPDLLfnInit lpdllInit;
	LPDLLfnDoModule lpdllDoModule;
	LPDLLfnDeInit lpdllDeInit;
	LPDLLfnGetProgress lpdllGetProgress;
	LPDLLfnGetModuleRequirements lpdllGetModuleSpecification;
	LPDLLfnInteropCustomWork lpdllInteropCustomWork;

	//cout << module_name << endl;
	VMLOADLIBRARY(hMouleLib, module_name.c_str());
	if(hMouleLib == NULL)
	{
		if(hMouleLib)
			VMFREELIBRARY(hMouleLib);
		printf("VmModuleArbiter::RegisterModule - Import Error! -- DLL\n");// , module_name);
		return 0;
	}

	lpdllCheckModuleParameters = (LPDLLfnCheckModuleParameters)VMGETPROCADDRESS(hMouleLib, "CheckModuleParameters");
	lpdllInit = (LPDLLfnInit)VMGETPROCADDRESS(hMouleLib, "InitModule");
	lpdllDoModule = (LPDLLfnDoModule)VMGETPROCADDRESS(hMouleLib, "DoModule");
	lpdllDeInit = (LPDLLfnDeInit)VMGETPROCADDRESS(hMouleLib, "DeInitModule");
	lpdllGetProgress = (LPDLLfnGetProgress)VMGETPROCADDRESS(hMouleLib, "GetProgress");
	lpdllGetModuleSpecification = (LPDLLfnGetModuleRequirements)VMGETPROCADDRESS(hMouleLib, "GetModuleSpecification");
	lpdllInteropCustomWork = (LPDLLfnInteropCustomWork)VMGETPROCADDRESS(hMouleLib, "InteropCustomWork");

	if(lpdllCheckModuleParameters == NULL || lpdllInit == NULL || lpdllDoModule == NULL || lpdllDeInit == NULL)
	{
		if (lpdllCheckModuleParameters == NULL) printf("------------> A\n");
		if (lpdllInit == NULL) printf("------------> B\n");
		if (lpdllDoModule == NULL) printf("------------> C\n");
		if (lpdllDeInit == NULL) printf("------------> D\n");
		if(hMouleLib)
			VMFREELIBRARY(hMouleLib);
		printf("VmModuleArbiter::RegisterModule - Import Error!");
		return 0;
	}

	int module_id = 0;
	switch(mtype)
	{
	case ModuleTypeRENDER:
		module_id = (((int)mtype)<<24) | render_module_count_id;
		if(render_module_count_id == 65535)
		{
			printf("VmModuleArbiter::RegisterModule - No More Count : 65535");
			if(hMouleLib)
				VMFREELIBRARY(hMouleLib);
			printf("VmModuleArbiter::RegisterModule - Import Error!");
			return 0;
		}
		render_module_count_id++;
		break;
	case ModuleTypeTMAP:
		module_id = (((int)mtype)<<24) | tmap_module_count_id;
		if(tmap_module_count_id == 65535)
		{
			printf("VmModuleArbiter::RegisterModule - No More Count : 65535");
			if(hMouleLib)
				VMFREELIBRARY(hMouleLib);
			printf("VmModuleArbiter::RegisterModule - Import Error!");
			return 0;
		}
		tmap_module_count_id++;
		break;
	case ModuleTypeVGENERATION:
		module_id = (((int)mtype)<<24) | vgen_module_count_id;
		if(vgen_module_count_id == 65535)
		{
			printf("VmModuleArbiter::RegisterModule - No More Count : 65535");
			if(hMouleLib)
				VMFREELIBRARY(hMouleLib);
			printf("VmModuleArbiter::RegisterModule - Import Error!");
			return 0;
		}
		vgen_module_count_id++;
		break;
	case ModuleTypeETC:
		module_id = (((int)mtype)<<24) | helper_module_count_id;
		if(helper_module_count_id == 65535)
		{
			printf("VmModuleArbiter::RegisterModule - No More Count : 65535");
			if(hMouleLib)
				VMFREELIBRARY(hMouleLib);
			printf("VmModuleArbiter::RegisterModule - Import Error!");
			return 0;
		}
		helper_module_count_id++;
		break;
	case ModuleTypeNONE:
	default:
		if(hMouleLib)
			VMFREELIBRARY(hMouleLib);
		printf("VmModuleArbiter::RegisterModule - Import Error!");
		return 0;
	}

	VmModule* module_ptr = new VmModule();
	module_ptr->hMouleLib = hMouleLib;
	module_ptr->lpdllCheckModuleParameters = lpdllCheckModuleParameters;
	module_ptr->lpdllInit = lpdllInit;
	module_ptr->lpdllDoModule = lpdllDoModule;
	module_ptr->lpdllDeInit = lpdllDeInit;
	module_ptr->lpdllGetProgress = lpdllGetProgress;
	module_ptr->lpdllGetModuleRequirements = lpdllGetModuleSpecification;
	module_ptr->lpdllInteropCustomWork = lpdllInteropCustomWork;
	module_ptr->mtype = mtype;
	module_ptr->descriptor = descriptor;

	map_modules[module_id] = module_ptr;
	map_module_names[module_name] = module_id;

	printf("Module Register Success : %s, %x\n", module_name.c_str(), module_id);

	return module_id;
}

bool VmModuleArbiter::CheckModuleParameters(const int module_id, VmFnContainer& vfn)
{
	map<int, VmModule*>::iterator itrModule = map_modules.find(module_id);
	if(itrModule == map_modules.end())
	{
		printf("VmModuleArbiter::ExecuteModule - No Module as iModuleID");
		return false;
	}
	if(itrModule->second->lpdllCheckModuleParameters(vfn) == false)
	{
		printf("VmModuleArbiter::ExecuteModule - CheckIsAvailableParameters Fail");
		printf("ExecuteModule : Parameter Checker returns Failure!\n");
		return false;
	}
	return true;
}

bool VmModuleArbiter::ExecuteModule(const int module_id, VmFnContainer& vfn, bool* is_initialized)
{
	map<int, VmModule*>::iterator itrModule = map_modules.find(module_id);

	if(is_initialized)
	{
		*is_initialized = true;
		if(itrModule->second->bInitialized)
			*is_initialized = false;
	}

	if(itrModule->second->bInitialized == false)
	{
		if(itrModule->second->lpdllInit(vfn) == false)
		{
			printf("VmModuleArbiter::ExecuteModule - Initialization Fail");
			printf("ExecuteModule : Initialization returns Failure!\n");
			return false;
		}
		itrModule->second->bInitialized = true;
	}
	bool bRet = itrModule->second->lpdllDoModule(vfn);
	if(!bRet)
	{
		printf("VmModuleArbiter::ExecuteModule - Do Module Fail");
		printf("ExecuteModule : Do Module returns Failure!\n");
	}
	return bRet;
}

bool VmModuleArbiter::ClearModule(const int module_id, VmFnContainer& vfn)
{
	map<int, VmModule*>::iterator itrModule = map_modules.find(module_id);
	if(itrModule == map_modules.end())
	{
		printf("VmModuleArbiter::ClearModule - No Module as iModuleID");
		return false;
	}
	if(itrModule->second->bInitialized == true)
	{
		itrModule->second->lpdllDeInit(vfn);
		itrModule->second->bInitialized = false;
		//printf("ClearModule : DeInitialization Success!\n");
	}

	return true;
}

double VmModuleArbiter::GetProgress(const int module_id)
{
	map<int, VmModule*>::iterator itrModule = map_modules.find(module_id);
	if(itrModule == map_modules.end())
	{
		printf("VmModuleArbiter::GetProgress - No Module as iModuleID");
		return 0;
	}
	return itrModule->second->lpdllGetProgress();
}

bool VmModuleArbiter::InteropModuleCustomWork(const int module_id, VmFnContainer& vfn)
{
	map<int, VmModule*>::iterator itrModule = map_modules.find(module_id);
	if(itrModule == map_modules.end())
	{
		printf("VmModuleArbiter::GetProgress - No Module as iModuleID");
		return false;
	}
	if(itrModule->second->lpdllInteropCustomWork == NULL)
	{
		printf("VmModuleArbiter::InteropCustomWork - Undefined");
		return false;
	}
	itrModule->second->lpdllInteropCustomWork(vfn);
	return true;
}

string VmModuleArbiter::GetModuleDescriptor(const int module_id)
{
	map<int, VmModule*>::iterator itrModule = map_modules.find(module_id);
	if(itrModule == map_modules.end())
	{
		printf("VmModuleArbiter::GetModuleSpecifier - No Module as iModuleID");
		return "N/A";
	}
	return itrModule->second->descriptor;
}

int VmModuleArbiter::GetModuleIDByModuleName(const string& module_name)
{
	map<string /*ModuleName*/, int /*ID*/>::iterator itrRegName = map_module_names.find(module_name);
	if(itrRegName == map_module_names.end())
	{
		printf("VmModuleArbiter::GetModuleIDByModuleName - No Module as strModuleName");
		return 0;
	}

	return itrRegName->second;
}

bool VmModuleArbiter::GetModuleRequirements(vector<string>& requirements, const int module_id)
{
	map<int, VmModule*>::iterator itrModule = map_modules.find(module_id);
	if(itrModule == map_modules.end())
	{
		printf("VmModuleArbiter::GetProgress - No Module as iModuleID");
		return false;
	}
	
	if(itrModule->second->lpdllGetModuleRequirements == NULL)
	{
		return false;
	}

	itrModule->second->lpdllGetModuleRequirements(requirements);

	return true;
}

EvmModuleType VmModuleArbiter::GetModuleType(int module_id)
{
	return (EvmModuleType)(module_id >>24);
}