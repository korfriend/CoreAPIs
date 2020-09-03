#include "VimCommon.h"
#include "VisMtvApi.h"
#include "../GpuManager/GpuManager.h"
#include "ModuleArbiter.h"
#include "ResourceManager.h"

#include <iostream>

#pragma warning (disable:4756)

bool g_is_display = true;

#define TESTOUT(NAME, P) {if(g_is_display){cout << NAME << P.x << ", " << P.y << ", " << P.z << endl;}}

using namespace vzm;
using namespace std;
using namespace vmmath;
using namespace vmhelpers;
using namespace vmgpuinterface;

VmModuleArbiter* module_arbitor = NULL;
VmResourceManager* res_manager = NULL;
VmLObject* global_buf_obj = NULL;

#define __cv3__ *(glm::fvec3*)
#define __cv4__ *(glm::fvec4*)
#define __cm4__ *(glm::fmat4x4*)

struct CamResources
{
	CameraParameters cam_params;
	vector<VmObject*> scene_fbuf_objs; // VmIObject
	VmLObject* scene_lbuf_obj;
	ullong recent_rendered_time;
	CamResources()
	{
		scene_lbuf_obj = NULL;
		recent_rendered_time = 0;
	}
	void Delete()
	{
		res_manager->EraseObject(scene_lbuf_obj->GetObjectID());
		for (int i = 0; i < scene_fbuf_objs.size(); i++)
			res_manager->EraseObject(scene_fbuf_objs[i]->GetObjectID());
	}
};

struct SceneInfo
{
	map<int, CamResources> cam_res; // <cam_id, ram_res>
	map<int, ObjStates> scene_objs; // <obj_id, state>
	SceneEnvParameters env_params;
	bool is_changed;
	bool is_defined_CameraParameters;
	bool is_defined_SceneEnvParameters;
	bool is_defined_ObjStates;

	SceneInfo()
	{
		is_changed = true;
		is_defined_CameraParameters = is_defined_SceneEnvParameters = is_defined_ObjStates = false;
	}
	void Delete()
	{
		for (auto it = cam_res.begin(); it != cam_res.end(); it++)
		{
			CamResources& cam_res = it->second;
			cam_res.Delete();
		}
	}
};

auto fail_ret = [](const string& err_str)
{
	if(g_is_display)
		cout << err_str << endl;
	return false;
};

map<int, VmGpuManager*> gpu_manager;
map<int, SceneInfo> scene_manager;
map <string, VmHMODULE> dll_import;
std::map<std::tuple<int/*scene id*/, int/*cam_id*/>, std::map<int/*obj_id*/, std::map<std::string, std::tuple<size_t, byte*>>>> _test_dojo_scripts;

template <typename T>
T LoadDLL(string dll_name, string function_name)
{
	VmHMODULE hMouleLib = NULL;
	auto it = dll_import.find(dll_name);
	if (it == dll_import.end())
	{
		VMLOADLIBRARY(hMouleLib, dll_name.c_str());
		if (hMouleLib != NULL)
			dll_import[dll_name] = hMouleLib;
	}
	else
		hMouleLib = it->second;
	if (hMouleLib == NULL)
	{
		if(g_is_display)
			cout << "DLL import ERROR : " << dll_name << endl;
		return NULL;
	}

	T lpdll_function = NULL;
	lpdll_function = (T)VMGETPROCADDRESS(hMouleLib, function_name.c_str());
	if (lpdll_function == NULL && g_is_display)
		cout << "FUNC import ERROR : " << function_name << endl;
	return lpdll_function;
}

double g_dProgress = 0;
bool is_initialized_engine = false;
int __module_rendererCpu = -1;
int __module_rendererGpu = -1;
int __module_RWFiles = -1;
int __module_TMapGen = -1;
int __module_CoreProcP = -1;
int __module_CoreProcV = -1;

size_t renderer_excutable_count = 0;

bool vzm::InitEngineLib()
{
	// register in-bulit modules //
	if (module_arbitor || res_manager || gpu_manager.size() != 0 || is_initialized_engine)
		return fail_ret("ERROR - ALREADY INITIALIZED!!");

	std::cout << "\nDongjoon's VisMotive Framework "<< VERSION << "\n" << std::endl;

	res_manager = new VmResourceManager();
	module_arbitor = new VmModuleArbiter("Dongjoon's VisMotive Framework " + string(VERSION));

#if defined (_WIN64)
	//#if _MSC_VER >= 1800
	_set_FMA3_enable(0);
	//#endif
#endif
	is_initialized_engine = true;

	__module_RWFiles = module_arbitor->RegisterModule("vismtv_inbuilt_rwfiles", ModuleTypeETC, "");
	__module_TMapGen = module_arbitor->RegisterModule("vismtv_inbuilt_tfmapgenerator", ModuleTypeETC, "");
	__module_CoreProcP = module_arbitor->RegisterModule("vismtv_inbuilt_ptypeprocessing", ModuleTypeVGENERATION, "");
	__module_CoreProcV = module_arbitor->RegisterModule("vismtv_inbuilt_vtypeprocessing", ModuleTypeVGENERATION, "");
	__module_rendererCpu = module_arbitor->RegisterModule("vismtv_inbuilt_renderercpu", ModuleTypeRENDER, "");
	__module_rendererGpu = module_arbitor->RegisterModule("vismtv_inbuilt_renderergpudx", ModuleTypeRENDER, "");

	if(__module_rendererGpu > 0)
	{
		vector<string> vtrModuleSpec;
		if (module_arbitor->GetModuleRequirements(vtrModuleSpec, __module_rendererGpu))
		{
			for (int i = 0; i < (int)vtrModuleSpec.size(); i++)
			{
				if (vtrModuleSpec[i] == "GPUMANAGER")
				{
					VmFnContainer svxModuleParam;
					VmGpuManager* pCGM = NULL;
					svxModuleParam.rmw_buffers["_outptr_class_GpuManager"] = (void*)&pCGM;
					if (module_arbitor->InteropModuleCustomWork(__module_rendererGpu, svxModuleParam))
					{
						if (pCGM != NULL)
							gpu_manager[__module_rendererGpu] = pCGM;
					}
					break;
				}
			}
		}
		if (gpu_manager.size() == 0)
		{
			DeinitEngineLib();
			return fail_ret("NO GPU MANAGER!");
		}
	}

	global_buf_obj = new VmLObject();
	res_manager->RegisterLObject(global_buf_obj);

	if (__module_RWFiles <= 0 || __module_TMapGen <= 0 || 
		__module_CoreProcP <= 0 || __module_CoreProcV <= 0 ||
		__module_rendererCpu <= 0 || __module_rendererGpu <= 0) {
		DeinitEngineLib();
		return fail_ret("CORE MODULE LOAD FAILURE!");
	}

	std::cout << "VisMotive Initialization Success" << std::endl;
	return true;
}

bool vzm::DeinitEngineLib()
{
	typedef bool(*LPDLL_CALL_dxDeinitialize)();
	LPDLL_CALL_dxDeinitialize lpdll_function = LoadDLL<LPDLL_CALL_dxDeinitialize>("vismtv_dxutwrapper", "dxDeinitialize");
	if (lpdll_function != NULL) lpdll_function();

	if (!is_initialized_engine)
		return fail_ret("NOT INITIALIZED!");
	if (res_manager)
	{
		// This is for Module DLL Static function's DELEGATION
		res_manager->EraseAllLObjects();
	}
	
	VMSAFE_DELETE(module_arbitor); // module_arbitor must be deleted BEFORE g_pCResourceManager and g_pCGpuManager
	VMSAFE_DELETE(res_manager);

	for (auto it = _test_dojo_scripts.begin(); it != _test_dojo_scripts.end(); it++)
	{
		for (auto it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			for (auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++)
				delete[] std::get<1>(it2->second);
		}
	}

	std::cout << "***Scene Summary***" << std::endl;
	std::cout << "   # of scenes : " << scene_manager.size() << std::endl;
	for (auto it = scene_manager.begin(); it != scene_manager.end(); it++)
	{
		std::cout << "   # of cameras           in scene (" << it->first << ") : " << it->second.cam_res.size() << std::endl;
		std::cout << "   # of remaining objects in scene (" << it->first << ") : " << it->second.scene_objs.size() << std::endl;
	}

	scene_manager.clear();
	gpu_manager.clear();

	for (auto it = dll_import.begin(); it != dll_import.end(); it++)
		VMFREELIBRARY(it->second);
	dll_import.clear();

	is_initialized_engine = false;

	std::cout << "Render (successfully) calls : " << renderer_excutable_count << std::endl;
	std::cout << "Bye Bye~ ^^" << std::endl;

	return true;
}

void vzm::DebugTestSet(const std::string& _script, const void* _pvalue, const size_t size_bytes, const int scene_id, const int cam_id, const int obj_id)
{
	std::map<int/*obj_id*/, std::map<std::string, std::tuple<size_t, byte*>>>& container1 = _test_dojo_scripts[std::tuple<int, int>(scene_id, cam_id)];
	std::map<std::string, std::tuple<size_t, byte*>>& container2 = container1[obj_id];
	auto it = container2.find(_script);
	if (it != container2.end()) delete[] std::get<1>(it->second);
	byte* pv = new byte[size_bytes];
	memcpy(pv, _pvalue, size_bytes);
	container2[_script] = std::tuple<size_t, byte*>(size_bytes, pv);
}

void vzm::DisplayConsoleMessages(const bool is_display)
{
	g_is_display = is_display;
}

bool vzm::ExecuteModule2(const std::string& module_dll_file, const std::string& dll_function, const std::initializer_list<int>& io_obj_ids, const std::map<string, any>& parameters)
{
	typedef bool(*LPDLL_CALL_FUNTION)(const std::vector<vmobjects::VmObject*>& io_objs, const std::map<string, any>& parameters);
	LPDLL_CALL_FUNTION lpdll_function_launcher = LoadDLL<LPDLL_CALL_FUNTION>(module_dll_file, dll_function);

	if (!lpdll_function_launcher)
		return fail_ret("INVALID MODULE FUNCTION");

	std::vector<VmObject*> io_vmobjs;
	for (int obj_id : io_obj_ids)
	{
		VmObject* vmobj = res_manager->GetObjectW(obj_id);
		if (vmobj == NULL)
			return fail_ret("INVALID VMOBJ ID DETECTED : " + to_string(obj_id));
		io_vmobjs.push_back(vmobj);
		//switch (VmObject::GetObjectTypeFromID(obj_id))
		//{
		//case EvmObjectType::ObjectTypePRIMITIVE:
		//case EvmObjectType::ObjectTypeVOLUME:
		//case EvmObjectType::ObjectTypeTMAP:
		//case EvmObjectType::ObjectTypeIMAGEPLANE:
		//case EvmObjectType::ObjectTypeBUFFER:
		//default:
		//	return false;
		//}
	}

	lpdll_function_launcher(io_vmobjs, parameters);
	
	return true;
}

#define SET_FUNC(vfn, OT, is_in, obj) vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(OT, is_in), vector<VmObject*>(1, obj)))

auto splitpath = [](const std::string& str, const std::set<char>& delimiters) -> std::vector<std::string>
{
	std::vector<std::string> result;

	char const* pch = str.c_str();
	char const* start = pch;
	for (; *pch; ++pch)
	{
		if (delimiters.find(*pch) != delimiters.end())
		{
			if (start != pch)
			{
				std::string str(start, pch);
				result.push_back(str);
			}
			else
			{
				result.push_back("");
			}
			start = pch + 1;
		}
	}
	result.push_back(start);

	return result;
};

bool vzm::LoadModelFile(const std::string& filename, int& obj_id, const bool unify_redundancy)
{
	bool is_prim = true;
	string file_ext = filename.substr(filename.find_last_of(".") + 1);
	string usage = ""; // to do //
	if (file_ext == "stl")
		usage = "IMPORTMESH_STL";
	else if (file_ext == "obj")
		usage = "IMPORTMESH_OBJ";
	else if (file_ext == "ply")
		usage = "IMPORTMESH_PLY";
	else
	{
		is_prim = false;

		if (file_ext == "den")
			usage = "IMPORTVOLUME_RAW";
		else if (file_ext == "x3d")
			usage = "IMPORTVOLUME_X3D";
		else
		{
			if(g_is_display)
				std::cout << "Invalid Format! (Not Yet Supported.. of " << file_ext << ")" << std::endl;
			return fail_ret("LoadModelFile FAILURE!");
		}
	}

	// 1st .stl and .obj
	VmVObject* obj = res_manager->GetVObject(obj_id);
	bool is_new = obj == NULL;
	int tmp_obj_id = obj_id;

	VmFnContainer vfn;
	vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeBUFFER, true), vector<VmObject*>(1, global_buf_obj)));
	vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeBUFFER, false), vector<VmObject*>(1, global_buf_obj)));
	vfn.vmparams.insert(pair<string, void*>("_string_UsageMode", &usage));

	// primitives
	bool is_center_aligned = false;
	bool is_from_ccw = false;
	bool is_topology_connected = unify_redundancy;
	bool is_wireframe = false;
	vmdouble4 obj_color = vmdouble4(1, 1, 0, 1);
	double scale_factor = 1.0;

	// volumes
	vmint3 vol_size;
	vmdouble3 vol_pitch;
	string data_type("USHORT");
	vmdouble3 vec_axis_osx2wsx, vec_axis_osy2wsy;
	int header_offset;
	bool is_axis_z_rhs;

	if (is_prim)
	{
		VmVObjectPrimitive* prim_obj = (VmVObjectPrimitive*)obj;
		if (is_new)
		{
			obj = prim_obj = new VmVObjectPrimitive();
			tmp_obj_id = res_manager->RegisterVObject(prim_obj, ObjectTypePRIMITIVE);
		}

		global_buf_obj->ReplaceOrAddStringBuffer("_vlist_STRING_FileNames", &filename, 1);
		//global_buf_obj->ReplaceOrAddBufferPtr("_vlist_INT_ObjectIDs", &tmp_obj_id, 1, sizeof(int)); // will be deprecated

		vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypePRIMITIVE, false), vector<VmObject*>(1, prim_obj)));

		vfn.vmparams.insert(pair<string, void*>("_bool_IsCenterAligned", &is_center_aligned));
		//vfn.vmparams.insert(pair<string, void*>("_bool_IsFromCCW", &is_from_ccw)); // will be deprecated
		vfn.vmparams.insert(pair<string, void*>("_bool_IsTopologyConnected", &is_topology_connected));
		vfn.vmparams.insert(pair<string, void*>("_bool_IsWireframe", &is_wireframe));
		vfn.vmparams.insert(pair<string, void*>("_double4_MeshColor", &obj_color));
		vfn.vmparams.insert(pair<string, void*>("_double_ScaleFactor", &scale_factor));
	}
	else
	{
		if (file_ext == "den")
		{
			std::set<char> delims{ '\\', '/' };
			std::vector<std::string> name = splitpath(filename, delims);
			string _file = name.back();
			std::set<char> __delim{ '_' };
			std::vector<std::string> file_info = splitpath(_file, __delim);
			if (file_info.size() != 4)
				return false;

			std::set<char> x_delim{ 'x' };
			std::vector<std::string> s_vol_size = splitpath(file_info[1], x_delim);
			if (s_vol_size.size() != 3)
				return false;
			vol_size = vmint3(std::stoi(s_vol_size[0]), std::stoi(s_vol_size[1]), std::stoi(s_vol_size[2]));

			std::vector<std::string> s_vol_pitch = splitpath(file_info[2], x_delim);
			if (s_vol_pitch.size() != 3)
				return false;
			vol_pitch = vmdouble3(std::stod(s_vol_pitch[0]), std::stod(s_vol_pitch[1]), std::stod(s_vol_pitch[2]));
			
			if (file_info[3] == "byte.den")
				data_type = "BYTE";
			else if (file_info[3] == "ushort.den")
				data_type = "USHORT";
			else if (file_info[3] == "char.den")
				data_type = "CHAR";
			else if (file_info[3] == "short.den")
				data_type = "SHORT";
			else if (file_info[3] == "float.den")
				data_type = "FLOAT";

			global_buf_obj->ReplaceOrAddStringBuffer("_vlist_STRING_FileNames", &filename, 1);
			vfn.vmparams.insert(pair<string, void*>("_int3_VolumeFileSize", &vol_size));
			vfn.vmparams.insert(pair<string, void*>("_double3_VoxelPitch", &vol_pitch));
			vfn.vmparams.insert(pair<string, void*>("_string_DataType", &data_type));
		}
		else if (file_ext == "x3d")
		{
			string _filename = filename;
			string _info_file_name = _filename.replace(_filename.length() - 3, 3, "txt");
			vfn.vmparams.insert(pair<string, void*>("_string_FileName", &_info_file_name));
			usage = "GET_FILE_INFO_X3D"; // pointer set
			if (!module_arbitor->ExecuteModule(__module_RWFiles, vfn))
			{
				if (is_new) res_manager->EraseVObject(tmp_obj_id);
				return fail_ret("Failed to load the file in the file load module!");
			}

			int* ptr_int;
			double* ptr_double;
			size_t buf_size;
			int num_eles;
			global_buf_obj->LoadBufferPtr("_vlist_INT_HeaderSize", (void**)&ptr_int, buf_size, &num_eles);
			header_offset = ptr_int[0];
			global_buf_obj->LoadBufferPtr("_vlist_INT_ImageSize", (void**)&ptr_int, buf_size, &num_eles);
			vol_size = vmint3(ptr_int[0], ptr_int[1], ptr_int[2]);
			global_buf_obj->LoadBufferPtr("_vlist_INT_DataType", (void**)&ptr_int, buf_size, &num_eles);
			switch (ptr_int[0])
			{
			case 1: data_type = "BYTE"; break;
			case 3: data_type = "CHAR"; break;
			case 4: data_type = "USHORT"; break;
			case 5: data_type = "SHORT"; break;
			case 11: data_type = "FLOAT"; break;
			default:
				return fail_ret("NOT SUPPORTED DATA TYPE IN LoadModelFile!");
			}
			global_buf_obj->LoadBufferPtr("_vlist_DOUBLE_PitchSize", (void**)&ptr_double, buf_size, &num_eles);
			vol_pitch = vmdouble3(ptr_double[0], ptr_double[1], ptr_double[2]);
			//global_buf_obj->LoadBufferPtr("_vlist_DOUBLE_Thickness", (void**)&ptr_double, buf_size, &num_eles);
			//global_buf_obj->LoadBufferPtr("_vlist_DOUBLE_Pos", (void**)&ptr_double, buf_size, &num_eles);
			global_buf_obj->LoadBufferPtr("_vlist_DOUBLE_AlignOS2WS", (void**)&ptr_double, buf_size, &num_eles);
			vec_axis_osx2wsx = vmdouble3(ptr_double[0], ptr_double[1], ptr_double[2]);
			vec_axis_osy2wsy = vmdouble3(ptr_double[3], ptr_double[4], ptr_double[5]);
			is_axis_z_rhs = ptr_double[6] != 0;
			vfn.vmparams.insert(pair<string, void*>("_int3_VolumeFileSize", &vol_size));
			vfn.vmparams.insert(pair<string, void*>("_double3_VoxelPitch", &vol_pitch));
			vfn.vmparams.insert(pair<string, void*>("_string_DataType", &data_type));
			vfn.vmparams.insert(pair<string, void*>("_bool_IsAxisZ_RHS", &is_axis_z_rhs));
			vfn.vmparams.insert(pair<string, void*>("_int_HeaderOffetBytes", &header_offset));
			vfn.vmparams.insert(pair<string, void*>("_double3_VecAxisOSX2WSX", &vec_axis_osx2wsx));
			vfn.vmparams.insert(pair<string, void*>("_double3_VecAxisOSY2WSY", &vec_axis_osy2wsy));
			usage = "IMPORTVOLUME_X3D";
			global_buf_obj->ReplaceOrAddStringBuffer("_vlist_STRING_FileNames", &filename, 1);
		}
		else
		{
			// header work
		}

		VmVObjectVolume* vol_obj = (VmVObjectVolume*)obj;
		tmp_obj_id = obj_id;
		if (is_new)
		{
			obj = vol_obj = new VmVObjectVolume();
			tmp_obj_id = res_manager->RegisterVObject(vol_obj, ObjectTypeVOLUME);
		}


		vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeVOLUME, false), vector<VmObject*>(1, vol_obj)));
	}

	if (!module_arbitor->ExecuteModule(__module_RWFiles, vfn))
	{
		global_buf_obj->ClearAllBuffers();
		global_buf_obj->ClearAllDstObjValues();
		if(g_is_display)
			std::cout << "Failed to load the file in the file load module" << std::endl;
		if (is_new) res_manager->EraseVObject(tmp_obj_id);
		return fail_ret("Failed to load the file in the file load module!");
	}
	global_buf_obj->ClearAllBuffers();
	global_buf_obj->ClearAllDstObjValues();
	obj_id = tmp_obj_id;

	if (is_prim)
	{
		PrimitiveData* prim_data = ((VmVObjectPrimitive*)obj)->GetPrimitiveData();
		if (g_is_display)
		{
			std::cout << "Success to load " << filename << std::endl;
			std::cout << "==> min/max pos of the object are (" <<
				prim_data->aabb_os.pos_min.x << ", " << prim_data->aabb_os.pos_min.y << ", " << prim_data->aabb_os.pos_min.z << ") / (" <<
				prim_data->aabb_os.pos_max.x << ", " << prim_data->aabb_os.pos_max.y << ", " << prim_data->aabb_os.pos_max.z << ")" << std::endl;
		}
	}
	else
	{
		VolumeData* vol_data = ((VmVObjectVolume*)obj)->GetVolumeData();
		if (g_is_display)
		{
			std::cout << "Success to load " << filename << std::endl;
			std::cout << "==> volume size / pitch (" <<
				vol_data->vol_size.x << ", " << vol_data->vol_size.y << ", " << vol_data->vol_size.z << ") / (" <<
				vol_data->vox_pitch.x << ", " << vol_data->vox_pitch.y << ", " << vol_data->vox_pitch.z << ")" << std::endl;
		}

		obj->RegisterCustomParameter("_matrix_originalOS2WS", obj->GetMatrixOS2WS());
	}

	//if (!is_new)
	//{
	//	for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
	//		itg->second->ReleaseGpuResourcesBySrcID(obj_id);
	//}

	return true;
}

