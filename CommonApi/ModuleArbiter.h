#pragma once

#include "../CommonUnits/VimCommon.h"

using namespace std;
using namespace vmobjects;
using namespace fncontainer;

typedef bool (*LPDLLfnCheckModuleParameters)(VmFnContainer& vfn);
typedef bool (*LPDLLfnInit)(VmFnContainer& vfn);
typedef bool (*LPDLLfnDoModule)(VmFnContainer& vfn);
typedef void (*LPDLLfnDeInit)(VmFnContainer& vfn);
typedef double (*LPDLLfnGetProgress)();	// 0 to 100%
typedef void (*LPDLLfnGetModuleRequirements)(vector<string>& requirements); // Optional
typedef void (*LPDLLfnInteropCustomWork)(VmFnContainer& vfn); // Optional

enum EvmModuleType {
	ModuleTypeNONE = 0,/*!< Undefined, There is no such a module */
	ModuleTypeRENDER,/*!< Rendering module */
	ModuleTypeTMAP,/*!< OTF setting ���� module */
	ModuleTypeVGENERATION,/*!< VObject ����/ó��/�м� ���� module */
	ModuleTypeETC/*!< Customized-Define Module */
};
/**
 * @class VmModule
 * @brief Framework���� module�� �����ϴ� �ڷᱸ��
 */
struct VmModule{
	/**
	 * @brief module�� type
	 */
	EvmModuleType mtype;
	/**
	 * @brief module�� ���� mention
	 */
	string descriptor;
	/**
	 * @brief module�� ���� dll handle, win32 ��
	 */
	VmHMODULE hMouleLib;
	/**
	 * @brief module�� ���� function interface�� ���� pointer, CheckIsAvailableParameters
	 */
	LPDLLfnCheckModuleParameters lpdllCheckModuleParameters;
	/**
	 * @brief module�� ���� function interface�� ���� pointer, InitModule
	 */
	LPDLLfnInit lpdllInit;
	/**
	 * @brief module�� ���� function interface�� ���� pointer, DoModule
	 */
	LPDLLfnDoModule lpdllDoModule;
	/**
	 * @brief module�� ���� function interface�� ���� pointer, DeinitModule
	 */
	LPDLLfnDeInit lpdllDeInit;
	/**
	 * @brief module�� ���� function interface�� ���� pointer, GetProgress
	 */
	LPDLLfnGetProgress lpdllGetProgress;	 // Optional
	/**
	 * @brief module�� ���� function interface�� ���� pointer, GetModuleSpecification
	 */
	LPDLLfnGetModuleRequirements lpdllGetModuleRequirements; // Optional
	/**
	 * @brief module�� ���� function interface�� ���� pointer, InteropCustomWork
	 */
	LPDLLfnInteropCustomWork lpdllInteropCustomWork; // Optional
	/**
	 * @brief ���� ����� �ʱ�ȭ �Ǿ��°� ����, �� "lpdllInit" �� ȣ��Ǿ��°� ����
	 */
	bool bInitialized;

	/// constructor, NULL, zero, false 
	VmModule(){
		mtype = ModuleTypeNONE;
		hMouleLib = NULL;
		lpdllCheckModuleParameters = NULL;
		lpdllInit = NULL;
		lpdllDoModule = NULL;
		lpdllDeInit = NULL;
		lpdllGetProgress = NULL;
		lpdllGetModuleRequirements = NULL;
		lpdllInteropCustomWork = NULL;
		bInitialized = false;
	}
	~VmModule(){
		if (lpdllDeInit)
		{
			VmFnContainer vfn_end;
			vfn_end.descriptor = "~VmModule()";
			lpdllDeInit(vfn_end);
		}
		if(hMouleLib)
			VMFREELIBRARY(hMouleLib);
	}
};

/**
 * @class VmModuleArbiter
 * @brief module ���, ������ ���� class
 */
class VmModuleArbiter
{
private:
protected:
	// Module ID : 32bit
	// 8 bit : Module Type, 8 bit : Specific bits, 16bit : Module Count-based ID
	/*!
	 * @cond
	 */
	string version;	// module arbiter version�� ���� specification
	uint render_module_count_id;	// count-based ID for render module, 0 to 65535
	uint tmap_module_count_id;	// count-based ID for tf module, 0 to 65535
	uint vgen_module_count_id;	// count-based ID for vg module, 0 to 65535
	uint helper_module_count_id;	// count-based ID for helper module, 0 to 65535
	map<int /*ID*/, VmModule*> map_modules; // module ID container
	map<string /*ModuleName*/, int /*ID*/> map_module_names; // module file name container
	/*!
	 * @endcond
	 */

public:
	/*!
	 * @brief constructor
	 * @param strArbiterVersion [in] \n wstring \n module arbiter version�� ���� specification
	 */
	VmModuleArbiter(string _version);
	~VmModuleArbiter();
	
