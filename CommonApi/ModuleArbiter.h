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
	ModuleTypeTMAP,/*!< OTF setting 관련 module */
	ModuleTypeVGENERATION,/*!< VObject 생성/처리/분석 관련 module */
	ModuleTypeETC/*!< Customized-Define Module */
};
/**
 * @class VmModule
 * @brief Framework에서 module을 정의하는 자료구조
 */
struct VmModule{
	/**
	 * @brief module의 type
	 */
	EvmModuleType mtype;
	/**
	 * @brief module에 대한 mention
	 */
	string descriptor;
	/**
	 * @brief module에 대한 dll handle, win32 용
	 */
	VmHMODULE hMouleLib;
	/**
	 * @brief module의 공통 function interface에 대한 pointer, CheckIsAvailableParameters
	 */
	LPDLLfnCheckModuleParameters lpdllCheckModuleParameters;
	/**
	 * @brief module의 공통 function interface에 대한 pointer, InitModule
	 */
	LPDLLfnInit lpdllInit;
	/**
	 * @brief module의 공통 function interface에 대한 pointer, DoModule
	 */
	LPDLLfnDoModule lpdllDoModule;
	/**
	 * @brief module의 공통 function interface에 대한 pointer, DeinitModule
	 */
	LPDLLfnDeInit lpdllDeInit;
	/**
	 * @brief module의 공통 function interface에 대한 pointer, GetProgress
	 */
	LPDLLfnGetProgress lpdllGetProgress;	 // Optional
	/**
	 * @brief module의 공통 function interface에 대한 pointer, GetModuleSpecification
	 */
	LPDLLfnGetModuleRequirements lpdllGetModuleRequirements; // Optional
	/**
	 * @brief module의 공통 function interface에 대한 pointer, InteropCustomWork
	 */
	LPDLLfnInteropCustomWork lpdllInteropCustomWork; // Optional
	/**
	 * @brief 현재 모듈이 초기화 되었는가 여부, 즉 "lpdllInit" 이 호출되었는가 여부
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
 * @brief module 등록, 관리를 위한 class
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
	string version;	// module arbiter version에 대한 specification
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
	 * @param strArbiterVersion [in] \n wstring \n module arbiter version에 대한 specification
	 */
	VmModuleArbiter(string _version);
	~VmModuleArbiter();
	
	/*!
	 * @brief module의 file 이름으로 module을 등록하는 함수
	 * @param module_name [in] \n string \n module file의 경로 및 이름
	 * @param mtype [in] \n EvmModuleType \n 등록할 module type
	 * @param descriptor [in] \n string \n 등록할 module에 대한 descriptor, 별도의 형식은 없으며, string 타입의 아무 값을 넣어도 됨
	 * @return int \n 등록된 module의 ID
	 * @remarks 23 ~ 31 bit : Module Type, 16 ~ 23 bit : Specific bits, 0 ~ 15 bit : Module Count-based ID
	 */
	int RegisterModule(const string& module_name, const EvmModuleType mtype, const string& descriptor);
	
	/*!
	 * @brief 해당 ID의 module에 들어가는 parameter의 유효성을 검사하는 함수
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module을 위한 parameter가 저장된 VmFnContainer 의 포인터
	 * @return bool \n module parameter가 유효하면 true, 그렇지 않으면 false 반환
	 * @remarks 모듈 수행 전 Check를 하는 것이 좋음
	 */
	bool CheckModuleParameters(const int module_id, VmFnContainer& vfn);
	/*!
	 * @brief 해당 ID의 module을 수행하는 함수
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module을 위한 parameter가 저장된 VmFnContainer 의 포인터
	 * @param is_initialized [out] \n bool \n true면 내부에서 initialize가 수행, false 면 이미 initialized 된 상태
	 * @return bool \n module 이 성공적으로 수행하면 true, 그렇지 않으면 false 반환
	 * @remarks module common interface의 DoModule이 실행되며, 최초 실행 시 InitModule이 먼저 수행됨
	 */
	bool ExecuteModule(const int module_id, VmFnContainer& vfn, bool* is_initialized = NULL);
	/*!
	 * @brief 해당 ID의 module을 등록 해지하는 함수
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module을 위한 parameter가 저장된 VmFnContainer 의 포인터
	 * @return bool \n module 이 성공적으로 해제되면 true, 그렇지 않으면 false 반환
	 * @remarks 해제 시 module common interface의 DeinitModule이 수행됨
	 */
	bool ClearModule(const int module_id, VmFnContainer& vfn);
	/*!
	 * @brief 해당 ID의 module에서 작업 수행 중인 progress를 얻음
	 * @param module_id [in] \n int \n module ID
	 * @return double \n 0.0 ~ 100.0
	 * @remarks module 개발 시 code 단계에서 vxobjects::LocalProgress 에 적용된 progress 를 반환
	 * @ra vmobjects::LocalProgress
	 */
	double GetProgress(const int module_id);

	/*!
	 * @brief 해당 ID의 module과 customized interoperation 을 하기 위한 함수
	 * @param module_id [in] \n int \n module ID
	 * @param vfn [in] \n VmFnContainer \n module을 위한 parameter가 저장된 VmFnContainer 의 포인터
	 * @return bool \n module과의 interoperation이 성공적으로 수행되면 true, 그렇지 않으면 false 반환
	 * @remarks 
	 * 각각의 container에 Module 공통이 아닌 특별하게 사용할 container 형식으로 parameter를 제공할 수도 있음 \n
	 * container의 사용법은 @ref vmobjects::VmModuleExecute 과 동일
	 * @ra VmModuleArbiter::ExecuteModule
	 */
	bool InteropModuleCustomWork(const int module_id, VmFnContainer& vfn);

	/*!
	 * @brief 현재 module arbiter의 버젼을 확인
	 * @return string \n 버젼에 대한 문자열 반환
	 */
	string GetVersionInfo(){ return version; }
	/*!
	 * @brief 해당 ID의 module에 대한 mention
	 * @param module_id [in] \n int \n module ID
	 * @return string \n module에 대한 mention을 반환
	 */
	string GetModuleDescriptor(const int module_id);
	/*!
	 * @brief module에 대한 file 이름으로 부터 module을 ID 얻음
	 * @param module_name [in] \n string \n module에 대한 file 이름
	 * @return int \n module ID를 반환
	 */
	int GetModuleIDByModuleName(const string& module_name);
	/*!
	 * @brief 해당 module의 requirements 을 얻음
	 * @param requirements [out] \n vector<string> \n Module의 requirements 을 저장할 string 의 vector
	 * @param module_id [in] \n int \n module ID
	 * @return bool \n Module에 GetModuleSpecification 이 정의되어 있으면 true, 그렇지 않으면 false
	 */
	bool GetModuleRequirements(vector<string>& requirements, const int module_id);
	/*!
	 * @brief module ID에 encoding 되어 있는 module type을 얻는 static helper function
	 * @param module_id [in] \n int \n module ID
	 * @return EvmModuleType \n module type
	 */
	static EvmModuleType GetModuleType(const int module_id);
};