bool vzm::GenerateEmptyVolume(int& vol_id, const int ref_vol_id, const std::string& data_type, const double min_v, const double max_v, const double fill_v)
{
	VmVObjectVolume* vol_obj = (VmVObjectVolume*)res_manager->GetVObject(vol_id);
	if (vol_obj == NULL)
	{
		vol_obj = new VmVObjectVolume();
		vol_id = res_manager->RegisterVObject(vol_obj, ObjectTypeVOLUME, vol_id);
	}

	if (vol_id == NULL) return false;

	VmVObjectVolume* ref_vol_obj = (VmVObjectVolume*)res_manager->GetVObject(ref_vol_id);
	if (ref_vol_obj)
	{
		VmFnContainer vfn;
		vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeVOLUME, true), { ref_vol_obj }));
		vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeVOLUME, false), { vol_obj }));
		vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeBUFFER, true), { global_buf_obj }));

		VolumeData* ref_vol_data = ref_vol_obj->GetVolumeData();
		
		string _funcmode = "NEWALLOCATION";
		vmint3 _vol_size = ref_vol_data->vol_size;
		vmint3 _vol_bnd = ref_vol_data->bnd_size;
		vmdouble3 _vol_pitch = ref_vol_data->vox_pitch;
		vmmat44 _mat_vs2ws = ref_vol_obj->GetMatrixOS2WS();
		bool _is_zrhs = ref_vol_data->axis_info.is_rhs;
		vmdouble3 _axis_os2ws_x = ref_vol_data->axis_info.vec_axisx_ws;
		vmdouble3 _axis_os2ws_y = ref_vol_data->axis_info.vec_axisy_ws;

		vfn.vmparams.insert(pair<string, void*>("_string_FunctionMode", &_funcmode));
		vfn.vmparams.insert(pair<string, void*>("_int3_VolumeSize", &_vol_size));
		vfn.vmparams.insert(pair<string, void*>("_int3_ExBoundarySize", &_vol_bnd));
		vfn.vmparams.insert(pair<string, void*>("_double3_VolumePitchSize", &_vol_pitch));
		vfn.vmparams.insert(pair<string, void*>("_matrix44_VS2WS", &_mat_vs2ws));
		vfn.vmparams.insert(pair<string, void*>("_bool_IsAxisZRHS", &_is_zrhs));
		vfn.vmparams.insert(pair<string, void*>("_double3_VecAxisOSX2WSX", &_axis_os2ws_x));
		vfn.vmparams.insert(pair<string, void*>("_double3_VecAxisOSX2WSY", &_axis_os2ws_y));

		vmdouble2 minmax_v(min_v, max_v);
		vfn.vmparams.insert(pair<string, void*>("_string_DataType", (void*)&data_type));
		vfn.vmparams.insert(pair<string, void*>("_double2_MinMaxValue", &minmax_v));
		vfn.vmparams.insert(pair<string, void*>("_double_FillingValue", (void*)&fill_v));

		if (!module_arbitor->ExecuteModule(__module_CoreProcV, vfn))
			return fail_ret("GenerateEmptyVolume :: Module Execution Failure!");
	}

	return false;
}

bool vzm::GenerateEmptyPrimitive(int& prim_id)
{
	VmVObjectPrimitive* prim_obj = (VmVObjectPrimitive*)res_manager->GetVObject(prim_id);
	if (prim_obj != NULL)
		DeleteObject(prim_id);
	prim_obj = new VmVObjectPrimitive();
	prim_id = res_manager->RegisterVObject(prim_obj, ObjectTypePRIMITIVE, prim_id);
	return true;
}

auto __update_picking_state = [](VmVObjectPrimitive* prim_obj, bool force_to_update)
{
	if (prim_obj->GetCustumDataStructure("_SVXDXUTData_DxUtMesh") == NULL && !force_to_update)
		return true; // 이 경우 picking 으로 등록되지 않음

	typedef bool(*LPDLL_CALL_dxUpdate)(VmVObjectPrimitive* prim_obj);
	LPDLL_CALL_dxUpdate lpdll_function = LoadDLL<LPDLL_CALL_dxUpdate>("vismtv_dxutwrapper", "dxUpdateDx10Mesh");
	if (lpdll_function == NULL)
		return fail_ret("MODULE LOAD FAILURE IN __update_picking_state!");

	//if(force_to_update)
	//	printf("__update_picking_state ==> %x\n", prim_obj->GetObjectID());
	return lpdll_function(prim_obj);
};


auto __GetPObject = [](int& obj_id) -> VmVObjectPrimitive*
{
	VmVObjectPrimitive* prim_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_id);
	if (prim_obj == NULL)
	{
		prim_obj = new VmVObjectPrimitive();
		obj_id = res_manager->RegisterVObject(prim_obj, ObjectTypePRIMITIVE);
	}
	//else
	//{
	//	for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
	//		itg->second->ReleaseGpuResourcesBySrcID(obj_id);
	//}
	return prim_obj;
};

auto __GetVObject = [](int& obj_id) -> VmVObjectVolume*
{
	VmVObjectVolume* vol_obj = (VmVObjectVolume*)res_manager->GetVObject(obj_id);
	if (vol_obj == NULL)
	{
		vol_obj = new VmVObjectVolume();
		obj_id = res_manager->RegisterVObject(vol_obj, ObjectTypeVOLUME);
	}
	//else
	//{
	//	for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
	//		itg->second->ReleaseGpuResourcesBySrcID(obj_id);
	//}
	return vol_obj;
};

bool vzm::GenerateArrowObject(const float* pos_s, const float* pos_e, const float radius, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	PrimitiveData prim_data;
	vmdouble3 _pos_s = __cv3__ pos_s;
	vmdouble3 _pos_e = __cv3__ pos_e;
	vmgeom::GeneratePrimitive_Arrow(prim_data, _pos_s, _pos_e, 0.8, vmdouble2(radius * 0.8, radius * 1.2), 20);
	if (!prim_obj->RegisterPrimitiveData(prim_data))
	{
		prim_data.Delete();
		return fail_ret("INVALID DATA IN GenerateArrowObject!");
	}
	__update_picking_state(prim_obj, false);
	return true;
}

bool vzm::GenerateSpheresObject(const float* xyzr_list, const float* rgb_list, const int num_spheres, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	vmfloat4* _xyzr_list = (vmfloat4*)xyzr_list;
	vmfloat3* _rgb_list = (vmfloat3*)rgb_list;

	{
		uint uiNumSpheres = (uint)num_spheres;

		PrimitiveData stUnitSphereArchive;
		vmgeom::GeneratePrimitive_Sphere(stUnitSphereArchive, vmdouble3(), 1.0, 3);

		PrimitiveData prim_data;
		prim_data.is_ccw = stUnitSphereArchive.is_ccw;
		prim_data.is_stripe = stUnitSphereArchive.is_stripe;
		prim_data.ptype = stUnitSphereArchive.ptype;
		prim_data.idx_stride = stUnitSphereArchive.idx_stride;
		prim_data.num_vtx = stUnitSphereArchive.num_vtx * uiNumSpheres;
		prim_data.num_prims = stUnitSphereArchive.num_prims * uiNumSpheres;
		prim_data.num_vidx = stUnitSphereArchive.num_vidx * uiNumSpheres;

		vmfloat3* spheres_vtx_buf = new vmfloat3[prim_data.num_vtx];
		vmfloat3* spheres_nrl_buf = new vmfloat3[prim_data.num_vtx];
		uint* spheres_idx_buf = new uint[prim_data.num_vidx];
		vmfloat3* spheres_clr_buf = NULL;
		if (_rgb_list != NULL)
			spheres_clr_buf = new vmfloat3[prim_data.num_vtx];

		vmfloat3* unit_vtx_buf = stUnitSphereArchive.GetVerticeDefinition("POSITION");
		vmfloat3* unit_nrl_buf = stUnitSphereArchive.GetVerticeDefinition("NORMAL");

		for (uint i = 0; i < uiNumSpheres; i++)
		{
			vmmat44f matT, matTr, matS;
			vmfloat4 f4SphereShape = _xyzr_list[i];
			fMatrixScaling(&matS, &vmfloat3(f4SphereShape.w, f4SphereShape.w, f4SphereShape.w));
			fMatrixTranslation(&matTr, &vmfloat3(f4SphereShape.x, f4SphereShape.y, f4SphereShape.z));
			matT = matS * matTr;

			for (uint j = 0; j < stUnitSphereArchive.num_vtx; j++)
			{
				fTransformPoint(&spheres_vtx_buf[i * stUnitSphereArchive.num_vtx + j], &unit_vtx_buf[j], &matT);
				fTransformVector(&spheres_nrl_buf[i * stUnitSphereArchive.num_vtx + j], &unit_nrl_buf[j], &matT);
				if (spheres_clr_buf) spheres_clr_buf[i * stUnitSphereArchive.num_vtx + j] = _rgb_list[i];
			}

			for (uint j = 0; j < stUnitSphereArchive.num_vidx; j++)
			{
				spheres_idx_buf[i * stUnitSphereArchive.num_vidx + j] = stUnitSphereArchive.vidx_buffer[j] + stUnitSphereArchive.num_vtx * i;
			}
		}
		stUnitSphereArchive.Delete();

		prim_data.ReplaceOrAddVerticeDefinition("POSITION", spheres_vtx_buf);
		prim_data.ReplaceOrAddVerticeDefinition("NORMAL", spheres_nrl_buf);
		if (_rgb_list != NULL)
			prim_data.ReplaceOrAddVerticeDefinition("TEXCOORD0", spheres_clr_buf);

		prim_data.vidx_buffer = spheres_idx_buf;
		prim_data.ComputeOrthoBoundingBoxWithCurrentValues();
		if (!prim_obj->RegisterPrimitiveData(prim_data))
		{
			prim_data.Delete();
			return fail_ret("INVALID DATA IN GenerateSpheresObject!");
		}
		__update_picking_state(prim_obj, false);
	}
	return true;
}