	/*!
	 * @brief module�� file �̸����� module�� ����ϴ� �Լ�
	 * @param module_name [in] \n string \n module file�� ��� �� �̸�
	 * @param mtype [in] \n EvmModuleType \n ����� module type
	 * @param descriptor [in] \n string \n ����� module�� ���� descriptor, ������ ������ ������, string Ÿ���� �ƹ� ���� �־ ��
	 * @return int \n ��ϵ� module�� ID
	 * @remarks 23 ~ 31 bit : Module Type, 16 ~ 23 bit : Specific bits, 0 ~ 15 bit : Module Count-based ID
	 */
	int RegisterModule(const string& module_name, const EvmModuleType mtype, const string& descriptor);
	
	/*!
	 * @brief �ش� ID�� module�� ���� parameter�� ��ȿ���� �˻��ϴ� �Լ�
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module�� ���� parameter�� ����� VmFnContainer �� ������
	 * @return bool \n module parameter�� ��ȿ�ϸ� true, �׷��� ������ false ��ȯ
	 * @remarks ��� ���� �� Check�� �ϴ� ���� ����
	 */
	bool CheckModuleParameters(const int module_id, VmFnContainer& vfn);
	/*!
	 * @brief �ش� ID�� module�� �����ϴ� �Լ�
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module�� ���� parameter�� ����� VmFnContainer �� ������
	 * @param is_initialized [out] \n bool \n true�� ���ο��� initialize�� ����, false �� �̹� initialized �� ����
	 * @return bool \n module �� ���������� �����ϸ� true, �׷��� ������ false ��ȯ
	 * @remarks module common interface�� DoModule�� ����Ǹ�, ���� ���� �� InitModule�� ���� �����
	 */
	bool ExecuteModule(const int module_id, VmFnContainer& vfn, bool* is_initialized = NULL);
	/*!
	 * @brief �ش� ID�� module�� ��� �����ϴ� �Լ�
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module�� ���� parameter�� ����� VmFnContainer �� ������
	 * @return bool \n module �� ���������� �����Ǹ� true, �׷��� ������ false ��ȯ
	 * @remarks ���� �� module common interface�� DeinitModule�� �����
	 */
	bool ClearModule(const int module_id, VmFnContainer& vfn);
	/*!
	 * @brief �ش� ID�� module���� �۾� ���� ���� progress�� ����
	 * @param module_id [in] \n int \n module ID
	 * @return double \n 0.0 ~ 100.0
	 * @remarks module ���� �� code �ܰ迡�� vxobjects::LocalProgress �� ����� progress �� ��ȯ
	 * @ra vmobjects::LocalProgress
	 */
	double GetProgress(const int module_id);

	/*!
	 * @brief �ش� ID�� module�� customized interoperation �� �ϱ� ���� �Լ�
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module�� ���� parameter�� ����� VmFnContainer �� ������
	 * @return bool \n module���� interoperation�� ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
	 * @remarks 
	 * ������ container�� Module ������ �ƴ� Ư���ϰ� ����� container �������� parameter�� ������ ���� ���� \n
	 * container�� ������ @ref vmobjects::VmModuleExecute �� ����
	 * @ra VmModuleArbiter::ExecuteModule
	 */
	bool InteropModuleCustomWork(const int module_id, VmFnContainer& vfn);

	/*!
	 * @brief ���� module arbiter�� ������ Ȯ��
	 * @return string \n ������ ���� ���ڿ� ��ȯ
	 */
	string GetVersionInfo(){ return version; }
	/*!
	 * @brief �ش� ID�� module�� ���� mention
	 * @param module_id [in] \n int \n module ID
	 * @return string \n module�� ���� mention�� ��ȯ
	 */
	string GetModuleDescriptor(const int module_id);
	/*!
	 * @brief module�� ���� file �̸����� ���� module�� ID ����
	 * @param module_name [in] \n string \n module�� ���� file �̸�
	 * @return int \n module ID�� ��ȯ
	 */
	int GetModuleIDByModuleName(const string& module_name);
	/*!
	 * @brief �ش� module�� requirements �� ����
	 * @param requirements [out] \n vector<string> \n Module�� requirements �� ������ string �� vector
	 * @param module_id [in] \n int \n module ID
	 * @return bool \n Module�� GetModuleSpecification �� ���ǵǾ� ������ true, �׷��� ������ false
	 */
	bool GetModuleRequirements(vector<string>& requirements, const int module_id);
	/*!
	 * @brief module ID�� encoding �Ǿ� �ִ� module type�� ��� static helper function
	 * @param module_id [in] \n int \n module ID
	 * @return EvmModuleType \n module type
	 */
	static EvmModuleType GetModuleType(const int module_id);
};
