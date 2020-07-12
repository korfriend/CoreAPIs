#include "VimCommon.h"
#include <unordered_map>
//#include <assert.h>
#include <thread>
#pragma warning (disable:4756)

using namespace std;


#include "nanoflann.hpp"
//using namespace nanoflann;
template <typename T, typename TT>
struct PointCloud
{
	//public:
	const uint num_vtx;
	const TT* pts;
	PointCloud(const TT* _pts, const uint _num_vtx) : pts(_pts), num_vtx(_num_vtx) { }

	// Must return the number of data points
	inline size_t kdtree_get_point_count() const { return num_vtx; }

	// Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
	inline T kdtree_distance(const T *p1, const size_t idx_p2, size_t) const
	{
		const T d0 = p1[0] - pts[idx_p2].x;
		const T d1 = p1[1] - pts[idx_p2].y;
		const T d2 = p1[2] - pts[idx_p2].z;
		return d0 * d0 + d1 * d1 + d2 * d2;
	}

	// Returns the dim'th component of the idx'th point in the class:
	// Since this is inlined and the "dim" argument is typically an immediate value, the
	//  "if/else's" are actually solved at compile time.
	inline T kdtree_get_pt(const size_t idx, int dim) const
	{
		if (dim == 0) return pts[idx].x;
		else if (dim == 1) return pts[idx].y;
		else return pts[idx].z;
	}

	// Optional bounding-box computation: return false to default to a standard bbox computation loop.
	//   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
	//   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
	template <class BBOX>
	bool kdtree_get_bbox(BBOX&) const { return false; }
};

typedef nanoflann::KDTreeSingleIndexAdaptor<
	nanoflann::L2_Simple_Adaptor<float, PointCloud<float, vmfloat3> >,
	PointCloud<float, vmfloat3>,
	3 // dim 
> kd_tree_t;

namespace vmobjects {
	string dtype_bool_name = typeid(bool).name();
	string dtype_char_name = typeid(char).name();
	string dtype_byte_name = typeid(byte).name();
	string dtype_byte4_name = typeid(vmbyte4).name();
	string dtype_short_name = typeid(short).name();
	string dtype_ushort_name = typeid(ushort).name();
	string dtype_int_name = typeid(int).name();
	string dtype_uint_name = typeid(uint).name();
	string dtype_float_name = typeid(float).name();
	string dtype_double_name = typeid(double).name();
	string dtype_string_name = typeid(string).name();
	string dtype_vector_byte_name = typeid(vector<byte>).name();
	string dtype_vector_int_name = typeid(vector<int>).name();
	string dtype_vector_float_name = typeid(vector<float>).name();
	string dtype_vector_double_name = typeid(vector<double>).name();

	string dtype_int2_name = typeid(vmint2).name();
	string dtype_int3_name = typeid(vmint3).name();
	string dtype_int4_name = typeid(vmint4).name();
	string dtype_double2_name = typeid(vmdouble2).name();
	string dtype_double3_name = typeid(vmdouble3).name();
	string dtype_double4_name = typeid(vmdouble4).name();
	string dtype_mat44_name = typeid(vmmat44).name();