bool vzm::GenerateCylindersObject(const float* xyz_01_list, const float* radius_list, const float* rgb_list, const int num_cylinders, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	vmfloat3* _xyz_01_list = (vmfloat3*)xyz_01_list;
	vmfloat3* _rgb_list = (vmfloat3*)rgb_list;

	{
		uint uiNumCylinders = (uint)num_cylinders;

		PrimitiveData stUnitCylinder;
		vmgeom::GeneratePrimitive_Cylinder(stUnitCylinder, vmdouble3(), vmdouble3(0, 0, 1.0), 1.0, false, false, 72, 1, 1);

		PrimitiveData prim_data;
		prim_data.is_ccw = stUnitCylinder.is_ccw;
		prim_data.is_stripe = stUnitCylinder.is_stripe;
		prim_data.ptype = stUnitCylinder.ptype;
		prim_data.idx_stride = stUnitCylinder.idx_stride;
		prim_data.num_vtx = stUnitCylinder.num_vtx * uiNumCylinders;
		prim_data.num_prims = stUnitCylinder.num_prims * uiNumCylinders;
		prim_data.num_vidx = stUnitCylinder.num_vidx * uiNumCylinders;

		vmfloat3* pos_buf = new vmfloat3[prim_data.num_vtx];
		vmfloat3* nrl_buf = new vmfloat3[prim_data.num_vtx];
		vmfloat3* clr_buf = NULL;
		if (_rgb_list != NULL)
			clr_buf = new vmfloat3[prim_data.num_vtx];

		prim_data.vidx_buffer = new uint[prim_data.num_vidx];

		vmfloat3* pos_buf_unit = stUnitCylinder.GetVerticeDefinition("POSITION");
		vmfloat3* nrl_buf_unit = stUnitCylinder.GetVerticeDefinition("NORMAL");
		AaBbMinMax* aabb_os_ptr = &prim_data.aabb_os;

		for (uint k = 0; k < uiNumCylinders; k++)
		{
			// 변환된 Cylinder 만들기
			vmdouble3 dPosStart(_xyz_01_list[2 * k + 0]), dPosEnd(_xyz_01_list[2 * k + 1]);
			double radius = (double)radius_list[k];
			vmdouble3 dVecS2E = dPosEnd - dPosStart;
			double dHeight = LengthVector(&dVecS2E);

			vmmat44 matScale, matLookAt, matRotTranslate, matTr;
			MatrixScaling(&matScale, &vmdouble3(radius, radius, dHeight));
			vmdouble3 dVecUp = vmdouble3(0, 0, 1.f), dVecCrossTest;
			CrossDotVector(&dVecCrossTest, &dVecUp, &dVecS2E);
			if (LengthVector(&dVecCrossTest) <= FLT_MIN)
				dVecUp = vmfloat3(0, 1.f, 0);
			MatrixWS2CS(&matLookAt, &dPosStart, &dVecUp, &(-dVecS2E));
			MatrixInverse(&matRotTranslate, &matLookAt);
			MatrixMultiply(&matTr, &matScale, &matRotTranslate);
			vmmat44f fmatTr = matTr;
			vmfloat3 clr_unit = _rgb_list ? _rgb_list[k] : vmfloat3();
			for (int i = 0; i < (int)stUnitCylinder.num_vtx; i++)
			{
				fTransformPoint(&pos_buf[k * stUnitCylinder.num_vtx + i], &pos_buf_unit[i], &fmatTr);
				fTransformVector(&nrl_buf[k * stUnitCylinder.num_vtx + i], &nrl_buf_unit[i], &fmatTr);
				if (clr_buf) clr_buf[k * stUnitCylinder.num_vtx + i] = clr_unit;
			}
			for (uint i = 0; i < stUnitCylinder.num_vidx; i++)
			{
				prim_data.vidx_buffer[k * stUnitCylinder.num_vidx + i] = stUnitCylinder.vidx_buffer[i] + stUnitCylinder.num_vtx * k;
			}
		}

		stUnitCylinder.Delete();

		prim_data.ReplaceOrAddVerticeDefinition("POSITION", pos_buf);
		prim_data.ReplaceOrAddVerticeDefinition("NORMAL", nrl_buf);
		if (_rgb_list != NULL)
			prim_data.ReplaceOrAddVerticeDefinition("TEXCOORD0", clr_buf);
		prim_data.ComputeOrthoBoundingBoxWithCurrentValues();
		if (!prim_obj->RegisterPrimitiveData(prim_data))
		{
			prim_data.Delete();
			return fail_ret("INVALID DATA IN GenerateCylindersObject!");
		}
		__update_picking_state(prim_obj, false);
	}
	return true;
}

bool vzm::GenerateLinesObject(const float* xyz_01_list, const float* rgb_01_list, const int num_lines, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	PrimitiveData line_data;
	line_data.is_ccw = true;
	line_data.is_stripe = false;
	line_data.ptype = PrimitiveTypeLINE;
	line_data.num_prims = num_lines;
	line_data.num_vtx = num_lines * 2;
	line_data.num_vidx = num_lines * 2;
	line_data.idx_stride = 2;
	vmfloat3* line_vtx_buf = new vmfloat3[line_data.num_vtx];
	vmfloat3* line_clr_buf = rgb_01_list? new vmfloat3[line_data.num_vtx] : NULL;
	line_data.ReplaceOrAddVerticeDefinition("POSITION", line_vtx_buf);
	if(line_clr_buf)
		line_data.ReplaceOrAddVerticeDefinition("TEXCOORD0", line_clr_buf);
	line_data.vidx_buffer = new uint[line_data.num_vidx];

	vmfloat3* __clr_list = (vmfloat3*)rgb_01_list;
	memcpy(line_vtx_buf, xyz_01_list, sizeof(vmfloat3) * num_lines * 2);
	for (int i = 0; i < num_lines; i++)
	{
		if (line_clr_buf)
		{
			line_clr_buf[i * 2 + 0] = __clr_list[i * 2 + 0];
			line_clr_buf[i * 2 + 1] = __clr_list[i * 2 + 1];
		}
		line_data.vidx_buffer[i * 2 + 0] = i * 2 + 0;
		line_data.vidx_buffer[i * 2 + 1] = i * 2 + 1;
	}

	line_data.ComputeOrthoBoundingBoxWithCurrentValues();
	if (!prim_obj->RegisterPrimitiveData(line_data))
	{
		line_data.Delete();
		return fail_ret("INVALID DATA IN GenerateLinesObject!");
	}
	__update_picking_state(prim_obj, false);

	return true;
}

auto __ComputeNormalVector = [](PrimitiveData* prim_data) -> bool
{
	if (prim_data->is_stripe || prim_data->ptype != vmenums::PrimitiveTypeTRIANGLE)
	{
		::MessageBox(NULL, L"ERROR! in __ComputeNormalVector!!", NULL, MB_OK);
		return fail_ret("NORMAL COMPUTATION ERROR!");
	}

	vmfloat3* tris_vtx_buf = prim_data->GetVerticeDefinition("POSITION");

	//vertex normal update
	vmfloat3* pf3VecNormals = new vmfloat3[prim_data->num_vtx];
	ZeroMemory(pf3VecNormals, sizeof(vmfloat3)*prim_data->num_vtx);
	for (uint i = 0; i < prim_data->num_prims; i++)
	{
		int iIndex0, iIndex1, iIndex2;
		iIndex0 = prim_data->vidx_buffer[3 * i + 0];
		iIndex1 = prim_data->vidx_buffer[3 * i + 1];
		iIndex2 = prim_data->vidx_buffer[3 * i + 2];

		vmfloat3 f3Pos0 = tris_vtx_buf[iIndex0];
		vmfloat3 f3Pos1 = tris_vtx_buf[iIndex1];
		vmfloat3 f3Pos2 = tris_vtx_buf[iIndex2];

		vmfloat3 f3Vec01 = f3Pos1 - f3Pos0;
		vmfloat3 f3Vec02 = f3Pos2 - f3Pos0;

		vmfloat3 f3VecTriangleNormal;
		fCrossDotVector(&f3VecTriangleNormal, &f3Vec01, &f3Vec02); // CCW 가정
		fNormalizeVector(&f3VecTriangleNormal, &f3VecTriangleNormal);

		pf3VecNormals[iIndex0] += f3VecTriangleNormal;
		pf3VecNormals[iIndex1] += f3VecTriangleNormal;
		pf3VecNormals[iIndex2] += f3VecTriangleNormal;
	}

	for (uint i = 0; i < prim_data->num_vtx; i++)
	{
		fNormalizeVector(&pf3VecNormals[i], &pf3VecNormals[i]);
	}
	prim_data->ReplaceOrAddVerticeDefinition("NORMAL", pf3VecNormals);

	return true;
};

bool vzm::GenerateTrianglesObject(const float* xyz_012_list, const float* rgb_012_list, const int num_tris, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	PrimitiveData tris_data;
	tris_data.is_ccw = false; // is_ccw; will be deprecated
	tris_data.is_stripe = false;
	tris_data.ptype = PrimitiveTypeTRIANGLE;
	tris_data.num_prims = num_tris;
	tris_data.num_vtx = num_tris * 3;
	tris_data.num_vidx = num_tris * 3;
	tris_data.idx_stride = 3;
	vmfloat3* tris_vtx_buf = new vmfloat3[tris_data.num_vtx];
	vmfloat3* tris_clr_buf = new vmfloat3[tris_data.num_vtx];
	tris_data.ReplaceOrAddVerticeDefinition("POSITION", tris_vtx_buf);
	tris_data.ReplaceOrAddVerticeDefinition("TEXCOORD0", tris_clr_buf);
	tris_data.vidx_buffer = new uint[tris_data.num_vidx];

	vmfloat3* __clr_list = (vmfloat3*)rgb_012_list;
	memcpy(tris_vtx_buf, xyz_012_list, sizeof(vmfloat3) * num_tris * 3);
	for (int i = 0; i < num_tris; i++)
	{
		vmfloat3 clr = __clr_list[i];
		tris_clr_buf[i * 3 + 0] = __clr_list[i * 3 + 0];
		tris_clr_buf[i * 3 + 1] = __clr_list[i * 3 + 1];
		tris_clr_buf[i * 3 + 2] = __clr_list[i * 3 + 2];
		tris_data.vidx_buffer[i * 3 + 0] = i * 3 + 0;
		tris_data.vidx_buffer[i * 3 + 1] = i * 3 + 1;
		tris_data.vidx_buffer[i * 3 + 2] = i * 3 + 2;
	}
	__ComputeNormalVector(&tris_data);
	tris_data.ComputeOrthoBoundingBoxWithCurrentValues();
	if (!prim_obj->RegisterPrimitiveData(tris_data))
	{
		tris_data.Delete();
		return fail_ret("INVALID DATA IN GenerateTrianglesObject!");
	}
	__update_picking_state(prim_obj, false);

	return true;
}

bool vzm::GeneratePrimitiveObject(const float* xyz_list, const float* nrl_list, const float* rgb_list, const float* tex_list, const int num_vtx, const unsigned int* idx_prims, const int num_prims, const int stride_prim_idx, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	if (xyz_list == NULL)
		return fail_ret("INVALID DATA IN GeneratePrimitiveObject-1!");

	PrimitiveData tris_data;
	tris_data.is_ccw = false; // is_ccw; will be deprecated
	tris_data.is_stripe = false;
	switch (stride_prim_idx)
	{
	case 1: tris_data.ptype = PrimitiveTypePOINT; break;
	case 2: tris_data.ptype = PrimitiveTypeLINE; break;
	case 3: tris_data.ptype = PrimitiveTypeTRIANGLE; break;
	default:
		return fail_ret("INVALID DATA IN GeneratePrimitiveObject-2!");
	}
	tris_data.num_prims = num_prims;
	tris_data.num_vtx = num_prims * stride_prim_idx;
	tris_data.num_vidx = num_prims * stride_prim_idx;
	tris_data.idx_stride = stride_prim_idx;
	vmfloat3* xyz_vtx_buf = new vmfloat3[tris_data.num_vtx];
	tris_data.ReplaceOrAddVerticeDefinition("POSITION", xyz_vtx_buf);
	memcpy(xyz_vtx_buf, xyz_list, sizeof(vmfloat3) * tris_data.num_vtx);

	if (nrl_list)
	{
		vmfloat3* nrl_vtx_buf = new vmfloat3[tris_data.num_vtx];
		tris_data.ReplaceOrAddVerticeDefinition("NORMAL", nrl_vtx_buf);
		memcpy(nrl_vtx_buf, nrl_list, sizeof(vmfloat3) * tris_data.num_vtx);
	}
	if (rgb_list)
	{
		vmfloat3* rgb_vtx_buf = new vmfloat3[tris_data.num_vtx];
		tris_data.ReplaceOrAddVerticeDefinition("TEXCOORD0", rgb_vtx_buf);
		memcpy(rgb_vtx_buf, rgb_list, sizeof(vmfloat3) * tris_data.num_vtx);
	}
	if (tex_list)
	{
		vmfloat3* tex_vtx_buf = new vmfloat3[tris_data.num_vtx];
		tris_data.ReplaceOrAddVerticeDefinition("TEXCOORD1", tex_vtx_buf);
		memcpy(tex_vtx_buf, tex_list, sizeof(vmfloat3) * tris_data.num_vtx);
	}

	if (stride_prim_idx > 1)
	{
		tris_data.vidx_buffer = new uint[tris_data.num_vidx];
		memcpy(tris_data.vidx_buffer, idx_prims, sizeof(uint) * tris_data.num_vidx);
	}
	tris_data.ComputeOrthoBoundingBoxWithCurrentValues();

	if (nrl_list == NULL && stride_prim_idx == 3)
		__ComputeNormalVector(&tris_data);

	if (!prim_obj->RegisterPrimitiveData(tris_data))
	{
		tris_data.Delete();
		return fail_ret("INVALID DATA IN GeneratePrimitiveObject!-3");
	}
	__update_picking_state(prim_obj, false);

	return true;
}

bool vzm::GeneratePointCloudObject(const float* xyz_list, const float* nrl_list, const float* rgb_list, const int num_points, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	vmfloat3* _xyz_list = (vmfloat3*)xyz_list;
	vmfloat3* _nrl_list = (vmfloat3*)nrl_list;
	vmfloat3* _rgb_list = (vmfloat3*)rgb_list;

	{
		uint uiNumPoints = (uint)num_points;

		PrimitiveData pc_data;
		pc_data.is_ccw = false;
		pc_data.is_stripe = false;
		pc_data.ptype = PrimitiveTypePOINT;
		pc_data.idx_stride = 1;
		pc_data.num_vtx = uiNumPoints;
		pc_data.num_prims = uiNumPoints;
		pc_data.num_vidx = 0;
		pc_data.vidx_buffer = NULL;

		if (_xyz_list)
		{
			vmfloat3* pbuf = new vmfloat3[pc_data.num_vtx];
			memcpy(pbuf, _xyz_list, sizeof(vmfloat3) * pc_data.num_vtx);
			pc_data.ReplaceOrAddVerticeDefinition("POSITION", pbuf);
		}
		if (nrl_list)
		{
			vmfloat3* pbuf = new vmfloat3[pc_data.num_vtx];
			memcpy(pbuf, nrl_list, sizeof(vmfloat3) * pc_data.num_vtx);
			pc_data.ReplaceOrAddVerticeDefinition("NORMAL", pbuf);
		}
		if (rgb_list)
		{
			vmfloat3* pbuf = new vmfloat3[pc_data.num_vtx];
			memcpy(pbuf, rgb_list, sizeof(vmfloat3) * pc_data.num_vtx);
			pc_data.ReplaceOrAddVerticeDefinition("TEXCOORD0", pbuf);
		}

		pc_data.ComputeOrthoBoundingBoxWithCurrentValues();
		if (!prim_obj->RegisterPrimitiveData(pc_data))
		{
			pc_data.Delete();
			return fail_ret("INVALID DATA IN GeneratePointCloudObject!");
		}
		__update_picking_state(prim_obj, false);
	}
	return true;
}

bool vzm::GenerateTextObject(const float* xyz_LT_view_up, const std::string& text, const float font_height, bool bold, bool italic, int& obj_id)
{
	VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

	VmFnContainer vfn;
	vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypePRIMITIVE, false), vector<VmObject*>(1, prim_obj)));

	std::string _font = "Arial";
	std::string _text = text;
	bool _bold = bold;
	bool _italic = italic;
	double dfont_height = font_height;
	double dist_from_plane = 0;

	vmdouble3 pos_LT = vmdouble3(xyz_LT_view_up[0], xyz_LT_view_up[1], xyz_LT_view_up[2]);
	vmdouble3 view = vmdouble3(xyz_LT_view_up[3], xyz_LT_view_up[4], xyz_LT_view_up[5]);
	vmdouble3 up = vmdouble3(xyz_LT_view_up[6], xyz_LT_view_up[7], xyz_LT_view_up[8]);

	std::string operation_type("MESH_GENERATOR");
	std::string generate_type("MAKE_ENGRAVING_TEXT");

	vfn.vmparams.insert(pair<string, void*>("_string_OperationType", &operation_type));
	vfn.vmparams.insert(pair<string, void*>("_string_GenerateType", &generate_type));

	vfn.vmparams.insert(pair<string, void*>("_string_TextFont", &_font));
	vfn.vmparams.insert(pair<string, void*>("_bool_IsBold", &_bold));
	vfn.vmparams.insert(pair<string, void*>("_bool_IsItalic", &_italic));
	vfn.vmparams.insert(pair<string, void*>("_string_TextContents", &_text));
	vfn.vmparams.insert(pair<string, void*>("_double_FontHeight", &dfont_height));
	vfn.vmparams.insert(pair<string, void*>("_double_DistanceFromHitPoint", &dist_from_plane));
	vfn.vmparams.insert(pair<string, void*>("_double3_PointPlaneLT", &pos_LT));
	vfn.vmparams.insert(pair<string, void*>("_double3_PlaneView", &view));
	vfn.vmparams.insert(pair<string, void*>("_double3_PlaneUp", &up));

	if (!module_arbitor->ExecuteModule(__module_CoreProcP, vfn))
	{
		if (g_is_display)
			std::cout << "Failed to execute the ptype_module" << std::endl;
		return fail_ret("INVALID DATA IN GenerateTextObject!");
	}
	__update_picking_state(prim_obj, false);

	return true;
}

bool vzm::GenerateMappingTable(const int table_size, const int num_alpha_ctrs, const float* ctr_alpha_idx_list, const int num_rgb_ctrs, const float* ctr_rgb_idx_list, int& tmap_id)
{
	bool is_new = false;
	VmTObject* t_obj = res_manager->GetTObject(tmap_id);
	if (t_obj == NULL)
	{
		is_new = true;
		t_obj = new VmTObject();
		tmap_id = res_manager->RegisterTObject(t_obj);
	}
	else
	{
		for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
			itg->second->ReleaseGpuResourcesBySrcID(tmap_id);
	}

	VmFnContainer vfn;
	SET_FUNC(vfn, ObjectTypeBUFFER, true, global_buf_obj);
	SET_FUNC(vfn, ObjectTypeTMAP, false, t_obj);
	int _table_size = table_size;
	vfn.vmparams.insert(pair<string, void*>("_int_TableSize", &_table_size));

	bool is_alpha_update = false;
	bool is_color_update = false;
	vfn.vmparams.insert(pair<string, void*>("_bool_IsAlphaUpdate", &is_alpha_update));
	vfn.vmparams.insert(pair<string, void*>("_bool_IsColorUpdate", &is_color_update));
	vector<double> alpha_list;
	vector<vmdouble4> rgb_list;
	if (num_alpha_ctrs > 0)
	{
		alpha_list.assign(num_alpha_ctrs * 2 + 1, 0);
		alpha_list[0] = num_alpha_ctrs;
		for (int i = 0; i < num_alpha_ctrs; i++)
		{
			alpha_list[1 + 2 * i + 0] = ctr_alpha_idx_list[2 * i + 1]; // index
			alpha_list[1 + 2 * i + 1] = ctr_alpha_idx_list[2 * i + 0]; // alpha
		}

		is_alpha_update = true; // ref
		global_buf_obj->ReplaceOrAddBufferPtr("_vlist_DOUBLE_Alpha_Polyline", &alpha_list[0], (int)alpha_list.size(), sizeof(double));
	}
	if (num_rgb_ctrs > 0)
	{
		rgb_list.assign(num_rgb_ctrs, vmdouble4(0));
		for (int i = 0; i < num_rgb_ctrs; i++)
		{
			rgb_list[i] = vmdouble4(ctr_rgb_idx_list[4 * i + 0], ctr_rgb_idx_list[4 * i + 1], ctr_rgb_idx_list[4 * i + 2], ctr_rgb_idx_list[4 * i + 3]);
		}

		is_color_update = true; // ref
		global_buf_obj->ReplaceOrAddBufferPtr("_vlist_DOUBLE4_OtfColor", &rgb_list[0], (int)rgb_list.size(), sizeof(vmdouble4));
	}

	if (!module_arbitor->ExecuteModule(__module_TMapGen, vfn))
	{
		global_buf_obj->ClearAllBuffers();
		global_buf_obj->ClearAllDstObjValues();
		if (g_is_display)
			std::cout << "Failed to load the TMapGen" << std::endl;
		if (is_new) res_manager->EraseVObject(tmap_id);
		return fail_ret("INVALID DATA IN GenerateMappingTable!");
	}
	global_buf_obj->ClearAllBuffers();
	global_buf_obj->ClearAllDstObjValues();


	return true;
}

bool vzm::GenerateCopiedObject(const int obj_src_id, int& obj_id)
{
	if (VmObject::GetObjectTypeFromID(obj_src_id) == ObjectTypePRIMITIVE)
	{
		VmVObjectPrimitive* prim_obj = __GetPObject(obj_id);

		VmVObjectPrimitive* primsrc_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_src_id);
		if (primsrc_obj == NULL)
			return false;

		if ((PrimitiveData*)primsrc_obj->GetPrimitiveData() == NULL)
			return false;

		if ((PrimitiveData*)prim_obj->GetPrimitiveData() != NULL)
			((PrimitiveData*)prim_obj->GetPrimitiveData())->Delete();

		PrimitiveData* primdata_src = (PrimitiveData*)primsrc_obj->GetPrimitiveData();
		PrimitiveData _primdata_src = *primdata_src;

		_primdata_src.ClearVertexDefinitionContainer();
		_primdata_src.vidx_buffer = NULL;

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
			vmfloat3* pf3Vertex = primdata_src->GetVerticeDefinition(vtypes[i]);
			if (pf3Vertex)
			{
				vmfloat3* pf3VertexNew = new vmfloat3[primdata_src->num_vtx];
				memcpy(pf3VertexNew, pf3Vertex, sizeof(vmfloat3) * primdata_src->num_vtx);
				_primdata_src.ReplaceOrAddVerticeDefinition(vtypes[i], pf3VertexNew);
			}
		}

		_primdata_src.vidx_buffer = new uint[primdata_src->num_vidx];
		memcpy(_primdata_src.vidx_buffer, primdata_src->vidx_buffer, sizeof(uint) * primdata_src->num_vidx);

		if (primdata_src->texture_res_info.size() > 0)
		{
			// Always Byte Type
			_primdata_src.texture_res_info = primdata_src->texture_res_info;
			for (auto it = primdata_src->texture_res_info.begin(); it != primdata_src->texture_res_info.end(); it++)
			{
				int w = get<0>(it->second);
				int h = get<1>(it->second);
				int bypp = get<2>(it->second);

				auto& dst = _primdata_src.texture_res_info[it->first];
				get<3>(dst) = new byte[w * h * bypp];
				memcpy(get<3>(dst), get<3>(it->second), w * h * bypp);
			}
		}

		prim_obj->RegisterPrimitiveData(_primdata_src);

		bool _visibility = true;
		primsrc_obj->GetCustomParameter("_bool_visibility", data_type::dtype<bool>(), &_visibility);
		vmdouble4 _color(1.);
		primsrc_obj->GetCustomParameter("_double4_color", data_type::dtype<vmdouble4>(), &_color);
		prim_obj->RegisterCustomParameter("_bool_visibility", _visibility);
		prim_obj->RegisterCustomParameter("_double4_color", _color);
		prim_obj->SetTransformMatrixOS2WS(primsrc_obj->GetMatrixOS2WS());
		bool visibleWireSrc;
		vmdouble4 wireframeColorSrc;
		primsrc_obj->GetPrimitiveWireframeVisibilityColor(&visibleWireSrc, &wireframeColorSrc);
		prim_obj->SetPrimitiveWireframeVisibilityColor(visibleWireSrc, wireframeColorSrc);

		prim_obj->RemoveCustomDataStructures();
		__update_picking_state(prim_obj, false);
	}
	else if (VmObject::GetObjectTypeFromID(obj_src_id) == ObjectTypeVOLUME)
	{
		VmVObjectVolume* vol_obj = __GetVObject(obj_id);

		VmVObjectVolume* volsrc_obj = (VmVObjectVolume*)res_manager->GetVObject(obj_src_id);
		if (volsrc_obj == NULL)
			return false;

		if ((VolumeData*)volsrc_obj->GetVolumeData() == NULL)
			return false;

		if ((VolumeData*)vol_obj->GetVolumeData() != NULL)
			((VolumeData*)vol_obj->GetVolumeData())->Delete();

		VolumeData* voldata_src = (VolumeData*)volsrc_obj->GetVolumeData();
		VolumeData _voldata_src = *voldata_src;

		_voldata_src.vol_slices = NULL;
		_voldata_src.histo_values = NULL;

		int iSizeOfSlice = (_voldata_src.vol_size.x + _voldata_src.bnd_size.x * 2)*(_voldata_src.vol_size.y + _voldata_src.bnd_size.y * 2);
		AllocateVoidPointer2D(&_voldata_src.vol_slices,
			_voldata_src.vol_size.z + _voldata_src.bnd_size.z * 2, iSizeOfSlice * _voldata_src.store_dtype.type_bytes);

		int iSizeOfType = _voldata_src.store_dtype.type_bytes;
		for (int i = 0; i < _voldata_src.vol_size.z + _voldata_src.bnd_size.z * 2; i++)
		{
			memcpy(_voldata_src.vol_slices[i], voldata_src->vol_slices[i], iSizeOfSlice*iSizeOfType);
		}

		uint uiRangeHistogram = _voldata_src.GetHistogramSize();
		_voldata_src.histo_values = new ullong[uiRangeHistogram];

		memcpy(_voldata_src.histo_values, voldata_src->histo_values, uiRangeHistogram * sizeof(ullong));

		if (volsrc_obj->GetVolumeBlock(0) == NULL)
		{
			vol_obj->RegisterVolumeData(_voldata_src, NULL, 0);
		}
		else
		{
			vmint3 i3BlockSizes[2] = { volsrc_obj->GetVolumeBlock(0)->blk_vol_size, volsrc_obj->GetVolumeBlock(1)->blk_vol_size };
			vol_obj->RegisterVolumeData(_voldata_src, i3BlockSizes, 0);
		}

		vmmat44 ori_mat_vs2os;
		volsrc_obj->GetCustomParameter("_matrix_originalOS2WS", data_type::dtype<vmmat44>(), &ori_mat_vs2os);
		vol_obj->RegisterCustomParameter("_matrix_originalOS2WS", ori_mat_vs2os);

		vol_obj->SetTransformMatrixOS2WS(volsrc_obj->GetMatrixOS2WS());
	}
	else 
		return false;

	return true;
}

bool vzm::ReplaceOrAddSceneObject(const int scene_id, const int obj_id, const ObjStates& obj_states)
{
	VmVObject* scene_obj = res_manager->GetVObject(obj_id);
	if (scene_obj == NULL)
		return fail_ret("NO OBJECT! id : " + to_string(obj_id));

	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
	{
		SceneInfo init_scene_info;
		init_scene_info.is_changed = true;
		scene_manager[scene_id] = init_scene_info;
		printf("new scene (#%d) is registered in ReplaceOrAddSceneObject\n", scene_id);
	}
	SceneInfo& curr_scene_info = scene_manager[scene_id];
	curr_scene_info.scene_objs[obj_id] = obj_states;
	curr_scene_info.is_changed = true;
	curr_scene_info.is_defined_ObjStates = true;

	// vobj params
	vmmat44f fmatT = __cm4__ obj_states.os2ws;
	vmmat44 matT = glm::transpose(fmatT);

	if (VmObject::GetObjectTypeFromID(obj_id) == ObjectTypePRIMITIVE)
		scene_obj->SetTransformMatrixOS2WS(matT);
	else if (VmObject::GetObjectTypeFromID(obj_id) == ObjectTypeVOLUME)
	{
		vmmat44 mat_vs2os;
		scene_obj->GetCustomParameter("_matrix_originalOS2WS", data_type::dtype<vmmat44>(), &mat_vs2os);
		scene_obj->SetTransformMatrixOS2WS(mat_vs2os * matT);
	}

	return true;
}

bool vzm::GetSceneObjectState(const int scene_id, const int obj_id, ObjStates& obj_states)
{
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return false; // fail_ret("NO SCENE! id : " + to_string(scene_id));

	auto it_obj = it->second.scene_objs.find(obj_id);
	if (it_obj == it->second.scene_objs.end())
		return false; //fail_ret("NO OBJECT! id : " + to_string(obj_id));

	obj_states = it_obj->second;
	return true;
}

bool vzm::RemoveSceneObject(const int scene_id, const int obj_id)
{
	VmVObject* scene_obj = res_manager->GetVObject(obj_id);
	if (scene_obj == NULL)
		return fail_ret("NO SCENE! id : " + to_string(scene_id));
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return fail_ret("NO OBJECT! id : " + to_string(obj_id));
	it->second.scene_objs.erase(obj_id);
	return true;
}

bool vzm::RemoveScene(const int scene_id)
{
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return fail_ret("NO SCENE! id : " + to_string(scene_id));
	it->second.Delete();
	scene_manager.erase(it);
	return true;
}

bool vzm::DeleteObject(const int obj_id)
{
	VmVObject* scene_obj = res_manager->GetVObject(obj_id);
	if (scene_obj == NULL)
		return fail_ret("NO OBJECT! id : " + to_string(obj_id));
	
	for (auto it = scene_manager.begin(); it != scene_manager.end(); it++)
		it->second.scene_objs.erase(obj_id);

	for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
		itg->second->ReleaseGpuResourcesBySrcID(obj_id);

	return res_manager->EraseObject(obj_id);
}

bool vzm::SetCameraParameters(const int scene_id, const CameraParameters& cam_params, const int cam_id)
{
	switch (cam_params.projection_mode)
	{
	case 1:
	case 4: 
		if (cam_params.ip_w <= 0 && cam_params.ip_h <= 0) return fail_ret("INVALID CAMERA PARAMETERS!");
		break;
	case 2:
		if (cam_params.fov_y <= 0 && cam_params.aspect_ratio <= 0) return fail_ret("INVALID CAMERA PARAMETERS!");
		break;
	case 3:
		if (cam_params.fx != cam_params.fx || cam_params.fy != cam_params.fy || cam_params.sc != cam_params.sc || cam_params.cx != cam_params.cx || cam_params.cy != cam_params.cy) return fail_ret("INVALID CAMERA PARAMETERS!");
		break;
	case 0:
	default:
		return fail_ret("INVALID PROJECTION MODE!!");
	}

	if (cam_params.w <= 0 || cam_params.h <= 0 || cam_params.fp - cam_params.np <= 0)
		return fail_ret("INVALID CAMERA PARAMETERS (whfn)!");

	vmfloat3 v = __cv3__ cam_params.view;
	vmfloat3 u = __cv3__ cam_params.up;
	v /= vmmath::fLengthVectorSq(&v);
	u /= vmmath::fLengthVectorSq(&u);

#define CHECK_NAN(_p) _p.x != _p.x || _p.y != _p.y || _p.z != _p.z

	if (CHECK_NAN(v) || CHECK_NAN(u))
		return fail_ret("INVALID CAMERA PARAMETERS (zero vectors)!");

	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
	{
		SceneInfo init_scene_info;
		init_scene_info.is_changed = true;
		scene_manager[scene_id] = init_scene_info;
		printf("new scene (#%d) is registered in SetCameraParameters\n", scene_id);
	}

	SceneInfo& curr_scene_info = scene_manager[scene_id];
	auto it_cam = curr_scene_info.cam_res.find(cam_id);
	if (it_cam == curr_scene_info.cam_res.end())
		printf("new camera (#%d) is registered in scene (#%d) \n", cam_id, scene_id);
	curr_scene_info.cam_res[cam_id].cam_params = cam_params;

	CamResources& cam_res = curr_scene_info.cam_res[cam_id];
	if (cam_res.scene_lbuf_obj == NULL)
	{
		cam_res.scene_lbuf_obj = new VmLObject();
		res_manager->RegisterLObject(cam_res.scene_lbuf_obj);
	}

	/////////////////////////////////////
	// Set Camera Matrix Parameters
	vector<VmObject*>& scene_fbuf_objs = cam_res.scene_fbuf_objs;

	if (scene_fbuf_objs.size() == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			VmIObject* iobj = new VmIObject(cam_params.w, cam_params.h);
			res_manager->RegisterIObject(iobj, 0);
			scene_fbuf_objs.push_back(iobj);
		}
	}
	const double _f = cam_params.fp, _n = cam_params.np;
	double fov_y, aspect_ratio, ip_w, ip_h;
	double fx, fy, sc, cx, cy;
	if (cam_params.projection_mode == 1 || cam_params.projection_mode == 4)
	{
		fov_y = 2. * atan(cam_params.ip_h * 0.5 / _n);
		aspect_ratio = cam_params.ip_w / cam_params.ip_h;
		ip_w = cam_params.ip_w;
		ip_h = cam_params.ip_h;
	}
	else if (cam_params.projection_mode == 2)
	{
		fov_y = cam_params.fov_y;
		aspect_ratio = cam_params.aspect_ratio;
		ip_h = 2. * _n * tan(fov_y * 0.5);
		ip_w = ip_h * aspect_ratio;
	}
	else if (cam_params.projection_mode == 3)
	{
		fx = cam_params.fx;
		fy = cam_params.fy;
		sc = cam_params.sc;
		cx = cam_params.cx;
		cy = cam_params.cy;
	}

	bool perspective_mode = cam_params.projection_mode != 4;
	for (int i = 0; i < (int)scene_fbuf_objs.size(); i++)
	{
		VmIObject* iobj = (VmIObject*)scene_fbuf_objs[i];
		VmCObject* cam_obj = iobj->GetCameraObject();
		if (cam_obj == NULL)
		{
			// Dummy Setting
			AaBbMinMax aabb_stage;
			aabb_stage.pos_min = vmdouble3();
			aabb_stage.pos_max = vmdouble3(1);
			iobj->AttachCamera(aabb_stage, StageViewORTHOBOXOVERVIEW);
			cam_obj = iobj->GetCameraObject();
		}

		vmfloat3 pos_cam, up_cam, view_cam;
		memcpy(&pos_cam, cam_params.pos, sizeof(vmfloat3));
		memcpy(&up_cam, cam_params.up, sizeof(vmfloat3));
		memcpy(&view_cam, cam_params.view, sizeof(vmfloat3));
		cam_obj->SetCameraExtStatef(&pos_cam, &view_cam, &up_cam);
		if (cam_params.projection_mode == 3)
			cam_obj->SetCameraIntStateAR(&fx, &fy, &sc, &cx, &cy, &_n, &_f, &vmint2(cam_params.w, cam_params.h));
		else
			cam_obj->SetCameraIntState(&vmdouble2(ip_w, ip_h), &_n, &_f, &vmint2(cam_params.w, cam_params.h), &vmdouble2(ip_w, ip_h));
		cam_obj->SetPerspectiveViewingState(perspective_mode, &fov_y);

		// invalid check
		if (i == 0)
		{
			vmmat44 m[3];
			cam_obj->GetMatrixWStoSS(&m[0], &m[1], &m[2]);
			double* v__ = (double*)m;
			for (int j = 0; j < 16 * 3; j++)
				if (v__[j] != v__[j] || v__[j] >= HUGE_VAL)
					return fail_ret("INVALID PROJECTION MATRIX!");
		}
	}

	curr_scene_info.is_changed = true;
	curr_scene_info.is_defined_CameraParameters = true;
	return true;
}