	template <typename T> bool _FillVolumeBoundary(T** vol_slices, const vmint3& vol_size, const vmint3& bnd_size, const double v, const bool clamp_z, LocalProgress* progress)
	{
		vmint3 volex_size = vol_size + bnd_size * 2;

		uint xy_size = (uint)volex_size.x*(uint)volex_size.y;

		if (progress)
			*progress->progress_ptr = progress->start;

		for (int z = 0; z < vol_size.z; z++)
		{
			if (progress)
				*progress->progress_ptr = (progress->start + (double)z / vol_size.z*progress->range);

			for (int y = 0; y < bnd_size.y; y++)
			{
				for (int x = 0; x < volex_size.x; x++)
				{
					vol_slices[z + bnd_size.z][x + y * volex_size.x] = (T)v;
					vol_slices[z + bnd_size.z][x + (y + vol_size.y + bnd_size.y)*volex_size.x] = (T)v;
				}
			}
			for (int y = 0; y < volex_size.y; y++)
			{
				for (int x = 0; x < bnd_size.x; x++)
				{
					vol_slices[z + bnd_size.z][x + (y)*volex_size.x] = (T)v;
					vol_slices[z + bnd_size.z][x + vol_size.x + bnd_size.x + (y)*volex_size.x] = (T)v;
				}
			}
		}

		for (int z = 0; z < bnd_size.z; z++)
		{
			if (clamp_z)
				for (uint xy = 0; xy < xy_size; xy++)
				{
					vol_slices[z][xy] = vol_slices[bnd_size.z][xy];
					vol_slices[z + vol_size.z + bnd_size.z][xy] = vol_slices[vol_size.z + bnd_size.z - 1][xy];
				}
			else
				for (uint xy = 0; xy < xy_size; xy++)
				{
					vol_slices[z][xy] = (T)v;
					vol_slices[z + vol_size.z + bnd_size.z][xy] = (T)v;
				}
		}

		return true;
	}
	template <typename T> bool _GenerateHistogramBasedOnVolumeValues(ullong* histo_values, const T** vol_slices, const vmint3& vol_size, const vmint3& bnd_size, const double min_v, const uint histo_range, LocalProgress* progress)
	{
		double progress_step = 0.0f;
		if (progress)
		{
			*progress->progress_ptr = progress->start;
			progress_step = 1.0 / (double)vol_size.z * progress->range;
		}
	
		int num_threads = 8;
		int thread_idx = 0;
		std::thread *threads = new std::thread[num_threads];
		ullong** histo_value_threads = new ullong*[num_threads];
		for (int i = 0; i < num_threads; i++)
		{
			histo_value_threads[i] = new ullong[histo_range];
			memset(histo_value_threads[i], 0, sizeof(ullong)*histo_range);
		}
	
		int volex_size = vol_size.x + bnd_size.x * 2;
		bool ret = true;
		
		for (int z = 0; z < vol_size.z; z++)
		{
			if (progress)
				*progress->progress_ptr += progress_step;
	
			//for(int iY = 0; iY < i3VolumeSize.y; iY++)
			//{
			//	// Histogram : pVolumeData->pullHistogram //
			//	for(int iX = 0; iX < i3VolumeSize.x; iX++)
			//	{
			//		double dValue = (double)pptVolume[iZ + i3SizeBoundary.z]
			//			[iX + i3SizeBoundary.x + (iY + i3SizeBoundary.y)*iVolumeAddrSizeX];
			//		uint uiAddr = uint(dValue - dMinValue + 0.5);
			//		if(uiAddr < uiRangeHistogram)
			//			pullHistogram[uiAddr]++;
			//		else
			//		{
			//			printf("Histogram value(x) range violation!");
			//			return false;
			//		}
			//	}
			//}
	
			auto func = [&](int fixed_z)
			{
				int idx = fixed_z % num_threads;
				for (int y = 0; y < vol_size.y; y++)
				{
					// Histogram : pVolumeData->pullHistogram //
					for (int iX = 0; iX < vol_size.x; iX++)
					{
						double dValue = (double)vol_slices[fixed_z + bnd_size.z]
							[iX + bnd_size.x + (y + bnd_size.y)*volex_size];
						uint uiAddr = uint(dValue - min_v + 0.5);
						if (uiAddr < histo_range)
							histo_value_threads[idx][uiAddr]++;
						else
						{
							ret = false;
							return;
						}
					}
				}
			};
			thread_idx = z % num_threads;
			if (z >= num_threads && threads[thread_idx].joinable())
				threads[thread_idx].join();
			if (ret == false)
				break;
			threads[thread_idx] = std::thread(func, z);
		}
		for (int i = 0; i < num_threads && i < vol_size.z; i++)
		if (threads[i].joinable() == true)
			threads[i].join();
		if (ret == true)
		{
			for (int i = 0; i < num_threads && i < vol_size.z; i++)
			for (uint j = 0; j < histo_range; j++)
				histo_values[j] = histo_values[j] + histo_value_threads[i][j];
		}
		else
		{
			printf("Histogram value(x) range violation!");
		}
		VMSAFE_DELETEARRAY(threads);
		VMSAFE_DELETE2DARRAY(histo_value_threads, num_threads);
		return ret;
	}
	bool VmVObjectVolume::FillBoundaryWithValue(VolumeData& vol_data, const double v, const bool clamp_z, LocalProgress* progress)
	{
		if (vol_data.vol_slices == NULL)
		{
			printf("FillBoundaryWithValue - Undefined Volume");
			return false;
		}
		if (vol_data.store_Mm_values.x >= 1.7976931348623158e+300)
		{
			printf("FillBoundaryWithValue - Undefined Minimum Value");
			return false;
		}

		bool ret = false;
		if (vol_data.store_dtype.type_name == dtype_byte_name)
			ret = _FillVolumeBoundary<byte>((byte**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, v, clamp_z, progress);
		else if (vol_data.store_dtype.type_name == dtype_char_name)
			ret = _FillVolumeBoundary<char>((char**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, v, clamp_z, progress);
		else if (vol_data.store_dtype.type_name == dtype_ushort_name)
			ret = _FillVolumeBoundary<ushort>((ushort**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, v, clamp_z, progress);
		else if (vol_data.store_dtype.type_name == dtype_short_name)
			ret = _FillVolumeBoundary<short>((short**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, v, clamp_z, progress);
		else if (vol_data.store_dtype.type_name == dtype_int_name)
			ret = _FillVolumeBoundary<int>((int**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, v, clamp_z, progress);
		else if (vol_data.store_dtype.type_name == dtype_float_name)
			ret = _FillVolumeBoundary<float>((float**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, v, clamp_z, progress);
		else if (vol_data.store_dtype.type_name == dtype_double_name)
			ret = _FillVolumeBoundary<double>((double**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, v, clamp_z, progress);
		else
		{
			printf("FillBoundaryWithValue - Not Supported Volume Type");
			VMSAFE_DELETEARRAY(vol_data.histo_values);
		}
		return ret;
	}
	bool VmVObjectVolume::FillHistogram(VolumeData& vol_data, LocalProgress* progress)
	{
		if(vol_data.vol_slices == NULL)
		{
			printf("FillHistogram - Undefined Volume");
			return false;
		}
		VMSAFE_DELETEARRAY(vol_data.histo_values);
		
		if(vol_data.store_Mm_values.y < vol_data.store_Mm_values.x)
		{
			printf("FillHistogram - Unvalid Min/Max Value");
			return false;
		}
		
		uint histo_range = (uint)((double)max(vol_data.store_Mm_values.y - vol_data.store_Mm_values.x + 1.5, 1.));
		if((double)histo_range * 8.0 / 1024.0 / 1024.0 > 256)
		{
			printf("FillHistogram : Too Large HistoRange. Larger than 256 MB");
			return false;
		}
		
		vol_data.histo_values = new ullong[histo_range];
		memset(vol_data.histo_values, 0, sizeof(ullong)*histo_range);
		
		bool ret = false;
		if (vol_data.store_dtype.type_name == dtype_byte_name)
			ret = _GenerateHistogramBasedOnVolumeValues<byte>(vol_data.histo_values, (const byte**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, vol_data.store_Mm_values.x, histo_range, progress);
		else if (vol_data.store_dtype.type_name == dtype_char_name)
			ret = _GenerateHistogramBasedOnVolumeValues<char>(vol_data.histo_values, (const char**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, vol_data.store_Mm_values.x, histo_range, progress);
		else if (vol_data.store_dtype.type_name == dtype_ushort_name)
			ret = _GenerateHistogramBasedOnVolumeValues<ushort>(vol_data.histo_values, (const ushort**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, vol_data.store_Mm_values.x, histo_range, progress);
		else if (vol_data.store_dtype.type_name == dtype_short_name)
			ret = _GenerateHistogramBasedOnVolumeValues<short>(vol_data.histo_values, (const short**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, vol_data.store_Mm_values.x, histo_range, progress);
		else if (vol_data.store_dtype.type_name == dtype_int_name)
			ret = _GenerateHistogramBasedOnVolumeValues<int>(vol_data.histo_values, (const int**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, vol_data.store_Mm_values.x, histo_range, progress);
		else if (vol_data.store_dtype.type_name == dtype_float_name)
			ret = _GenerateHistogramBasedOnVolumeValues<float>(vol_data.histo_values, (const float**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, vol_data.store_Mm_values.x, histo_range, progress);
		else if (vol_data.store_dtype.type_name == dtype_double_name)
			ret = _GenerateHistogramBasedOnVolumeValues<double>(vol_data.histo_values, (const double**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, vol_data.store_Mm_values.x, histo_range, progress);
		else
		{
			printf("FillHistogram - Not Supported Volume Type");
			VMSAFE_DELETEARRAY(vol_data.histo_values);
		}
		if (!ret) VMSAFE_DELETEARRAY(vol_data.histo_values);

		return ret;
	}

	template <typename T> void GetMinMaxValue(vmdouble2& mM_values/*out*/, const T** vol_slices, 
		const vmint3& vol_size, const vmint3& bnd_size, LocalProgress* progress)
	{
		if (progress) progress->Init();

		int width = vol_size.x + bnd_size.x * 2;
		double min_v = DBL_MAX;
		double max_v = -DBL_MAX;
		for (int iZ = 0; iZ < vol_size.z; iZ++)
		{
			if (progress) progress->SetProgress(iZ, vol_size.z);
			for (int iY = 0; iY < vol_size.y; iY++)
			{
				for (int iX = 0; iX < vol_size.x; iX++)
				{
					double dValue = (double)vol_slices[iZ + bnd_size.z][(iY + bnd_size.y)*width + iX + bnd_size.x];
					min_v = min(min_v, dValue);
					max_v = max(max_v, dValue);
				}
			}
		}
		if (progress) progress->Deinit();
		mM_values.x = min_v;
		mM_values.y = max_v;
	}
	bool VmVObjectVolume::FillMinMaxStoreValues(VolumeData& vol_data, LocalProgress* progress)
	{
		if (vol_data.store_dtype.type_name == dtype_char_name)
			GetMinMaxValue<char>(vol_data.store_Mm_values, (const char**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else if (vol_data.store_dtype.type_name == dtype_byte_name)
			GetMinMaxValue<byte>(vol_data.store_Mm_values, (const byte**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else if (vol_data.store_dtype.type_name == dtype_short_name)
			GetMinMaxValue<short>(vol_data.store_Mm_values, (const short**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else if (vol_data.store_dtype.type_name == dtype_ushort_name)
			GetMinMaxValue<ushort>(vol_data.store_Mm_values, (const ushort**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else if (vol_data.store_dtype.type_name == dtype_int_name)
			GetMinMaxValue<int>(vol_data.store_Mm_values, (const int**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else if (vol_data.store_dtype.type_name == dtype_uint_name)
			GetMinMaxValue<uint>(vol_data.store_Mm_values, (const uint**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else if (vol_data.store_dtype.type_name == dtype_float_name)
			GetMinMaxValue<float>(vol_data.store_Mm_values, (const float**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else if (vol_data.store_dtype.type_name == dtype_double_name)
			GetMinMaxValue<double>(vol_data.store_Mm_values, (const double**)vol_data.vol_slices, vol_data.vol_size, vol_data.bnd_size, progress);
		else return false;
		return true;
	}
#pragma region // VmObject //
	struct ObjectArchive
	{
		// Object ID
		int object_id;
		// mention
		string descriptor;
		// 내용물이 정의되었는가 여부
		bool is_defined;
		// 내용물이 참조한 Object ID
		int ref_object_id;

		VmLObject* local_lobj = NULL;
	
		// VmObject 에 등록되는 customized information을 저장된 map container
		map<string, string> mapStringParameter;
		map<string, bool> mapBoolParameter;
		map<string, int> mapIntParameter;  
		map<string, vmint2> mapInt2Parameter;  // for legacy
		map<string, vmint3> mapInt3Parameter;  // for legacy
		map<string, vmint4> mapInt4Parameter;  // for legacy
		map<string, double> mapDoubleParameter;
		map<string, vmdouble2> mapDouble2Parameter; // for legacy
		map<string, vmdouble3> mapDouble3Parameter; // for legacy
		map<string, vmdouble4> mapDouble4Parameter; // for legacy
		map<string, vmmat44> mapMat44Parameter; // for legacy
	};

	VmObject::VmObject()
	{
		oa_res = new ObjectArchive();
		oa_res->object_id = 0;
		oa_res->ref_object_id = 0;
		oa_res->is_defined = false;
	}

	VmObject::~VmObject()
	{
		VMSAFE_DELETE(oa_res->local_lobj);
		VMSAFE_DELETE(oa_res)
	}

	bool VmObject::IsDefined()
	{
		return oa_res->is_defined;
	}

	void VmObject::SetObjectID(const int obj_id)
	{
		oa_res->object_id = obj_id;
	}
	int VmObject::GetObjectID() const
	{
		return oa_res->object_id;
	}

	void VmObject::SetReferenceObjectID(const int ref_obj_id)
	{
		oa_res->ref_object_id = ref_obj_id;
	}

	int VmObject::GetReferenceObjectID() const
	{
		return oa_res->ref_object_id;
	}

	void VmObject::SetDescriptor(const string& str)
	{
		oa_res->descriptor = str;
	}

	string VmObject::GetDescriptor() const
	{
		return oa_res->descriptor;
	}

	EvmObjectType VmObject::GetObjectType()
	{
		return (EvmObjectType)((oa_res->object_id >> 24) & 0xFF);
	}

	EvmObjectType VmObject::GetObjectTypeFromID(const int object_id)
	{
		return (EvmObjectType)((object_id >> 24) & 0xFF);
	}

	bool VmObject::IsVObject(int object_id)
	{
		EvmObjectType eObjectType = GetObjectTypeFromID(object_id);
		switch (eObjectType)
		{
		case ObjectTypeVOLUME:
		case ObjectTypePRIMITIVE:
			return true;
		default:
			return false;
		}
		return false;
	}

	bool VmObject::RegisterCustomParameter(const std::string& _key, const std::string& _data)
	{
		oa_res->mapStringParameter[_key] = _data;
		return true;
	}

	bool VmObject::RegisterCustomParameter(const std::string& _key, const bool _data)
	{
		oa_res->mapBoolParameter[_key] = _data;
		return true;
	}

	bool VmObject::RegisterCustomParameter(const std::string& _key, const int _data)
	{
		oa_res->mapIntParameter[_key] = _data;
		return true;
	}

	bool VmObject::RegisterCustomParameter(const std::string& _key, const vmint2 _data)
	{
		oa_res->mapInt2Parameter[_key] = _data;
		return true;
	}

	bool VmObject::RegisterCustomParameter(const std::string& _key, const vmint3 _data)
	{
		oa_res->mapInt3Parameter[_key] = _data;
		return true;
	}

	bool VmObject::RegisterCustomParameter(const std::string& _key, const vmint4 _data)
	{
		oa_res->mapInt4Parameter[_key] = _data;
		return true;
	}
	bool VmObject::RegisterCustomParameter(const std::string& _key, const double _data)
	{
		oa_res->mapDoubleParameter[_key] = _data;
		return true;
	}
	bool VmObject::RegisterCustomParameter(const std::string& _key, const vmdouble2 _data)
	{
		oa_res->mapDouble2Parameter[_key] = _data;
		return true;
	}
	bool VmObject::RegisterCustomParameter(const std::string& _key, const vmdouble3 _data)
	{
		oa_res->mapDouble3Parameter[_key] = _data;
		return true;
	}
	bool VmObject::RegisterCustomParameter(const std::string& _key, const vmdouble4 _data)
	{
		oa_res->mapDouble4Parameter[_key] = _data;
		return true;
	}
	bool VmObject::RegisterCustomParameter(const std::string& _key, const vmmat44 _data)
	{
		oa_res->mapMat44Parameter[_key] = _data;
		return true;
	}

	bool VmObject::GetCustomParameter(const std::string& _key, const data_type& dtype, void* _data) const
	{
#define GET_VALUE(_type, _map) {auto itr = _map.find(_key);\
		if (itr == _map.end()) return false;\
		*(_type*)_data = itr->second;}

		if (dtype.type_name == dtype_bool_name)
			GET_VALUE(bool, oa_res->mapBoolParameter)
		else if (dtype.type_name == dtype_int_name)
			GET_VALUE(int, oa_res->mapIntParameter)
		else if (dtype.type_name == dtype_double_name)
			GET_VALUE(double, oa_res->mapDoubleParameter)
		else if (dtype.type_name == dtype_string_name)
			GET_VALUE(string, oa_res->mapStringParameter)

			// will deprecated!
		else if (dtype.type_name == dtype_int2_name)
			GET_VALUE(vmint2, oa_res->mapInt2Parameter)
		else if (dtype.type_name == dtype_int3_name)
			GET_VALUE(vmint3, oa_res->mapInt3Parameter)
		else if (dtype.type_name == dtype_int4_name)
			GET_VALUE(vmint4, oa_res->mapInt4Parameter)

		else if (dtype.type_name == dtype_double2_name)
			GET_VALUE(vmdouble2, oa_res->mapDouble2Parameter)
		else if (dtype.type_name == dtype_double3_name)
			GET_VALUE(vmdouble3, oa_res->mapDouble3Parameter)
		else if (dtype.type_name == dtype_double4_name)
			GET_VALUE(vmdouble4, oa_res->mapDouble4Parameter)

		else if (dtype.type_name == dtype_mat44_name)
			GET_VALUE(vmmat44, oa_res->mapMat44Parameter)

		else
		{
			printf("INVALID TYPE"); return false;
		}

		return true;
	}
	VmLObject* VmObject::GetBufferObject()
	{
		if (oa_res->local_lobj == NULL)
			oa_res->local_lobj = new VmLObject();
		return oa_res->local_lobj;
	}
	void VmObject::RemoveBufferObject()
	{
		VMSAFE_DELETE(oa_res->local_lobj);
	}
#pragma endregion

#pragma region // VmVObject //
	struct VObjectArchive
	{
		// VXVObject의 axis-aligned bounding box를 정의
		AaBbMinMax aabbMm;
	
		// VXVObject가 OS에서 WS로 배치되는 geometry가 정의되었는가 여부
		bool isDefinedGeometry;
		// OS 와 WS 간 변환 행렬
		vmmat44 mat_os2ws, mat_ws2os;
		vmmat44 fmat_os2ws, fmat_ws2os;
	
		map<string, VmVObjectBaseCustomData*> mapCustomDataStructures;
	
		void Delete(){
			for (map<string, VmVObjectBaseCustomData*>::iterator itr = mapCustomDataStructures.begin(); itr != mapCustomDataStructures.end(); itr++)
			{
				itr->second->Delete();
				//VMSAFE_DELETE(itr->second);
			}
			mapCustomDataStructures.clear();
		}
	};
	
	VmVObject::VmVObject()
	{
		voa_res = new VObjectArchive();
		voa_res->isDefinedGeometry = false;
	}

	VmVObject::~VmVObject()
	{
		voa_res->Delete();
		VMSAFE_DELETE(voa_res);
	}
	
	void VmVObject::GetOrthoBoundingBox(AaBbMinMax& aabbMm_os)
	{
		if(!voa_res->isDefinedGeometry)
		{
			printf("VmVObject - UNDEFINED GEOMERY STATE!");
			return;
		}
		aabbMm_os = voa_res->aabbMm;
	}
	
	bool VmVObject::IsGeometryDefined()
	{
		return voa_res->isDefinedGeometry;
	}
	
	void VmVObject::SetTransformMatrixOS2WS(const vmmat44& mat_os2ws)
	{
		voa_res->mat_os2ws = mat_os2ws;
		voa_res->fmat_os2ws = vmmat44f(mat_os2ws);
		vmmath::MatrixInverse(&voa_res->mat_ws2os, &mat_os2ws);
		voa_res->fmat_ws2os = vmmat44f(voa_res->mat_ws2os);
	}

	vmmat44 VmVObject::GetMatrixOS2WS()
	{
		return voa_res->mat_os2ws;
	}

	vmmat44 VmVObject::GetMatrixWS2OS()
	{
		return voa_res->mat_ws2os;
	}

	vmmat44f VmVObject::GetMatrixOS2WSf()
	{
		return voa_res->fmat_os2ws;
	}

	vmmat44f VmVObject::GetMatrixWS2OSf()
	{
		return voa_res->fmat_ws2os;
	}
	
	void VmVObject::ReplaceOrAddCustumDataStructure(const std::string& _key, VmVObjectBaseCustomData* _data)
	{
		VmVObjectBaseCustomData* data_old = GetCustumDataStructure(_key);
		if (data_old != NULL)
		{
			data_old->Delete();
			voa_res->mapCustomDataStructures.erase(_key);
		}
		voa_res->mapCustomDataStructures.insert(pair<string, VmVObjectBaseCustomData*>(_key, _data));
	}
	
	VmVObjectBaseCustomData* VmVObject::GetCustumDataStructure(const std::string& _key)
	{
		map<string, VmVObjectBaseCustomData*>::iterator itr = voa_res->mapCustomDataStructures.find(_key);
		if (itr == voa_res->mapCustomDataStructures.end())
			return NULL;
		return itr->second;
	}
	void VmVObject::RemoveCustomDataStructure(const std::string& _key)
	{
		map<string, VmVObjectBaseCustomData*>::iterator itr = voa_res->mapCustomDataStructures.find(_key);
		if (itr == voa_res->mapCustomDataStructures.end())
			return;
		itr->second->Delete();
		voa_res->mapCustomDataStructures.erase(itr);
	}
	void VmVObject::RemoveCustomDataStructures()
	{
		voa_res->Delete();
	}
#pragma endregion

#pragma region // VmVObjectVolume //
	struct VObjectVolumeArchive
	{
		VolumeBlocks vol_blks[2];
		VolumeData vol_data;
	};
	
	VmVObjectVolume::VmVObjectVolume()
	{
		// Initialized with Structure's constructor
		voavol_res = new VObjectVolumeArchive();
	}
	
	VmVObjectVolume::~VmVObjectVolume()
	{
		Destroy();
		VMSAFE_DELETE(voavol_res);
	}
	
	void VmVObjectVolume::Destroy()
	{
		if(oa_res->ref_object_id == 0)
		{
			voavol_res->vol_data.Delete();
			voavol_res->vol_blks[0].Delete();
			voavol_res->vol_blks[1].Delete();
		}
	
		VolumeBlocks blk_init;
		voavol_res->vol_blks[0] = blk_init;
		voavol_res->vol_blks[1] = blk_init;
	}
	
	bool __ComputeIntialAlignmentMatrixOS2WS(vmmat44& mat_os2ws, AxisInfoOS2WS& axis_info, const vmdouble3& vox_pitch, const AaBbMinMax& aabbMm_os)
	{
		axis_info.ComputeInitalMatrix(); // update axis_info.mat_os2ws

		vmmat44 matScale;
		vmmath::MatrixScaling(&matScale, (vmdouble3*)&vox_pitch);

		vmdouble3 pos_min_pitch_scale, pos_max_pitch_scale;
		vmmath::TransformPoint(&pos_min_pitch_scale, &aabbMm_os.pos_min, &matScale);
		vmmath::TransformPoint(&pos_max_pitch_scale, &aabbMm_os.pos_max, &matScale);

		vmdouble3 pos_min_ws, pos_max_ws;
		vmmath::TransformPoint(&pos_min_ws, &pos_min_pitch_scale, &axis_info.mat_os2ws);
		vmmath::TransformPoint(&pos_max_ws, &pos_max_pitch_scale, &axis_info.mat_os2ws);

		vmdouble3 translatevec = -(pos_min_ws + pos_max_ws) * 0.5;
		vmmat44 matTranslate;
		vmmath::MatrixTranslation(&matTranslate, &translatevec);

		mat_os2ws = matScale * axis_info.mat_os2ws * matTranslate;
		return true;
	}

	bool VmVObjectVolume::RegisterVolumeData(const VolumeData& vol_data, vmint3 blk_size2[2]/* 0 : Large, 1: Small */, const int ref_obj_id, LocalProgress* progress)
	{
		vmint3 vol_size = vol_data.vol_size;
		vmdouble3 vox_pitch = vol_data.vox_pitch;
		if(blk_size2 != NULL)
		{
			while(blk_size2[0].x >= vol_size.x)
			{
				blk_size2[0].x /= 2;
				blk_size2[1].x /= 2;
			}
			blk_size2[0].x = max(blk_size2[0].x, 1);
			blk_size2[1].x = max(blk_size2[1].x, 1);
			while(blk_size2[0].y >= vol_size.y)
			{
				blk_size2[0].y /= 2;
				blk_size2[1].y /= 2;
			}
			blk_size2[0].y = max(blk_size2[0].y, 1);
			blk_size2[1].y = max(blk_size2[1].y, 1);
			while(blk_size2[0].z >= vol_size.z)
			{
				blk_size2[0].z /= 2;
				blk_size2[1].z /= 2;
			}
			blk_size2[0].z = max(blk_size2[0].z, 1);
			blk_size2[1].z = max(blk_size2[1].z, 1);
	
			if(blk_size2[0].x <= 0 || blk_size2[0].y <= 0 || blk_size2[0].z <= 0
				|| blk_size2[0].x > vol_size.x || blk_size2[0].y > vol_size.y || blk_size2[0].z > vol_size.z
				|| blk_size2[1].x <= 0 || blk_size2[1].y <= 0 || blk_size2[1].z <= 0
				|| blk_size2[1].x > vol_size.x || blk_size2[1].y > vol_size.y || blk_size2[1].z > vol_size.z
				|| blk_size2[0].x < blk_size2[1].x || blk_size2[0].y < blk_size2[1].y || blk_size2[0].z < blk_size2[1].z)
			{
				printf("VmVObjectVolume::RegisterVolumeData - NOT AVAILABLE VOLUME INFO");
				return false;
			}
		}
		
		if(vol_size.x <= 0 || vol_size.y <= 0 || vol_size.z <= 0
			|| vox_pitch.x <= 0 || vox_pitch.z <= 0 || vox_pitch.z <= 0 || vol_data.store_dtype.type_bytes == 0)
		{
			printf("VmVObjectVolume::RegisterVolumeData - NOT AVAILABLE VOLUME INFO");
			return false;
		}
	
		Destroy();	// Including m_VolumeData.Delete()
	
		voavol_res->vol_data = vol_data;
		if(voavol_res->vol_data.origin_dtype.type_bytes == 0)
			voavol_res->vol_data.origin_dtype = voavol_res->vol_data.store_dtype;
		
		oa_res->is_defined = true;
		
		if(blk_size2 != NULL)
		{
			if(blk_size2[0].x > 0 && blk_size2[1].x > 0
				&& blk_size2[0].y > 0 && blk_size2[1].y > 0
				&& blk_size2[0].z > 0 && blk_size2[1].z > 0)
			{
				UpdateVolumeMinMaxBlocks(progress, blk_size2);
			}
		}
	
		// Define initial GEOMETRY //
		voa_res->aabbMm = AaBbMinMax(vol_size);
		__ComputeIntialAlignmentMatrixOS2WS(voa_res->mat_os2ws, voavol_res->vol_data.axis_info , vox_pitch, voa_res->aabbMm);
		vmmath::MatrixInverse(&voa_res->mat_ws2os, &voa_res->mat_os2ws);

		voa_res->fmat_os2ws = vmmat44f(voa_res->mat_os2ws);
		voa_res->fmat_ws2os = vmmat44f(voa_res->mat_ws2os);

		voa_res->isDefinedGeometry = true;
	
		return true;
	}
	
	template <typename TVox, typename TBox/*Same Type and 2 Channel*/> void _TGenerateVolumeBlocksAndComputeBlockArchive(VolumeBlocks* vol_blks, const vmint3& blk_size, const VolumeData& vol_data, LocalProgress* progress)
	{
		vol_blks->Delete();

		double minInit = (double)DBL_MAX;			// Min
		double maxInit = (double)(-DBL_MAX);		// Max
		if (vol_data.store_dtype.type_name == dtype_byte_name)
		{
			minInit = 255; maxInit = 0;
		}
		else if (vol_data.store_dtype.type_name == dtype_char_name)
		{
			minInit = -128; maxInit = 127;
		}
		else if (vol_data.store_dtype.type_name == dtype_ushort_name)
		{
			minInit = 65535; maxInit = 0;
		}
		else if (vol_data.store_dtype.type_name == dtype_short_name)
		{
			minInit = -32768; maxInit = 32767;
		}
		else
		{
			printf("UNSUPPORTED DATA TYPE : BLOCK GENERATION");
			return;
		}
	
		// Size Setting
		void** vol_slices = vol_data.vol_slices;
		vmint3 vol_size = vol_data.vol_size;
		vmint3 ex_size = vol_data.bnd_size;
	
		vol_blks->dtype = vol_data.store_dtype;
		vol_blks->unitblk_size = blk_size;
		vmint3 blk_bnd_size = vol_blks->blk_bnd_size = vmint3(2, 2, 2);
		int modX = vol_size.x % blk_size.x;
		int modY = vol_size.y % blk_size.y;
		int modZ = vol_size.z % blk_size.z;
		int blockNumX = vol_blks->blk_vol_size.x = vol_size.x/blk_size.x + (modX != 0);
		int blockNumY = vol_blks->blk_vol_size.y = vol_size.y/blk_size.y + (modY != 0);
		int blockNumZ = vol_blks->blk_vol_size.z = vol_size.z/blk_size.z + (modZ != 0);
		int xy = blockNumX * blockNumY;
	
		int blkNumAllX = blockNumX + blk_bnd_size.x * 2;
		int blkNumAllY = blockNumY + blk_bnd_size.y * 2;
		int blkNumAllZ = blockNumZ + blk_bnd_size.z * 2;
	
		int numAllBlocks = blkNumAllX * blkNumAllY * blkNumAllZ;
	
		// MinMax Block Setting
		if (vol_blks->mM_blks == NULL)
			vol_blks->mM_blks = new TBox[numAllBlocks];
	
		for (int i = 0; i < numAllBlocks; i++)
		{
			TBox c2MinMax;
			// Coundary Sampling 위함.
			c2MinMax.x = (TVox)(maxInit);
			c2MinMax.y = (TVox)(minInit);
			((TBox*)vol_blks->mM_blks)[i] = c2MinMax;
		}
	
		// MinMax Block Initial Setting
		vmint3 volx_size;
		volx_size.x = vol_size.x + ex_size.x*2;
		volx_size.y = vol_size.y + ex_size.y*2;
		volx_size.z = vol_size.z + ex_size.z*2;
	
		if (progress) progress->Init();
	
		vmint3 blk_idx = vmint3(0, 0, 0);
		for(int z = 0; z < vol_size.z; z += blk_size.z, blk_idx.z++)
		{
			if (progress) progress->SetProgress((double)z, (double)vol_size.z);
	
			blk_idx.y = 0;
			for(int y = 0; y < vol_size.y; y += blk_size.y, blk_idx.y++)
			{
				blk_idx.x = 0;
				for(int x = 0; x < vol_size.x; x += blk_size.x, blk_idx.x++)
				{
	
					double dMin = minInit;		// Min
					double dMax = maxInit;		// Max
	
					vmint3 i3Boundary = blk_size;
					if(vol_size.x < x + blk_size.x)
						i3Boundary.x = modX;
					if(vol_size.y < y + blk_size.y)
						i3Boundary.y = modY;
					if(vol_size.z < z + blk_size.z)
						i3Boundary.z = modZ;
	
	#define CONSIDERNEARBYSAMPLE
	#ifdef CONSIDERNEARBYSAMPLE
					vmint3 i3StartIndex = vmint3(0, 0, 0);
					vmint3 i3EndIndex = i3Boundary;
					if(blk_idx.x > 0)
						i3StartIndex.x = -1;
					if(blk_idx.y > 0)
						i3StartIndex.y = -1;
					if(blk_idx.z > 0)
						i3StartIndex.z = -1;
					if(blk_idx.x == blockNumX - 1)
						i3EndIndex.x += -1;
					if(blk_idx.y == blockNumY - 1)
						i3EndIndex.y += -1;
					if(blk_idx.z == blockNumZ - 1)
						i3EndIndex.z += -1;
					for(int iSubZ = i3StartIndex.z; iSubZ <= i3EndIndex.z; iSubZ++)
					{
						for(int iSubY = i3StartIndex.y; iSubY <= i3EndIndex.y; iSubY++)
						{
							for(int iSubX = i3StartIndex.x; iSubX <= i3EndIndex.x; iSubX++)
							{
								double dDensity = (double)((TVox**)vol_slices)[z + iSubZ + ex_size.z]
								[(y + iSubY + ex_size.y)*volx_size.x + x + iSubX + ex_size.x];
	
								if(dDensity < dMin)
									dMin = dDensity;
								if(dDensity > dMax)
									dMax = dDensity;
							}
						}
					}
	#else
					for(int iSubZ = 0; iSubZ < i3Boundary.z; iSubZ++)
					{
						for(int iSubY = 0; iSubY < i3Boundary.y; iSubY++)
						{
							for(int iSubX = 0; iSubX < i3Boundary.x; iSubX++)
							{
								double dDensity = (double)((TVox**)vol_slices)[z + iSubZ + ex_size.z]
								[(y + iSubY + ex_size.y)*volx_size.x + x + iSubX + ex_size.x];
	
								if(dDensity < dMin)
									dMin = dDensity;
								if(dDensity > dMax)
									dMax = dDensity;
							}
						}
					}
	#endif
	
					int iBlockIdxX = x / blk_size.x;
					int iBlockIdxY = y / blk_size.y;
					int iBlockIdxZ = z / blk_size.z;
					int iBlockID = (iBlockIdxZ + blk_bnd_size.z)*blkNumAllX*blkNumAllY 
						+ (iBlockIdxY + blk_bnd_size.y)*blkNumAllX + iBlockIdxX + blk_bnd_size.x;
	
					TBox c2MinMax;
					c2MinMax.x = (TVox)dMin;
					c2MinMax.y = (TVox)dMax;
					((TBox*)vol_blks->mM_blks)[iBlockID] = c2MinMax;
				}
			}
		}

		if (progress) progress->Deinit();
	
		return;
	}
	
	bool VmVObjectVolume::UpdateVolumeMinMaxBlocks(LocalProgress* progress, const vmint3 blk_size2[2])
	{
		//printf("%d: Trying to Update Block Structures...\n", this->GetObjectID());
		vmint3 new_blk_size2[2];
		bool is_regen = false;
		if (voavol_res->vol_blks[0].mM_blks != NULL && voavol_res->vol_blks[1].mM_blks != NULL)
		{
			if (blk_size2 != NULL &&
				(blk_size2[0] != voavol_res->vol_blks[0].unitblk_size
				|| blk_size2[1] != voavol_res->vol_blks[1].unitblk_size))
				{
					is_regen = true;
					new_blk_size2[0] = blk_size2[0];
					new_blk_size2[1] = blk_size2[1];
				}
			else
			{
				//if (m_pVAVOVolume->stVolArchiveInfoWhenGeneratingBlock.d2ActualMinMaxValue != m_pVAVOVolume->VolumeData.d2ActualMinMaxValue
				//	|| m_pVAVOVolume->stVolArchiveInfoWhenGeneratingBlock.d2MinMaxValue != m_pVAVOVolume->VolumeData.d2MinMaxValue
				//	|| m_pVAVOVolume->stVolArchiveInfoWhenGeneratingBlock.f3VoxelPitch != m_pVAVOVolume->VolumeData.f3VoxelPitch
				//	|| m_pVAVOVolume->stVolArchiveInfoWhenGeneratingBlock.i3VolumeSize != m_pVAVOVolume->VolumeData.i3VolumeSize
				//	|| m_pVAVOVolume->stVolArchiveInfoWhenGeneratingBlock.ppvVolumeSlices != m_pVAVOVolume->VolumeData.ppvVolumeSlices)
				{
					is_regen = true;
					if (blk_size2 != NULL)
					{
						new_blk_size2[0] = blk_size2[0];
						new_blk_size2[1] = blk_size2[1];
					}
					else
					{
						VolumeBlocks::ComputeOctreeBlockSize(voavol_res->vol_data.vol_size, &new_blk_size2[0], &new_blk_size2[1]);
					}
				}
			}
		}
		else 
		{
			is_regen = true;
	
			if (blk_size2 != NULL)
			{
				new_blk_size2[0] = blk_size2[0];
				new_blk_size2[1] = blk_size2[1];
			}
			else
			{
				VolumeBlocks::ComputeOctreeBlockSize(voavol_res->vol_data.vol_size, &new_blk_size2[0], &new_blk_size2[1]);
			}
		}
	
		if (is_regen)
		{
			double range_old;
			if (progress)
			{
				range_old = progress->range;
				progress->range = range_old * 0.8;
				progress->Init();
			}
	
			// 재활용?!?!?!?!?!?
			voavol_res->vol_blks[0].Delete();
			voavol_res->vol_blks[1].Delete();
	
			printf("%d: Updating Block Structures...\n", this->GetObjectID());
	
			VolumeData* vol_data_ptr = this->GetVolumeData();
			for (int iLevel = 0; iLevel < 2; iLevel++)
			{
				double range_old2;
				if (progress)
				{
					range_old2 = progress->range;
					progress->range = range_old2 * 0.5;
					progress->Init();
				}

				if (voavol_res->vol_data.store_dtype.type_name == dtype_char_name)
					_TGenerateVolumeBlocksAndComputeBlockArchive<char, vmchar2>(&voavol_res->vol_blks[iLevel], new_blk_size2[iLevel], *vol_data_ptr, progress);
				else if (voavol_res->vol_data.store_dtype.type_name == dtype_byte_name)
					_TGenerateVolumeBlocksAndComputeBlockArchive<byte, vmbyte2>(&voavol_res->vol_blks[iLevel], new_blk_size2[iLevel], *vol_data_ptr, progress);
				else if (voavol_res->vol_data.store_dtype.type_name == dtype_short_name)
					_TGenerateVolumeBlocksAndComputeBlockArchive<short, vmshort2>(&voavol_res->vol_blks[iLevel], new_blk_size2[iLevel], *vol_data_ptr, progress);
				else if (voavol_res->vol_data.store_dtype.type_name == dtype_ushort_name)
					_TGenerateVolumeBlocksAndComputeBlockArchive<ushort, vmushort2>(&voavol_res->vol_blks[iLevel], new_blk_size2[iLevel], *vol_data_ptr, progress);
				else
				{
					printf("VmVObjectVolume::UpdateVolumeMinMaxBlocks - Type Error");
					return false;
				}

				if (progress)
				{
					progress->Deinit();
					progress->range = range_old2;
				}
			}
	
			if (progress)
			{
				progress->Deinit();
				progress->range = range_old;
			}
			//printf("%d: Finish Updating Block Structures!\n", this->GetObjectID());
		}
	
		return true;
	}
	
	template <typename TBox> void _TUpdateTagBlocks(const vmdouble2& targetMm, VolumeBlocks* vol_blks, byte* tflag_blks, LocalProgress* progress)
	{
		vmint3 blk_vol_size = vol_blks->blk_vol_size;
	
		vmint3 blk_ex_size = vol_blks->blk_bnd_size;
		int blk_volx_size_x = blk_vol_size.x + blk_ex_size.x * 2;
		int blk_volx_size_y = blk_vol_size.y + blk_ex_size.y * 2;
		int blk_volx_size_z = blk_vol_size.z + blk_ex_size.z * 2;
	
		for(int blk_z = 0; blk_z < blk_vol_size.z; blk_z++)
		{
			if(progress)
			{
				double singleLevRange = progress->range;
				*progress->progress_ptr =
					(progress->start + singleLevRange + (double)blk_z/ (double)blk_vol_size.z*singleLevRange);
			}
	
			for (int blk_y = 0; blk_y < blk_vol_size.y; blk_y++)
			{
				for (int blk_x = 0; blk_x < blk_vol_size.x; blk_x++)
				{
					int iBlockID = (blk_z + blk_ex_size.z)*blk_volx_size_x*blk_volx_size_y
						+ (blk_y + blk_ex_size.y)*blk_volx_size_x + blk_x + blk_ex_size.x;
	
					bool bTaggedOTF = ((double)((TBox*)vol_blks->mM_blks)[iBlockID].y < targetMm.x)
						|| ((double)((TBox*)vol_blks->mM_blks)[iBlockID].x > targetMm.y);
	
					if (bTaggedOTF)
					{
						tflag_blks[iBlockID] = 0;
					}
					else
					{
						tflag_blks[iBlockID] = 1;
					}
				}
			}
		}
	}
	
	void VmVObjectVolume::UpdateTagBlocks(const int tobj_id, const int level, const vmdouble2& targetMm, LocalProgress* progress)
	{
		if(level < 0 || level > 1)
		{
			printf("VmVObjectVolume::INVALID BLOCK LEVEL!");
			return;
		}
	
		if(voavol_res->vol_data.vol_slices == NULL
			|| voavol_res->vol_blks[level].mM_blks == NULL
			|| targetMm.x > targetMm.y
			|| oa_res->is_defined == false)
		{
			printf("VmVObjectVolume::UpdateTagBlocks - Needed Resources are NOT Registered!");
			return;
		}
		
		byte* tf_blks = voavol_res->vol_blks[level].GetTaggedActivatedBlocks(tobj_id);
		if (tf_blks == NULL)
		{
			vmint3 blk_vol_size = voavol_res->vol_blks[level].blk_vol_size;
			vmint3 blk_bnd_size = voavol_res->vol_blks[level].blk_bnd_size;
			int blk_volx_size_x = blk_vol_size.x + blk_bnd_size.x * 2;
			int blk_volx_size_y = blk_vol_size.y + blk_bnd_size.y * 2;
			int blk_volx_size_z = blk_vol_size.z + blk_bnd_size.z * 2;
	
			// TaggedBlocks Setting
			tf_blks = new byte[blk_volx_size_x*blk_volx_size_y*blk_volx_size_z];
			memset(tf_blks, 0, sizeof(byte)*blk_volx_size_x*blk_volx_size_y*blk_volx_size_z);
	
			if (!voavol_res->vol_blks[level].ReplaceOrAddTaggedActivatedBlocks(tobj_id, tf_blks))
			{
				printf("VmVObjectVolume::UpdateTagBlocks - INVALID TOBJECT ID!");
				VMSAFE_DELETEARRAY(tf_blks);
				return;
			}
		}
	
		printf("Updating %d-level ActivatingTagBlocks (%d of %d)\n", level, tobj_id, this->GetObjectID());
	
		if(progress)
			*progress->progress_ptr = progress->start;

		if (voavol_res->vol_data.store_dtype.type_name == dtype_char_name)
			_TUpdateTagBlocks<vmchar2>(targetMm, &voavol_res->vol_blks[level], tf_blks, progress);
		else if (voavol_res->vol_data.store_dtype.type_name == dtype_byte_name)
			_TUpdateTagBlocks<vmbyte2>(targetMm, &voavol_res->vol_blks[level], tf_blks, progress);
		else if (voavol_res->vol_data.store_dtype.type_name == dtype_short_name)
			_TUpdateTagBlocks<vmshort2>(targetMm, &voavol_res->vol_blks[level], tf_blks, progress);
		else if (voavol_res->vol_data.store_dtype.type_name == dtype_ushort_name)
			_TUpdateTagBlocks<vmushort2>(targetMm, &voavol_res->vol_blks[level], tf_blks, progress);
		else
		{
			goto ERROR_UpdateTagBlocks;
		}

		if(progress)
			*progress->progress_ptr = progress->start + progress->range;
	
		return;
	
ERROR_UpdateTagBlocks:
		printf("VmVObjectVolume::UpdateTagBlocks - Type Error");
	}
	
	VolumeBlocks* VmVObjectVolume::GetVolumeBlock(int iLevel)
	{
		if(iLevel > 1 || iLevel < 0 || oa_res->is_defined == false)
			return NULL;
	
		if (voavol_res->vol_blks[iLevel].mM_blks == NULL || voavol_res->vol_blks[iLevel].unitblk_size == vmint3(0))
			return NULL;
	
		return &voavol_res->vol_blks[iLevel];
	}
	
	VolumeData* VmVObjectVolume::GetVolumeData()
	{
		if(oa_res->is_defined == false)
			return NULL;
		return &voavol_res->vol_data;
	}
	
#pragma endregion

#pragma region // VmTObject //
	struct TObjectArchive
	{
		TMapData tmap_data;
	};
	
	VmTObject::VmTObject()
	{
		oa_res->ref_object_id = 0;
		toa_res = new TObjectArchive();
	}
	
	VmTObject::~VmTObject()
	{
		toa_res->tmap_data.Delete();
		VMSAFE_DELETE(toa_res);
	}
	
	bool VmTObject::RegisterTMap(const TMapData& tmap_data, int ref_obj_id)
	{
		if(tmap_data.tmap_buffers == NULL || tmap_data.dtype.type_name == "") goto ERROR_RegisterTransferFunctionArchive;
	
		switch(tmap_data.num_dim)
		{
		case 1:
			if(tmap_data.array_lengths.x <= 0
				|| tmap_data.bin_size.x < 0
				|| tmap_data.valid_min_idx.x < 0
				|| tmap_data.valid_max_idx.x >= tmap_data.array_lengths.x
				)
				goto ERROR_RegisterTransferFunctionArchive;
			break;
		case 2:
			if(tmap_data.array_lengths.x <= 0
				|| tmap_data.array_lengths.y <= 0
				|| tmap_data.bin_size.x < 0
				|| tmap_data.bin_size.y < 0
				|| tmap_data.valid_min_idx.x < 0 
				|| tmap_data.valid_min_idx.y < 0
				|| tmap_data.valid_max_idx.x >= tmap_data.array_lengths.x 
				|| tmap_data.valid_max_idx.y >= tmap_data.array_lengths.y
				)
				goto ERROR_RegisterTransferFunctionArchive;
			break;
		case 3:
			if(tmap_data.array_lengths.x <= 0
				|| tmap_data.array_lengths.y <= 0
				|| tmap_data.array_lengths.z <= 0 
				|| tmap_data.bin_size.x < 0
				|| tmap_data.bin_size.y < 0 
				|| tmap_data.bin_size.z < 0
				|| tmap_data.valid_min_idx.x < 0 
				|| tmap_data.valid_min_idx.y < 0 
				|| tmap_data.valid_min_idx.z < 0
				|| tmap_data.valid_max_idx.x >= tmap_data.array_lengths.x 
				|| tmap_data.valid_max_idx.y >= tmap_data.array_lengths.y 
				|| tmap_data.valid_max_idx.z >= tmap_data.array_lengths.z 
				)
				goto ERROR_RegisterTransferFunctionArchive;
			break;
		default:
			goto ERROR_RegisterTransferFunctionArchive;
		}

		toa_res->tmap_data.Delete();
		toa_res->tmap_data = tmap_data;
	
		oa_res->ref_object_id = ref_obj_id;
		oa_res->is_defined = true;
	
		return true;
	
	ERROR_RegisterTransferFunctionArchive:
		printf("VmTObject::RegisterTransferFunctionStructure - Not Available Parameters!");
		return false;
	}
	
	TMapData* VmTObject::GetTMapData()
	{
		if(!oa_res->is_defined)
			return NULL;
		return &toa_res->tmap_data;
	}

	bool VmTObject::CreateTMapBuffer(const int num_dim, const vmint3& dim_length, const data_type& dtype, void*** res_tmap/*out*/)
	{
		if (num_dim <= 0 || num_dim > 3)
		{
			printf("VmTObject::CreateTMapBuffer - UNAVAILABLE INPUT");
			return false;
		}

		int type_size = dtype.type_bytes;
		switch (num_dim)
		{
		case 1:
		case 2:
			if (dim_length.x <= 0 || dim_length.y <= 0)
			{
				printf("VmTObject::CreateTMapBuffer - Type Error 2");
				return false;
			}
			vmhelpers::AllocateVoidPointer2D(res_tmap, dim_length.y, type_size * dim_length.x);
			break;
		case 3:
			if (dim_length.x <= 0 || dim_length.y <= 0 || dim_length.z <= 0)
			{
				printf("VmTObject::CreateTMapBuffer - Type Error 2");
				return false;
			}
			vmhelpers::AllocateVoidPointer2D(res_tmap, dim_length.z, type_size * dim_length.x*dim_length.y);
			break;
		default:
			printf("VmTObject::CreateTMapBuffer - UNAVAILABLE INPUT");
			return false;
		}

		return true;
	}
#pragma endregion

#pragma region // VmCObject //
	// Default Interest Coordinate System is World Space
	// Camera Position : f3PosOriginInGlobalSpace
	// Viewing Direction : -f3VecAxisZInGlobalSpace
	// Up Vector : f3VecAxisYInGlobalSpace
	// Left Vector : f3VecAxisXInGlobalSpace
	struct CObjectArchive
	{
		vmdouble3 pos_cam;
		vmdouble3 view_cam;
		vmdouble3 up_cam;
		vmdouble3 left_cam;
	
		bool is_perspective;
		// Perspective view Param
		double fov_y;	
		// Default view Param W/H
		double aspect_ratio;
		double near_p, far_p;
		vmdouble2 ip_size;
	
		vmint2 pix_size;
	
		vmdouble2 fitting_ip_size;
		double fitting_fov_y;

		// AR.. in this case, is_perspective is forced to be true
		// and fov_y, aspect_ratio, ip_size, fitting_ip_size, fitting_fov_y are not used
		bool is_ar_mode;
		double fx, fy, sc, cx, cy;
	
		// Default
		vmmat44 mat_ws2cs, mat_cs2ps, mat_ps2ss;
		vmmat44 mat_cs2ws, mat_ps2cs, mat_ss2ps;
	};
	
	VmCObject::VmCObject(const AaBbMinMax& aabbMm, const EvmStageViewType stage_vtype, const int w, const int h)
	{
		coa_res = new CObjectArchive();
		// All matrices are already initialized as identity matrix //
		coa_res->is_perspective = false;
	
		coa_res->fov_y = VM_PI / 4.;
		coa_res->pix_size = vmint2(w, h);
		coa_res->aspect_ratio = (double)w/(double)h;
	
		coa_res->near_p = 0;
		coa_res->far_p = 1000.f;
	
		coa_res->ip_size.x = 1.f;
		coa_res->ip_size.y = 1.f / coa_res->aspect_ratio;
	
		coa_res->fitting_ip_size = coa_res->ip_size;
		coa_res->fitting_fov_y = VM_PI / 4.;
		coa_res->is_ar_mode = false;
	
		SetViewStage(&aabbMm, stage_vtype);
	}
	
	VmCObject::~VmCObject()
	{
		VMSAFE_DELETE(coa_res);
	}
	
	void __ComputeMatrix(CObjectArchive& _coa_res)
	{
		vmmath::MatrixWS2CS(&_coa_res.mat_ws2cs, &_coa_res.pos_cam, &_coa_res.up_cam, &_coa_res.view_cam);
	
		if (_coa_res.is_ar_mode)
		{
			double q = _coa_res.far_p / (_coa_res.near_p - _coa_res.far_p);
			double qn = _coa_res.far_p * _coa_res.near_p / (_coa_res.near_p - _coa_res.far_p);

			double fx = _coa_res.fx;
			double fy = _coa_res.fy;
			double sc = _coa_res.sc;
			double cx = _coa_res.cx;
			double cy = _coa_res.cy;
			double width = (double)_coa_res.pix_size.x;
			double height = (double)_coa_res.pix_size.y;
			double x0 = 0, y0 = 0;

			_coa_res.mat_cs2ps[0][0] = 2.0*fx / width;
			_coa_res.mat_cs2ps[1][0] = -2.0*sc / width;
			_coa_res.mat_cs2ps[2][0] = (width + 2.0 * x0 - 2.0*cx) / width;
			_coa_res.mat_cs2ps[3][0] = 0;
			_coa_res.mat_cs2ps[0][1] = 0;
			_coa_res.mat_cs2ps[1][1] = 2.0*fy / height;
			_coa_res.mat_cs2ps[2][1] = -(height + 2.0*y0 - 2.0*cy) / height;
			_coa_res.mat_cs2ps[3][1] = 0;
			_coa_res.mat_cs2ps[0][2] = 0;
			_coa_res.mat_cs2ps[1][2] = 0;
			_coa_res.mat_cs2ps[2][2] = q;
			_coa_res.mat_cs2ps[3][2] = qn;
			_coa_res.mat_cs2ps[0][3] = 0;
			_coa_res.mat_cs2ps[1][3] = 0;
			_coa_res.mat_cs2ps[2][3] = -1.0;
			_coa_res.mat_cs2ps[3][3] = 0;
			_coa_res.mat_cs2ps = glm::transpose(_coa_res.mat_cs2ps);
		}
		else
		{
			if (!_coa_res.is_perspective)
				vmmath::MatrixOrthogonalCS2PS(&_coa_res.mat_cs2ps, _coa_res.ip_size.x, _coa_res.ip_size.y, _coa_res.near_p, _coa_res.far_p);
			else
				vmmath::MatrixPerspectiveCS2PS(&_coa_res.mat_cs2ps, _coa_res.fov_y, _coa_res.aspect_ratio, _coa_res.near_p, _coa_res.far_p);
		}
		vmmath::MatrixPS2SS(&_coa_res.mat_ps2ss, _coa_res.pix_size.x, _coa_res.pix_size.y);
	
		vmmath::MatrixInverse(&_coa_res.mat_cs2ws, &_coa_res.mat_ws2cs);
		vmmath::MatrixInverse(&_coa_res.mat_ps2cs, &_coa_res.mat_cs2ps);
		vmmath::MatrixInverse(&_coa_res.mat_ss2ps, &_coa_res.mat_ps2ss);
	}
	
	void VmCObject::SetViewStage(const AaBbMinMax* aabbMm, const EvmStageViewType stage_vtype)
	{
		vmdouble3 diffmM = aabbMm->pos_max - aabbMm->pos_min;
		double max_len = max(max(diffmM.x, diffmM.y), diffmM.z);
	
		if(coa_res->is_perspective)
		{
			if(coa_res->near_p * 100000 < coa_res->far_p)
			{
				coa_res->near_p = coa_res->far_p * 0.1;
				coa_res->far_p = max(max_len*4., coa_res->near_p * 100000.);
			}
		}
		else
		{
			coa_res->near_p = 0;
			coa_res->far_p = max_len*4.;
			coa_res->far_p = max(max_len*4., 100000.);
		}
	
		switch(stage_vtype)
		{
		case StageViewCENTERFRONT:
			coa_res->pos_cam = (aabbMm->pos_max + aabbMm->pos_min)*0.5;
			coa_res->view_cam = vmdouble3(0, 1., 0);
			coa_res->up_cam = vmdouble3(0, 0, 1.f);
			coa_res->left_cam = vmdouble3(-1., 0, 0);
	
			if(coa_res->aspect_ratio < diffmM.x/diffmM.z)
			{
				coa_res->ip_size.x = diffmM.x;
				coa_res->ip_size.y = coa_res->ip_size.x / coa_res->aspect_ratio;
			}
			else
			{
				coa_res->ip_size.y = diffmM.z;
				coa_res->ip_size.x = coa_res->ip_size.y * coa_res->aspect_ratio;
			}
			coa_res->fitting_ip_size.x = diffmM.x;
			coa_res->fitting_ip_size.y = diffmM.z;
	
			break;
		case StageViewCENTERRIGHT:
			coa_res->pos_cam = (aabbMm->pos_max + aabbMm->pos_min)*0.5;
			coa_res->view_cam = vmdouble3(-1., 0, 0);
			coa_res->up_cam = vmdouble3(0, 0, 1.);
			coa_res->left_cam = vmdouble3(0, -1., 0);
	
			if(coa_res->aspect_ratio < diffmM.y/diffmM.z)
			{
				coa_res->ip_size.x = diffmM.y;
				coa_res->ip_size.y = coa_res->ip_size.x/coa_res->aspect_ratio;
			}
			else
			{
				coa_res->ip_size.y = diffmM.z;
				coa_res->ip_size.x = coa_res->ip_size.y*coa_res->aspect_ratio;
			}
			coa_res->fitting_ip_size.x = diffmM.y;
			coa_res->fitting_ip_size.y = diffmM.z;
	
			break;
		case StageViewCENTERHORIZON:
			coa_res->pos_cam = (aabbMm->pos_max + aabbMm->pos_min)*0.5;
			coa_res->view_cam = vmdouble3(0, 0, -1.);
			coa_res->up_cam = vmdouble3(0, 1., 0);
			coa_res->left_cam = vmdouble3(-1., 0, 0);
	
			if(coa_res->aspect_ratio < diffmM.x/diffmM.y)
			{
				coa_res->ip_size.x = diffmM.x;
				coa_res->ip_size.y = coa_res->ip_size.x/coa_res->aspect_ratio;
			}
			else
			{
				coa_res->ip_size.y = diffmM.y;
				coa_res->ip_size.x = coa_res->ip_size.y*coa_res->aspect_ratio;
			}
			coa_res->fitting_ip_size.x = diffmM.x;
			coa_res->fitting_ip_size.y = diffmM.y;
	
			break;
		case StageViewORTHOBOXOVERVIEW:
			{
				vmdouble3 pos_min(aabbMm->pos_min);
				vmdouble3 pos_max(aabbMm->pos_max);
				vmdouble3 pos_center = (pos_max + pos_min)*0.5;
				double max_len = max(max(pos_max.x - pos_min.x, pos_max.y - pos_min.y), pos_max.z - pos_min.z);
				double default_len = max_len*0.5 * 0.8;	// 0.8 for initial zoom-in
	
				vmdouble3 pos_hyper_min(pos_center.x - default_len, pos_center.y - default_len, pos_center.z - default_len);
				vmdouble3 pos_hyper_max(pos_center.x + default_len, pos_center.y + default_len, pos_center.z + default_len);
				vmdouble3 diff_hyper_mM = pos_hyper_max - pos_hyper_min;
				double hyper_diagonal = vmmath::LengthVector(&diff_hyper_mM);
				coa_res->fov_y = VM_PI / 4;
				double dist_center2cam = hyper_diagonal*0.5/tan(coa_res->fov_y*0.5);
				vmdouble3 diff_min2max_unit = diff_hyper_mM/hyper_diagonal;
				diff_min2max_unit.y *= -1.;
				vmdouble3 pos_cam_ov = pos_center + diff_min2max_unit*dist_center2cam;
				vmdouble3 view_ov = -diff_min2max_unit;
				vmdouble3 up_ov = vmdouble3(0, 0, 1);
				vmdouble3 left_ov;
				vmmath::CrossDotVector(&left_ov, &up_ov, &view_ov);
				vmdouble3 f3VecUp;
				vmmath::CrossDotVector(&f3VecUp, &view_ov, &left_ov);
				vmmath::NormalizeVector(&f3VecUp, &f3VecUp);
				vmmath::NormalizeVector(&left_ov, &left_ov);
	
				coa_res->pos_cam = pos_cam_ov;
				coa_res->view_cam = view_ov;
				coa_res->up_cam = f3VecUp;
				coa_res->left_cam = left_ov;
	
				if(coa_res->is_perspective)
				{
					coa_res->far_p = (dist_center2cam + hyper_diagonal) * 2.;
					coa_res->near_p = coa_res->far_p*0.001;
				}
				else
				{
					coa_res->near_p = 0;
					coa_res->far_p = (dist_center2cam + hyper_diagonal) * 2.;
				}
	
				if(coa_res->pix_size.x < coa_res->pix_size.y)
				{
					coa_res->ip_size.x = hyper_diagonal;
					coa_res->ip_size.y = coa_res->ip_size.x/coa_res->aspect_ratio;
				}
				else
				{
					coa_res->ip_size.y = hyper_diagonal;
					coa_res->ip_size.x = coa_res->ip_size.y*coa_res->aspect_ratio;
				}
				coa_res->fitting_ip_size.x = hyper_diagonal;
				coa_res->fitting_ip_size.y = hyper_diagonal;
			}
			break;
		default:
			printf("CVXCObject::SetViewStage - Unavailable Stage View!");
			return;
		}
	
		__ComputeMatrix(*coa_res);
	}
	
	void VmCObject::SetPerspectiveViewingState(const bool is_perspective, const double* fov_y)
	{
		if(fov_y)
		{
			coa_res->fov_y = *fov_y;
		}
	
		coa_res->is_perspective = is_perspective;
	
		if(is_perspective)
			coa_res->near_p = coa_res->far_p*0.0001;
	
		__ComputeMatrix(*coa_res);
	}
	
	bool VmCObject::GetPerspectiveViewingState(double* fov_y)
	{
		if (fov_y) *fov_y = coa_res->fov_y;
		return coa_res->is_perspective;
	}
	
	void VmCObject::SetCameraIntState(const vmdouble2* ipsize_cs, const double* near_p, const double* far_p, const vmint2* wh_ss, vmdouble2* fit_ipsize, const double* fit_fovy)
	{
		coa_res->is_ar_mode = false;

		// 상식적으로는 ipsize_cs 에 대해 ratio 를 재계산하는 것이 맞으나
		// 여기서는 window resize call-back 의 편의성을 위해, wh_ss 에 우선하여 ratio 를 계산한다.
		if(fit_ipsize)
			coa_res->fitting_ip_size = *fit_ipsize;
		if(fit_fovy)
			coa_res->fitting_fov_y = *fit_fovy;
		if(ipsize_cs)// Perspective 면서 pf2ImagePlaneSize값이 있는 경우는 뒤에 SetPerspectiveViewingState를 호출해야 한다. && coa_res->bPerspective == false)
		{
			coa_res->ip_size = *ipsize_cs;
			coa_res->aspect_ratio = coa_res->ip_size.x/coa_res->ip_size.y;
		}
		if(near_p)
			coa_res->near_p = *near_p;
		if(far_p)
			coa_res->far_p = *far_p;

		// correction or recompute
		if (wh_ss)
		{
			coa_res->pix_size = *wh_ss;

			//if (!ipsize_cs)
			{
				coa_res->aspect_ratio = (double)coa_res->pix_size.x / (double)coa_res->pix_size.y;

				vmdouble2 ipsize_old = coa_res->ip_size;
				vmdouble2 ip_size_new;
				ip_size_new.x = coa_res->fitting_ip_size.x;
				ip_size_new.y = coa_res->fitting_ip_size.x / coa_res->aspect_ratio;
				if (ip_size_new.y < coa_res->fitting_ip_size.y)
				{
					ip_size_new.y = coa_res->fitting_ip_size.y;
					ip_size_new.x = coa_res->fitting_ip_size.y * coa_res->aspect_ratio;
				}

				coa_res->ip_size = ip_size_new;
			}
		}
	
		__ComputeMatrix(*coa_res);
	}

	void VmCObject::SetCameraIntStateAR(const double* fx, const double* fy, const double* sc, const double* cx, const double* cy, const double* near_p, const double* far_p, const vmint2* wh_ss)
	{
		coa_res->is_ar_mode = true;
		if (near_p)
			coa_res->near_p = *near_p;
		if (far_p)
			coa_res->far_p = *far_p;
		if (fx)
			coa_res->fx = *fx;
		if (fy)
			coa_res->fy = *fy;
		if (sc)
			coa_res->sc = *sc;
		if (cx)
			coa_res->cx = *cx;
		if (cy)
			coa_res->cy = *cy;
		if (wh_ss)
			coa_res->pix_size = *wh_ss;

		__ComputeMatrix(*coa_res);
	}
	
	void VmCObject::GetCameraIntState(vmdouble2* ipsize_cs, double* near_p, double* far_p, vmint2* wh_ss, vmdouble2* fit_ipsize, double* fit_fovy)
	{
		if (coa_res->is_ar_mode) return;
		if(ipsize_cs)
			*ipsize_cs = coa_res->ip_size;
		if(near_p)
			*near_p = coa_res->near_p;
		if(far_p)
			*far_p = coa_res->far_p;
		if(wh_ss)
			*wh_ss = coa_res->pix_size;
		if(fit_ipsize)
			*fit_ipsize = coa_res->fitting_ip_size;
		if(fit_fovy)
			*fit_fovy = coa_res->fitting_fov_y;
	}

	void VmCObject::GetCameraIntStateAR(double* fx, double* fy, double* sc, double* cx, double* cy, double* near_p, double* far_p)
	{
		if (!coa_res->is_ar_mode) return;

		if (near_p)
			*near_p = coa_res->near_p;
		if (far_p)
			*far_p = coa_res->far_p;
		if (fx)
			*fx = coa_res->fx;
		if (fy)
			*fy = coa_res->fy;
		if (sc)
			*sc = coa_res->sc;
		if (cx)
			*cx = coa_res->cx;
		if (cy)
			*cy = coa_res->cy;
	}

	void VmCObject::SetCameraExtState(const vmdouble3* pos, const vmdouble3* view, const vmdouble3* up)
	{
		if (pos)
			coa_res->pos_cam = *pos;
		if (view)
		{
			coa_res->view_cam = *view;

			if (view->x == 0 && view->y == 0 && view->z == 0)
				vmmath::CrossDotVector(&coa_res->view_cam, &coa_res->left_cam, &coa_res->up_cam);
		}
		if (up)
		{
			coa_res->up_cam = *up;

			vmmath::CrossDotVector(&coa_res->left_cam, up, &coa_res->view_cam);
			vmmath::NormalizeVector(&coa_res->left_cam, &coa_res->left_cam);

			vmmath::CrossDotVector(&coa_res->up_cam, &coa_res->view_cam, &coa_res->left_cam);
			vmmath::NormalizeVector(&coa_res->up_cam, &coa_res->up_cam);
		}

		__ComputeMatrix(*coa_res);
	}
	void VmCObject::SetCameraExtStatef(const vmfloat3* pos, const vmfloat3* view, const vmfloat3* up)
	{
		SetCameraExtState(pos ? &vmdouble3(*pos) : NULL, view ? &vmdouble3(*view) : NULL, up ? &vmdouble3(*up) : NULL);
	}

	void VmCObject::GetCameraExtState(vmdouble3* pos, vmdouble3* view, vmdouble3* up)
	{
		if(pos)
			*pos = coa_res->pos_cam;
	
		if(view)
			*view = coa_res->view_cam;
	
		if(up)
			*up = coa_res->up_cam;
	}
	void VmCObject::GetCameraExtStatef(vmfloat3* pos, vmfloat3* view, vmfloat3* up)
	{
		vmdouble3 _pos, _view, _up;
		GetCameraExtState(&_pos, &_view, &_up);
		if (pos)*pos = _pos;
		if (view)*view = _view;
		if (up)*up = _up;
	}
	
	void VmCObject::SetZoomfactorFovY(const double fitting_fov_y)
	{
		coa_res->fitting_fov_y = fitting_fov_y;
	}
	
	double VmCObject::GetZoomfactorFovY()
	{
		return coa_res->fitting_fov_y;
	}
	
	bool VmCObject::IsPerspective()
	{
		return coa_res->is_perspective;
	}
	
	void VmCObject::GetMatrixWStoSS(vmmat44* mat_ws2cs, vmmat44* mat_cs2ps, vmmat44* mat_ps2ss)
	{
		if(mat_ws2cs)
			*mat_ws2cs = coa_res->mat_ws2cs;
		if(mat_cs2ps)
			*mat_cs2ps = coa_res->mat_cs2ps;
		if(mat_ps2ss)
			*mat_ps2ss = coa_res->mat_ps2ss;
	}
	
	void VmCObject::GetMatrixSStoWS(vmmat44* mat_ss2ps, vmmat44* mat_ps2cs, vmmat44* mat_cs2ws)
	{
		if(mat_ss2ps)
			*mat_ss2ps = coa_res->mat_ss2ps;
		if(mat_ps2cs)
			*mat_ps2cs = coa_res->mat_ps2cs;
		if(mat_cs2ws)
			*mat_cs2ws = coa_res->mat_cs2ws;
	}
	
	void VmCObject::GetImagePlaneRectPoints(const EvmCoordSpace coord_space, vmdouble3* pos_tl, vmdouble3* pos_tr, vmdouble3* pos_bl, vmdouble3* pos_br)
	{
		// PS //
		vmdouble3 pos_tl_ps(-1., 1., 0);
		vmdouble3 pos_tr_ps(1., 1., 0);
		vmdouble3 pos_bl_ps(-1., -1., 0);
		vmdouble3 pos_br_ps(1., -1., 0);
	
		vmmat44 mat_ps2dsts = vmmat44(); // glm::mat4() and glm::mat4(1.0) build identity matrices.
		switch(coord_space)
		{
		case CoordSpaceWORLD:
			mat_ps2dsts = coa_res->mat_ps2cs * coa_res->mat_cs2ws;
			break;
		case CoordSpaceCAMERA:
			mat_ps2dsts = coa_res->mat_ps2cs;
			break;
		case CoordSpacePROJECTION:
			break;
		case CoordSpaceSCREEN:
			mat_ps2dsts = coa_res->mat_ps2ss;
			break;
		case CoordSpaceOBJECT:
		default:
			printf("VmCObject::GetImagePlaneRectPoints - NOT AVAILABLE SPACE");
			return;
		}
	
		if(pos_tl)
			vmmath::TransformPoint(pos_tl, &pos_tl_ps, &mat_ps2dsts);
		if(pos_tr)
			vmmath::TransformPoint(pos_tr, &pos_tr_ps, &mat_ps2dsts);
		if(pos_bl)
			vmmath::TransformPoint(pos_bl, &pos_bl_ps, &mat_ps2dsts);
		if(pos_br)
			vmmath::TransformPoint(pos_br, &pos_br_ps, &mat_ps2dsts);
	}
	void VmCObject::GetImagePlaneRectPointsf(const EvmCoordSpace coord_space, vmfloat3* pos_tl, vmfloat3* pos_tr, vmfloat3* pos_bl, vmfloat3* pos_br)
	{
		vmdouble3 _pos_tl, _pos_tr, _pos_bl, _pos_br;
		GetImagePlaneRectPoints(coord_space, &_pos_tl, &_pos_tr, &_pos_bl, &_pos_br);
		if (pos_tl)*pos_tl = _pos_tl;
		if (pos_tr)*pos_tr = _pos_tr;
		if (pos_bl)*pos_bl = _pos_bl;
		if (pos_br)*pos_br = _pos_br;
	}
#pragma endregion

#pragma region // VmIObject //
	struct IObjectArchive
	{
		VmCObject* cam_obj;
		int w, h;
	
		vector<FrameBuffer> render_buffers;
		vector<FrameBuffer> depth_Buffers;
		vector<FrameBuffer> custom_buffers;
		vector<FrameBuffer> aligned_buffers;
		vector<FrameBuffer> virtual_buffers;

		map<string, int> dtypes_bytes;
	};
	
	VmIObject::VmIObject(int w, int h)
	{
		ioa_res = new IObjectArchive();
	
		if(w > 0 && h > 0)
			oa_res->is_defined = true;
		else
			w = h = 0;
		
		ioa_res->cam_obj = NULL;
	
		// Initial State Setting
		ioa_res->w = max(w, 0);
		ioa_res->h = max(h, 0);
	}
	
	VmIObject::~VmIObject()
	{
		int num_buffers = (int)ioa_res->render_buffers.size();
		for(int i = 0; i < num_buffers; i++)
		{
			ioa_res->render_buffers[i].Delete();
		}
		ioa_res->render_buffers.clear();
	
		num_buffers = (int)ioa_res->depth_Buffers.size();
		for(int i = 0; i < num_buffers; i++)
		{
			ioa_res->depth_Buffers[i].Delete();
		}
		ioa_res->depth_Buffers.clear();
	
		num_buffers = (int)ioa_res->custom_buffers.size();
		for(int i = 0; i < num_buffers; i++)
		{
			ioa_res->custom_buffers[i].Delete();
		}
		ioa_res->custom_buffers.clear();
	
		num_buffers = (int)ioa_res->aligned_buffers.size();
		for (int i = 0; i < num_buffers; i++)
		{
			ioa_res->aligned_buffers[i].Delete();
		}
	
		num_buffers = (int)ioa_res->virtual_buffers.size();
		for (int i = 0; i < num_buffers; i++)
		{
			ioa_res->virtual_buffers[i].Delete();
		}
		ioa_res->virtual_buffers.clear();
	
		VMSAFE_DELETE(ioa_res->cam_obj);
		VMSAFE_DELETE(ioa_res);
	}
	
	void VmIObject::AttachCamera(const AaBbMinMax& aabbMm, const EvmStageViewType stage_vtype)
	{
		if(ioa_res->cam_obj == NULL)
			ioa_res->cam_obj = new VmCObject(aabbMm, stage_vtype, ioa_res->w, ioa_res->h);
	}
	
	VmCObject* VmIObject::GetCameraObject()
	{
		return ioa_res->cam_obj;
	}
	
	vector<FrameBuffer>* VmIObject::GetBufferPointerList(const EvmFrameBufferUsage fb_usage)
	{
		switch (fb_usage)
		{
		case FrameBufferUsageRENDEROUT:
			return &ioa_res->render_buffers;
		case FrameBufferUsageDEPTH:
			return &ioa_res->depth_Buffers;
		case FrameBufferUsageCUSTOM:
			return &ioa_res->custom_buffers;
		case FrameBufferUsageALIGNEDSTURCTURE:
			return &ioa_res->aligned_buffers;
		case FrameBufferUsageVIRTUAL:
			return &ioa_res->virtual_buffers;
		default:
			break;
		}
		return NULL;
	}
	
	void VmIObject::ResizeFrameBuffer(const int w, const int h)
	{
		if(w <= 0 || h <= 0)
		{
			printf("VmIObject::ResizeFrameBuffer - Must be initialized with valid size of framebuffer");
			return;
		}
		
		if (ioa_res->w != w || ioa_res->h != h)
		{
			int num_pixs = w*h;
			int num_render_bufs = (int)ioa_res->render_buffers.size();
			for (int i = 0; i < num_render_bufs; i++)
			{
				// BYTE4
				ioa_res->render_buffers[i].Delete();
	#ifdef __WINDOWS
	#ifdef __FILEMAP
				ioa_res->render_buffers[i].hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4 * num_pixs, NULL);
				ioa_res->render_buffers[i].fbuffer = (vmbyte4*)MapViewOfFile(ioa_res->render_buffers[i].hFileMap, FILE_MAP_WRITE, 0, 0, sizeof(vmbyte4)*num_pixs);
	#else
				ioa_res->render_buffers[i].fbuffer = new vmbyte4[num_pixs];
	#endif
	#endif
				ioa_res->render_buffers[i].w = w;
				ioa_res->render_buffers[i].h = h;
			}
	
			int num_depth_bufs = (int)ioa_res->depth_Buffers.size();
			for(int i = 0; i < num_depth_bufs; i++)
			{
				ioa_res->depth_Buffers[i].Delete();
				ioa_res->depth_Buffers[i].w = w;
				ioa_res->depth_Buffers[i].h = h;
				ioa_res->depth_Buffers[i].fbuffer = new byte[num_pixs * ioa_res->depth_Buffers[i].dtype.type_bytes];
			}
	
			int num_custom_bufs = (int)ioa_res->custom_buffers.size();
			for (int i = 0; i < num_custom_bufs; i++)
			{
				ioa_res->custom_buffers[i].Delete();
				ioa_res->custom_buffers[i].w = w;
				ioa_res->custom_buffers[i].h = h;
				ioa_res->custom_buffers[i].fbuffer = new byte[num_pixs * ioa_res->custom_buffers[i].dtype.type_bytes];
			}
	
			int num_aligned_bufs = (int)ioa_res->aligned_buffers.size();
			for (int i = 0; i < num_aligned_bufs; i++)
			{
				ioa_res->aligned_buffers[i].Delete();
				ioa_res->aligned_buffers[i].w = w;
				ioa_res->aligned_buffers[i].h = h;
				ioa_res->aligned_buffers[i].fbuffer = _aligned_malloc(ioa_res->aligned_buffers[i].dtype.type_bytes * num_pixs, 16);
			}
	
			int num_virtual_bufs = (int)ioa_res->virtual_buffers.size();
			for (int i = 0; i < num_virtual_bufs; i++)
			{
				ioa_res->virtual_buffers[i].Delete();
				ioa_res->virtual_buffers[i].w = w;
				ioa_res->virtual_buffers[i].h = h;
				ioa_res->virtual_buffers[i].fbuffer = NULL;
			}
		}
	
		if(ioa_res->cam_obj)
		{
			// Fixed (100% 상태의 W, H 를 구하는 루틴으로 들어간다.)
			// 그리고 ImagePlane 의 사이즈가 Default 가 된다.
			ioa_res->cam_obj->SetCameraIntState(NULL, NULL, NULL, &vmint2(w, h));
			// 같은 맥락에서 이해.
			double fovY = ioa_res->cam_obj->GetZoomfactorFovY();
			ioa_res->cam_obj->SetPerspectiveViewingState(ioa_res->cam_obj->IsPerspective(), &fovY);
		}
	
		ioa_res->w = w;
		ioa_res->h = h;
		oa_res->is_defined = true;
	}
	
	void VmIObject::GetFrameBufferInfo(vmint2* buffer_size/*out*/, int* num_buffers/*out*/, int* bytes_per_pixel/*out*/)
	{
		if(buffer_size)
		{
			buffer_size->x = ioa_res->w;
			buffer_size->y = ioa_res->h;
		}
	
		if(num_buffers || bytes_per_pixel)
		{
			int num_bufs = 0;
			int num_out_bufs = 0;
			int _bytes_per_pixel = 0;
			int iNumRenderBuffers = (int)ioa_res->render_buffers.size();
			num_bufs += iNumRenderBuffers;
			for(int i = 0; i < iNumRenderBuffers; i++)
			{
				_bytes_per_pixel += (int)ioa_res->render_buffers[i].dtype.type_bytes;
				if(ioa_res->render_buffers[i].buffer_usage == FrameBufferUsageRENDEROUT)
					num_out_bufs++;
			}
	
			int iNumDepthBuffers = (int)ioa_res->depth_Buffers.size();
			num_bufs += iNumDepthBuffers;
			for(int i = 0; i < iNumDepthBuffers; i++)
				_bytes_per_pixel += (int)ioa_res->depth_Buffers[i].dtype.type_bytes;
	
			int iNumCustomBuffers = (int)ioa_res->custom_buffers.size();
			num_bufs += iNumCustomBuffers;
			for(int i = 0; i < iNumCustomBuffers; i++)
				_bytes_per_pixel += (int)ioa_res->custom_buffers[i].dtype.type_bytes;
	
			int iNumAlignedBuffers = (int)ioa_res->aligned_buffers.size();
			num_bufs += iNumAlignedBuffers;
			for (int i = 0; i < iNumAlignedBuffers; i++)
				_bytes_per_pixel += (int)ioa_res->aligned_buffers[i].dtype.type_bytes;
	
			int iNumVirtualBuffers = (int)ioa_res->virtual_buffers.size();
			num_bufs += iNumVirtualBuffers;
	
			if(num_buffers)
				*num_buffers = num_bufs;
			if(bytes_per_pixel)
				*bytes_per_pixel = _bytes_per_pixel;
		}
	}
	
	FrameBuffer* VmIObject::GetFrameBuffer(const EvmFrameBufferUsage fb_usage, const int buffer_idx)
	{
		if(!oa_res->is_defined)
			return NULL;
	
		switch(fb_usage)
		{
		case FrameBufferUsageRENDEROUT:
			if(ioa_res->render_buffers.size() <= buffer_idx)
				return NULL;
			return &(ioa_res->render_buffers[buffer_idx]);
		case FrameBufferUsageDEPTH:
			if(ioa_res->depth_Buffers.size() <= buffer_idx)
				return NULL;
			return &(ioa_res->depth_Buffers[buffer_idx]);
		case FrameBufferUsageCUSTOM:
			if(ioa_res->custom_buffers.size() <= buffer_idx)
				return NULL;
			return &(ioa_res->custom_buffers[buffer_idx]);
		case FrameBufferUsageALIGNEDSTURCTURE:
			if (ioa_res->aligned_buffers.size() <= buffer_idx)
				return NULL;
			return &(ioa_res->aligned_buffers[buffer_idx]);
		case FrameBufferUsageVIRTUAL:
			if (ioa_res->virtual_buffers.size() <= buffer_idx)
				return NULL;
			return &(ioa_res->virtual_buffers[buffer_idx]);
		default:
			break;
		}
	
		return NULL;
	}
	
	void VmIObject::InsertFrameBuffer(const data_type& dtype, const EvmFrameBufferUsage fb_usage, const std::string& descriptor)
	{
		if (ioa_res->w <= 0 || ioa_res->h <= 0 || oa_res->is_defined == false
			|| (fb_usage == FrameBufferUsageRENDEROUT && dtype.type_name != dtype_byte4_name))
		{
			printf("VmIObject::InsertFrameBuffer - NOT AVAILABLE FRAME BUFFER");
			return;
		}
	
		FrameBuffer fbuffer_info;
		fbuffer_info.w = ioa_res->w;
		fbuffer_info.h = ioa_res->h;
		fbuffer_info.descriptor = descriptor;
		fbuffer_info.buffer_usage = fb_usage;
		fbuffer_info.dtype = dtype;
	
		int num_pixs = ioa_res->w*ioa_res->h;
		switch(fb_usage)
		{
		case FrameBufferUsageRENDEROUT:
	#ifdef __WINDOWS
	#ifdef __FILEMAP
			fbuffer_info.hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4*num_pixs, NULL);
			fbuffer_info.fbuffer = (vmbyte4*)MapViewOfFile(fbuffer_info.hFileMap, FILE_MAP_WRITE, 0, 0, sizeof(vmbyte4)*num_pixs);
	#else
			fbuffer_info.fbuffer = new vmbyte4[num_pixs];
	#endif
	#endif
			ioa_res->render_buffers.push_back(fbuffer_info);
			break;
		case FrameBufferUsageDEPTH:
			fbuffer_info.fbuffer = new byte[num_pixs * dtype.type_bytes];
			ioa_res->depth_Buffers.push_back(fbuffer_info);
			break;
		case FrameBufferUsageCUSTOM:
			fbuffer_info.fbuffer = new byte[num_pixs * dtype.type_bytes];
			ioa_res->custom_buffers.push_back(fbuffer_info);
			break;
		case FrameBufferUsageALIGNEDSTURCTURE:
			fbuffer_info.fbuffer = _aligned_malloc(dtype.type_bytes * num_pixs, 16);
			ioa_res->aligned_buffers.push_back(fbuffer_info);
			break;
		case FrameBufferUsageVIRTUAL:
			fbuffer_info.fbuffer = NULL;
			ioa_res->virtual_buffers.push_back(fbuffer_info);
			break;
		default:
			printf("VmIObject::InsertFrameBuffer - NOT AVAILABLE BUFFER USAGE");
			return;
		}
		return;
	}
	
	bool VmIObject::ReplaceFrameBuffer(const EvmFrameBufferUsage fb_usage, const int buffer_idx, const data_type& dtype, const std::string& descriptor)
	{
		if (ioa_res->w <= 0 || ioa_res->h <= 0 || oa_res->is_defined == false 
			|| (fb_usage == FrameBufferUsageRENDEROUT && dtype.type_name != dtype_byte4_name))
		{
			printf("VmIObject::ReplaceFrameBuffer - NOT AVAILABLE FRAME BUFFER");
			return false;
		}
	
		int num_pixs = ioa_res->w * ioa_res->h;
		FrameBuffer* fbuffer_info_ptr = NULL;
		switch(fb_usage)
		{
		case FrameBufferUsageRENDEROUT:
			if(ioa_res->render_buffers.size() <= buffer_idx)
				return false;
	
			fbuffer_info_ptr = &ioa_res->render_buffers[buffer_idx];
			if(fbuffer_info_ptr->dtype.type_bytes == dtype.type_bytes)
				return true;
	
			fbuffer_info_ptr->Delete();
	#ifdef __WINDOWS
	#ifdef __FILEMAP
			fbuffer_info_ptr->hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4*num_pixs, NULL);
			fbuffer_info_ptr->fbuffer = (vmbyte4*)MapViewOfFile(fbuffer_info_ptr->hFileMap, FILE_MAP_WRITE, 0, 0, sizeof(vmbyte4)*num_pixs);
	#else
			fbuffer_info_ptr->fbuffer = new vmbyte4[num_pixs];
	#endif
	#endif
			break;
		case FrameBufferUsageDEPTH:
			if(ioa_res->depth_Buffers.size() <= buffer_idx)
				return false;
	
			fbuffer_info_ptr = &ioa_res->depth_Buffers[buffer_idx];
			if(fbuffer_info_ptr->dtype.type_bytes == dtype.type_bytes)
				return true;
			
			fbuffer_info_ptr->Delete();
			fbuffer_info_ptr->fbuffer = new byte[dtype.type_bytes * num_pixs];
			break;
		case FrameBufferUsageCUSTOM:
			if(ioa_res->custom_buffers.size() <= buffer_idx)
				return false;
	
			fbuffer_info_ptr = &ioa_res->custom_buffers[buffer_idx];
			if (fbuffer_info_ptr->dtype.type_bytes == dtype.type_bytes)
				return true;
	
			fbuffer_info_ptr->Delete();
			fbuffer_info_ptr->fbuffer = new byte[dtype.type_bytes * num_pixs];
			break;
		case FrameBufferUsageALIGNEDSTURCTURE:
			if (ioa_res->aligned_buffers.size() <= buffer_idx)
				return false;
	
			fbuffer_info_ptr = &ioa_res->aligned_buffers[buffer_idx];
			if (fbuffer_info_ptr->dtype.type_bytes == dtype.type_bytes)
				return true;
	
			fbuffer_info_ptr->Delete();
			fbuffer_info_ptr->fbuffer = _aligned_malloc(dtype.type_bytes * num_pixs, 16);
			if (fbuffer_info_ptr->fbuffer == NULL)
				return false;
			break;
		case FrameBufferUsageVIRTUAL:
			if (ioa_res->virtual_buffers.size() <= buffer_idx)
				return false;
			fbuffer_info_ptr = &ioa_res->virtual_buffers[buffer_idx];
	
			if (fbuffer_info_ptr->dtype.type_bytes == dtype.type_bytes)
				return true;
			fbuffer_info_ptr->Delete();
			fbuffer_info_ptr->fbuffer = NULL;
			break;
		default:
			return false;
		}
	
		fbuffer_info_ptr->w = ioa_res->w;
		fbuffer_info_ptr->h = ioa_res->h;
		fbuffer_info_ptr->descriptor = descriptor;
		fbuffer_info_ptr->buffer_usage = fb_usage;
		fbuffer_info_ptr->dtype = dtype;
		return true;
	}
	
	bool VmIObject::DeleteFrameBuffer(const EvmFrameBufferUsage fb_usage, const int buffer_idx)
	{
		switch(fb_usage)
		{
		case FrameBufferUsageRENDEROUT:
			if(ioa_res->render_buffers.size() <= buffer_idx)
				return false;
			ioa_res->render_buffers[buffer_idx].Delete();
			ioa_res->render_buffers.erase(ioa_res->render_buffers.begin() + buffer_idx);
			return true;
		case FrameBufferUsageDEPTH:
			if(ioa_res->depth_Buffers.size() <= buffer_idx)
				return false;
			ioa_res->depth_Buffers[buffer_idx].Delete();
			ioa_res->depth_Buffers.erase(ioa_res->depth_Buffers.begin() + buffer_idx);
			return true;
		case FrameBufferUsageCUSTOM:
			if (ioa_res->custom_buffers.size() <= buffer_idx)
				return false;
			ioa_res->custom_buffers[buffer_idx].Delete();
			ioa_res->custom_buffers.erase(ioa_res->custom_buffers.begin() + buffer_idx);
			return true;
		case FrameBufferUsageALIGNEDSTURCTURE:
			if (ioa_res->aligned_buffers.size() <= buffer_idx)
				return false;
			ioa_res->aligned_buffers[buffer_idx].Delete();
			ioa_res->aligned_buffers.erase(ioa_res->aligned_buffers.begin() + buffer_idx);
			return true;
		case FrameBufferUsageVIRTUAL:
			if (ioa_res->virtual_buffers.size() <= buffer_idx)
				return false;
			ioa_res->virtual_buffers[buffer_idx].Delete();
			ioa_res->virtual_buffers.erase(ioa_res->virtual_buffers.begin() + buffer_idx);
			return true;
		default:
			break;
		}
	
		return false;
	}
#pragma endregion
	
#pragma region // VmVObjectPrimitive //
	struct VObjectPrimitiveArchive
	{
		PrimitiveData prim_data;
		bool is_wireframe;
		vmdouble4 color;

		PointCloud<float, vmfloat3>* pc;
		kd_tree_t* kdt;
		int num_kdt_updated;
	
		VObjectPrimitiveArchive()
		{
			pc = NULL;
			kdt = NULL;
			is_wireframe = false;
			num_kdt_updated = 0;
			color = vmdouble4(0, 0, 0, 1.f);
		}
	};
	
	VmVObjectPrimitive::VmVObjectPrimitive()
	{
		voaprim_res = new VObjectPrimitiveArchive();
		oa_res->ref_object_id = 0;
	}
	
	VmVObjectPrimitive::~VmVObjectPrimitive()
	{
		voaprim_res->prim_data.Delete();
		VMSAFE_DELETE(voaprim_res->kdt);
		VMSAFE_DELETE(voaprim_res->pc);
		VMSAFE_DELETE(voaprim_res);
	}
	
	bool VmVObjectPrimitive::RegisterPrimitiveData(const PrimitiveData& prim_data, LocalProgress* progress)
	{
		if(prim_data.num_prims == 0 || prim_data.ptype == PrimitiveTypeUNDEFINED
			|| prim_data.idx_stride < 1 || prim_data.GetNumVertexDefinitions() == 0)
		{
			printf("VmVObjectPrimitive::RegisterPrimitiveData - NOT AVAILABLE PRIMITIVE INFO\n");
			return false;
		}
	
		PrimitiveData prim_data_tmp = prim_data;
		if(!prim_data.aabb_os.IsAvailableBox())
		{
			vmdouble3 diff_mM = prim_data.aabb_os.pos_max - prim_data.aabb_os.pos_min;
			double len = 0;
			if((len = vmmath::LengthVector(&diff_mM)) < DBL_EPSILON)
				len = 1.;
	
			if(diff_mM.x < DBL_EPSILON)
			{
				prim_data_tmp.aabb_os.pos_min.x -= len * 0.001;
				prim_data_tmp.aabb_os.pos_max.x += len * 0.001;
			}
			if(diff_mM.y < DBL_EPSILON)
			{
				prim_data_tmp.aabb_os.pos_min.y -= len * 0.001;
				prim_data_tmp.aabb_os.pos_max.y += len * 0.001;
			}
			if(diff_mM.z < DBL_EPSILON)
			{
				prim_data_tmp.aabb_os.pos_min.z -= len * 0.001;
				prim_data_tmp.aabb_os.pos_max.z += len * 0.001;
			}
		}
	
		if(prim_data.ptype != PrimitiveTypePOINT)
			if(prim_data.vidx_buffer == NULL || prim_data.idx_stride <= 1 || prim_data.num_vtx == 0)
				return false;
		
		uint num_vidx = prim_data.num_vidx;
		uint num_prims = prim_data.num_prims;
		uint idx_stride = prim_data.idx_stride;
	
		if(prim_data.ptype != PrimitiveTypePOINT)
		{
			if(prim_data.is_stripe)
			{
				if(num_vidx != num_prims + (idx_stride - 1))
				{
					printf("VmVObjectPrimitive::RegisterPrimitiveData - NOT AVAILABLE PRIMITIVE INFO\n");
					return false;
				}
			}
			else
			{
				if(num_vidx != num_prims*idx_stride)
				{
					printf("VmVObjectPrimitive::RegisterPrimitiveData - NOT AVAILABLE PRIMITIVE INFO\n");
					return false;
				}
			}
		}
		voaprim_res->prim_data.Delete();
		VMSAFE_DELETE(voaprim_res->kdt);
		VMSAFE_DELETE(voaprim_res->pc);
		voaprim_res->num_kdt_updated = 0;
		voaprim_res->prim_data = prim_data_tmp;
		oa_res->is_defined = true;
		oa_res->ref_object_id = 0;
		
		voa_res->isDefinedGeometry = true;
		vmmat44 mat_id;
		voa_res->mat_os2ws = mat_id;
		voa_res->mat_ws2os = mat_id;
		voa_res->fmat_os2ws = voa_res->mat_os2ws;
		voa_res->fmat_ws2os = voa_res->mat_ws2os;
	
		voa_res->aabbMm = prim_data.aabb_os;
		return true;
	}

	
	PrimitiveData* VmVObjectPrimitive::GetPrimitiveData()
	{
		if(!voa_res->isDefinedGeometry) return NULL;
		return &voaprim_res->prim_data;
	}
	
	void VmVObjectPrimitive::SetPrimitiveWireframeVisibilityColor(const bool is_wireframe, const vmdouble4& color)
	{
		voaprim_res->is_wireframe = is_wireframe;
		voaprim_res->color = color;
	}
	
	void VmVObjectPrimitive::GetPrimitiveWireframeVisibilityColor(bool* is_wireframe, vmdouble4* color)
	{
		if (is_wireframe)
			*is_wireframe = voaprim_res->is_wireframe;
		if (color)
			*color = voaprim_res->color;
	}

	bool VmVObjectPrimitive::HasKDTree(int* num_updated)
	{
		if (num_updated) *num_updated = voaprim_res->num_kdt_updated;
		return voaprim_res->kdt != NULL;
	}

	void VmVObjectPrimitive::UpdateKDTree()
	{
		if (voaprim_res->prim_data.num_vtx == 0) return;

		VMSAFE_DELETE(voaprim_res->kdt);
		VMSAFE_DELETE(voaprim_res->pc);

		voaprim_res->pc = new PointCloud<float, vmfloat3>(voaprim_res->prim_data.GetVerticeDefinition("POSITION"), voaprim_res->prim_data.num_vtx);
		voaprim_res->kdt = new kd_tree_t(3, *voaprim_res->pc, nanoflann::KDTreeSingleIndexAdaptorParams(10));
		voaprim_res->kdt->buildIndex();

		voaprim_res->num_kdt_updated++;
	}

	uint VmVObjectPrimitive::KDTSearchRadius(const vmfloat3& p_src, const float r_sq, const bool is_sorted, vector<pair<size_t, float>>& ret_matches)
	{
		if (voaprim_res->kdt == NULL) return 0;

		static nanoflann::SearchParams params;
		params.sorted = is_sorted;

		return (uint)voaprim_res->kdt->radiusSearch((float*)&p_src, r_sq, ret_matches, params);
	}

	uint VmVObjectPrimitive::KDTSearchKnn(const vmfloat3& p_src, const int k, size_t* out_ids, float* out_dists)
	{
		if (voaprim_res->kdt == NULL) return 0;
		return (uint)voaprim_res->kdt->knnSearch((float*)&p_src, k, out_ids, out_dists, 10);
	}
#pragma endregion
	
#pragma region // VmLObject //
	struct _lobj_buffer {
		void* buffer_ptr;
		size_t type_bytes;
		int num_elements;
	};
	struct _lobj_dvalue {
		void* dvbuf_ptr;
		size_t dvalue_bytes;
	};
	struct _lobj_string {
		string* str_buf_ptr;
		int num_elements;
	};
	struct LObjectArchive
	{
		map< string, _lobj_buffer > map_buffers;
		map< string, _lobj_string > map_str_buffers;
		map< int, std::map<std::string, _lobj_dvalue> > map_dstobj_values;
	};
	
	VmLObject::VmLObject()
	{
		loa_res = new LObjectArchive();
	}
	
	VmLObject::~VmLObject()
	{
		ClearAllBuffers(); 
		ClearAllDstObjValues();
	
		VMSAFE_DELETE(loa_res);
	}
	
	void VmLObject::ClearAllBuffers()
	{
		for (auto it = loa_res->map_buffers.begin(); it != loa_res->map_buffers.end(); it++)
		{
			VMSAFE_DELETEARRAY(it->second.buffer_ptr);
		}
		loa_res->map_buffers.clear();

		for (auto it = loa_res->map_str_buffers.begin(); it != loa_res->map_str_buffers.end(); it++)
		{
			VMSAFE_DELETEARRAY(it->second.str_buf_ptr);
		}
		loa_res->map_str_buffers.clear();
	}
	
	bool VmLObject::RemoveBuffer(const std::string& _key)
	{
		auto it = loa_res->map_buffers.find(_key);
		if (it == loa_res->map_buffers.end()) return false;

		VMSAFE_DELETEARRAY(it->second.buffer_ptr);
		loa_res->map_buffers.erase(it);
		return true;
	}
	
	bool VmLObject::LoadBufferPtr(const std::string& _key, void** buffer_ptr, size_t& size_buffer, int* num_elements)
	{
		auto it = loa_res->map_buffers.find(_key);
		if (it == loa_res->map_buffers.end())
		{
			*buffer_ptr = NULL;
			size_buffer = 0;
			return false;
		}

		*buffer_ptr = it->second.buffer_ptr;
		size_buffer = it->second.type_bytes * (size_t)it->second.num_elements;
		if (num_elements) *num_elements = it->second.num_elements;
		return true;
	}
	
	void VmLObject::ReplaceOrAddBufferPtr(const std::string& _key, const void* buffer_ptr, const int num_elements, const int type_bytes, void* dst_ptr)
	{
		if (num_elements <= 0) return;
		auto it = loa_res->map_buffers.find(_key);
		if (it != loa_res->map_buffers.end())
		{
			if (it->second.buffer_ptr == buffer_ptr)
			{
				printf("WARNNING!! ReplaceOrAddBufferPtr ==> Same buffer pointer is used!");
				return;
			}
			else
			{
				VMSAFE_DELETEARRAY(it->second.buffer_ptr);
				loa_res->map_buffers.erase(it);
			}
		}

		_lobj_buffer buf;
		buf.type_bytes = (size_t)type_bytes;
		buf.num_elements = num_elements;

		buf.buffer_ptr = dst_ptr;
		if(buf.buffer_ptr == NULL) buf.buffer_ptr = new byte[buf.type_bytes * buf.num_elements];
		if(buf.buffer_ptr != buffer_ptr)
			memcpy(buf.buffer_ptr, buffer_ptr, buf.type_bytes * buf.num_elements);
		loa_res->map_buffers[_key] = buf;
	}
	
	size_t VmLObject::GetSizeOfRegisteredBuffers()
	{
		size_t size_dbytes = 0;
		for (auto it = loa_res->map_buffers.begin(); it != loa_res->map_buffers.end(); it++)
			size_dbytes += it->second.type_bytes * it->second.num_elements;
		return size_dbytes;
	}

	// map< int, std::map<std::string, _lobj_buffer> > map_dstobj_values;
	void VmLObject::ClearAllDstObjValues()
	{
		for (auto it = loa_res->map_dstobj_values.begin(); it != loa_res->map_dstobj_values.end(); it++)
		{
			for (auto __it = it->second.begin(); __it != it->second.end(); __it++)
				VMSAFE_DELETEARRAY(__it->second.dvbuf_ptr);
			it->second.clear();
		}
		loa_res->map_dstobj_values.clear();
	}

	bool VmLObject::RemoveDstObjValues(const int dst_obj_id)
	{
		auto it = loa_res->map_dstobj_values.find(dst_obj_id);
		if(it == loa_res->map_dstobj_values.end()) return false;

		for (auto __it = it->second.begin(); __it != it->second.end(); __it++)
			VMSAFE_DELETEARRAY(__it->second.dvbuf_ptr);
		it->second.clear();
		loa_res->map_dstobj_values.erase(it);
		return true;
	}

	bool VmLObject::RemoveDstObjValue(const int dst_obj_id, const std::string& _key)
	{
		auto it = loa_res->map_dstobj_values.find(dst_obj_id);
		if (it == loa_res->map_dstobj_values.end()) return false;
		auto __it = it->second.find(_key);
		if (__it == it->second.end()) return false;

		VMSAFE_DELETEARRAY(__it->second.dvbuf_ptr);
		it->second.erase(__it);
		return true;
	}

	void VmLObject::ReplaceOrAddDstObjValue(const int dst_obj_id, const std::string& _key, const void* v_ptr, const int bytes_dstobj_value, void* dst_ptr)
	{
		auto it = loa_res->map_dstobj_values.find(dst_obj_id);
		if (it != loa_res->map_dstobj_values.end())
		{
			auto __it = it->second.find(_key);
			if (__it != it->second.end())
			{
				if (__it->second.dvbuf_ptr == v_ptr)
				{
					printf("WARNNING!! ReplaceOrAddDstObjValue ==> Same buffer pointer is used!");
					return;
				}
				else
				{
					VMSAFE_DELETEARRAY(__it->second.dvbuf_ptr);
					it->second.erase(__it);
				}
			}
		}
		else // if (it == loa_res->map_dstobj_values.end())
		{
			loa_res->map_dstobj_values.insert(pair<int, map<std::string, _lobj_dvalue>>(dst_obj_id, map<std::string, _lobj_dvalue>()));
			it = loa_res->map_dstobj_values.find(dst_obj_id);
		}
		
		_lobj_dvalue buf;
		buf.dvalue_bytes = bytes_dstobj_value;
		buf.dvbuf_ptr = dst_ptr;
		if(buf.dvbuf_ptr == NULL)
			buf.dvbuf_ptr = new byte[buf.dvalue_bytes];
		if(buf.dvbuf_ptr != v_ptr)
			memcpy(buf.dvbuf_ptr, v_ptr, buf.dvalue_bytes);
		it->second[_key] = buf;
	}

	bool VmLObject::GetDstObjValue(const int dst_obj_id, const std::string& _key, void* v_ptr)
	{
		auto it = loa_res->map_dstobj_values.find(dst_obj_id);
		if (it == loa_res->map_dstobj_values.end()) return false;
		auto __it = it->second.find(_key);
		if (__it == it->second.end()) return false;
		memcpy(v_ptr, __it->second.dvbuf_ptr, __it->second.dvalue_bytes);
		return true;
	}

	bool VmLObject::GetDstObjValueBufferPtr(const int dst_obj_id, const std::string& _key, void** vbuf_ptr, size_t& size_bytes)
	{
		auto it = loa_res->map_dstobj_values.find(dst_obj_id);
		if (it == loa_res->map_dstobj_values.end())
		{
			*vbuf_ptr = NULL;
			return false;
		}
		auto __it = it->second.find(_key);
		if (__it == it->second.end())
		{
			*vbuf_ptr = NULL;
			return false;
		}
		*vbuf_ptr = __it->second.dvbuf_ptr;
		size_bytes = __it->second.dvalue_bytes;
		return true;
	}

	void VmLObject::ReplaceOrAddStringBuffer(const std::string& _key, const string* str_buffer_ptr, const int num_elements)
	{
		if (num_elements <= 0) return;
		auto it = loa_res->map_str_buffers.find(_key);
		if (it != loa_res->map_str_buffers.end())
		{
			VMSAFE_DELETEARRAY(it->second.str_buf_ptr);
			loa_res->map_str_buffers.erase(it);
		}

		_lobj_string buf;
		buf.str_buf_ptr = new string[num_elements];
		buf.num_elements = num_elements;
		for (int i = 0; i < num_elements; i++)
			buf.str_buf_ptr[i] = str_buffer_ptr[i];

		loa_res->map_str_buffers[_key] = buf;
	}

	bool VmLObject::GetStringBuffer(const std::string& _key, string** str_buffer_ptr, int& num_elements)
	{
		auto it = loa_res->map_str_buffers.find(_key);
		if (it == loa_res->map_str_buffers.end())
		{
			*str_buffer_ptr = NULL;
			num_elements = 0;
			return false;
		}

		*str_buffer_ptr = it->second.str_buf_ptr;
		num_elements = it->second.num_elements;
		return true;
	}

	bool VmLObject::RemoveStringBuffer(const std::string& _key)
	{
		auto it = loa_res->map_str_buffers.find(_key);
		if (it == loa_res->map_str_buffers.end()) return false;

		VMSAFE_DELETEARRAY(it->second.str_buf_ptr);
		loa_res->map_str_buffers.erase(it);
		return true;
	}

	bool VmLObject::CopyFrom(const VmLObject& src_lobj)
	{
		const LObjectArchive* src_loa_res = src_lobj.loa_res;

		for (auto it = src_loa_res->map_buffers.begin(); it != src_loa_res->map_buffers.end(); it++)
		{
			const _lobj_buffer& src_obj_buf = it->second;
			_lobj_buffer obj_buf;
			obj_buf.buffer_ptr = new byte[src_obj_buf.type_bytes * src_obj_buf.num_elements];
			obj_buf.type_bytes = src_obj_buf.type_bytes;
			obj_buf.num_elements = src_obj_buf.num_elements;
			memcpy(obj_buf.buffer_ptr, src_obj_buf.buffer_ptr, src_obj_buf.type_bytes * src_obj_buf.num_elements);

			auto dst_it = loa_res->map_buffers.find(it->first);
			if (dst_it != loa_res->map_buffers.end())
			{
				VMSAFE_DELETEARRAY(dst_it->second.buffer_ptr);
				dst_it->second = obj_buf;
			}
			else
				loa_res->map_buffers[it->first] = obj_buf;

		}

		for (auto it = src_loa_res->map_dstobj_values.begin(); it != src_loa_res->map_dstobj_values.end(); it++)
		{
			const std::map<std::string, _lobj_dvalue>& src_map_buffers = it->second;
			std::map<std::string, _lobj_dvalue> new_map_buffers;

			for (auto it_buf = src_map_buffers.begin(); it_buf != src_map_buffers.end(); it_buf++)
			{
				const _lobj_dvalue& src_obj_buf = it_buf->second;
				_lobj_dvalue obj_buf;
				obj_buf.dvbuf_ptr = new byte[src_obj_buf.dvalue_bytes];
				obj_buf.dvalue_bytes = src_obj_buf.dvalue_bytes;
				memcpy(obj_buf.dvbuf_ptr, src_obj_buf.dvbuf_ptr, src_obj_buf.dvalue_bytes);
				new_map_buffers[it_buf->first] = obj_buf;
			}

			auto dst_it = loa_res->map_dstobj_values.find(it->first);
			if (dst_it != loa_res->map_dstobj_values.end())
			{
				std::map<std::string, _lobj_dvalue>& dst_map_buffers = dst_it->second;
				for (auto it_buf = dst_map_buffers.begin(); it_buf != dst_map_buffers.end(); it_buf++)
					VMSAFE_DELETEARRAY(it_buf->second.dvbuf_ptr);

				dst_it->second = new_map_buffers;
			}
			else 
			{
				loa_res->map_dstobj_values[it->first] = new_map_buffers;
			}
		}
		
		for (auto it = src_loa_res->map_str_buffers.begin(); it != src_loa_res->map_str_buffers.end(); it++)
		{
			const _lobj_string& src_obj_buf = it->second;
			_lobj_string obj_buf;
			obj_buf.str_buf_ptr = new string[src_obj_buf.num_elements];
			obj_buf.num_elements = src_obj_buf.num_elements;
			for (int i = 0; i < src_obj_buf.num_elements; i++)
				obj_buf.str_buf_ptr[i] = src_obj_buf.str_buf_ptr[i];

			auto dst_it = loa_res->map_str_buffers.find(it->first);
			if (dst_it != loa_res->map_str_buffers.end())
			{
				VMSAFE_DELETEARRAY(dst_it->second.str_buf_ptr);
				dst_it->second = obj_buf;
			}
			else
				loa_res->map_str_buffers[it->first] = obj_buf;

		}
		
		return true;
	}

	ullong VmLObject::GetSizeOfAllLists()
	{
		ullong res_bytes = 0;

		for (auto it = loa_res->map_buffers.begin(); it != loa_res->map_buffers.end(); it++)
			res_bytes += it->second.type_bytes * it->second.num_elements;
		
		for (auto it = loa_res->map_dstobj_values.begin(); it != loa_res->map_dstobj_values.end(); it++)
		{
			std::map<std::string, _lobj_dvalue>& map_buffers = it->second;
			for (auto it_buf = map_buffers.begin(); it_buf != map_buffers.end(); it_buf++)
				res_bytes += it_buf->second.dvalue_bytes;
		}
		return res_bytes;
		// ignore loa_res->map_dstobj_values
	}
#pragma endregion
}

namespace vmgeom {
	using namespace std;
	using namespace vmmath;
	using namespace vmobjects;

	void GeneratePrimitive_Sphere(PrimitiveData& prim_data/*out*/, const vmdouble3& pos_center, const double radius, const int num_iter)
	{
		prim_data.Delete();
		if (radius <= 0.0 || num_iter < 0)
		{
			printf("GeneratePrimitive_Sphere - INVALIDATE PARAMETER");
			return;
		}
		prim_data.ptype = PrimitiveTypeTRIANGLE;
		prim_data.is_stripe = false;
		prim_data.is_ccw = true;
		AaBbMinMax* aabb_os_ptr = &prim_data.aabb_os;

		float r = (float)radius;
		int nVertexTotal = (1 << (num_iter + 1) * 2) + 2;
		int nEdgeTotal = (1 << (num_iter + 1) * 2) * 3;
		int nFaceTotal = (1 << (num_iter + 1) * 2) * 2;
		if (nVertexTotal < 6)
		{
			printf("NOT ENOUGH VERTEX in VXHPrimitiveGenerateTemplate_Sphere!");
			return;
		}

		// Initialize.
		aabb_os_ptr->pos_max.x = aabb_os_ptr->pos_max.y = aabb_os_ptr->pos_max.z = r;
		aabb_os_ptr->pos_min.x = aabb_os_ptr->pos_min.y = aabb_os_ptr->pos_min.z = -r;

		aabb_os_ptr->pos_min.x += pos_center.x;	aabb_os_ptr->pos_max.x += pos_center.x;
		aabb_os_ptr->pos_min.y += pos_center.y;	aabb_os_ptr->pos_max.y += pos_center.y;
		aabb_os_ptr->pos_min.z += pos_center.z;	aabb_os_ptr->pos_max.z += pos_center.z;

		prim_data.num_vtx = nVertexTotal;
		vmfloat3* vtxpos_buffer = new vmfloat3[nVertexTotal];
		prim_data.ReplaceOrAddVerticeDefinition("POSITION", vtxpos_buffer);
		vmfloat3* vtxnrl_buffer = new vmfloat3[nVertexTotal];
		prim_data.ReplaceOrAddVerticeDefinition("NORMAL", vtxnrl_buffer);

		prim_data.num_prims = nFaceTotal;
		prim_data.idx_stride = 3;
		prim_data.num_vidx = nFaceTotal * prim_data.idx_stride;
		prim_data.vidx_buffer = new uint[prim_data.num_vidx];

		float sqrt2 = sqrtf(2.0f);
		memcpy(&vtxpos_buffer[0], &vmfloat3(0, 0, r), sizeof(vmfloat3));
		memcpy(&vtxpos_buffer[1], &vmfloat3(0, 0, -r), sizeof(vmfloat3));
		memcpy(&vtxpos_buffer[2], &vmfloat3(-r / sqrt2, -r / sqrt2, 0), sizeof(vmfloat3));
		memcpy(&vtxpos_buffer[3], &vmfloat3(r / sqrt2, -r / sqrt2, 0), sizeof(vmfloat3));
		memcpy(&vtxpos_buffer[4], &vmfloat3(r / sqrt2, r / sqrt2, 0), sizeof(vmfloat3));
		memcpy(&vtxpos_buffer[5], &vmfloat3(-r / sqrt2, r / sqrt2, 0), sizeof(vmfloat3));

		int* idx_edges = new int[nEdgeTotal * 2];

		int iBlankSize = (1 << num_iter);
		idx_edges[0 * 2 + 0] = 0, idx_edges[0 * 2 + 1] = 2;
		idx_edges[iBlankSize * 1 * 2 + 0] = 0, idx_edges[iBlankSize * 1 * 2 + 1] = 3;
		idx_edges[iBlankSize * 2 * 2 + 0] = 0, idx_edges[iBlankSize * 2 * 2 + 1] = 4;
		idx_edges[iBlankSize * 3 * 2 + 0] = 0, idx_edges[iBlankSize * 3 * 2 + 1] = 5;
		idx_edges[iBlankSize * 4 * 2 + 0] = 1, idx_edges[iBlankSize * 4 * 2 + 1] = 2;
		idx_edges[iBlankSize * 5 * 2 + 0] = 1, idx_edges[iBlankSize * 5 * 2 + 1] = 3;
		idx_edges[iBlankSize * 6 * 2 + 0] = 1, idx_edges[iBlankSize * 6 * 2 + 1] = 4;
		idx_edges[iBlankSize * 7 * 2 + 0] = 1, idx_edges[iBlankSize * 7 * 2 + 1] = 5;
		idx_edges[iBlankSize * 8 * 2 + 0] = 2, idx_edges[iBlankSize * 8 * 2 + 1] = 3;
		idx_edges[iBlankSize * 9 * 2 + 0] = 3, idx_edges[iBlankSize * 9 * 2 + 1] = 4;
		idx_edges[iBlankSize * 10 * 2 + 0] = 4, idx_edges[iBlankSize * 10 * 2 + 1] = 5;
		idx_edges[iBlankSize * 11 * 2 + 0] = 5, idx_edges[iBlankSize * 11 * 2 + 1] = 2;

		// [HACK] Use prim_data.vidx_buffer as edge index array instead of (originally intended) vertex index array.
		// Later, edge indices are converted to the corresponding vertex indices for each polygon.

		prim_data.vidx_buffer[0 * 3 + 0] = iBlankSize * 0, prim_data.vidx_buffer[0 * 3 + 1] = iBlankSize * 8, prim_data.vidx_buffer[0 * 3 + 2] = iBlankSize * 1;  // octahedron
		prim_data.vidx_buffer[1 * 3 + 0] = iBlankSize * 1, prim_data.vidx_buffer[1 * 3 + 1] = iBlankSize * 9, prim_data.vidx_buffer[1 * 3 + 2] = iBlankSize * 2;
		prim_data.vidx_buffer[2 * 3 + 0] = iBlankSize * 2, prim_data.vidx_buffer[2 * 3 + 1] = iBlankSize * 10, prim_data.vidx_buffer[2 * 3 + 2] = iBlankSize * 3;
		prim_data.vidx_buffer[3 * 3 + 0] = iBlankSize * 3, prim_data.vidx_buffer[3 * 3 + 1] = iBlankSize * 11, prim_data.vidx_buffer[3 * 3 + 2] = iBlankSize * 0;
		prim_data.vidx_buffer[4 * 3 + 0] = iBlankSize * 4, prim_data.vidx_buffer[4 * 3 + 1] = iBlankSize * 5, prim_data.vidx_buffer[4 * 3 + 2] = iBlankSize * 8;
		prim_data.vidx_buffer[5 * 3 + 0] = iBlankSize * 5, prim_data.vidx_buffer[5 * 3 + 1] = iBlankSize * 6, prim_data.vidx_buffer[5 * 3 + 2] = iBlankSize * 9;
		prim_data.vidx_buffer[6 * 3 + 0] = iBlankSize * 6, prim_data.vidx_buffer[6 * 3 + 1] = iBlankSize * 7, prim_data.vidx_buffer[6 * 3 + 2] = iBlankSize * 10;
		prim_data.vidx_buffer[7 * 3 + 0] = iBlankSize * 7, prim_data.vidx_buffer[7 * 3 + 1] = iBlankSize * 4, prim_data.vidx_buffer[7 * 3 + 2] = iBlankSize * 11;

		int nVertex = 6;
		int nEdge = 12;
		int nFace = 8;

#define COMMON_VERTEX_INDEX_IN_TWO_EDGES(edgeIdx1, edgeIdx2) ( \
	idx_edges[edgeIdx1*2 + 0] == idx_edges[edgeIdx2*2 + 0] || \
	idx_edges[edgeIdx1*2 + 0] == idx_edges[edgeIdx2*2 + 1] ? \
	idx_edges[edgeIdx1*2 + 0] : ( \
	idx_edges[edgeIdx1*2 + 1] == idx_edges[edgeIdx2*2 + 0] || \
	idx_edges[edgeIdx1*2 + 1] == idx_edges[edgeIdx2*2 + 1] ? \
	idx_edges[edgeIdx1*2 + 1] : -1 ) )

		// Surface refinement iterations.
		for (int i = 1; i <= num_iter; ++i)
		{
			// Bisect edges and generate new vertices.
			int nEdgeOld = nEdge;
			for (int j = 0; j < nEdgeOld; ++j, ++nVertex, ++nEdge)
			{
				assert(nVertex <= nVertexTotal);
				assert(nEdge <= nEdgeTotal);
				assert(nFace <= nFaceTotal);

				// A new edge.		
				int* edge = &idx_edges[iBlankSize*j * 2];
				int* edgeNew = &idx_edges[(iBlankSize*j + iBlankSize / 2) * 2];
				edgeNew[1] = edge[1];
				edge[1] = edgeNew[0] = nVertex;

				// A new vertex.
				vmfloat3 midpoint;
				int iIndex0 = edge[0];
				int iIndex1 = edgeNew[1];
				midpoint.x = (vtxpos_buffer[iIndex0].x + vtxpos_buffer[iIndex1].x) / 2.0f;
				midpoint.y = (vtxpos_buffer[iIndex0].y + vtxpos_buffer[iIndex1].y) / 2.0f;
				midpoint.z = (vtxpos_buffer[iIndex0].z + vtxpos_buffer[iIndex1].z) / 2.0f;

				vmfloat3 newpoint;
				fNormalizeVector(&newpoint, &midpoint);
				newpoint *= r;
				memcpy(&vtxpos_buffer[nVertex], &newpoint, sizeof(vmfloat3));
			}

			iBlankSize /= 2;

			// Generate new edges and update polygon data.
			int nFaceOld = nFace;
			for (int j = 0; j < nFaceOld; ++j, nEdge += 3, nFace += 3)
			{
				assert(nVertex <= nVertexTotal);
				assert(nEdge <= nEdgeTotal);
				assert(nFace <= nFaceTotal);

				// Three new edges.
				uint* face = &prim_data.vidx_buffer[j * 3];
				int idxVertexNew[3] = { idx_edges[face[0] * 2 + 1],idx_edges[face[1] * 2 + 1], idx_edges[face[2] * 2 + 1] };
				int idxEdgeNew[3] = { iBlankSize*nEdge, iBlankSize*(nEdge + 1), iBlankSize*(nEdge + 2) };
				int* edgeNew[3] = { &idx_edges[idxEdgeNew[0] * 2], &idx_edges[idxEdgeNew[1] * 2], &idx_edges[idxEdgeNew[2] * 2] };
				edgeNew[0][0] = idxVertexNew[0], edgeNew[0][1] = idxVertexNew[1];
				edgeNew[1][0] = idxVertexNew[1], edgeNew[1][1] = idxVertexNew[2];
				edgeNew[2][0] = idxVertexNew[2], edgeNew[2][1] = idxVertexNew[0];

				// Three new faces.
				uint* faceNew[3] = { &prim_data.vidx_buffer[nFace * 3], &prim_data.vidx_buffer[(nFace + 1) * 3], &prim_data.vidx_buffer[(nFace + 2) * 3] };
				int faceIdx = 0;
				bool flag[6] = { false, false, false, false, false, false };
				int x, y;
				for (x = 0; x < 6; ++x)
				{
					if (flag[x]) continue;
					int xEdgeIdx, yEdgeIdx;
					xEdgeIdx = face[x / 2] + iBlankSize * (x % 2);
					bool adjacent = false;
					for (y = x + 1; y < 6; ++y)
					{
						if (flag[y] || x / 2 == y / 2) continue;
						yEdgeIdx = face[y / 2] + iBlankSize * (y % 2);
						if (COMMON_VERTEX_INDEX_IN_TWO_EDGES(xEdgeIdx, yEdgeIdx) >= 0)
						{
							adjacent = true;
							break;
						}
					}
					assert(adjacent);

					if ((y / 2 - x / 2 + 2) % 3 == 0)    // in natural order
						faceNew[faceIdx][0] = xEdgeIdx, faceNew[faceIdx][1] = yEdgeIdx, faceNew[faceIdx][2] = idxEdgeNew[x / 2];
					else
						faceNew[faceIdx][0] = xEdgeIdx, faceNew[faceIdx][1] = idxEdgeNew[y / 2], faceNew[faceIdx][2] = yEdgeIdx;
					++faceIdx;

					flag[x] = flag[y] = true;
				}
				assert(faceIdx == 3);

				// Update the old face.
				face[0] = idxEdgeNew[0], face[1] = idxEdgeNew[1], face[2] = idxEdgeNew[2];
			}
		}

		assert(nVertex == nVertexTotal);
		assert(nEdge == nEdgeTotal);
		assert(nFace == nFaceTotal);

		// [END-HACK] Now convert edge indices to vertex indices for each polygon.
		for (int j = 0; j < nFace; ++j)
		{
			uint* face = &prim_data.vidx_buffer[j * 3];
			int v[3] = { COMMON_VERTEX_INDEX_IN_TWO_EDGES(face[0], face[1]),
				COMMON_VERTEX_INDEX_IN_TWO_EDGES(face[1], face[2]),
				COMMON_VERTEX_INDEX_IN_TWO_EDGES(face[2], face[0]) };
			face[0] = v[0], face[1] = v[1], face[2] = v[2];
		}

		vmfloat3 fpos_center = pos_center;
		for (int j = 0; j < nVertex; ++j)
		{
			// Populate normal vector array.
			fNormalizeVector(&vtxnrl_buffer[j], &vtxpos_buffer[j]);

			// Offset
			vtxpos_buffer[j].x += fpos_center.x;
			vtxpos_buffer[j].y += fpos_center.y;
			vtxpos_buffer[j].z += fpos_center.z;
		}

		// Release temporary resources.
		VMSAFE_DELETEARRAY(idx_edges);
	}

	void GeneratePrimitive_Cone(PrimitiveData& prim_data/*out*/, const vmdouble3& pos_s, const vmdouble3& pos_e,
		const double radius, const bool open_cone, const int num_interpolations)
	{
		prim_data.Delete();
		if (radius <= 0.0 || num_interpolations < 3)
		{
			printf("VXHPrimitiveGenerateTemplate_Sphere - INVALIDATE PARAMETER");
			return;
		}
		prim_data.ptype = PrimitiveTypeTRIANGLE;
		prim_data.is_stripe = false;
		prim_data.is_ccw = true;
		AaBbMinMax* aabb_os_ptr = &prim_data.aabb_os;

		// Initialize.
		aabb_os_ptr->pos_max.x = aabb_os_ptr->pos_max.y = aabb_os_ptr->pos_max.z = -FLT_MAX;
		aabb_os_ptr->pos_min.x = aabb_os_ptr->pos_min.y = aabb_os_ptr->pos_min.z = FLT_MAX;

		// Z축에 대해 Height 1, Radius 1 의 콘 만들기
		prim_data.num_vtx = num_interpolations * 3 * 2/*노말을 위한 아래원 용, 옆면 용*/;
		prim_data.num_prims = num_interpolations * 2;
		if (open_cone)
		{
			prim_data.num_vtx = num_interpolations * 3;
			prim_data.num_prims = num_interpolations;
		}

		vmfloat3* pos_buf = new vmfloat3[prim_data.num_vtx];
		//prim_data_ptr.mapVerticeDefinitions.insert(pair<wstring, vmfloat3*>(_T("POSITION"), pf3PosVertice));
		prim_data.ReplaceOrAddVerticeDefinition("POSITION", pos_buf);
		vmfloat3* nrl_buf = new vmfloat3[prim_data.num_vtx];
		//prim_data_ptr.mapVerticeDefinitions.insert(pair<wstring, vmfloat3*>(_T("NORMAL"), pf3VecNormal));
		prim_data.ReplaceOrAddVerticeDefinition("NORMAL", nrl_buf);
		prim_data.idx_stride = 3;
		prim_data.num_vidx = prim_data.num_prims * prim_data.idx_stride;
		uint* vidx_buffer = new uint[prim_data.num_vidx];
		prim_data.vidx_buffer = vidx_buffer;

		double dInterRadian = 2 * VM_PI / num_interpolations;

		if (open_cone)
		{
			for (int i = 0; i < num_interpolations; i++)
			{
				pos_buf[0 + i * 3] = vmfloat3(0, 0, 1.f);

				double dX = cos(dInterRadian*i);
				double dY = sin(dInterRadian*i);
				double dX2 = cos(dInterRadian*(i + 1));
				double dY2 = sin(dInterRadian*(i + 1));

				pos_buf[2 + i * 3] = vmfloat3((float)dX2, (float)dY2, 0);
				pos_buf[1 + i * 3] = vmfloat3((float)dX, (float)dY, 0);
			}
		}
		else
		{
			for (int i = 0; i < num_interpolations; i++)
			{
				pos_buf[0 + i * 3] = vmfloat3(0, 0, 1.f);
				pos_buf[0 + (i + num_interpolations) * 3] = vmfloat3(0, 0, 0);

				double dX = cos(dInterRadian*i);
				double dY = sin(dInterRadian*i);
				double dX2 = cos(dInterRadian*(i + 1));
				double dY2 = sin(dInterRadian*(i + 1));

				pos_buf[2 + i * 3] = vmfloat3((float)dX2, (float)dY2, 0);
				pos_buf[1 + i * 3] = vmfloat3((float)dX, (float)dY, 0);

				pos_buf[2 + (i + num_interpolations) * 3] = vmfloat3((float)dX, (float)dY, 0);
				pos_buf[1 + (i + num_interpolations) * 3] = vmfloat3((float)dX2, (float)dY2, 0);
			}
		}

		// Indexing
		for (int i = 0; i < (int)prim_data.num_vtx; i++)
		{
			vidx_buffer[i] = i;
		}

		// 변환된 콘 만들기
		vmdouble3 dPosStart(pos_s), dPosEnd(pos_e);
		vmdouble3 dVecS2E = dPosEnd - dPosStart;
		double dHeight = LengthVector(&dVecS2E);

		vmmat44 matScale, matLookAt, matRotTranslate, matTr;
		MatrixScaling(&matScale, &vmdouble3(radius, radius, dHeight));
		vmdouble3 dVecUp = vmdouble3(0, 0, 1.f), dVecCrossTest;
		CrossDotVector(&dVecCrossTest, &dVecUp, &dVecS2E);
		if (LengthVector(&dVecCrossTest) <= FLT_MIN)
			dVecUp = vmdouble3(0, 1.f, 0);
		MatrixWS2CS(&matLookAt, &dPosStart, &dVecUp, &(-dVecS2E));
		MatrixInverse(&matRotTranslate, &matLookAt);
		MatrixMultiply(&matTr, &matScale, &matRotTranslate);

		vmmat44f fmatTr = matTr;
		for (int i = 0; i < (int)prim_data.num_vtx; i++)
		{
			fTransformPoint(&pos_buf[i], &pos_buf[i], &fmatTr);
			// Normal vector 의 plane orthogonality 가 유지 되지 않으므로 다시 계산

			aabb_os_ptr->pos_max.x = max(aabb_os_ptr->pos_max.x, (double)pos_buf[i].x);
			aabb_os_ptr->pos_max.y = max(aabb_os_ptr->pos_max.y, (double)pos_buf[i].y);
			aabb_os_ptr->pos_max.z = max(aabb_os_ptr->pos_max.z, (double)pos_buf[i].z);
			aabb_os_ptr->pos_min.x = min(aabb_os_ptr->pos_min.x, (double)pos_buf[i].x);
			aabb_os_ptr->pos_min.y = min(aabb_os_ptr->pos_min.y, (double)pos_buf[i].y);
			aabb_os_ptr->pos_min.z = min(aabb_os_ptr->pos_min.z, (double)pos_buf[i].z);
		}

		if (open_cone)
		{
			//for (int i = 0; i < num_interpolations; i++)
			//{
			//	vmfloat3 f3VecUp01 = pos_buf[1 + i * 3] - pos_buf[0 + i * 3];
			//	vmfloat3 f3VecUp02 = pos_buf[2 + i * 3] - pos_buf[0 + i * 3];
			//	vmfloat3 f3VecUp;
			//	fCrossDotVector(&f3VecUp, &f3VecUp02, &f3VecUp01);
			//	fNormalizeVector(&f3VecUp, &f3VecUp);
			//}
		}
		else
		{
			for (int i = 0; i < num_interpolations; i++)
			{
				vmfloat3 f3VecUp01 = pos_buf[1 + i * 3] - pos_buf[0 + i * 3];
				vmfloat3 f3VecUp02 = pos_buf[2 + i * 3] - pos_buf[0 + i * 3];
				vmfloat3 f3VecUp;
				//fCrossDotVector(&f3VecUp, &f3VecUp02, &f3VecUp01);
				fCrossDotVector(&f3VecUp, &f3VecUp01, &f3VecUp02);
				fNormalizeVector(&f3VecUp, &f3VecUp);

				vmfloat3 f3VecDown01 = pos_buf[1 + (i + num_interpolations) * 3] - pos_buf[0 + (i + num_interpolations) * 3];
				vmfloat3 f3VecDown02 = pos_buf[2 + (i + num_interpolations) * 3] - pos_buf[0 + (i + num_interpolations) * 3];
				vmfloat3 f3VecDown;
				//fCrossDotVector(&f3VecDown, &f3VecDown02, &f3VecDown01);
				fCrossDotVector(&f3VecDown, &f3VecDown01, &f3VecDown02);
				fNormalizeVector(&f3VecDown, &f3VecDown);

				nrl_buf[0 + i * 3] = nrl_buf[1 + i * 3] = nrl_buf[2 + i * 3] = f3VecUp;
				nrl_buf[0 + (i + num_interpolations) * 3] = nrl_buf[1 + (i + num_interpolations) * 3] = nrl_buf[2 + (i + num_interpolations) * 3] = f3VecDown;
			}
		}
	}

	void GeneratePrimitive_Cylinder(PrimitiveData& prim_data/*out*/, const vmdouble3& pos_s, const vmdouble3& pos_e, const double radius, const bool open_top, const bool open_bootom,
		const int num_interpolations, const int num_circle_interpolations, const int num_sideheight_interpolations)
	{
		prim_data.Delete();
		if (radius <= 0.0 || num_interpolations < 3)
		{
			printf("GeneratePrimitive_Cylinder - INVALIDATE PARAMETER");
			return;
		}

		prim_data.ptype = PrimitiveTypeTRIANGLE;
		prim_data.is_stripe = false;
		prim_data.is_ccw = true;
		AaBbMinMax* aabb_os_ptr = &prim_data.aabb_os;

		// Initialize.
		aabb_os_ptr->pos_max.x = aabb_os_ptr->pos_max.y = aabb_os_ptr->pos_max.z = -FLT_MAX;
		aabb_os_ptr->pos_min.x = aabb_os_ptr->pos_min.y = aabb_os_ptr->pos_min.z = FLT_MAX;

		// Z축에 대해 Height 1, Radius 1 의 실린더 만들기
		prim_data.num_vtx = num_interpolations * num_circle_interpolations * 2 + num_interpolations * (num_sideheight_interpolations + 1) + 2/*위아래 원 + 옆면 + 위아래중심*/;
		if (open_top && open_bootom)
			prim_data.num_vtx = num_interpolations * (num_sideheight_interpolations + 1);
		else if (open_top || open_bootom)
			prim_data.num_vtx = num_interpolations * num_circle_interpolations + num_interpolations * (num_sideheight_interpolations + 1) + 1;

		vmfloat3* pos_buf = new vmfloat3[prim_data.num_vtx];
		prim_data.ReplaceOrAddVerticeDefinition("POSITION", pos_buf);
		vmfloat3* nrl_buf = new vmfloat3[prim_data.num_vtx];
		prim_data.ReplaceOrAddVerticeDefinition("NORMAL", nrl_buf);
		prim_data.num_prims = (num_interpolations + num_interpolations * 2 * (num_circle_interpolations - 1)) * 2 + num_interpolations * num_sideheight_interpolations * 2;
		if (open_top && open_bootom)
			prim_data.num_prims = num_interpolations * num_sideheight_interpolations * 2;
		else if (open_top || open_bootom)
			prim_data.num_prims = num_interpolations + num_interpolations * 2 * (num_circle_interpolations - 1) + num_interpolations * num_sideheight_interpolations * 2;

		prim_data.idx_stride = 3;
		prim_data.num_vidx = prim_data.num_prims * prim_data.idx_stride;
		uint* vidx_buffer = new uint[prim_data.num_vidx];
		prim_data.vidx_buffer = vidx_buffer;

		int iIndex = 0;
		double dInterRadian = 2 * VM_PI / num_interpolations;
		int iOffsetSide;
		int iOffsetBottom;
		int iOffsetCenter;
		if (open_top || open_bootom)
		{
			if (open_top && open_bootom)
			{
				iOffsetSide = 0;
				for (int i = 0; i < num_interpolations; i++)
				{
					double dX = cos(dInterRadian*i);
					double dY = sin(dInterRadian*i);

					// Side
					for (int j = 0; j <= num_sideheight_interpolations; j++)
					{
						double dZ = (double)(num_sideheight_interpolations - j) / num_sideheight_interpolations;
						pos_buf[i + j * num_interpolations + iOffsetSide] = vmfloat3((float)dX, (float)dY, (float)dZ);
					}
				}
				//side
				for (int i = 0; i < num_interpolations; i++)
				{
					for (int j = 0; j < num_sideheight_interpolations; j++)
					{
						vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetSide;
						vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
						vidx_buffer[iIndex++] = (j + 1) * num_interpolations + i + iOffsetSide;
						vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetSide;
						vidx_buffer[iIndex++] = j * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
						vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
					}
				}
			}
			else
			{
				iOffsetCenter = num_interpolations * num_circle_interpolations + num_interpolations * (num_sideheight_interpolations + 1);
				if (open_bootom)
				{
					iOffsetSide = num_interpolations * num_circle_interpolations;
					for (int i = 0; i < num_interpolations; i++)
					{
						double dX = cos(dInterRadian*i);
						double dY = sin(dInterRadian*i);
						// Top 
						for (int j = 0; j < num_circle_interpolations; j++)
						{
							double dNewX = (double)(j + 1) / num_circle_interpolations * dX;
							double dNewY = (double)(j + 1) / num_circle_interpolations * dY;
							pos_buf[i + j * num_interpolations] = vmfloat3((float)dNewX, (float)dNewY, 1.f);
						}

						// Side
						for (int j = 0; j <= num_sideheight_interpolations; j++)
						{
							double dZ = (double)(num_sideheight_interpolations - j) / num_sideheight_interpolations;
							pos_buf[i + j * num_interpolations + iOffsetSide] = vmfloat3((float)dX, (float)dY, (float)dZ);
						}
					}
					pos_buf[iOffsetCenter] = vmfloat3(0, 0, 1);

					// Indexing Top
					for (int i = 0; i < num_interpolations; i++)
					{
						vidx_buffer[iIndex++] = iOffsetCenter;
						vidx_buffer[iIndex++] = i;
						vidx_buffer[iIndex++] = (i + 1) % num_interpolations;
						for (int j = 0; j < num_circle_interpolations - 1; j++)
						{
							vidx_buffer[iIndex++] = j * num_interpolations + i;
							vidx_buffer[iIndex++] = (j + 1) * num_interpolations + i;
							vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations;
							vidx_buffer[iIndex++] = j * num_interpolations + i;
							vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations;
							vidx_buffer[iIndex++] = j * num_interpolations + (i + 1) % num_interpolations;
						}
					}
				}
				else if (open_top)
				{
					iOffsetSide = 0;
					iOffsetBottom = num_interpolations * (num_sideheight_interpolations + 1);
					for (int i = 0; i < num_interpolations; i++)
					{
						double dX = cos(dInterRadian*i);
						double dY = sin(dInterRadian*i);

						// Bottom
						for (int j = 0; j < num_circle_interpolations; j++)
						{
							double dNewX = (double)(j + 1) / num_circle_interpolations * dX;
							double dNewY = (double)(j + 1) / num_circle_interpolations * dY;
							pos_buf[i + j * num_interpolations + iOffsetBottom] = vmfloat3((float)dNewX, (float)dNewY, 0.f);
						}

						// Side
						for (int j = 0; j <= num_sideheight_interpolations; j++)
						{
							double dZ = (double)(num_sideheight_interpolations - j) / num_sideheight_interpolations;
							pos_buf[i + j * num_interpolations + iOffsetSide] = vmfloat3((float)dX, (float)dY, (float)dZ);
						}
					}
					pos_buf[iOffsetCenter] = vmfloat3(0, 0, 0);

					// Indexing bottom
					for (int i = 0; i < num_interpolations; i++)
					{
						vidx_buffer[iIndex++] = iOffsetCenter;
						vidx_buffer[iIndex++] = (i + 1) % num_interpolations + iOffsetBottom;
						vidx_buffer[iIndex++] = i + iOffsetBottom;
						for (int j = 0; j < num_circle_interpolations - 1; j++)
						{
							vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetBottom;
							vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetBottom;
							vidx_buffer[iIndex++] = (j + 1) * num_interpolations + i + iOffsetBottom;
							vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetBottom;
							vidx_buffer[iIndex++] = j * num_interpolations + (i + 1) % num_interpolations + iOffsetBottom;
							vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetBottom;
						}
					}
				}

				//Indexing Side
				for (int i = 0; i < num_interpolations; i++)
				{
					for (int j = 0; j < num_sideheight_interpolations; j++)
					{
						vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetSide;
						vidx_buffer[iIndex++] = (j + 1) * num_interpolations + i + iOffsetSide;
						vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
						vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetSide;
						vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
						vidx_buffer[iIndex++] = j * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
					}
				}
			}
		}
		else
		{
			iOffsetSide = num_interpolations * num_circle_interpolations;
			iOffsetBottom = num_interpolations * num_circle_interpolations + num_interpolations * (num_sideheight_interpolations + 1);
			iOffsetCenter = num_interpolations * num_circle_interpolations * 2 + num_interpolations * (num_sideheight_interpolations + 1);
			for (int i = 0; i < num_interpolations; i++)
			{
				double dX = cos(dInterRadian*i);
				double dY = sin(dInterRadian*i);

				// Top and Bottom
				for (int j = 0; j < num_circle_interpolations; j++)
				{
					double dNewX = (double)(j + 1) / num_circle_interpolations * dX;
					double dNewY = (double)(j + 1) / num_circle_interpolations * dY;
					pos_buf[i + j * num_interpolations] = vmfloat3((float)dNewX, (float)dNewY, 1.f);
					pos_buf[i + j * num_interpolations + iOffsetBottom] = vmfloat3((float)dNewX, (float)dNewY, 0.f);
				}

				// Side
				for (int j = 0; j <= num_sideheight_interpolations; j++)
				{
					double dZ = (double)(num_sideheight_interpolations - j) / num_sideheight_interpolations;
					pos_buf[i + j * num_interpolations + iOffsetSide] = vmfloat3((float)dX, (float)dY, (float)dZ);
				}
			}
			pos_buf[iOffsetCenter] = vmfloat3(0, 0, 1);
			pos_buf[iOffsetCenter + 1] = vmfloat3(0, 0, 0.f);

			// Indexing Top
			for (int i = 0; i < num_interpolations; i++)
			{
				vidx_buffer[iIndex++] = iOffsetCenter;
				vidx_buffer[iIndex++] = i;
				vidx_buffer[iIndex++] = (i + 1) % num_interpolations;
				for (int j = 0; j < num_circle_interpolations - 1; j++)
				{
					vidx_buffer[iIndex++] = j * num_interpolations + i;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + i;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations;
					vidx_buffer[iIndex++] = j * num_interpolations + i;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations;
					vidx_buffer[iIndex++] = j * num_interpolations + (i + 1) % num_interpolations;
				}
			}
			//side
			for (int i = 0; i < num_interpolations; i++)
			{
				for (int j = 0; j < num_sideheight_interpolations; j++)
				{
					vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetSide;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + i + iOffsetSide;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
					vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetSide;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
					vidx_buffer[iIndex++] = j * num_interpolations + (i + 1) % num_interpolations + iOffsetSide;
				}
			}
			//iOffset Bottom
			for (int i = 0; i < num_interpolations; i++)
			{
				vidx_buffer[iIndex++] = iOffsetCenter + 1;
				vidx_buffer[iIndex++] = (i + 1) % num_interpolations + iOffsetBottom;
				vidx_buffer[iIndex++] = i + iOffsetBottom;
				for (int j = 0; j < num_circle_interpolations - 1; j++)
				{
					vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetBottom;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetBottom;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + i + iOffsetBottom;
					vidx_buffer[iIndex++] = (j + 1) * num_interpolations + (i + 1) % num_interpolations + iOffsetBottom;
					vidx_buffer[iIndex++] = j * num_interpolations + i + iOffsetBottom;
					vidx_buffer[iIndex++] = j * num_interpolations + (i + 1) % num_interpolations + iOffsetBottom;
				}
			}
		}

		// 변환된 Cylinder 만들기
		vmdouble3 dPosStart(pos_s), dPosEnd(pos_e);
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
		for (int i = 0; i < (int)prim_data.num_vtx; i++)
		{
			fTransformPoint(&pos_buf[i], &pos_buf[i], &fmatTr);
			aabb_os_ptr->pos_max.x = max(aabb_os_ptr->pos_max.x, (double)pos_buf[i].x);
			aabb_os_ptr->pos_max.y = max(aabb_os_ptr->pos_max.y, (double)pos_buf[i].y);
			aabb_os_ptr->pos_max.z = max(aabb_os_ptr->pos_max.z, (double)pos_buf[i].z);
			aabb_os_ptr->pos_min.x = min(aabb_os_ptr->pos_min.x, (double)pos_buf[i].x);
			aabb_os_ptr->pos_min.y = min(aabb_os_ptr->pos_min.y, (double)pos_buf[i].y);
			aabb_os_ptr->pos_min.z = min(aabb_os_ptr->pos_min.z, (double)pos_buf[i].z);
		}

		for (uint i = 0; i < prim_data.num_prims; i++)
		{
			int iIndex0 = vidx_buffer[3 * i + 0];
			int iIndex1 = vidx_buffer[3 * i + 1];
			int iIndex2 = vidx_buffer[3 * i + 2];

			vmfloat3 f3Vec01 = pos_buf[iIndex1] - pos_buf[iIndex0];
			vmfloat3 f3Vec02 = pos_buf[iIndex2] - pos_buf[iIndex0];
			vmfloat3 f3VecNormal;
			//fCrossDotVector(&f3VecNormal, &f3Vec02, &f3Vec01);
			fCrossDotVector(&f3VecNormal, &f3Vec01, &f3Vec02);
			fNormalizeVector(&f3VecNormal, &f3VecNormal);
			nrl_buf[iIndex0] += f3VecNormal;
			nrl_buf[iIndex1] += f3VecNormal;
			nrl_buf[iIndex2] += f3VecNormal;
			fNormalizeVector(&nrl_buf[iIndex0], &nrl_buf[iIndex0]);
			fNormalizeVector(&nrl_buf[iIndex1], &nrl_buf[iIndex1]);
			fNormalizeVector(&nrl_buf[iIndex2], &nrl_buf[iIndex2]);
		}
	}

	void GeneratePrimitive_Cube(PrimitiveData& prim_data/*out*/, const vmdouble3& pos_min, const vmdouble3& pos_max, const double edge_nrl_weight/*0.0 to 1.0*/, const bool cube_frame_mode)
	{
		prim_data.Delete();

		// Actually, VOS2WS
		AaBbMinMax* aabb_os_ptr = &prim_data.aabb_os;

		// Initialize.
		aabb_os_ptr->pos_min = vmfloat3(pos_min);
		aabb_os_ptr->pos_max = vmfloat3(pos_max);

		vmfloat3 f3PosCubeMinOS(pos_min), f3PosCubeMaxOS(pos_max);

		// Cube //
		vmfloat3 f3PosCubeOS[8];
		f3PosCubeOS[0] = vmfloat3(f3PosCubeMinOS.x, f3PosCubeMinOS.y, f3PosCubeMaxOS.z);
		f3PosCubeOS[1] = vmfloat3(f3PosCubeMaxOS.x, f3PosCubeMinOS.y, f3PosCubeMaxOS.z);
		f3PosCubeOS[2] = vmfloat3(f3PosCubeMinOS.x, f3PosCubeMinOS.y, f3PosCubeMinOS.z);
		f3PosCubeOS[3] = vmfloat3(f3PosCubeMaxOS.x, f3PosCubeMinOS.y, f3PosCubeMinOS.z);
		f3PosCubeOS[4] = vmfloat3(f3PosCubeMinOS.x, f3PosCubeMaxOS.y, f3PosCubeMaxOS.z);
		f3PosCubeOS[5] = vmfloat3(f3PosCubeMaxOS.x, f3PosCubeMaxOS.y, f3PosCubeMaxOS.z);
		f3PosCubeOS[6] = vmfloat3(f3PosCubeMinOS.x, f3PosCubeMaxOS.y, f3PosCubeMinOS.z);
		f3PosCubeOS[7] = vmfloat3(f3PosCubeMaxOS.x, f3PosCubeMaxOS.y, f3PosCubeMinOS.z);

		if (!cube_frame_mode)
		{
			// Cube Plane Setting //
			prim_data.num_vtx = 24;
			vmfloat3* pos_buf = new vmfloat3[prim_data.num_vtx];
			vmfloat3* nrl_buf = new vmfloat3[prim_data.num_vtx];

			float fEdgeNormalFactor = (float)edge_nrl_weight;
			// (-y)
			pos_buf[0] = f3PosCubeOS[0];
			pos_buf[1] = f3PosCubeOS[1];
			pos_buf[2] = f3PosCubeOS[2];
			pos_buf[3] = f3PosCubeOS[3];
			nrl_buf[0] = vmfloat3(-fEdgeNormalFactor, -1.f, +fEdgeNormalFactor);
			nrl_buf[1] = vmfloat3(+fEdgeNormalFactor, -1.f, +fEdgeNormalFactor);
			nrl_buf[2] = vmfloat3(-fEdgeNormalFactor, -1.f, -fEdgeNormalFactor);
			nrl_buf[3] = vmfloat3(+fEdgeNormalFactor, -1.f, -fEdgeNormalFactor);
			// (+y)
			pos_buf[4] = f3PosCubeOS[5];
			pos_buf[5] = f3PosCubeOS[4];
			pos_buf[6] = f3PosCubeOS[7];
			pos_buf[7] = f3PosCubeOS[6];
			nrl_buf[4] = vmfloat3(+fEdgeNormalFactor, 1.f, +fEdgeNormalFactor);
			nrl_buf[5] = vmfloat3(-fEdgeNormalFactor, 1.f, +fEdgeNormalFactor);
			nrl_buf[6] = vmfloat3(+fEdgeNormalFactor, 1.f, -fEdgeNormalFactor);
			nrl_buf[7] = vmfloat3(-fEdgeNormalFactor, 1.f, -fEdgeNormalFactor);
			// (-x)
			pos_buf[8] = f3PosCubeOS[4];
			pos_buf[9] = f3PosCubeOS[0];
			pos_buf[10] = f3PosCubeOS[6];
			pos_buf[11] = f3PosCubeOS[2];
			nrl_buf[8] = vmfloat3(-1.f, +fEdgeNormalFactor, +fEdgeNormalFactor);
			nrl_buf[9] = vmfloat3(-1.f, -fEdgeNormalFactor, +fEdgeNormalFactor);
			nrl_buf[10] = vmfloat3(-1.f, +fEdgeNormalFactor, -fEdgeNormalFactor);
			nrl_buf[11] = vmfloat3(-1.f, -fEdgeNormalFactor, -fEdgeNormalFactor);
			// (+x)
			pos_buf[12] = f3PosCubeOS[1];
			pos_buf[13] = f3PosCubeOS[5];
			pos_buf[14] = f3PosCubeOS[3];
			pos_buf[15] = f3PosCubeOS[7];
			nrl_buf[12] = vmfloat3(1.f, -fEdgeNormalFactor, +fEdgeNormalFactor);
			nrl_buf[13] = vmfloat3(1.f, +fEdgeNormalFactor, +fEdgeNormalFactor);
			nrl_buf[14] = vmfloat3(1.f, -fEdgeNormalFactor, -fEdgeNormalFactor);
			nrl_buf[15] = vmfloat3(1.f, +fEdgeNormalFactor, -fEdgeNormalFactor);
			// (-z)
			pos_buf[16] = f3PosCubeOS[2];
			pos_buf[17] = f3PosCubeOS[3];
			pos_buf[18] = f3PosCubeOS[6];
			pos_buf[19] = f3PosCubeOS[7];
			nrl_buf[16] = vmfloat3(-fEdgeNormalFactor, -fEdgeNormalFactor, -1.f);
			nrl_buf[17] = vmfloat3(+fEdgeNormalFactor, -fEdgeNormalFactor, -1.f);
			nrl_buf[18] = vmfloat3(-fEdgeNormalFactor, +fEdgeNormalFactor, -1.f);
			nrl_buf[19] = vmfloat3(+fEdgeNormalFactor, +fEdgeNormalFactor, -1.f);
			// (+z)
			pos_buf[20] = f3PosCubeOS[4];
			pos_buf[21] = f3PosCubeOS[5];
			pos_buf[22] = f3PosCubeOS[0];
			pos_buf[23] = f3PosCubeOS[1];
			nrl_buf[20] = vmfloat3(-fEdgeNormalFactor, +fEdgeNormalFactor, 1.f);
			nrl_buf[21] = vmfloat3(+fEdgeNormalFactor, +fEdgeNormalFactor, 1.f);
			nrl_buf[22] = vmfloat3(-fEdgeNormalFactor, -fEdgeNormalFactor, 1.f);
			nrl_buf[23] = vmfloat3(+fEdgeNormalFactor, -fEdgeNormalFactor, 1.f);

			for (int i = 0; i < 24; i++)
				fNormalizeVector(&nrl_buf[i], &nrl_buf[i]);

			prim_data.is_ccw = true;
			prim_data.is_stripe = false;
			prim_data.ptype = PrimitiveTypeTRIANGLE;

			prim_data.ReplaceOrAddVerticeDefinition("POSITION", pos_buf);
			prim_data.ReplaceOrAddVerticeDefinition("NORMAL", nrl_buf);
			prim_data.num_vidx = 3 * 2 * 6;

			uint* puiPlaneIndex = new uint[prim_data.num_vidx];
			for (int i = 0; i < 6; i++)
			{
				puiPlaneIndex[6 * i + 0] = 4 * i + 0;
				puiPlaneIndex[6 * i + 2] = 4 * i + 1;
				puiPlaneIndex[6 * i + 1] = 4 * i + 2;

				puiPlaneIndex[6 * i + 3] = 4 * i + 1;
				puiPlaneIndex[6 * i + 5] = 4 * i + 3;
				puiPlaneIndex[6 * i + 4] = 4 * i + 2;
			}

			prim_data.vidx_buffer = puiPlaneIndex;
			prim_data.num_prims = 12;
			prim_data.idx_stride = 3;

			prim_data.aabb_os.pos_min = f3PosCubeMinOS;
			prim_data.aabb_os.pos_max = f3PosCubeMaxOS;
		}
		else
		{
			// Cube Line Setting //
			prim_data.num_vtx = 4 * 3 * 2;
			vmfloat3* pos_buf = new vmfloat3[prim_data.num_vtx];

			pos_buf[0] = f3PosCubeOS[0];
			pos_buf[1] = f3PosCubeOS[1];
			pos_buf[2] = f3PosCubeOS[1];
			pos_buf[3] = f3PosCubeOS[3];
			pos_buf[4] = f3PosCubeOS[3];
			pos_buf[5] = f3PosCubeOS[2];
			pos_buf[6] = f3PosCubeOS[2];
			pos_buf[7] = f3PosCubeOS[0];

			pos_buf[8] = f3PosCubeOS[0];
			pos_buf[9] = f3PosCubeOS[4];
			pos_buf[10] = f3PosCubeOS[1];
			pos_buf[11] = f3PosCubeOS[5];
			pos_buf[12] = f3PosCubeOS[3];
			pos_buf[13] = f3PosCubeOS[7];
			pos_buf[14] = f3PosCubeOS[2];
			pos_buf[15] = f3PosCubeOS[6];

			pos_buf[16] = f3PosCubeOS[4];
			pos_buf[17] = f3PosCubeOS[5];
			pos_buf[18] = f3PosCubeOS[5];
			pos_buf[19] = f3PosCubeOS[7];
			pos_buf[20] = f3PosCubeOS[7];
			pos_buf[21] = f3PosCubeOS[6];
			pos_buf[22] = f3PosCubeOS[6];
			pos_buf[23] = f3PosCubeOS[4];

			prim_data.is_ccw = true;
			prim_data.is_stripe = false;
			prim_data.ptype = PrimitiveTypeLINE;
			prim_data.ReplaceOrAddVerticeDefinition("POSITION", pos_buf);
			prim_data.num_vidx = 4 * 3 * 2;
			uint* puiLineIndex = new uint[prim_data.num_vidx];
			for (uint i = 0; i < prim_data.num_vidx / 2; i++)
			{
				puiLineIndex[2 * i] = 2 * i;
				puiLineIndex[2 * i + 1] = 2 * i + 1;
			}
			prim_data.vidx_buffer = puiLineIndex;
			prim_data.num_prims = 12;
			prim_data.idx_stride = 2;
			prim_data.aabb_os.pos_min = f3PosCubeMinOS;
			prim_data.aabb_os.pos_max = f3PosCubeMaxOS;
		}
	}

	void GeneratePrimitive_Line(PrimitiveData& prim_data/*out*/, const vmdouble3& pos_s, const vmdouble3& pos_e)
	{
		prim_data.Delete();

		prim_data.ptype = PrimitiveTypeLINE;
		prim_data.is_stripe = false;
		prim_data.is_ccw = true;
		AaBbMinMax* aabb_os_ptr = &prim_data.aabb_os;

		// Initialize.
		aabb_os_ptr->pos_max.x = aabb_os_ptr->pos_max.y = aabb_os_ptr->pos_max.z = -FLT_MAX;
		aabb_os_ptr->pos_min.x = aabb_os_ptr->pos_min.y = aabb_os_ptr->pos_min.z = FLT_MAX;

		// Z축에 대해 Height 1, Radius 1 의 실린더 만들기
		prim_data.num_vtx = 2;
		vmfloat3* pf3PosVertice = new vmfloat3[prim_data.num_vtx];
		//prim_data_ptr.mapVerticeDefinitions.insert(pair<wstring, vmfloat3*>(_T("POSITION"), pf3PosVertice));
		prim_data.ReplaceOrAddVerticeDefinition("POSITION", pf3PosVertice);
		prim_data.num_prims = 1;
		prim_data.idx_stride = 2;
		prim_data.num_vidx = 2;
		uint* vidx_buffer = new uint[prim_data.num_vidx];
		prim_data.vidx_buffer = vidx_buffer;

		pf3PosVertice[0] = vmfloat3(pos_s);
		pf3PosVertice[1] = vmfloat3(pos_e);

		vidx_buffer[0] = 0;
		vidx_buffer[1] = 1;

		for (int i = 0; i < 2; i++)
		{
			aabb_os_ptr->pos_max.x = max(aabb_os_ptr->pos_max.x, (double)pf3PosVertice[i].x);
			aabb_os_ptr->pos_max.y = max(aabb_os_ptr->pos_max.y, (double)pf3PosVertice[i].y);
			aabb_os_ptr->pos_max.z = max(aabb_os_ptr->pos_max.z, (double)pf3PosVertice[i].z);
			aabb_os_ptr->pos_min.x = min(aabb_os_ptr->pos_min.x, (double)pf3PosVertice[i].x);
			aabb_os_ptr->pos_min.y = min(aabb_os_ptr->pos_min.y, (double)pf3PosVertice[i].y);
			aabb_os_ptr->pos_min.z = min(aabb_os_ptr->pos_min.z, (double)pf3PosVertice[i].z);
		}
	}

	void GeneratePrimitive_Arrow(PrimitiveData& prim_data/*out*/, const vmdouble3& pos_s, const vmdouble3& pos_e, const double arrow_body_ratio, const vmdouble2& arrow_components_radius, const int num_interpolation)
	{
		// Unit Height and Applied Radius Arrow //
		vmfloat3 f3PosStart = vmfloat3(pos_s), f3PosEnd = vmfloat3(pos_e);
		vmfloat3 f3VecS2E = f3PosEnd - f3PosStart;
		float fHeight = fLengthVector(&f3VecS2E);
		fNormalizeVector(&f3VecS2E, &f3VecS2E);

		PrimitiveData svxPrimArchiveCone, svxPrimArchiveCylinder;
		vmgeom::GeneratePrimitive_Cone(svxPrimArchiveCone, vmdouble3(0, 0, arrow_body_ratio * fHeight), vmdouble3(0, 0, 1.f * fHeight),
			arrow_components_radius.y, false, num_interpolation);
		vmgeom::GeneratePrimitive_Cylinder(svxPrimArchiveCylinder, vmdouble3(0, 0, 0), vmdouble3(0, 0, arrow_body_ratio * fHeight),
			arrow_components_radius.x, false, false, num_interpolation, 1, 1);

		// Unify Arrow //
		prim_data.ptype = PrimitiveTypeTRIANGLE;
		prim_data.is_stripe = false;
		prim_data.is_ccw = true; // Front is CW
		prim_data.aabb_os.pos_max = vmfloat3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		prim_data.aabb_os.pos_min = vmfloat3(FLT_MAX, FLT_MAX, FLT_MAX);

		prim_data.num_vtx = svxPrimArchiveCone.num_vtx + svxPrimArchiveCylinder.num_vtx;
		vmfloat3* pf3PosVertice = new vmfloat3[prim_data.num_vtx];
		//svxPrimArchiveArrow.mapVerticeDefinitions.insert(pair<wstring, vmfloat3*>(_T("POSITION"), pf3PosVertice));
		prim_data.ReplaceOrAddVerticeDefinition("POSITION", pf3PosVertice);
		vmfloat3* pf3VecNormal = new vmfloat3[prim_data.num_vtx];
		//svxPrimArchiveArrow.mapVerticeDefinitions.insert(pair<wstring, vmfloat3*>(_T("NORMAL"), pf3VecNormal));
		prim_data.ReplaceOrAddVerticeDefinition("NORMAL", pf3VecNormal);
		prim_data.num_prims = svxPrimArchiveCone.num_prims + svxPrimArchiveCylinder.num_prims;
		prim_data.idx_stride = 3;
		prim_data.num_vidx = svxPrimArchiveCone.num_vidx + svxPrimArchiveCylinder.num_vidx;
		uint* puiIndexList = new uint[prim_data.num_vidx];
		prim_data.vidx_buffer = puiIndexList;

		vmfloat3* pf3PosConeVertice = svxPrimArchiveCone.GetVerticeDefinition("POSITION");
		vmfloat3* pf3VecConeNormal = svxPrimArchiveCone.GetVerticeDefinition("NORMAL");
		for (uint i = 0; i < svxPrimArchiveCone.num_vtx; i++)
		{
			pf3PosVertice[i] = pf3PosConeVertice[i];
			pf3VecNormal[i] = pf3VecConeNormal[i];
		}
		vmfloat3* pf3PosCylinderVertice = svxPrimArchiveCylinder.GetVerticeDefinition("POSITION");
		vmfloat3* pf3VecCylinderNormal = svxPrimArchiveCylinder.GetVerticeDefinition("NORMAL");
		for (uint i = 0; i < svxPrimArchiveCylinder.num_vtx; i++)
		{
			pf3PosVertice[svxPrimArchiveCone.num_vtx + i] = pf3PosCylinderVertice[i];
			pf3VecNormal[svxPrimArchiveCone.num_vtx + i] = pf3VecCylinderNormal[i];
		}
		for (uint i = 0; i < svxPrimArchiveCone.num_vidx; i++)
		{
			puiIndexList[i] = svxPrimArchiveCone.vidx_buffer[i];
		}
		for (uint i = 0; i < svxPrimArchiveCylinder.num_vidx; i++)
		{
			puiIndexList[svxPrimArchiveCone.num_vidx + i] = svxPrimArchiveCylinder.vidx_buffer[i] + svxPrimArchiveCone.num_vtx;
		}

		svxPrimArchiveCone.Delete();
		svxPrimArchiveCylinder.Delete();

		// Transform //

		vmmat44f matScale, matLookAt, matRotTranslate, matTr;
		fMatrixScaling(&matScale, &vmfloat3(1.f, 1.f, 1.f));
		vmfloat3 f3VecUp = vmfloat3(0, 0, 1.f), v3VecCrossTest;
		fCrossDotVector(&v3VecCrossTest, &f3VecUp, &f3VecS2E);
		if (fLengthVector(&v3VecCrossTest) <= FLT_MIN)
			f3VecUp = vmfloat3(0, 1.f, 0);
		fMatrixWS2CS(&matLookAt, &f3PosStart, &f3VecUp, &(-f3VecS2E));
		fMatrixInverse(&matRotTranslate, &matLookAt);
		matTr = matScale * matRotTranslate;

		for (uint i = 0; i < prim_data.num_vtx; i++)
		{
			fTransformPoint(&pf3PosVertice[i], &pf3PosVertice[i], &matTr);
			fTransformVector(&pf3VecNormal[i], &pf3VecNormal[i], &matTr);

			prim_data.aabb_os.pos_max.x = max(prim_data.aabb_os.pos_max.x, (double)pf3PosVertice[i].x);
			prim_data.aabb_os.pos_max.y = max(prim_data.aabb_os.pos_max.y, (double)pf3PosVertice[i].y);
			prim_data.aabb_os.pos_max.z = max(prim_data.aabb_os.pos_max.z, (double)pf3PosVertice[i].z);
			prim_data.aabb_os.pos_min.x = min(prim_data.aabb_os.pos_min.x, (double)pf3PosVertice[i].x);
			prim_data.aabb_os.pos_min.y = min(prim_data.aabb_os.pos_min.y, (double)pf3PosVertice[i].y);
			prim_data.aabb_os.pos_min.z = min(prim_data.aabb_os.pos_min.z, (double)pf3PosVertice[i].z);
		}

		//for (uint i = 0; i < prim_data.num_prims; i++)
		//{
		//	int iIndex0 = puiIndexList[3 * i + 0];
		//	int iIndex1 = puiIndexList[3 * i + 1];
		//	int iIndex2 = puiIndexList[3 * i + 2];
		//
		//	vmfloat3 f3Vec01 = pf3PosVertice[iIndex1] - pf3PosVertice[iIndex0];
		//	vmfloat3 f3Vec02 = pf3PosVertice[iIndex2] - pf3PosVertice[iIndex0];
		//	vmfloat3 f3VecNormal;
		//	//fCrossDotVector(&f3VecNormal, &f3Vec02, &f3Vec01);
		//	fCrossDotVector(&f3VecNormal, &f3Vec01, &f3Vec02);
		//	fNormalizeVector(&f3VecNormal, &f3VecNormal);
		//	pf3VecNormal[iIndex0] += f3VecNormal;
		//	pf3VecNormal[iIndex1] += f3VecNormal;
		//	pf3VecNormal[iIndex2] += f3VecNormal;
		//	fNormalizeVector(&pf3VecNormal[iIndex0], &pf3VecNormal[iIndex0]);
		//	fNormalizeVector(&pf3VecNormal[iIndex1], &pf3VecNormal[iIndex1]);
		//	fNormalizeVector(&pf3VecNormal[iIndex2], &pf3VecNormal[iIndex2]);
		//}
	}
}