bool vzm::GetCameraParameters(const int scene_id, CameraParameters& cam_params, const int cam_id)
{
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return false; // fail_ret("NO SCENE! id : " + to_string(scene_id));

	SceneInfo& curr_scene_info = scene_manager[scene_id];
	auto it_cam = curr_scene_info.cam_res.find(cam_id);
	if (it_cam == curr_scene_info.cam_res.end())
		return false; // fail_ret("NO CAMERA! id : " + to_string(cam_id));
	cam_params = it_cam->second.cam_params;
	return true;
}

bool vzm::SetSceneEnvParameters(const int scene_id, const SceneEnvParameters& env_params)
{
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
	{
		SceneInfo init_scene_info;
		init_scene_info.is_changed = true;
		scene_manager[scene_id] = init_scene_info;
		printf("new scene (#%d) is registered in SetSceneEnvParameters\n", scene_id);
	}

	SceneInfo& curr_scene_info = scene_manager[scene_id];
	curr_scene_info.env_params = env_params;
	curr_scene_info.is_changed = true;
	curr_scene_info.is_defined_SceneEnvParameters = true;
	return true;
}

bool vzm::GetSceneEnvParameters(const int scene_id, SceneEnvParameters& env_params)
{
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return false;

	SceneInfo& curr_scene_info = it->second;
	env_params = curr_scene_info.env_params;
	return true;
}

bool vzm::GetCamProjMatrix(const int scene_id, const int cam_id, float* mat_ws2ss, float* mat_ss2ws, const bool is_col_major)
{
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return false; // fail_ret("NO SCENE! id : " + to_string(scene_id));

	SceneInfo& curr_scene_info = scene_manager[scene_id];
	auto it_cam = curr_scene_info.cam_res.find(cam_id);
	if (it_cam == curr_scene_info.cam_res.end())
		return false; // fail_ret("NO CAMERA! id : " + to_string(cam_id));

	vector<VmObject*>& scene_fbuf_objs = it_cam->second.scene_fbuf_objs;
	VmIObject* iobj = (VmIObject*)scene_fbuf_objs[0];
	VmCObject* cam_obj = iobj->GetCameraObject();

	if (mat_ws2ss)
	{
		vmmat44 ws2cs, cs2ps, ps2ss;
		cam_obj->GetMatrixWStoSS(&ws2cs, &cs2ps, &ps2ss);
		vmmat44 ws2ss = ws2cs * cs2ps * ps2ss;
		*(vmmat44f*)mat_ws2ss = is_col_major ? glm::transpose(ws2ss) : ws2ss;
	}
	if (mat_ss2ws)
	{
		vmmat44 ss2ps, ps2cs, cs2ws;
		cam_obj->GetMatrixSStoWS(&ss2ps, &ps2cs, &cs2ws);
		vmmat44 ss2ws = ss2ps * ps2cs * cs2ws;
		*(vmmat44f*)mat_ss2ws = is_col_major ? glm::transpose(ss2ws) : ss2ws;
	}
	return true;
}

bool vzm::RenderScene(const int scene_id, const int cam_id)
{
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return fail_ret("RenderScene :: NO SCENE! id : " + to_string(scene_id));

	SceneInfo& curr_scene_info = it->second;
	if (curr_scene_info.scene_objs.size() == 0)
		return fail_ret("RenderScene :: THERE IS NO OBJECT IN SCENE id : " + to_string(scene_id));

	auto it_cam = curr_scene_info.cam_res.find(cam_id);
	if (it_cam == curr_scene_info.cam_res.end())
		return fail_ret("RenderScene :: NO CAMERA! id : " + to_string(cam_id));

	CamResources& cam_res = it_cam->second;
	if (cam_res.scene_lbuf_obj == NULL || cam_res.scene_fbuf_objs.size() == 0)
		return fail_ret("RenderScene :: NO CAMERA RESOURCES!");
	CameraParameters& cam_params = cam_res.cam_params;
	if (!curr_scene_info.is_defined_CameraParameters) { printf("Please Set CameraParameters!!\n");  return false; }
	if (!curr_scene_info.is_defined_SceneEnvParameters) { printf("Please Set SceneEnvParameters!!\n"); return false; }
	if (!curr_scene_info.is_defined_ObjStates) { printf("Please Set ObjStates!!\n"); return false; }

	/////////////////////////////////////
	// Set Objects and their Parameters 
	/////////////////////////////////////
	bool is_sr = false;
	bool is_vr = false;
	vector<VmObject*> sr_prims;
	vector<VmObject*> sr_volumes; // to do
	vector<VmObject*> sr_tmaps; // to do
	vector<VmObject*> vr_volumes;
	vector<VmObject*> vr_tmaps;
	vector<int> main_volumes_vr;
	for (auto it_obj = curr_scene_info.scene_objs.begin(); it_obj != curr_scene_info.scene_objs.end(); it_obj++)
	{
		int obj_id = it_obj->first;
		VmVObject* vobj = res_manager->GetVObject(obj_id);
		if (vobj)
		{
			ObjStates& obj_states = it_obj->second;
			auto SetAssociateObjID = [&obj_id, &obj_states, &cam_res](const string& usage, const string& name, vector<VmObject*>& render_objs)
			{
				auto it = obj_states.associated_obj_ids.find(usage);
				if (it != obj_states.associated_obj_ids.end())
				{
					int associated_obj_id = it->second;
					cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, name, &associated_obj_id, sizeof(int));
					if (VmObject::GetObjectTypeFromID(associated_obj_id) == ObjectTypePRIMITIVE || VmObject::GetObjectTypeFromID(associated_obj_id) == ObjectTypeVOLUME)
					{
						VmVObject* vp_obj = res_manager->GetVObject(associated_obj_id);
						render_objs.push_back(vp_obj);
					}
					else if (VmObject::GetObjectTypeFromID(associated_obj_id) == ObjectTypeTMAP)
					{
						VmTObject* t_obj = res_manager->GetTObject(associated_obj_id);
						render_objs.push_back(t_obj);
					}
					else
					{
						printf("%d is not supported in vzm::RenderScene!!\n", obj_id);
					}
				}
			};
			if (VmObject::GetObjectTypeFromID(obj_id) == ObjectTypePRIMITIVE)
			{
				is_sr = true;

				sr_prims.push_back(vobj);

				vmfloat4 _wire_color = __cv4__ obj_states.wire_color;
				VmVObjectPrimitive* pobj = (VmVObjectPrimitive*)vobj;
				pobj->SetPrimitiveWireframeVisibilityColor(obj_states.is_wireframe, vmdouble4(_wire_color));
				double point_thickness = obj_states.point_thickness;
				double line_thickness = obj_states.line_thickness;
				cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_double_PointThickness", &point_thickness, sizeof(double));
				cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_double_LineThickness", &line_thickness, sizeof(double));
				cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_bool_UseVertexColor", &obj_states.use_vertex_color, sizeof(bool));
				cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_bool_UseVertexWireColor", &obj_states.use_vertex_wirecolor, sizeof(bool));

				//itr = obj_states.dojo_scripts.find("VzThickness");
				//if (itr != obj_states.dojo_scripts.end())
				//{
				//	double vz_thickness = itr->second;
				//	cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_double_VzThickness", &vz_thickness, sizeof(double));
				//}

				SetAssociateObjID("VOLUME_MAP", "_int_AssociatedVolumeID", sr_volumes);
				SetAssociateObjID("COLOR_MAP", "_int_AssociatedTObjectID", sr_tmaps);
			}
			else if (VmObject::GetObjectTypeFromID(obj_id) == ObjectTypeVOLUME)
			{
				is_vr = true;
				vr_volumes.push_back(vobj);
				main_volumes_vr.push_back(obj_id);
				if (cam_params.projection_mode == 4)
					SetAssociateObjID("MPR_WINDOWING", "_int_MainTObjectID", vr_tmaps);
				else
				{
					SetAssociateObjID("VR_OTF", "_int_MainTObjectID", vr_tmaps);
					SetAssociateObjID("MPR_WINDOWING", "_int_WindowingTObjectID", vr_tmaps);
				}
				SetAssociateObjID("VOLUME_MAP", "_int_AssociatedVolumeID", vr_volumes);
				SetAssociateObjID("COLOR_MAP", "_int_AssociatedTObjectID", vr_tmaps);
				//.. to do "_int_MaskVolumeObjectID" ...

				double sample_rate = obj_states.sample_rate;
				cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_double_SamplePrecisionLevel", &sample_rate, sizeof(double));
			}

			// common
			vmfloat4 _color = __cv4__ obj_states.color;
			vobj->RegisterCustomParameter("_bool_visibility", obj_states.is_visible);
			vobj->RegisterCustomParameter("_bool_TestVisible", obj_states.is_visible);
			vobj->RegisterCustomParameter("_double4_color", (vmdouble4)_color);
			cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_bool_visibility", &obj_states.is_visible, sizeof(bool));
			cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_bool_TestVisible", &obj_states.is_visible, sizeof(bool));
			cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_double4_color", &vmdouble4(_color), sizeof(vmdouble4));


			vmdouble4 shadingfactors(obj_states.emission, obj_states.diffusion, obj_states.specular, obj_states.sp_pow);
			cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_double4_ShadingFactors", &shadingfactors, sizeof(vmdouble4));
			cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, "_bool_ShowOutline", &obj_states.show_outline, sizeof(bool));
		}
		else
			printf("ERROR!\n");
	}
	if (main_volumes_vr.size() > 0) // to do
	{
		if (main_volumes_vr.size() > 1)
			printf("ONLY A SINGLE VOLUME (FIRST SET) IS AVAILABLE IN THIS VERSION!\n");
		cam_res.scene_lbuf_obj->ReplaceOrAddBufferPtr("_vlist_INT_MainVolumeIDs", &main_volumes_vr[0], 1, sizeof(int));
	}


	/////////////////////////////////////
	// Set Function Parameters
	/////////////////////////////////////
	VmFnContainer vfn_common;
	bool __false = false;
	bool __true = true;
	bool sr_is_first = true;
	bool sr_is_final = true;
	bool vr_is_first = true;
	bool vr_is_final = true;
	bool use_computeshader = true;
	int num_texture_layers = 4;
	string render_type_sr = "MESH";
	string render_type_vr = "VOLUME";
	vmdouble3 pos_light = *(vmfloat3*)curr_scene_info.env_params.pos_light;
	vmdouble3 dir_light = *(vmfloat3*)curr_scene_info.env_params.dir_light;
	bool is_on_camera = curr_scene_info.env_params.is_on_camera;
	bool is_pointlight = curr_scene_info.env_params.is_pointlight;
	bool is_rgba_write = cam_params.is_rgba_write;
	vfn_common.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeBUFFER, true), vector<VmObject*>(1, cam_res.scene_lbuf_obj)));
	vfn_common.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeIMAGEPLANE, false), cam_res.scene_fbuf_objs));
	vfn_common.vmparams.insert(pair<string, void*>("_bool_IsAvailableCS", &use_computeshader));
	vfn_common.vmparams.insert(pair<string, void*>("_int_NumTexureLayers", &num_texture_layers)); // legacy
	vfn_common.vmparams.insert(pair<string, void*>("_bool_IsLightOnCamera", &is_on_camera));
	vfn_common.vmparams.insert(pair<string, void*>("_bool_IsPointSpotLight", &is_pointlight));
	vfn_common.vmparams.insert(pair<string, void*>("_double3_PosLightWS", &pos_light));
	vfn_common.vmparams.insert(pair<string, void*>("_double3_VecLightWS", &dir_light));
	vfn_common.vmparams.insert(pair<string, void*>("_bool_IsRGBA", &is_rgba_write));

	bool is_on_ssao = curr_scene_info.env_params.effect_ssao.is_on_ssao;
	double kernel_r = curr_scene_info.env_params.effect_ssao.kernel_r;
	double ao_power = curr_scene_info.env_params.effect_ssao.ao_power;
	double tangent_bias = curr_scene_info.env_params.effect_ssao.tangent_bias;
	int num_dirs = curr_scene_info.env_params.effect_ssao.num_dirs;
	int num_steps = curr_scene_info.env_params.effect_ssao.num_steps;
	bool smooth_filter = curr_scene_info.env_params.effect_ssao.smooth_filter;
	vfn_common.vmparams.insert(pair<string, void*>("_bool_ApplySSAO", &is_on_ssao));
	vfn_common.vmparams.insert(pair<string, void*>("_double_SSAOKernalR", &kernel_r));
	vfn_common.vmparams.insert(pair<string, void*>("_int_SSAONumDirs", &num_dirs));
	vfn_common.vmparams.insert(pair<string, void*>("_int_SSAONumSteps", &num_steps));
	vfn_common.vmparams.insert(pair<string, void*>("_double_SSAOTangentBias", &tangent_bias));
	vfn_common.vmparams.insert(pair<string, void*>("_bool_BlurSSAO", &smooth_filter));

	// test for individual obj //
	// _int_RendererType
	// test for vfn_common //
	// _bool_TestOit
	// _bool_ReloadHLSLObjFiles
	// _double_VZThickness
	// _double_CopVZThickness
	// _bool_TestConsoleOut
	// _bool_IsLightOnCamera
	// _bool_IsPointSpotLight
	// _double3_PosLightWS
	std::map<int/*obj_id*/, std::map<std::string, std::tuple<size_t, byte*>>>& cam_container = _test_dojo_scripts[std::tuple<int, int>(scene_id, cam_id)];
	for (auto it_obj = cam_container.begin(); it_obj != cam_container.end(); it_obj++)
	{
		int obj_id = it_obj->first;
		for (auto it_obj_v = it_obj->second.begin(); it_obj_v != it_obj->second.end(); it_obj_v++)
		{
			const string& str = it_obj_v->first;
			const size_t size_bytes = std::get<0>(it_obj_v->second);
			byte* pv = std::get<1>(it_obj_v->second);
			if (obj_id > 0)
				cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, str, pv, size_bytes);
			else
				vfn_common.vmparams.insert(pair<string, void*>(str, pv));
		}
	}
	std::map<int/*obj_id*/, std::map<std::string, std::tuple<size_t, byte*>>>& gobal_param_container = _test_dojo_scripts[std::tuple<int, int>(-1, -1)];
	for (auto it_gp = gobal_param_container.begin(); it_gp != gobal_param_container.end(); it_gp++)
	{
		int obj_id = it_gp->first;
		for (auto it_obj_v = it_gp->second.begin(); it_obj_v != it_gp->second.end(); it_obj_v++)
		{
			const string& str = it_obj_v->first;
			size_t size_bytes = std::get<0>(it_obj_v->second);
			byte* pv = std::get<1>(it_obj_v->second);
			if(obj_id > 0)
				cam_res.scene_lbuf_obj->ReplaceOrAddDstObjValue(obj_id, str, pv, size_bytes);
			else
				vfn_common.vmparams.insert(pair<string, void*>(str, pv));
		}
	}
	
	vector<VmFnContainer> vfns;
	int module_ids[2] = { __module_rendererGpu, __module_rendererGpu };
	if (cam_params.projection_mode == 4)
	{
		render_type_sr = "SECTIONAL_MESH";
		render_type_vr = "SECTIONAL_VOLUME";
		module_ids[0] = module_ids[1] = __module_rendererCpu;
	}
	for (int i = 0; i < 2; i++)
	{
		VmFnContainer vfn = vfn_common;
		if (sr_prims.size() > 0 && i == 0)
		{
			vfn.vmparams.insert(pair<string, void*>("_string_RenderingSourceType", &render_type_sr));
			if (sr_prims.size() > 0)
				vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypePRIMITIVE, true), sr_prims));
			if (sr_volumes.size() > 0)
				vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeVOLUME, true), sr_volumes));
			if (sr_tmaps.size() > 0)
				vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeTMAP, true), sr_tmaps));

			sr_is_final = !is_vr;
			vfn.vmparams.insert(pair<string, void*>("_bool_IsFirstRenderer", &sr_is_first));
			vfn.vmparams.insert(pair<string, void*>("_bool_IsFinalRenderer", &sr_is_final));
		}

		if (vr_volumes.size() > 0 && i == 1)
		{
			vfn.vmparams.insert(pair<string, void*>("_string_RenderingSourceType", &render_type_vr));
			if (vr_volumes.size() > 0)
				vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeVOLUME, true), vr_volumes));
			if (vr_tmaps.size() > 0)
				vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeTMAP, true), vr_tmaps));

			vr_is_first = !is_sr;
			vfn.vmparams.insert(pair<string, void*>("_bool_IsFirstRenderer", &vr_is_first));
			vfn.vmparams.insert(pair<string, void*>("_bool_IsFinalRenderer", &vr_is_final));
		}
		vfns.push_back(vfn);
	}

	//int level_sr = 0, level_vr = 0;
	//bool is_forced_per_rt = true;
	//vfn.vmparams.insert(pair<string, void*>("_int_LevelSR", &level_sr));
	//vfn.vmparams.insert(pair<string, void*>("_int_LevelVR", &level_vr));
	//vfn.vmparams.insert(pair<string, void*>("_bool_IsForcedPerRTarget", &is_forced_per_rt));

	for (int i = 0; i < 2; i++)
	{
		if (vfns[i].vmparams.find("_string_RenderingSourceType") == vfns[i].vmparams.end()) continue;
		//if (vfns[i].vmobjs.size() == 0) continue;
		if (!module_arbitor->ExecuteModule(module_ids[i], vfns[i]))
			return fail_ret("RenderScene :: Rendering Failure!");
	}

	cam_res.recent_rendered_time = vmhelpers::GetCurrentTimePack();
	renderer_excutable_count++;

	return true;
}

bool vzm::GetRenderBufferPtrs(const int scene_id, unsigned char** ptr_rgba, float** ptr_zdepth, int* fbuf_w, int* fbuf_h, int cam_id)
{
	auto false_ret = [&]()
	{
		if (ptr_rgba) *ptr_rgba = NULL;
		if (ptr_zdepth) *ptr_zdepth = NULL;
		if (fbuf_w) *fbuf_w = 0;
		if (fbuf_h) *fbuf_h = 0;
		return false;
	};

	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return false_ret();// fail_ret("NO SCENE");

	SceneInfo& curr_scene_info = it->second;
	auto it_cam = curr_scene_info.cam_res.find(cam_id);
	if (it_cam == curr_scene_info.cam_res.end())
		return false_ret();// fail_ret("NO CAM");

	if (it_cam->second.scene_fbuf_objs.size() == 0)
		return false_ret();// fail_ret("NO bUF");

	VmIObject* iobj = (VmIObject*)it_cam->second.scene_fbuf_objs[0];
	FrameBuffer* fbuf_rgba = iobj->GetFrameBuffer(FrameBufferUsageRENDEROUT, 0);
	if (fbuf_rgba == NULL)
		return false_ret();
	FrameBuffer* fbuf_depth = iobj->GetFrameBuffer(FrameBufferUsageDEPTH, 0);
	if (ptr_rgba) *ptr_rgba = (unsigned char*)fbuf_rgba->fbuffer;
	if (ptr_zdepth) *ptr_zdepth = fbuf_depth ? (float*)fbuf_depth->fbuffer : NULL;
	if (fbuf_w) *fbuf_w = fbuf_rgba->w;
	if (fbuf_h) *fbuf_h = fbuf_rgba->h;

	return true;
}

bool vzm::GetPModelData(const int obj_id, float** pos_vtx, float** nrl_vtx, float** rgb_vtx, float** tex_vtx, int& num_vtx, unsigned int** idx_prims, int& num_prims, int& stride_prim_idx)
{
	VmVObject* pobj = res_manager->GetVObject(obj_id);
	if (pobj == NULL)
		return fail_ret("NO OBJECT! id : " + to_string(obj_id));
	if (pobj->GetObjectType() != vmenums::EvmObjectType::ObjectTypePRIMITIVE)
		return fail_ret("NOT PMODEL OBJECT! id : " + to_string(obj_id));

	PrimitiveData* prim_data = ((VmVObjectPrimitive*)pobj)->GetPrimitiveData();
	num_vtx = prim_data->num_vtx;
	num_prims = prim_data->num_prims;
	if (pos_vtx && prim_data->GetVerticeDefinition("POSITION"))
	{
		*pos_vtx = new float[num_vtx * 3];
		memcpy(*pos_vtx, prim_data->GetVerticeDefinition("POSITION"), sizeof(float) * num_vtx * 3);
	}
	if (nrl_vtx && prim_data->GetVerticeDefinition("NORMAL"))
	{
		*nrl_vtx = new float[num_vtx * 3];
		memcpy(*nrl_vtx, prim_data->GetVerticeDefinition("NORMAL"), sizeof(float) * num_vtx * 3);
	}
	if (rgb_vtx && prim_data->GetVerticeDefinition("TEXCOORD0"))
	{
		*rgb_vtx = new float[num_vtx * 3];
		memcpy(*rgb_vtx, prim_data->GetVerticeDefinition("TEXCOORD0"), sizeof(float) * num_vtx * 3);
	}
	if (tex_vtx && prim_data->GetVerticeDefinition("TEXCOORD1"))
	{
		*tex_vtx = new float[num_vtx * 3];
		memcpy(*tex_vtx, prim_data->GetVerticeDefinition("TEXCOORD1"), sizeof(float) * num_vtx * 3);
	}

	//*idx_prims = NULL;
	stride_prim_idx = 1;
	if (prim_data->idx_stride == 1)
		return true;

	if (idx_prims && prim_data->vidx_buffer)
	{
		*idx_prims = new uint[prim_data->num_vidx];
		memcpy(*idx_prims, prim_data->vidx_buffer, sizeof(uint) * prim_data->num_vidx);
		stride_prim_idx = prim_data->idx_stride;
	}
	return true;
}

bool vzm::ValidatePickTarget(const int obj_id)
{
	if (VmObject::GetObjectTypeFromID(obj_id) != ObjectTypePRIMITIVE)
		return fail_ret("Only primitive model is allowed : " + to_string(obj_id));
	VmVObjectPrimitive* prim_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_id);
	if (prim_obj == NULL)
		return fail_ret("NO OBJECT! id : " + to_string(obj_id));

	static bool is_initialized = false;
	if (!is_initialized)
	{
		typedef bool(*LPDLL_CALL_dxInitialize)();
		LPDLL_CALL_dxInitialize lpdll_function = LoadDLL<LPDLL_CALL_dxInitialize>("vismtv_dxutwrapper", "dxInitialize");
		if (lpdll_function != NULL) is_initialized = lpdll_function();
	}

	return __update_picking_state(prim_obj, true);
}

bool vzm::PickObject(int& pick_obj_id, float* pos_pick, const int x, const int y, const int scene_id, const int cam_id)
{
	pick_obj_id = 0;
	auto it = scene_manager.find(scene_id);
	if (it == scene_manager.end())
		return fail_ret("NO SCENE! id : " + to_string(scene_id));

	SceneInfo& curr_scene_info = scene_manager[scene_id];
	auto it_cam = curr_scene_info.cam_res.find(cam_id);
	if (it_cam == curr_scene_info.cam_res.end())
		return fail_ret("NO CAMERA! id : " + to_string(cam_id));

	typedef bool(*LPDLL_CALL_dxPick)(uint* puiHitCount/*out*/, uint* puiPolygonID/*out*/,
		float* pfBaryU/*out*/, float* pfBaryV/*out*/, float* pfDist/*out*/,
		VmVObjectPrimitive* pCVObjMesh, VmIObject* pCIObject, vmfloat2 f2PosPickSS);
	LPDLL_CALL_dxPick lpdll_function = LoadDLL<LPDLL_CALL_dxPick>("vismtv_dxutwrapper", "dxPick");
	if (lpdll_function == NULL)
		return fail_ret("MODULE LOAD FAILIRE IN PickObject!");

	VmIObject* iobj = (VmIObject*)it_cam->second.scene_fbuf_objs[0];

	std::map<float, VmVObjectPrimitive*> depth_ordered_objs;
	for (auto it = curr_scene_info.scene_objs.begin(); it != curr_scene_info.scene_objs.end(); it++)
	{
		int obj_id = it->first;
		if (VmObject::GetObjectTypeFromID(obj_id) != EvmObjectType::ObjectTypePRIMITIVE) continue;

		VmVObjectPrimitive* prim_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_id);
		uint hitCount;
		uint polygonID;
		float baryU;
		float baryV;
		float depth;
		if (lpdll_function(&hitCount, &polygonID, &baryU, &baryV, &depth, prim_obj, iobj, vmfloat2(x, y)))
		{
			if (hitCount > 0)
				depth_ordered_objs[depth] = prim_obj;
		}
	}
	
	if (depth_ordered_objs.size() > 0)
	{
		auto doo = depth_ordered_objs.begin();
		pick_obj_id = doo->second->GetObjectID();
		if (pos_pick)
		{
			VmCObject* cobj = iobj->GetCameraObject();

			vmmat44 matSS2PS, matPS2CS, matCS2WS;
			cobj->GetMatrixSStoWS(&matSS2PS, &matPS2CS, &matCS2WS);
			vmmat44 matWS2OS = doo->second->GetMatrixWS2OS();
			vmmat44 matSS2OS = matSS2PS * matPS2CS * matCS2WS * matWS2OS;

			vmmat44f fmatWS2OS = matWS2OS;
			vmmat44f fmatSS2OS = matSS2OS;

			vmdouble3 d3PosCameraOriginWS;
			cobj->GetCameraExtState(&d3PosCameraOriginWS, NULL, NULL);
			vmfloat3 f3PosCameraOriginWS(d3PosCameraOriginWS), f3PosCameraOriginOS;

			vmfloat3 f3PosPickSS = vmfloat3(x, y, 0);
			vmfloat3 f3PosPickOS, f3VecRayDirOS;
			fTransformPoint(&f3PosPickOS, &f3PosPickSS, &fmatSS2OS);

			if (cobj->IsPerspective())
			{
				fTransformPoint(&f3PosCameraOriginOS, &f3PosCameraOriginWS, &fmatWS2OS);

				f3VecRayDirOS = f3PosPickOS - f3PosCameraOriginOS;
			}
			else
			{
				vmdouble3 d3VecViewWS;
				cobj->GetCameraExtState(NULL, &d3VecViewWS, NULL);

				f3PosCameraOriginOS = f3PosPickOS;
				fTransformVector(&f3VecRayDirOS, &(vmfloat3)d3VecViewWS, &fmatWS2OS);
			}

			fNormalizeVector(&f3VecRayDirOS, &f3VecRayDirOS);

			vmfloat3 pos_hit_os = f3PosCameraOriginOS + f3VecRayDirOS * doo->first;
			vmfloat3 pos_hit_ws;
			fTransformPoint(&pos_hit_ws, &pos_hit_os, &doo->second->GetMatrixOS2WSf());

			//TESTOUT("os : ", pos_hit_os);
			//TESTOUT("ws : ", pos_hit_ws);
			*(vmfloat3*)pos_pick = pos_hit_ws;
		}
		return true;
	}

	return false;
}

bool vzm::Pick1stHitSurfaceUsingDepthMap(float* pos_pick, const int x, const int y, const float valid_depth_range, const int scene_id, const int cam_id)
{
	unsigned char* ptr_rgba;
	float* ptr_zdepth;
	int w, h;
	if (!GetRenderBufferPtrs(scene_id, &ptr_rgba, &ptr_zdepth, &w, &h, cam_id)) 
		return fail_ret("Pick1stHitSurfaceUsingDepthMap :: NO buffers! (scene_id, cam_id): " + to_string(scene_id) + ", " + to_string(cam_id));

	float depth = ptr_zdepth[x + y * w];
	if (depth >= valid_depth_range) 
		return fail_ret("Pick1stHitSurfaceUsingDepthMap :: z depth is out of range : " + to_string(depth));
	// note that 3D view is always perspective
	CameraParameters camstate;
	GetCameraParameters(scene_id, camstate, cam_id);
	vmfloat3 pos_cam = __cv3__ camstate.pos;

	vmmat44f mat_ws2ss, mat_ss2ws;
	GetCamProjMatrix(scene_id, cam_id, __FP mat_ws2ss, __FP mat_ss2ws, false);

	vmfloat3 pick_pos_ss = glm::fvec3(x, y, 0);
	vmfloat3 pick_pos_ws, pick_dir_ws;
	fTransformPoint(&pick_pos_ws, &pick_pos_ss, &mat_ss2ws);
	pick_dir_ws = pick_pos_ws - pos_cam;
	fNormalizeVector(&pick_dir_ws, &pick_dir_ws);

	vmfloat3 pos_hit = pick_pos_ws + pick_dir_ws * depth;
	__cv3__ pos_pick = pos_hit;
	return true;
}

bool ReplaceOrAddArrowObject(const int scene_id, int& obj_id, const vzm::ObjStates& obj_states, const float* pos_s, const float* pos_e, const float radius)
{
	if (!vzm::GenerateArrowObject(pos_s, pos_e, radius, obj_id)) return false;
	return vzm::ReplaceOrAddSceneObject(scene_id, obj_id, obj_states);
}

bool ReplaceOrAddSpheresObject(const int scene_id, int& obj_id, const vzm::ObjStates& obj_states, const float* xyzr_list, const float* rgb_list, const int num_spheres)
{
	if (!vzm::GenerateSpheresObject(xyzr_list, rgb_list, num_spheres, obj_id)) return false;
	return vzm::ReplaceOrAddSceneObject(scene_id, obj_id, obj_states);
}

//#include "../../VmNativeModules/vismtv_modeling_vera/InteropHeader.h"
typedef struct __float3__
{
	float x, y, z;
	__float3__() { x = y = z = 0; };
	__float3__(float _x, float _y, float _z) { x = _x; y = _y; z = _z; };
	__float3__(int _color) {
		x = (float)((_color >> 16) & 0xFF) / 255.f;
		y = (float)((_color >> 8) & 0xFF) / 255.f;
		z = (float)((_color >> 0) & 0xFF) / 255.f;
	};
}  __float3;

#define __cv3__ *(glm::fvec3*)
#define __cv4__ *(glm::fvec4*)
#define __cm4__ *(glm::fmat4x4*)
#define __cm4d__ *(glm::dmat4x4*)

//bool helpers::ComputeRigidTransform(const float* xyz_from_list, const float* xyz_to_list, const int num_pts, float(&mat_tr)[16])
bool helpers::ComputeRigidTransform(const float* xyz_from_list, const float* xyz_to_list, const int num_pts, float* mat_tr /*float16*/)
{
	typedef bool(*LPDLL_CALL_PAIRMATCHING)(double(&mat)[16],
		const std::vector<std::pair<int, int>>& match_pairs,
		const std::vector<__float3>& pos_ref_pts,
		const std::vector<__float3>& pos_float_pts);

	LPDLL_CALL_PAIRMATCHING lpdll_function = LoadDLL<LPDLL_CALL_PAIRMATCHING>("vismtv_modeling_vera", "compute_pair_matching_transform");
	if (lpdll_function == NULL)
		return fail_ret("MODULE LOAD FAILIRE IN ComputeRigidTransform!");

	vector<__float3> vtr_xyz_from(num_pts);
	vector<__float3> vtr_xyz_to(num_pts);
	memcpy(&vtr_xyz_from[0], xyz_from_list, sizeof(__float3) * num_pts);
	memcpy(&vtr_xyz_to[0], xyz_to_list, sizeof(__float3) * num_pts);
	vector<pair<int, int>> match_pairs(num_pts);
	for (int i = 0; i < num_pts; i++)
		match_pairs[i] = pair<int, int>(i, i);

	double mat_tr_v[16];
	lpdll_function(mat_tr_v, match_pairs, vtr_xyz_to, vtr_xyz_from);

	glm::fmat4x4 fmat_tr = __cm4d__ mat_tr_v; // auto convert
	fmat_tr = glm::transpose(fmat_tr);

	memcpy(mat_tr, &fmat_tr, sizeof(float) * 16);

	return true;
}

//void helpers::ComputeCameraProjMatrix(const float fx, const float fy, const float ppx, const float ppy, const float* rm, const float* tv, float(&mat_p)[16])
//{
//	typedef bool(*LPDLL_CALL_CAMPROJ)(void* mat_p, const double fx, const double fy, const double ppx, const double ppy,
//		const void* rm, const void* tv);
//
//	LPDLL_CALL_CAMPROJ lpdll_function = LoadDLL<LPDLL_CALL_CAMPROJ>("vismtv_geometryanalyzer", "compute_camera_matrix_p");
//	if (lpdll_function == NULL)
//		return;
//
//	glm::dmat4x4 dmat_p;
//	glm::dmat3x3 dmat_r;
//	double* v_mat_r = glm::value_ptr(dmat_r);
//	for (int i = 0; i < 9; i++) v_mat_r[i] = (double)rm[i];
//	glm::dvec3 dtv(tv[0], tv[1], tv[2]);
//	lpdll_function(&dmat_p, fx, fy, ppx, ppy, &dmat_r, &dtv);
//
//	double* v_mat_p = glm::value_ptr(dmat_p);
//	for (int i = 0; i < 16; i++) mat_p[i] = (float)v_mat_p[i];
//}

bool vzmproc::SimplifyPModelByUGrid(const int obj_src_id, const float cell_width, int& obj_dst_id)
{
	VmVObjectPrimitive* prim_src_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_src_id);
	if (prim_src_obj == NULL)
		return fail_ret("NO OBJECT! id : " + to_string(obj_src_id));

	VmVObjectPrimitive* prim_dst_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_dst_id);
	bool is_new = prim_dst_obj == NULL;
	int tmp_obj_id = obj_dst_id;
	if (is_new)
	{
		prim_dst_obj = new VmVObjectPrimitive();
		tmp_obj_id = res_manager->RegisterVObject(prim_dst_obj, ObjectTypePRIMITIVE);
	}

	VmFnContainer vfn;
	vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypePRIMITIVE, true), vector<VmObject*>(1, prim_src_obj)));
	vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypePRIMITIVE, false), vector<VmObject*>(1, prim_dst_obj)));
	vfn.vmobjs.insert(pair<VmObjKey, vector<VmObject*>>(VmObjKey(ObjectTypeBUFFER, true), vector<VmObject*>(1, global_buf_obj)));

	string operation_type = "MODELING";
	string function_name = "CGIP_LAUNCHER";
	string modeling_func = "FILTER_POINTS";
	string filter_mode = "SIMPLIFY_GRID";
	double cellsize = (double)cell_width;
	vfn.vmparams.insert(pair<string, void*>("_string_OperationType", &operation_type));
	vfn.vmparams.insert(pair<string, void*>("_string_FunctionName", &function_name));
	vfn.vmparams.insert(pair<string, void*>("_string_ModelingFunction", &modeling_func));
	vfn.vmparams.insert(pair<string, void*>("_string_FilterMode", &filter_mode));
	vfn.vmparams.insert(pair<string, void*>("_double_Cellsize_SIMPLIFY_GRID", &cellsize));

	if (!module_arbitor->ExecuteModule(__module_CoreProcP, vfn))
	{
		global_buf_obj->ClearAllBuffers();
		global_buf_obj->ClearAllDstObjValues();
		if (is_new) res_manager->EraseVObject(tmp_obj_id);
		return fail_ret("NOFailed to execute the ptype_module in SimplifyPModelByUGrid");
	}
	global_buf_obj->ClearAllBuffers();
	global_buf_obj->ClearAllDstObjValues();
	obj_dst_id = tmp_obj_id;

	PrimitiveData* prim_src_data = prim_src_obj->GetPrimitiveData();
	PrimitiveData* prim_dst_data = prim_dst_obj->GetPrimitiveData();
	if (g_is_display)
		cout << "vertex number : " << prim_src_data->num_vtx << " to " << prim_dst_data->num_vtx << endl;

	//if (!is_new)
	//{
	//	for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
	//		itg->second->ReleaseGpuResourcesBySrcID(obj_dst_id);
	//}

	return true;
}

bool vzmproc::GenerateSamplePoints(const int obj_src_id, const float* pos_src, const float r, const float min_interval, int& obj_dst_id)
{
	VmVObjectPrimitive* prim_src_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_src_id);
	if (prim_src_obj == NULL)
		return fail_ret("NO OBJECT! id : " + to_string(obj_src_id));

	VmVObjectPrimitive* prim_dst_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_dst_id);
	bool is_new = prim_dst_obj == NULL;
	if (is_new)
	{
		prim_dst_obj = new VmVObjectPrimitive();
		obj_dst_id = res_manager->RegisterVObject(prim_dst_obj, ObjectTypePRIMITIVE);
	}

	typedef bool(*LPDLL_CALL_SAMPLEPTS)(VmVObjectPrimitive* pobj_dst, const vmfloat3& pos3d, const float r, const float cell_width, VmVObjectPrimitive* pobj_src);

	LPDLL_CALL_SAMPLEPTS lpdll_function = LoadDLL<LPDLL_CALL_SAMPLEPTS>("vismtv_modeling_vera", "get_pts_near_3dpos");
	if (lpdll_function == NULL)
		return fail_ret("MODULE LOAD FAILIRE IN GenerateSamplePoints!");

	vmfloat3 pos_src_3d = *(vmfloat3*)pos_src;
	if (lpdll_function(prim_dst_obj, pos_src_3d, r, min_interval, prim_src_obj))
	{
		if (g_is_display)
			cout << "# of sample points : " << prim_dst_obj->GetPrimitiveData()->num_vtx << endl;

		//for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
		//	itg->second->ReleaseGpuResourcesBySrcID(obj_dst_id);

		return true;
	}
	return false;
}

bool vzmproc::ComputeMatchingTransform(const int obj_from_id, const int obj_to_id, float* mat_tr /*float16*/)
{
	VmVObjectPrimitive* prim_from_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_from_id);
	VmVObjectPrimitive* prim_to_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_to_id);
	if (prim_from_obj == NULL || prim_to_obj == NULL)
		return fail_ret("INVALID OBJECT id");

	typedef bool(*LPDLL_CALL_MATCHING)(VmVObjectPrimitive* pobj_from, VmVObjectPrimitive* pobj_to, vmmat44f& mat_tr);

	LPDLL_CALL_MATCHING lpdll_function = LoadDLL<LPDLL_CALL_MATCHING>("vismtv_modeling_vera", "compute_icp");
	if (lpdll_function == NULL)
		return fail_ret("MODULE LOAD FAILIRE IN GenerateSamplePoints!");

	vmmat44f fmat_tr;
	if (lpdll_function(prim_from_obj, prim_to_obj, fmat_tr))
	{
		//if (g_is_display)
		//	cout << "complete matching!!" << prim_dst_obj->GetPrimitiveData()->num_vtx << endl;

		//for (auto itg = gpu_manager.begin(); itg != gpu_manager.end(); itg++)
		//	itg->second->ReleaseGpuResourcesBySrcID(obj_dst_id);

		fmat_tr = glm::transpose(glm::fmat4x4(fmat_tr));
		*(glm::fmat4x4*) mat_tr = fmat_tr;
		return true;
	}
	return false;
}

//__dojostatic bool ComputePCA(const int obj_id, float(&egvals)[3], float(&egvecs)[9]);
bool vzmproc::ComputePCA(const int obj_id, float* egvals /*float3*/, float* egvecs /*three of float3*/)
{
	VmVObjectPrimitive* prim_src_obj = (VmVObjectPrimitive*)res_manager->GetVObject(obj_id);
	if (prim_src_obj == NULL)
		return fail_ret("NO OBJECT! id : " + to_string(obj_id));

	PrimitiveData* pdata = prim_src_obj->GetPrimitiveData();
	int num_pts = pdata->num_vtx;

	float* xyz_list = (float*)pdata->GetVerticeDefinition("POSITION");
	//if (!GetPModelData(obj_id, &xyz_list, NULL, NULL, NULL, num_pts, NULL, num_idx)) return false;

	bool ret = helpers::ComputePCAc(xyz_list, num_pts, egvals, egvecs);
	//VMSAFE_DELETEARRAY(xyz_list);
	return ret;
}

//bool helpers::ComputePCAc(const float* xyz_list, const int num_points, float(&egvals)[3], float(&egvecs)[9])
bool helpers::ComputePCAc(const float* xyz_list, const int num_points, float* egvals /*float3*/, float* egvecs /*three of float3*/)
{
	typedef bool(*LPDLL_CALL_PCA)(float(&mat)[16],
		std::vector<float>& eigen_values, std::vector<__float3>& eigen_vectors,
		const std::vector<__float3>& pos_pts);

	LPDLL_CALL_PCA lpdll_function = LoadDLL<LPDLL_CALL_PCA>("vismtv_modeling_vera", "compute_eigen_vectors");
	if (lpdll_function == NULL)
		return fail_ret("MODULE LOAD FAILIRE IN ComputePCAc!");

	std::vector<__float3> pos_pts(num_points);
	memcpy(&pos_pts[0], xyz_list, sizeof(float) * num_points);

	float mat_t[16];
	std::vector<float> eigen_values;
	std::vector<__float3> eigen_vectors;
	
	bool ret = lpdll_function(mat_t, eigen_values, eigen_vectors, pos_pts);
	float sum_evs = eigen_values[0] + eigen_values[1] + eigen_values[2];
	for (int i = 0; i < 3; i++)
	{
		egvals[i] = eigen_values[i] / sum_evs;
		*(__float3*)&egvecs[3 * i] = eigen_vectors[i];
	}
	return ret;
}

bool helpers::ComputeCameraRendererMatrice(const float* mat_ext, 
	const float fx, const float fy, const float sc, const float cx, const float cy,
	const int w, const int h, const float zn, const float zf, const int api_mode,
	float* mat_ws2cs, float* mat_cs2ps, float* mat_ps2ss)
{
	typedef bool(*LPDLL_CALL_CAMPROJ)(float* mat_cs2ps, float* mat_ps2ss, const float* mat_int3x3,
		const float x0, const float y0, const float width, const float height, const float znear, const float zfar,
		const bool ignore_shear, const int api_mode);

	LPDLL_CALL_CAMPROJ lpdll_function = LoadDLL<LPDLL_CALL_CAMPROJ>("vismtv_geometryanalyzer", "compute_renderer_projmatrix");
	if (lpdll_function == NULL)
		return fail_ret("MODULE LOAD FAILIRE IN ComputeCameraRendererMatrice!");

	using namespace glm;
	fmat3x3 mat_int3x3;
	mat_int3x3[0][0] = fx;
	mat_int3x3[1][0] = sc;
	mat_int3x3[2][0] = cx;
	mat_int3x3[1][1] = fy;
	mat_int3x3[2][1] = cy;
	lpdll_function(mat_cs2ps, mat_ps2ss, (float*)&mat_int3x3, 0, 0, (float)w, (float)h, zn, zf, false, api_mode);

	auto __MatrixWS2CS = [](glm::fmat4x4 &mat, glm::fmat4x3 &mat_ext)
	{
		using namespace glm;
		dmat4x4 dmat_ws2cs = mat_ext;
		dmat4x4 dmat_cs2ws = inverse(dmat_ws2cs);
		dmat3x3 dmat_cs2ws_r = dmat_cs2ws;

		dvec4 _pos_eye4 = dmat_cs2ws * dvec4(0, 0, 0, 1);
		_pos_eye4 /= _pos_eye4.w;
		dvec3 _pos_eye = _pos_eye4;

		dvec3 _vec_axis_view = dmat_cs2ws_r * dvec3(0, 0, 1);
		dvec3 d3VecAxisZ = -glm::normalize(_vec_axis_view);

		dvec3 _vec_axis_up = dmat_cs2ws_r * dvec3(0, -1, 0);
		dvec3 d3VecAxisY = glm::normalize(_vec_axis_up);

		dvec3 d3VecAxisX = glm::cross(d3VecAxisY, d3VecAxisZ);
		d3VecAxisX = glm::normalize(d3VecAxisX);

		fmat4x4& _mat = mat;

		_mat[0][0] = (float)d3VecAxisX.x;
		_mat[0][1] = (float)d3VecAxisY.x;
		_mat[0][2] = (float)d3VecAxisZ.x;
		_mat[0][3] = 0;
		_mat[1][0] = (float)d3VecAxisX.y;
		_mat[1][1] = (float)d3VecAxisY.y;
		_mat[1][2] = (float)d3VecAxisZ.y;
		_mat[1][3] = 0;
		_mat[2][0] = (float)d3VecAxisX.z;
		_mat[2][1] = (float)d3VecAxisY.z;
		_mat[2][2] = (float)d3VecAxisZ.z;
		_mat[2][3] = 0;
		_mat[3][0] = (float)(-glm::dot(d3VecAxisX, _pos_eye));
		_mat[3][1] = (float)(-glm::dot(d3VecAxisY, _pos_eye));
		_mat[3][2] = (float)(-glm::dot(d3VecAxisZ, _pos_eye));
		_mat[3][3] = 1;
	};

	fmat4x4 vmat_ws2cs;
	fmat4x3 vmat_ext = *(fmat4x3*)mat_ext;
	__MatrixWS2CS(vmat_ws2cs, vmat_ext);
	*(fmat4x4*) mat_ws2cs = vmat_ws2cs;

	return true;
}

bool helpers::ComputeCameraRendererParameters(const float* pos_xyz_ws, const float* pos_xy_ss, const int num_mks,
	float* cam_pos, float* cam_view, float* cam_up, float* fx, float* fy, float* sc, float* cx, float* cy)
{
	typedef bool(*LPDLL_CALL_MPAAM)(float* mat_p4x3, const std::vector<float>& ws_xyz_pts, const std::vector<float>& ss_xy_pts, const bool dlt_mode);
	typedef bool(*LPDLL_CALL_DECOMP)(float* mat_ext4x3, float* mat_int3x3, const float* mat_p4x3, const bool given_intrinsic);

	LPDLL_CALL_MPAAM lpdll_function_mpaam = LoadDLL<LPDLL_CALL_MPAAM>("vismtv_geometryanalyzer", "compute_mpaam");
	LPDLL_CALL_DECOMP lpdll_function_decomp = LoadDLL<LPDLL_CALL_DECOMP>("vismtv_geometryanalyzer", "compute_decomp_camprojmatrix");
	if (lpdll_function_mpaam == NULL || lpdll_function_decomp == NULL)
		return fail_ret("MODULE LOAD FAILIRE IN ComputeCameraRendererParameters!");

	using namespace glm;

	std::vector<float> ws_xyz_pts(num_mks * 3);
	std::vector<float> ss_xy_pts(num_mks * 2);
	memcpy(&ws_xyz_pts[0], pos_xyz_ws, sizeof(float) * num_mks * 3);
	memcpy(&ss_xy_pts[0], pos_xy_ss, sizeof(float) * num_mks * 2);

	fmat4x3 mat_p4x3;
	lpdll_function_mpaam((float*)&mat_p4x3, ws_xyz_pts, ss_xy_pts, false); // false (Jacobian SVD) is much more accurate than false (DLT mode)
	fmat4x3 mat_ext4x3;
	fmat3x3 mat_int3x3;
	lpdll_function_decomp((float*)&mat_ext4x3, (float*)&mat_int3x3, (float*)&mat_p4x3, false);

	dmat4x4 dmat_ws2cs = mat_ext4x3;
	dmat4x4 dmat_cs2ws = inverse(dmat_ws2cs);
	dmat3x3 dmat_cs2ws_r = dmat_cs2ws;

	if (cam_pos)
	{
		dvec4 _pos_eye = dmat_cs2ws * dvec4(0, 0, 0, 1);
		_pos_eye /= _pos_eye.w;
		*(fvec3*)cam_pos = _pos_eye;
	}
	if (cam_view)
	{
		dvec3 _vec_axis_z = dmat_cs2ws_r * dvec3(0, 0, 1);
		fvec3 nrl_axis_z = glm::normalize(_vec_axis_z);
		*(fvec3*)cam_view = nrl_axis_z;
	}
	if (cam_up)
	{
		dvec3 _vec_axis_y = dmat_cs2ws_r * dvec3(0, -1, 0);
		fvec3 nrl_axis_y = glm::normalize(_vec_axis_y);
		*(fvec3*)cam_up = nrl_axis_y;
	}
	if (fx) *fx = mat_int3x3[0][0];
	if (fy) *fy = mat_int3x3[1][1];
	if (sc) *sc = mat_int3x3[1][0];
	if (cx) *cx = mat_int3x3[2][0];
	if (cy) *cy = mat_int3x3[2][1];

	return true;
}

bool helpers::ComputeArCameraCalibrateInfo(const float* mat_rbs2ts, const float* calrb_xyz_ts, const float* calrb_xy_ss, const int num_mks,
	float* mat_camcs2rbs, CameraParameters* cam_ar_mode_params)
{
	auto tr_pt = [](const glm::fmat4x4& mat, const glm::fvec3& p) -> glm::fvec3
	{
		glm::fvec4 _p(p, 1.f);
		_p = mat * _p;
		return glm::fvec3(_p.x / _p.w, _p.y / _p.w, _p.z / _p.w);
	};
	auto tr_nrvec = [](const glm::fmat4x4& mat, const glm::fvec3& v) -> glm::fvec3
	{
		glm::fmat3x3 mat_r = mat;
		return glm::normalize(mat_r * v);
	};

	glm::fmat4x4& fmat_rbs2ts = *(glm::fmat4x4*)mat_rbs2ts;

	glm::fvec3 cam_pos_ts, cam_up_ts, cam_view_ts;
	float _fx, _fy, _sc, _cx, _cy;
	helpers::ComputeCameraRendererParameters(calrb_xyz_ts, calrb_xy_ss, num_mks, &cam_pos_ts[0], &cam_view_ts[0], &cam_up_ts[0], &_fx, &_fy, &_sc, &_cx, &_cy);

	glm::fmat4x4 mat_ts2camcs;
	fMatrixWS2CS(&mat_ts2camcs, &cam_pos_ts, &cam_up_ts, &cam_view_ts); // note the convention of vis_motive core is row major
	mat_ts2camcs = glm::transpose(mat_ts2camcs);

	{
		// 
		glm::fvec3 pos_sample_ts = __cv3__ calrb_xyz_ts; // 1st item
		glm::fvec3 pos_camcs = tr_pt(mat_ts2camcs, pos_sample_ts);
		//cout << "z : " << pos_camcs.z << endl;
		if (pos_camcs.z > 0)
		{
			glm::fmat4x4 mat_reverse = glm::scale(glm::fvec3(-1));
			mat_ts2camcs = mat_reverse * mat_ts2camcs;
		}
	}

	glm::fmat4x4 mat_rbs2camcs = mat_ts2camcs * fmat_rbs2ts;
	glm::fmat4x4 fmat_camcs2rbs = glm::inverse(mat_rbs2camcs);

	if(mat_camcs2rbs)
		__cm4__ mat_camcs2rbs = fmat_camcs2rbs;

	//
	//if(mat_camcs2rbs)
	//	__cm4__ mat_camcs2rbs = glm::lookAtRH(cam_pos_rbs, cam_pos_rbs + cam_view_rbs, cam_up_rbs);

	if (cam_ar_mode_params)
	{
		cam_ar_mode_params->fx = _fx;
		cam_ar_mode_params->fy = _fy;
		cam_ar_mode_params->sc = _sc;
		cam_ar_mode_params->cx = _cx;
		cam_ar_mode_params->cy = _cy;
		cam_ar_mode_params->projection_mode = 3;

		glm::fvec3 cam_pos(0);
		glm::fvec3 cam_up(0, 1, 0);
		glm::fvec3 cam_view(0, 0, -1);
		glm::fvec3 cam_pos_rbs = tr_pt(fmat_camcs2rbs, cam_pos);
		glm::fvec3 cam_up_rbs = tr_nrvec(fmat_camcs2rbs, cam_up);
		glm::fvec3 cam_view_rbs = tr_nrvec(fmat_camcs2rbs, cam_view);
		
		*(glm::fvec3*)cam_ar_mode_params->pos = tr_pt(fmat_rbs2ts, cam_pos_rbs);
		*(glm::fvec3*)cam_ar_mode_params->up = tr_nrvec(fmat_rbs2ts, cam_up_rbs);
		*(glm::fvec3*)cam_ar_mode_params->view = tr_nrvec(fmat_rbs2ts, cam_view_rbs);
	}

	// test code
	//{
	//	glm::fvec3 cam_pos_clf, cam_up_clf, cam_view_clf;
	//	float _fx, _fy, _sc, _cx, _cy;
	//	helpers::ComputeCameraRendererParameters(__FP point3d[0], __FP point2d[0], num_stg_calib_pairs, __FP cam_pos_clf, __FP cam_view_clf, __FP cam_up_clf, &_fx, &_fy, &_sc, &_cx, &_cy);
	//	//TESTOUT("cam_pos_clf : ", cam_pos_clf);
	//	//TESTOUT("cam_view_clf : ", cam_pos_clf);
	//	//TESTOUT("cam_view_clf : ", cam_pos_clf);
	//
	//	glm::fmat4x4 mat_clf2stgcs = MatrixWS2CS(cam_pos_clf, cam_view_clf, cam_up_clf);
	//	mat_stgcs2clf = glm::inverse(mat_clf2stgcs);
	//	
	//	// TO DO // WHY??????????????????????????????? 4
	//	cam_state_calbirated.fx = _fx / 4;
	//	cam_state_calbirated.fy = _fy / 4;
	//	cam_state_calbirated.sc = _sc / 4;
	//	cam_state_calbirated.cx = _cx / 4;
	//	cam_state_calbirated.cy = _cy / 4;
	//
	//
	//	// to clf
	//	__cv3__ cam_state_calbirated.pos = tr_pt(mat_stgcs2clf, glm::fvec3(0));
	//	__cv3__ cam_state_calbirated.up = glm::normalize(tr_vec(mat_stgcs2clf, glm::fvec3(0, 1, 0)));
	//	__cv3__ cam_state_calbirated.view = glm::normalize(tr_vec(mat_stgcs2clf, glm::fvec3(0, 0, -1)));
	//
	//	//cout << "intrinsics : " << _fx << ", " << _fy << ", " << _sc << ", " << _cx << ", " << _cy << endl;
	//	glm::fmat4x4 mat_ws2stgcs = mat_clf2stgcs * mat_ws2clf;
	//	glm::fvec3 pos_stgcs = tr_pt(mat_ws2stgcs, ws_mk_spheres_xyzr[0]);
	//	cout << "z : " << pos_stgcs.z << endl;
	//	if (pos_stgcs.z > 0)
	//	{
	//		glm::fmat4x4 mat_reverse = glm::scale(glm::fvec3(-1));
	//		mat_stgcs2clf = mat_reverse * mat_stgcs2clf;
	//	}
	//}

	return true;
}

bool __ComputeArCameraCalibrateInfo(const float* mat_camrb2ws, const float* calrb_xyz_ws, const float* calrb_xy_ss, const int num_mks,
	float* cam_pos_crbs, float* cam_view_crbs, float* cam_up_crbs, CameraParameters& cam_ar_mode_params)
{
	glm::fmat4x4& fmat_camrb2ws = *(glm::fmat4x4*)mat_camrb2ws;

	glm::fvec3 cam_pos_ws, cam_up_ws, cam_view_ws;
	float _fx, _fy, _sc, _cx, _cy;
	helpers::ComputeCameraRendererParameters(calrb_xyz_ws, calrb_xy_ss, num_mks, &cam_pos_ws[0], &cam_view_ws[0], &cam_up_ws[0], &_fx, &_fy, &_sc, &_cx, &_cy);

	glm::fmat4x4 mat_ws2cam;
	fMatrixWS2CS(&mat_ws2cam, &cam_pos_ws, &cam_up_ws, &cam_view_ws);
	mat_ws2cam = glm::transpose(mat_ws2cam);
	glm::fmat4x4 mat_rbcam2cam = mat_ws2cam * fmat_camrb2ws;
	glm::fmat4x4 mat_cam2rbcam = glm::inverse(mat_rbcam2cam);
	glm::fmat3x3 mat_cam2rbcam_r = mat_cam2rbcam;

	glm::fvec3 cam_pos(0);
	glm::fvec3 cam_up(0, 1, 0);
	glm::fvec3 cam_view(0, 0, -1);

	auto tr_pt = [](const glm::fmat4x4& mat, const glm::fvec3& p) -> glm::fvec3
	{
		glm::fvec4 _p(p, 1.f);
		_p = mat * _p;
		return glm::fvec3(_p.x / _p.w, _p.y / _p.w, _p.z / _p.w);
	};
	auto tr_nrvec = [](const glm::fmat4x4& mat, const glm::fvec3& v) -> glm::fvec3
	{
		glm::fmat3x3 mat_r = mat;
		return glm::normalize(mat_r * v);
	};

	*(glm::fvec3*)cam_pos_crbs = tr_pt(mat_cam2rbcam, cam_pos);
	*(glm::fvec3*)cam_up_crbs = tr_nrvec(mat_cam2rbcam, cam_up);
	*(glm::fvec3*)cam_view_crbs = tr_nrvec(mat_cam2rbcam, cam_view);

	cam_ar_mode_params.fx = _fx;
	cam_ar_mode_params.fy = _fy;
	cam_ar_mode_params.sc = _sc;
	cam_ar_mode_params.cx = _cx;
	cam_ar_mode_params.cy = _cy;
	cam_ar_mode_params.projection_mode = 3;

	*(glm::fvec3*)cam_ar_mode_params.pos = tr_pt(fmat_camrb2ws, *(glm::fvec3*)cam_pos_crbs);
	*(glm::fvec3*)cam_ar_mode_params.up = tr_nrvec(fmat_camrb2ws, *(glm::fvec3*)cam_up_crbs);
	*(glm::fvec3*)cam_ar_mode_params.view = tr_nrvec(fmat_camrb2ws, *(glm::fvec3*)cam_view_crbs);

	return true;
}

#include "ArcBall.hpp"

std::map<helpers::arcball*, __arc_ball> map_arcballs;
helpers::arcball::arcball()
{
	map_arcballs.insert(std::pair<helpers::arcball*, __arc_ball>(this, __arc_ball()));
}
helpers::arcball::~arcball()
{
	map_arcballs.erase(this);
}
bool helpers::arcball::intializer(const float* stage_center, const float stage_radius)
{
	auto itr = map_arcballs.find(this);
	if (itr == map_arcballs.end())
		return fail_ret("NOT AVAILABLE ARCBALL!");
	__arc_ball& arc_ball = itr->second;

	glm::dvec3 dstage_center = *(glm::fvec3*)stage_center;
	arc_ball.FitArcballToSphere(dstage_center, (double)stage_radius);
	arc_ball.__is_set_stage = true;
	return true;
}


void compute_look_at_matrix(glm::dmat4x4& ws2cs, const glm::fvec3& pos_eye, const glm::fvec3& vec_view, const glm::fvec3& vec_up)
{
	glm::fvec3 _vec_view = glm::normalize(vec_view);
	glm::fvec3 _vec_up = glm::normalize(vec_up);
	glm::fvec3 _vec_right = glm::cross(_vec_view, _vec_up);
	_vec_up = glm::cross(_vec_right, _vec_view);

	ws2cs = glm::lookAtRH(pos_eye, pos_eye + _vec_view, _vec_up);
}

void compute_projection_matrix(glm::dmat4x4& cs2ps, const float w, const float h, const float z_near, const float z_far, const float fov)
{
	cs2ps = glm::perspectiveFovRH(fov * (float)M_PI / 180.f, w, h, z_near, z_far);
}

void compute_screen_matrix(glm::dmat4x4& ps2ss, const double w, const double h)
{
	glm::dmat4x4 matTranslate = glm::translate(glm::dvec3(1, -1, 1));
	glm::dmat4x4 matScale = glm::scale(glm::dvec3(0.5 * w, -0.5 * h, 0.5));

	glm::dmat4x4 matTranslateSampleModel = glm::translate(glm::dvec3(-0.5, -0.5, 0));

	ps2ss = matTranslateSampleModel * (matScale * matTranslate);
}

bool helpers::arcball::start(const int* pos_xy, const float* screen_size, const cam_pose& cam_pose, const float np, const float fp, const float sensitivity)
{
	glm::dvec2 pos = *(glm::ivec2*)pos_xy;
	auto itr = map_arcballs.find(this);
	if (itr == map_arcballs.end())
		return fail_ret("NOT AVAILABLE ARCBALL!");
	__arc_ball& arc_ball = itr->second;
	if (!arc_ball.__is_set_stage)
		return fail_ret("NO INITIALIZATION IN THIS ARCBALL!");

	__CameraState cam_pose_ac;
	cam_pose_ac.bIsPerspective = true;
	cam_pose_ac.d3PosCamera = *(glm::fvec3*)cam_pose.pos;
	cam_pose_ac.v3VecView = *(glm::fvec3*)cam_pose.view;
	cam_pose_ac.v3VecUp = *(glm::fvec3*)cam_pose.up;
	cam_pose_ac.dNearPlaneDistFromCamera = np;

	glm::dmat4x4 ws2cs, cs2ps, ps2ss, ws2ss;
	compute_look_at_matrix(ws2cs, cam_pose_ac.d3PosCamera, cam_pose_ac.v3VecView, cam_pose_ac.v3VecUp);
	compute_projection_matrix(cs2ps, screen_size[0], screen_size[1], np, fp, 45.f);
	compute_screen_matrix(ps2ss, screen_size[0], screen_size[1]);
	cam_pose_ac.matWS2SS = ps2ss * (cs2ps * ws2cs); // note that glm is based on column major
	cam_pose_ac.matSS2WS = glm::inverse(cam_pose_ac.matWS2SS);
	
	arc_ball.StartArcball(pos.x, pos.y, cam_pose_ac, 10. * sensitivity);

	return true;
}

bool helpers::arcball::move(const int* pos_xy, float* mat_r_onmove)
{
	glm::dvec2 pos = *(glm::ivec2*)pos_xy;
	auto itr = map_arcballs.find(this);
	if (itr == map_arcballs.end())
		return fail_ret("NOT AVAILABLE ARCBALL!");
	__arc_ball& arc_ball = itr->second;
	if (!arc_ball.__is_set_stage)
		return fail_ret("NO INITIALIZATION IN THIS ARCBALL!");

	glm::dmat4x4 mat_tr;
	arc_ball.MoveArcball(mat_tr, pos.x, pos.y, false); // cf. true

	__cm4__ mat_r_onmove = (glm::fmat4x4)mat_tr;
	return true;
}

bool helpers::arcball::move(const int* pos_xy, cam_pose& cam_pose)
{
	glm::dvec2 pos = *(glm::ivec2*)pos_xy;
	auto itr = map_arcballs.find(this);
	if (itr == map_arcballs.end())
		return fail_ret("NOT AVAILABLE ARCBALL!");
	__arc_ball& arc_ball = itr->second;
	if (!arc_ball.__is_set_stage)
		return fail_ret("NO INITIALIZATION IN THIS ARCBALL!");

	glm::dmat4x4 mat_tr;
	arc_ball.MoveArcball(mat_tr, pos.x, pos.y, true);

	__CameraState cam_pose_begin = arc_ball.GetCameraStateSetInStart();
	glm::dvec4 pos_eye_h = mat_tr * glm::dvec4(cam_pose_begin.d3PosCamera, 1.);
	glm::dvec3 pos_eye = pos_eye_h / pos_eye_h.w;

	glm::dmat3x3 mat_tr_r = mat_tr;
	glm::dvec3 vec_view = glm::normalize(mat_tr_r * cam_pose_begin.v3VecView);
	glm::dvec3 vec_up = glm::normalize(mat_tr_r * cam_pose_begin.v3VecUp);

	*(glm::fvec3*)cam_pose.view = (glm::fvec3)vec_view;
	*(glm::fvec3*)cam_pose.up = (glm::fvec3)vec_up;
	*(glm::fvec3*)cam_pose.pos = (glm::fvec3)pos_eye;

	//cam_pose.vec_view = (glm::fvec3)arc_ball.GetCenterStage() - cam_pose.pos_eye;
	//
	//cam_pose.vec_view = glm::normalize(cam_pose.vec_view);
	//cam_pose.vec_up = glm::normalize(cam_pose.vec_up);
	//
	//// correction //
	//glm::fvec3 right = glm::cross(cam_pose.vec_view, cam_pose.vec_up);
	//cam_pose.vec_up = glm::cross(right, cam_pose.vec_view);
	//cam_pose.vec_up = glm::normalize(cam_pose.vec_up);

	return true;
}

bool helpers::arcball::pan_move(const int* pos_xy, cam_pose& cam_pose)
{
	glm::dvec2 pos = *(glm::ivec2*)pos_xy;
	auto itr = map_arcballs.find(this);
	if (itr == map_arcballs.end())
		return fail_ret("NOT AVAILABLE ARCBALL!");
	__arc_ball& arc_ball = itr->second;
	if (!arc_ball.__is_set_stage)
		return fail_ret("NO INITIALIZATION IN THIS ARCBALL!");

	__CameraState cam_pose_begin = arc_ball.GetCameraStateSetInStart();

	glm::dmat4x4& mat_ws2ss = cam_pose_begin.matWS2SS;
	glm::dmat4x4& mat_ss2ws = cam_pose_begin.matSS2WS;

	auto transform_pt = [](glm::dvec3& pos, glm::dmat4x4& mat_tr)
	{
		glm::dvec4 pos_h = mat_tr * glm::dvec4(pos, 1.);
		return pos_h / pos_h.w;
	};

	if (!cam_pose_begin.bIsPerspective)
	{
		glm::dvec3 pos_eye_ss, pos_eye_ws;
		pos_eye_ws = cam_pose_begin.d3PosCamera;
		pos_eye_ss = transform_pt(pos_eye_ws, mat_ws2ss); 

		glm::dvec3 diff_ss = glm::dvec3(pos.x - arc_ball.__start_x, pos.y - arc_ball.__start_y, 0);

		pos_eye_ss = pos_eye_ss - diff_ss; // Think Panning! reverse camera moving
		pos_eye_ws = transform_pt(pos_eye_ss, mat_ss2ws);
		__cv3__ cam_pose.pos = (glm::fvec3)pos_eye_ws;
		__cv3__ cam_pose.up = (glm::fvec3)(cam_pose_begin.v3VecUp);
		__cv3__ cam_pose.view = (glm::fvec3)(cam_pose_begin.v3VecView);
	}
	else
	{
		glm::dvec3 pos_cur_ss = glm::dvec3(pos.x, pos.y, 0);
		glm::dvec3 pos_old_ss = glm::dvec3(arc_ball.__start_x, arc_ball.__start_y, 0);
		glm::dvec3 pos_cur_ws = transform_pt(pos_cur_ss, mat_ss2ws);
		glm::dvec3 pos_old_ws = transform_pt(pos_old_ss, mat_ss2ws);
		glm::dvec3 diff_ws = pos_cur_ws - pos_old_ws;

		if (glm::length(diff_ws) < DBL_EPSILON)
		{
			__cv3__ cam_pose.pos = (glm::fvec3)(cam_pose_begin.d3PosCamera);
			__cv3__ cam_pose.up = (glm::fvec3)(cam_pose_begin.v3VecUp);
			__cv3__ cam_pose.view = (glm::fvec3)(cam_pose_begin.v3VecView);
			return true;
		}

		//cout << "-----0> " << glm::length(diff_ws) << endl;
		//cout << "-----1> " << pos.x << ", " << pos.y << endl;
		//cout << "-----2> " << arc_ball.__start_x << ", " << arc_ball.__start_y << endl;
		glm::dvec3 pos_center_ws = arc_ball.GetCenterStage();
		glm::dvec3 vec_eye2center_ws = pos_center_ws - cam_pose_begin.d3PosCamera;

		double dPanningCorrected = glm::length(diff_ws) * glm::length(vec_eye2center_ws) / cam_pose_begin.dNearPlaneDistFromCamera;

		diff_ws = normalize(diff_ws);
		__cv3__ cam_pose.pos = (glm::fvec3)(cam_pose_begin.d3PosCamera - diff_ws * dPanningCorrected);
		__cv3__ cam_pose.up = (glm::fvec3)(cam_pose_begin.v3VecUp);
		__cv3__ cam_pose.view = (glm::fvec3)(cam_pose_begin.v3VecView);
	}
	return true;
}