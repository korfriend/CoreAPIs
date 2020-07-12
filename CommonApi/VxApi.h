#pragma once

#include "../CommonUnits/VimCommon.h"

/**
 * @file VXEngineFrame.h
 * @brief Platform 개발을 위해 VXFramework의 manager frame의 자료구조 및 interface를 정리한 API를 모은 헤더 파일
 * @section Include & Link 정보
 *		- Include : VXEngineFrame.h, VXEngineGlobalUnit.h
 *		- Library : VXEngineFrame.lib, VXEngineGlobalUnit.lib
 *		- Linking Binary : VXEngineFrame.dll, VXEngineGlobalUnit.dll
 */

/**
 * @package vxengineapi
 * @brief Platform 개발을 위한 Native Level VXFramework Engine API를 모은 namespace
 */
using namespace std;
using namespace vmobjects;

namespace vxengineapi
{
	enum EnumVXRDataType {
		vxrDataTypeUNDEFINED = 0,/*!< No Flip */
		vxrDataTypeBYTE,/*!< 1byte, unsigned char, defined as byte */
		vxrDataTypeBYTE4,/*!< 4bytes, unsigned char with 4 channels, defined as vxbyte4 */
		vxrDataTypeCHAR,/*!< 1byte, signed char, defined as char*/
		vxrDataTypeUNSIGNEDSHORT,/*!< 2bytes, unsigned short, defined as ushort*/
		vxrDataTypeSHORT,/*!< 2bytes, signed short, defined as short*/
		vxrDataTypeUNSIGNEDINT,/*!< 4bytes, unsigned int, defined as uint*/
		vxrDataTypeUNSIGNEDINT2,/*!< 8bytes, unsigned int, defined as vxuint2*/
		vxrDataTypeUNSIGNEDINT3,/*!< 12bytes, unsigned int, defined as vxuint3*/
		vxrDataTypeUNSIGNEDINT4,/*!< 16bytes, unsigned int, defined as vxuint4*/
		vxrDataTypeINT,/*!< 4bytes, signed int, defined as int*/
		vxrDataTypeINT2,/*!< 8bytes, signed int, defined as vxint2*/
		vxrDataTypeINT3,/*!< 12bytes, signed int, defined as vxint3*/
		vxrDataTypeINT4,/*!< 16bytes, signed int, defined as vxint4*/
		vxrDataTypeFLOAT,/*!< 4bytes, float, defined as float*/
		vxrDataTypeFLOAT2,/*!< 8bytes, float with 2 channels, defined as vxfloat2*/
		vxrDataTypeFLOAT3,/*!< 12bytes, float with 3 channels, defined as vxfloat3*/
		vxrDataTypeFLOAT4,/*!< 16bytes, float with 4 channels, defined as vxfloat4*/
		vxrDataTypeDOUBLE,/*!< 8bytes, float, defined as double*/
		vxrDataTypeDOUBLE2,/*!< 16bytes, double with 2 channels, defined as vxdouble2*/
		vxrDataTypeDOUBLE3,/*!< 24bytes, double with 3 channels, defined as vxdouble3*/
		vxrDataTypeDOUBLE4,/*!< 32bytes, double with 4 channels, defined as vxdouble4*/
		vxrDataTypeBITBINARY32,/*!< 4bytes, 1bits with 32 channels, defined as int and g_iBitMask*/
		vxrDataTypeSTRUCTURED,/*!< custom defined bytes, defined as custom structure*/
		vxrDataTypeALIGN16STRUCTURED,/*!< custom defined bytes, defined as custom structure, aligned by 16 bytes*/
		vxrDataTypeSTRING, /*!< undefined bytes size, wstring is used*/
		vxrDataTypeBOOLEAN,/*!< 1byte, bool, defined as bool */
		vxrDataTypeMATRIX /*!< 64bytes, vmmat44 is used*/
	};

	enum EnumVXMModuleType {
		vxmModuleTypeNONE = 0,/*!< Undefined, There is no such a module */
		vxmModuleTypeRENDER,/*!< Rendering module */
		vxmModuleTypeTMAP,/*!< OTF setting 관련 module */
		vxmModuleTypeVGENERATION,/*!< VObject 생성/처리/분석 관련 module */
		vxmModuleTypeETC/*!< Customized-Define Module */
	};
/**
 * @class SIVXCameraStateDescription
 * @brief Camera 관련 states를 [Get/Set] 하기 위한 자료구조
 * @sa
 * vxobjects::CVXCObject \n
 * vxengineapi::VXEImageplaneSetCameraState, vxengineapi::VXEImageplaneGetCameraState
 */
struct SIVXCameraStateDescription{
	/// Camera 위치
	vmfloat3 f3PosCamera; 
	/// Camera View Vector
	vmfloat3 f3VecView;
	/// Camera Up Vector
	vmfloat3 f3VecUp;
	/// Camera Space 상의 Image Plane 크기 (상이 맺히는 Projection Plane의 크기)
	vmfloat2 f2ImageplaneSize; 
	/// Camera 위치 기준 Near Plane, 상이 맺히는 Projection Plane
	float fNearPlaneDistFromCamera;	
	/// Camera 위치 기준 Far Plane
	float fFarPlaneDistFromCamera;	
	/// Perspective 모드(bIsPerspective == true)에서 Up Vector 방향 기준 Field of View 의 각도, radian.
	float fFovY;	
	/// Perspective 여부, true : Perspective, false : Orthogonal
	bool bIsPerspective;	
	/// Image Plane 의 해상도 변화시 가로 세로 ratio 를 유지시키기 위한 Fitting 정보로, Orthogonal Projection, Stage Setting 에서 결정됨
	vmfloat2 f2FittingSize;
	/// Image Plane 의 해상도 변화시 가로 세로 ratio 를 유지시키기 위한 Fitting 정보로, Perspective Projection, Stage Setting 에서 결정됨
	float fFittingFovY;
	/// constructor, 모두 0 (NULL or false)으로 초기화
	SIVXCameraStateDescription(){
		memset(&f3PosCamera, 0, sizeof(vmfloat3)); memset(&f3VecView, 0, sizeof(vmfloat3));
		memset(&f3VecUp, 0, sizeof(vmfloat3)); memset(&f2ImageplaneSize, 0, sizeof(vmfloat2));
		fNearPlaneDistFromCamera = fFarPlaneDistFromCamera = fFovY = 0;
		bIsPerspective = false;
	}
};

/**
 * @class SIVXTransferfunctionArchiveInfo
 * @brief VXTObject의 @ref vxobjects::SVXTransferFunctionArchive 정보를 [Get] 하기 위한 자료구조
 * @sa
 * vxobjects::SVXTransferFunctionArchive, vxobjects::CVXTObject \n
 * vxengineapi::VXETransferfunctionGetArchiveInfo
 */
struct SIVXTransferfunctionArchiveInfo{
	/// TObject가 참고해서 만든 VXVObjectVolume의 ID
	int iRefObjectID;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive 에 할당되어 있는 OTF array 의 pointer dimension
	 * @details iNumDims = 1 or 2 or 3
	 */
	int iNumDims;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive 에 할당되어 있는 OTF array 의 각 dimension 에 대한 array 크기
	 * @details 
	 * i3DimSizes.x : 1st dimension 의 array 크기 \n
	 * i3DimSizes.y : 2nd dimension 의 array 크기 \n
	 * i3DimSizes.z : 3rd dimension 의 array 크기 \n
	 * Valid dimension에 대하여 i3DimSizes.xyz > 0, Invalid demension에 대하여 i3DimSizes.xyz <= 0
	 */
	vmint3 i3DimSizes;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive 에 할당되어 있는 각각의 dimension에 대한 OTF array 값 중 최소값
	 * @details
	 * i3ValidMinIndex.x : 1st dimension 의 array 의 최소값 \n
	 * i3ValidMinIndex.y : 2nd dimension 의 array 의 최소값 \n
	 * i3ValidMinIndex.z : 3rd dimension 의 array 의 최소값
	 */
	vmint3 i3ValidMinIndex;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive 에 할당되어 있는 각각의 dimension에 대한 OTF array 값 중 최대값
	 * @details 
	 * i3ValidMaxIndex.x : 1st dimension 의 array 의 최대값 \n
	 * i3ValidMaxIndex.y : 2nd dimension 의 array 의 최대값 \n
	 * i3ValidMaxIndex.z : 3rd dimension 의 array 의 최대값
	 */
	vmint3 i3ValidMaxIndex;
	/// constructor, 모두 0 (NULL or false)으로 초기화
	SIVXTransferfunctionArchiveInfo(){
		iRefObjectID = 0; iNumDims = 0; 
		memset(&i3DimSizes, 0, sizeof(i3DimSizes));
		memset(&i3ValidMinIndex, 0, sizeof(i3ValidMinIndex));
		memset(&i3ValidMaxIndex, 0, sizeof(i3ValidMaxIndex));
	}
};
/**
 * @class SIVXTransferfunctionOpticalValue
 * @brief VXTObject의 @ref vxobjects::SVXTransferFunctionArchive 의 array 를 [Get/Set] 하기 위한 자료구조
 * @sa
 * vxengineapi::SIVXTransferfunctionArchiveInfo, vxengineapi::VXETransferfunctionGetOpticalValue \n
 * vxobjects::SVXTransferFunctionArchive, vxobjects::CVXTObject 
 * 
 */
struct SIVXTransferfunctionOpticalValue{
	/**
	 * @brief array pf4OpticalValue 의 크기
	 */
	int iSizeArray;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive 에 할당되어 있는 OTF array 를 담은 array
	 * @details
	 * pf4OpticalValue[0 ~ (iSizeArray - 1)].xyzw : normalized RGBA 에 대응 \n
	 * 1D OTF 로 정의되어 (dimension == 1 or 2 && 2nd dimension metric == vxrMetricTypeINDEX) 있을 때 @ref vxengineapi::SIVXTransferfunctionArchiveInfo 의 i3DimSizes.x 값과 같음 \n
	 * 2D OTF 로 정의되어 있을 때 @ref vxengineapi::SIVXTransferfunctionArchiveInfo 의 i3DimSizes.x*i3DimSizes.y 와 같음 \n
	 * 3D OTF 로 정의되어 있을 때 @ref vxengineapi::SIVXTransferfunctionArchiveInfo 의 i3DimSizes.x*i3DimSizes.y*i3DimSizes.z 와 같음
	 */
	vmbyte4* py4OpticalValue;	// xyzw : rgba
	/// constructor, 모두 0 (NULL or false)으로 초기화
	SIVXTransferfunctionOpticalValue(){
		iSizeArray = 0; py4OpticalValue = NULL;
	}
};
/**
 * @class SIVXOutBufferInfo
 * @brief VXIObject의 Frame Buffer를 [Get]하기 위한 자료구조
 * @sa
 * vxobjects::CVXIObject \n
 * vxengineapi::VXEImageplaneGetOutBufferInfo
 */
struct SIVXOutBufferInfo{
	/**
	 * @brief Frame Buffer 의 i2FrameBufferSize.xy : (width, height)
	 */
	vmint2 i2FrameBufferSize;
	/**
	 * @brief array로 정의된 Frame Buffer
	 */
	void* pvBuffer;
#ifdef __WINDOWS
	/**
	 * @brief win32에서 File Memory를 통한 buffer interopaeration을 위한 handle
	 */
	HANDLE hFileMap;
#endif
	/**
	 * @brief Frame Buffer 의 Data Type
	 */
	data_type dtype;
	/// constructor, 모두 0 (NULL or false)으로 초기화
	SIVXOutBufferInfo(){
		memset(&i2FrameBufferSize, 0, sizeof(vmint2));
		pvBuffer = NULL;
#ifdef __WINDOWS
		hFileMap = NULL;
#endif
	}
};
/**
 * @class SIVXPrimitiveInfo
 * @brief @ref vxobjects::SVXPrimitiveDataArchive 에 정의된 Primitive 정보를 [Get]하기 위한 자료구조
 * @sa
 * vxobjects::SVXPrimitiveDataArchive, vxobjects::CVXVObjectPrimitive \n
 * vxengineapi::VXEPrimitiveGetPrimitiveInfo
 */
struct SIVXPrimitiveInfo{
	/// Primitive로 구성 된 객체의 Vertex 개수
	uint uiNumVertice;
	/// Primitive 개수
	uint uiNumPrimitives;
	/// @ref vxobjects::CVXVObjectPrimitive 에 정의되어 있는 객체들의 OS 상의 Axis-aligned bounding box에 대한 최대 정점
	vmfloat3 f3PosMaxBox;
	/// @ref vxobjects::CVXVObjectPrimitive 에 정의되어 있는 객체들의 OS 상의 Axis-aligned bounding box에 대한 최소 정점
	vmfloat3 f3PosMinBox;
	/// Primitive로 구성 된 객체의 Polygon 에 대한 Topology 연결 상태
	bool bIsTopologyConnected;
};
/**
 * @class SIVXVolumeInfo
 * @brief @ref vxobjects::SVXVolumeDataArchive 에 정의된 Volume 정보 및 값을 [Get/Set]하기 위한 자료구조
 * @sa
 * vxobjects::SVXVolumeDataArchive, @ref vxobjects::CVXVObjectVolume \n
 * vxhelpers::VXHVolumeFillHistogramBasedOnVolumeValues \n
 * vxengineapi::VXEVolumeGetVolumeInfo, vxengineapi::VXEVolumeSetVolumeInfo
 */
struct SIVXVolumeInfo{
	/**
	 * @brief Volume에 대한 Histogram 을 정의하는 array
	 * @details 
	 * array 크기는 uint(d2MinMaxValue.y - d2MinMaxValue.x + 1.5) \n
	 * pullHistogram[volume value] = # of voxels
	 */
	ullong* pullHistogram;
	/**
	 * @brief Volume으로 저장된(ppvVolumeSlices)의 최소값 d2MinMaxValue.x, 최대값 d2MinMaxValue.y
	 */
	vmdouble2 d2MinMaxValue;
	/**
	 * @brief Volume으로 저장되기 전에 정의된 최소값 d2ActualMinMaxValue.x, 최대값 d2ActualMinMaxValue.y
	 * @par ex. 
	 * file format float으로 -1.5 ~ 2.5 저장된 볼륨을 ushort 으로 저장할 경우 
	 * @par 
	 * >> d2MinMaxValue = vmdouble(0, 65535), d2ActualMinMaxValue = vmdouble(-1.5, 2.5);
	 */
	vmdouble2 d2ActualMinMaxValue;
	/**
	 * @brief Volume 의 크기 i3VolumeSize = (width, height, depth or slices) \n
	 * i3SizeExtraBoundary 가 포함되지 않음
	 */
	vmint3 i3VolumeSize;
	/**
	 * @brief CPU Memory Access Violation을 피하기 위해 System Memory 상의 Extra Boundary 영역의 한쪽면 크기
	 * @details i3SizeExtraBoundary = (한쪽 x축 방향 크기, 한쪽 y축 방향 크기, 한쪽 z축 방향 크기)
	 */
	vmint3 i3SizeExtraBoundary;
	
	/**
	 * @brief 단위 Voxel에 대해 OS 상의 cell edge의 WS 상의 크기
	 * @details d3VoxelPitch = (x edge 크기, y edge 크기, z edge 크기)
	 */
	vmdouble3 d3VoxelPitch;
	/**
	 * @brief Volume을 저장한 2D array
	 * @details 
	 * 실제 할당된 x 축 방향 크기 = i3VolumeSize.x + i3SizeExtraBoundary.x*2 \n
	 * 실제 할당된 y 축 방향 크기 = i3VolumeSize.y + i3SizeExtraBoundary.y*2 \n
	 * 실제 할당된 z 축 방향 크기 = i3VolumeSize.z + i3SizeExtraBoundary.z*2 \n
	 * @par ex. 
	 * ushort 512x512x512 Volume에서 (100, 120, 150) index 값 sample \n
	 * @par
	 * >> int iSamplePosX = 100 + i3SizeExtraBoundary.x; \n
	 * >> int iSamplePosY = 120 + i3SizeExtraBoundary.y; \n
	 * >> int iSamplePosZ = 150 + i3SizeExtraBoundary.z; \n
	 * >> ushort usValue = ((ushort**)ppvVolumeSlices)[iSamplePosZ][iSamplePosX + iSamplePosY*(i3VolumeSize.x + i3SizeExtraBoundary.x*2)];
	 */
	void** ppvVolumeSlices;
	/**
	 * @brief Volume array 의 data type
	 */
	data_type stored_dtype;
	/**
	 * @brief 메모리에 저장되기 전 Volume Original data type
	 * @par ex. 
	 * file format float 저장된 볼륨을 ushort 으로 저장할 경우
	 * @par
	 * >> eDataType = vxrDataTypeUNSIGNEDSHORT, eDataTypeOriginal = vxrDataTypeFLOAT;
	 */
	data_type origin_dtype;
	
	/**
	 * @brief memory 에 저장된 volume space (샘플 좌표)와 초기 world space 에 배치되는 변환 matrix 정의
	 */
	AxisInfoOS2WS svxAlignAxisOS2WS;

	/// constructor, 모두 0 (NULL or false)으로 초기화
	SIVXVolumeInfo(){ pullHistogram = NULL; ppvVolumeSlices = NULL;
		d2MinMaxValue = d2ActualMinMaxValue = vmdouble2(0, 0);
		i3VolumeSize = i3SizeExtraBoundary = vmint3(0, 0, 0);
		d3VoxelPitch = vmdouble3(0, 0, 0);
	}
};
/**
 * @class SIVXVolumeSliceInfo
 * @brief @ref vxobjects::SVXVolumeDataArchive 에 정의된 Volume 의 z축 Slice 값을 [Get]하기 위한 자료구조
 * @sa
 * vxobjects::SVXVolumeDataArchive, vxobjects::CVXVObjectVolume \n
 * ****** 관련 함수 및 자료 구조 수정 계획! ******* \n
 * ****** 관련 함수 사용하지 마세요!!!!! ********** \n
 * vxengineapi::VXEVolumeGetVolumeZaxisSliceInfo
 */
struct SIVXVolumeSliceInfo{
	/**
	 * @brief Volume의 z축 방향 단면(직사각형)을 저장하기 위한 double 형 array
	 * @details
	 * 모든 data type의 Volume Value가 double 형으로 casting 되어 저장 \n
	 * 내부적으로 ...
	 */
	double* pdValues;
	/**
	 * @brief pdValues에 저장된 값에 대한 최소/최대값
	 * @details d2MinMaxValue = (최소값, 최대값)
	 */
	vmdouble2 d2MinMaxValue;
	
	/**
	 * @brief Volume의 z축 방향 단면(직사각형)의 pixel 단위 width, height 크기(or 개수)
	 * @details i2DimSize = (width, height)
	 */
	vmint2 i2DimSize;

	/// constructor, 모두 0 (NULL or false)으로 초기화
	SIVXVolumeSliceInfo(){ pdValues = NULL; }
	~SIVXVolumeSliceInfo(){ Delete(); }
	void Delete(){ if(pdValues){ delete[] pdValues; pdValues = NULL; } }
};

// NOTICE //
// Every parameter with ment '/*out*/' is used as return value which should be set!
// Every parameter with ment '/*optional out*/' can be set as NULL which means DO NOT USE or IGNORE.
// The naming policy : VXE[Object/VObject/Volume/Primitive/CustomList/Transferfunction/Imageplane/CUSTOM][Get/Set/CUSTOM][NAME]

// System Helper
/*!
 * @fn __vmstatic void vxengineapi::VXEGetSystemMemoryInfo(ullong* pullFreeMemoryBytes, ullong* pullAvailablePhysicalMemoryBytes)
 * @brief 현재 구동되고 있는 OS를 통한 System Memory 상태를 제공
 * @param pullFreeMemoryBytes [out] \n ullong \n 현재 사용 가능한 메모리 크기 (bytes)
 * @param pullAvailablePhysicalMemoryBytes [out] \n ullong \n 현재 System 에 인식되는 물리 메모리 크기 (bytes)
 * @remarks x86 또는 x64, 현재 구동 OS의 상태에 따라 실제 메모리와 다르게 잡힐 수 있음.
 * @sa
 * @ref vxhelpers::VXHGetSystemMemoryInfo
*/
__vmstatic void VXEGetSystemMemoryInfo(ullong* pullFreeMemoryBytes/*out*/, ullong* pullAvailablePhysicalMemoryBytes/*out*/);


/*!
* @brief 해당 ID의 VXVObject를 얻는 함수
* @param puiSizeOfVolumesKB [out] \n uint \n Volume 사이즈를 저장하는 uint 포이터로 KB 단위
* @param puiSizeOfMeshesKB [out] \n uint \n Mesh 사이즈를 저장하는 uint 포이터로 KB 단위
* @param puiSizeOfIObjectsKB [out] \n uint \n Frame buffer 사이즈를 저장하는 uint 포이터로 KB 단위
* @param puiSizeOfEtcKB [out] \n uint \n TObject 및 LObject 사이즈를 저장하는 uint 포이터로 KB 단위
* @remarks 포인터가 NULL 이면 값을 저장하지 않음
*/
__vmstatic void VXEGetAllocatedResources(uint* puiSizeOfVolumesKB /*out*/, 
	uint* puiSizeOfMeshesKB /*out*/, 
	uint* puiSizeOfIObjectsKB /*out*/, 
	uint* puiSizeOfEtcKB /*out*/);

// Pair
/*!
 * @fn __vmstatic bool vxengineapi::VXEBeginEngineLib()
 * @brief VXEngine (Manager or Engine Frame of VXFramework) 을 시작함
 * @return bool \n VXFramework이 구동 가능한 환경이면 true 반환, 그렇지 않으면 내부 Class Instance들을 생성 않고 false 반환
 * @remarks 
 * @ref vxengineapi::VXEEndEngineLib 와 pair 를 이루어 사용되야 함 \n
 * 내부적으로 각종 Manager Classes 생성 \n 
 * Singleton 으로 사용하지 않으면 Instance가 계속 생성될 수 있음.
 * @sa vxengineapi::VXEEndEngineLib
*/
__vmstatic bool VXEBeginEngineLib();
/*!
 * @fn __vmstatic bool vxengineapi::VXEEndEngineLib()
 * @brief VXEngine (Manager or Engine Frame of VXFramework) 을 종료함
 * @return bool \n 성공적으로 종료하면 true 반환
 * @remarks 
 * @ref vxengineapi::VXEBeginEngineLib 와 pair 를 이루어 사용되야 함 \n 
 * 내부적으로 각종 Manager Classes 및 Resources 모두 정리 및 삭제 \n 
 * @sa vxengineapi::VXEBeginEngineLib
*/
__vmstatic bool VXEEndEngineLib();

// Common Object
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectIsGenerated(int iObjectID)
 * @brief 해당 ID의 VXObject가 생성되어 있는지 확인하는 함수
 * @param iObjectID \n int \n VXObject ID
 * @return bool \n Resource Manager에 등록되어 있으면 true 반환, 그렇지 않으면 false 반환
 * @remarks 
*/
__vmstatic bool VXEObjectIsGenerated(int iObjectID);


// Common Object
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectRemove(int iObjectID)
 * @brief 해당 ID의 VXObject를 Resource에서 삭제하는 함수
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return bool \n 삭제가 완료되면 true 반환, 그렇지 않으면 (ex. 해당 VXObject가 없는 경우) false 반환
 * @remarks 
*/
__vmstatic bool VXEObjectRemove(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectSetMent(int iObjectID, string strHelpMent)
 * @brief 해당 VXObject에 Custom Mention을 작성하는 함수
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param strHelpMent [in] \n string \n Custom Mention
 * @return bool Mention이 등록 완료되면 true 반환, 그렇지 않으면 (ex. 해당 VXObject가 없는 경우) false 반환
 * @remarks 주로 VXObject 단위의 출처 및 사용 목적 등이 기입되며, Mention이 없는 경우 ""를 반환
 * @sa vxobjects::CVXObject, vxengineapi::VXEObjectGetMent
*/
__vmstatic bool VXEObjectSetMent(int iObjectID, string strHelpMent);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectGetMent(int iObjectID, string* pstrHelpMent)
 * @brief 해당 VXObject에 등록되어 있는 Custom Mention을 얻음
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param pstrHelpMent [out] \n string \n 해당 VXObject에 등록되어 있는 Custom Mention이 저장되어 있는 string의 포인터
 * @return bool \n Mention을 얻으면 true 반환, 그렇지 않으면 (ex. 해당 VXObject가 없는 경우) false 반환
 * @remarks Mention이 없는 경우 ""를 반환
 * @sa vxobjects::CVXObject, vxengineapi::VXEObjectSetMent
*/
__vmstatic bool VXEObjectGetMent(int iObjectID, string* pstrHelpMent/*out*/);
/*!
 * @fn __vmstatic int vxengineapi::VXEObjectGetRelatedObjectID(int iObjectID)
 * @brief 해당 VXObject와 직접적으로 연관된 최우선 순위 VXObject ID를 얻는 함수
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return int \n 연관된 최우선 순위 VXObject ID
 * @remarks 연관된 VXObject가 없는 경우 0 반환
 * @sa vxobjects::CVXObject
*/
__vmstatic int VXEObjectGetRelatedObjectID(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectIsDefined(int iObjectID)
 * @brief 생성되어 있는 해당 ID의 VXObject에 contents가 정의되어 있는지 확인
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return bool \n VXObject가 생성되어 Resource로 등록되어 있고, contents가 정의되어 있으면 true 반환, 그렇지 않으면 false 반환
 * @remarks 
 * @sa
 * vxobjects::CVXObject \n
 * vxobjects::CVXIObject::InsertFrameBuffer, vxobjects::CVXIObject::ResizeFrameBuffer \n
 * vxobjects::CVXTObject::RegisterTransferFunctionArchive \n
 * vxobjects::CVXVObjectVolume::RegisterVolumeArchive, vxobjects::CVXVObjectPrimitive::RegisterPrimitiveData \n
 * vxobjects::CVXLObject::RegisterList
*/
__vmstatic bool VXEObjectIsDefined(int iObjectID);
/*!
 * @fn __vmstatic EnumVXRObjectType vxengineapi::VXEObjectGetTypeFromID(int iObjectID)
 * @brief 생성되어 있는 해당 ID의 VXObject에 대한 @ref vxenums::EnumVXRObjectType 을 얻는 함수
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return EnumVXRObjectType \n 해당 VXObject가 생성되어 있지 않으면 @ref vxenums::vxrObjectTypeNONE 반환
 * @remarks 
 * @sa
 * vxengineapi::VXEImageplaneGenerateNew, vxengineapi::VXETransferfunctionGenerateNew \n
 * vxengineapi::VXEVolumeGenerateNew, vxengineapi::VXEPrimitiveGenerateNew \n
 * vxengineapi::VXECustomListGenerateNew
*/
__vmstatic EvmObjectType VXEObjectGetTypeFromID(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectSetCustomParameter(int iObjectID, string strParameterName, string strParameterValue)
 * @brief 생성되어 있는 해당 ID의 최상위 Object @ref vxobjects::CVXObject 에 사용자 정의 parameter 를 설정
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param strParameterName [in] \n string \n @ref 파라미터 이름
 * @param eDataType [in] \n EnumVXRDataType \n @ref 파라미터 타입 [string, bool, int, vmint[2, 3, 4], double, vmdouble[2, 3, 4], vmmat44]
 * @param *pvValue [in] \n void \n @ref 값을 저장하는 포인터
 * @return bool \n 해당 작업이 성공적으로 수행되면 true 반환, 그렇지 않으면 false 반환
 * @remarks 
 * @sa
 * vxobjects::CVXObject:RegisterCustomParameter \n
 */
__vmstatic bool VXEObjectSetCustomParameter(int iObjectID, string strParameterName, EnumVXRDataType eDataType, void* pvValue);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectGetCustomParameter(int iObjectID, string strParameterName, string* pstrParameterValue/)
 * @brief 생성되어 있는 해당 ID의 최상위 Object @ref vxobjects::CVXObject 에 저장되어 있는 사용자 정의 parameter 를 읽음
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param strParameterName [in] \n string \n @ref 파라미터 이름
 * @param eDataType [in] \n EnumVXRDataType \n @ref 파라미터 타입 [string, bool, int, vmint[2, 3, 4], double, vmdouble[2, 3, 4], vmmat44]
 * @param *pvValue [in] \n void \n @ref 값을 저장하는 포인터
 * @return bool \n 해당 작업이 성공적으로 수행되면 true 반환, 그렇지 않으면 false 반환
 * @remarks 
 * @sa
 * vxobjects::CVXObject:GetCustomParameter \n
 */
__vmstatic bool VXEObjectGetCustomParameter(int iObjectID, string strParameterName, EnumVXRDataType eDataType, void* pvValue/*out*/);

/*!
 * @fn __vmstatic double vxengineapi::VXEFrameWorkGetProgress()
 * @brief namespace @ref vxengineapi 에서 수행되는 static progress 를 반환
 * @return double \n 0.0 ~ 100.0
 * @remarks 작업 로드가 큰 function 외엔 거의 0.0 반환
*/
__vmstatic double VXEFrameWorkGetProgress();

// GPU Interface
/*!
 * @fn __vmstatic void vxengineapi::VXEGpuCleanAllResources()
 * @brief 현재 GPU Memory로 등록된 모든 resource를 해제함
 * @remarks @ref vxgpuinterface::EnumVXGSdkType 에 등록된 모든 GPU SDK에 대해 작업 수행
 * @sa @ref vxgpuinterface::EnumVXGSdkType, vxgpuinterface::CVXGPUManager
*/
__vmstatic void VXEGpuCleanAllResources();
/*!
 * @fn __vmstatic void vxengineapi::VXEGpuRemoveResource(int iIObectID)
 * @brief 해당 ID의 VXObject으로부터 생성된 모든 GPU resource를 GPU Memory에서 해제
 * @param iObectID [in] \n int \n VXObject ID 
 * @remarks 
 * @sa vxgpuinterface::CVXGPUManager
*/
__vmstatic void VXEGpuRemoveResource(int iObectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEGpuGetMemoryBytes(uint* puiDedicatedGpuMemoryBytes, uint* puiFreeMemoryBytes)
 * @brief CUDA API를 통해 GPU 메모리 상태를 얻음
 * @param puiDedicatedGpuMemoryBytes [out] \n uint \n GPU 전용 Memory 크기를 저장할 uint의 포인터, bytes
 * @param puiFreeMemoryBytes [out] \n uint \n 현재 사용 가능한 GPU Memory 크기를 저장할 uint의 포인터, bytes
 * @return bool \n CUDA API를 통해 GPU 메모리 상태를 얻는데 성공하면 true 반환, 그렇지 않으면 false 반환
 * @sa vxgpuinterface::CVXGPUManager
*/
__vmstatic bool VXEGpuGetMemoryBytes(uint* puiDedicatedGpuMemoryBytes/*out*/, uint* puiFreeMemoryBytes/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEGpuGetMemoryBytesUsedInVXFramework(uint* puiGpuAllMemoryBytes, uint* puiGpuVObjectMemoryBytes)
 * @brief 현재 활성화된 VXFramework 에서 사용 중인 GPU mamory size를 얻음
 * @param puiGpuAllMemoryBytes [out] \n uint \n GPU 전용 Memory 크기를 저장할 uint의 포인터, bytes
 * @param puiGpuVObjectMemoryBytes [out] \n uint \n 
 * GPU 에 할당되어 있는 VObject의 크기를 저장할 uint의 포인터, bytes \n
 * Default 포인터가 NULL일 경우 포인터에 값을 쓰지 않음
 * @return bool \n GPU 메모리 상태를 얻는데 성공하면 true 반환, 그렇지 않으면 false 반환
 * @remarks
 * @sa vxgpuinterface::CVXGPUManager
*/
__vmstatic bool VXEGpuGetMemoryBytesUsedInVXFramework(uint* puiGpuAllMemoryBytes/*out*/, uint* puiGpuVObjectMemoryBytes/*optional out*/ = NULL);

// Common VObject
/*!
 * @fn __vmstatic bool vxengineapi::VXEVObjectGetBoundingOrthoBox(int iObjectID, bool bIsBoxDefinedInOS, vmdouble3* pd3PosOrthoBoxMin, vmdouble3* pd3PosOrthoBoxMax)
 * @brief VXVObject에 대한 Bounding Box의 WS 위치를 얻음
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param bIsBoxDefinedInOS [in] \n bool \n 
 * true면 OS 상의 Bounding Box의 최소/최대점이 WS 상으로 변환된 위치점을 제공 \n
 * false면 WS 상의 Bounding Box의 최소/최대점을 제공 \n
 * @param pd3PosOrthoBoxMin [out] \n vmdouble3 \n (OS or WS상의) Bounding Box에 대한 WS 상의 최소점을 저장할 vmdouble의 포인터  
 * @param pd3PosOrthoBoxMax [out] \n vmdouble3 \n (OS or WS상의) Bounding Box에 대한 WS 상의 최대점을 저장할 vmdouble의 포인터
 * @return bool \n 함수가 성공적으로 수행되면 true, 그렇지 않으면 false 반환
 * @remarks 해당 VXObject는 @ref vxobjects::CVXVObject 이어야 함.
 * @sa vxobjects::CVXVObject
*/
__vmstatic bool VXEVObjectGetBoundingOrthoBox(int iObjectID, bool bIsBoxDefinedInOS /*false : WS*/, vmdouble3* pd3PosOrthoBoxMin/*out*/, vmdouble3* pd3PosOrthoBoxMax/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVObjectGetMatrixBetweenOSandWS(int iObjectID, vmmat44* pmatOS2WS, vmmat44* pmatWS2OS)
 * @brief VXVObject에 대하여 OS 와 WS 간 변환 matrix를 제공
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param pmatOS2WS [out] \n vmmat44 \n OS에서 WS에서 변환하는 matrix를 저장할 vmmat44 포인터, NULL이면 사용 안 함
 * @param pmatWS2OS [out] \n vmmat44 \n WS에서 OS에서 변환하는 matrix를 저장할 vmmat44 포인터, NULL이면 사용 안 함
 * @return bool \n 작업이 성공적으로 수행되면 true, 그렇지 않으면 false 반환
 * @remarks 
 * RHS, row major로 정의된 vmmat44 제공 \n
 * OS에서 정의되는 VXVObject가 WS상에 배치되는 변환을 정의
 * @sa 
 * vxobjects::CVXVObject \n
 * vxengineapi::VXEVObjectSetMatrixOS2WS
*/
__vmstatic bool VXEVObjectGetMatrixBetweenOSandWS(int iObjectID, vmmat44* pmatOS2WS/*optional out*/, vmmat44* pmatWS2OS/*optional out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVObjectSetMatrixOS2WS(int iObjectID, const vmmat44* pmatOS2WS)
 * @brief VXVObject에 대하여 OS 와 WS 간 변환 matrix를 지정
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param pmatOS2WS [in] \n vmmat44 \n OS에서 WS에서 변환하는 matrix에 대한 vmmat44 포인터
 * @return bool \n 작업이 성공적으로 수행되면 true, 그렇지 않으면 false 반환
 * @remarks 
 * VXVObject를 WS상에 배치하는 함수로 사용됨. \n
 * RHS, row major로 정의되야 함.
 * @sa
 * vxobjects::CVXVObject \n
 * vxengineapi::VXEVObjectGetMatrixBetweenOSandWS
*/
__vmstatic bool VXEVObjectSetMatrixOS2WS(int iObjectID, const vmmat44* pmatOS2WS);
/*!
* @fn __vmstatic bool vxengineapi::VXEVObjectDeleteCustomObjects(int iObjectID)
* @brief VXVObject에 등록되어 있는 vxobjects::SVXVObjectBaseCustomData 를 모두 삭제
* @param iObjectID [in] \n int \n VXObject ID
* @return bool \n 작업이 성공적으로 수행되면 true, 그렇지 않으면 false 반환
* @sa
* vxobjects::SVXVObjectBaseCustomData
*/
__vmstatic bool VXEVObjectRemoveCustomObjects(int iObjectID);


// Volume Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGenerateNew(int* piObjectID)
 * @brief VXVObjectVolume 을 생성하고 이를 Resource Manager에 등록함.
 * @param piObjectID [out] \n int \n Resource Manager에 등록된 VXVObjectVolume의 ID를 저장할 int의 포인터
 * @param iForcedObjectID [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks VXObejct가 생성만 됐을 뿐 아직 내용물이 채워지진 않은 상태.
 * @sa vxobjects::CVXVObjectVolume
*/
__vmstatic bool VXEVolumeGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeCopy(int iObjectDestID, int iObjectSrcID)
 * @brief iObjectSrcID의 VXVObjectVolume에 저장된 Volume을 iObjectDestID의 VXVObjectVolume에 복사
 * @param iObjectDestID [in] \n int \n Source VXVObjectVolume ID
 * @param iObjectSrcID [in] \n int \n Destination VXVObjectVolume ID
 * @return bool \n Volume 복사가 성공적으로 수행되면 true, 그렇지 않으면 false 반환
 * @remarks @ref vxobjects::SVXVolumeDataArchive의 Volume정보와 Volume Data를 저장한 array가 별도로 생성되어 Copy됨.
 * @sa 
 * vxobjects::SVXVolumeDataArchive \n
 * vxobjects::CVXVObjectVolume
*/
__vmstatic bool VXEVolumeCopy(int iObjectDestID, int iObjectSrcID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo)
 * @brief iObjectID의 VXVObjectVolume에 저장된 @ref vxobjects::SVXVolumeDataArchive의 정보를 얻음
 * @param iObjectID [in] \n int \n Source VXVObjectVolume ID
 * @param psivxVolumeInfo [out] \n SIVXVolumeInfo \n @ref vxobjects::SVXVolumeDataArchive 에 대응하는 Volume 정보를 저장할 자료구조 SIVXVolumeInfo 의 포인터
 * @return bool \n Volume 정보를 성공적으로 얻으면 true, 그렇지 않으면 false 반환
 * @remarks 
 * @sa vxobjects::SVXVolumeDataArchive
*/
__vmstatic bool VXEVolumeGetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeSetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo)
 * @brief 메모리에 저장되어 있는 volume으로 해당 ID의 VXVObjectVolume을 정의.
 * @param iObjectID [in] \n int \n volume이 정의될 Resource Manager에 등록된 VXVObjectVolume의 ID
 * @param psivxVolumeInfo [in] \n SIVXVolumeInfo \n volume을 정의하고 있는 SIVXVolumeInfo 의 포인터
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * SIVXVolumeInfo::pullHistogram 이 정의되어 있으면 그대로 VXVObjectVolume에 정의되며, NULL 이면 내부에서 Histogram 을 생성하여 정의 \n
 * SIVXVolumeInfo에서 정의된 포인터(pullHistogram, ppvVolumeSlices)가 가리키는 할당된 메모리는 Set과 동시에 Resource Manager에 등록되어, VXFramework에서 관리하게 됨.
 * @sa vxengineapi::VXEVolumeGenerateNew, vxobjects::CVXVObjectVolume
*/
__vmstatic bool VXEVolumeSetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGetVolumeZaxisSliceInfo(int iObjectID, int iSliceIndex, SIVXVolumeSliceInfo* psivxVolumeSliceInfo)
 * @brief 사용하지 마세요!!! 관련 자료구조 및 동작을 refactoring 할 예정!
 * @param iObjectID 
 * @param iSliceIndex 
 * @param psivxVolumeSliceInfo 
 * @return __vmstatic bool 
 * @remarks 
*/
__vmstatic bool VXEVolumeGetVolumeZaxisSliceInfo(int iObjectID, int iSliceIndex, SIVXVolumeSliceInfo* psivxVolumeSliceInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGetActualValuesAlongLine(int iObjectID, vmfloat3 f3PosStartWS, vmfloat3 f3PosEndWS, int iNumSamples, vector<double>* pvtrSampleValues)
 * @brief iObjectID의 VXVObjectVolume에 저장된 Volume에 대해 파라미터로 정의되는 Line 상의 Volume 값을 sample 하는 함수
 * @param iObjectID [in] \n int \n Source VXVObjectVolume ID
 * @param f3PosStartWS [in] \n vmfloat3 \n WS에서 정의되는 Line의 시작점
 * @param f3PosEndWS [in] \n vmfloat3 \n WS에서 정의되는 Line의 끝점
 * @param iNumSamples [in] \n int \n Line 상에서 sampling 할 점의 개수
 * @param pvtrSampleValues [out] \n vector<double> \n Line 상에서 sampling 된 값이 저장될 vector list에 대한 포인터
 * @return bool \n 성공적으로 함수가 동작하면 true, 그렇지 않으면 false 반환.
 * @remarks 정의되는 Line에서 iNumSamples 개만큼 sampling이 진행될 때 처음과 끝점 기준으로 등간격 sampling이 진행
*/
__vmstatic bool VXEVolumeGetActualValuesAlongLine(int iObjectID, vmfloat3 f3PosStartWS, vmfloat3 f3PosEndWS, int iNumSamples, vector<double>* pvtrSampleValues/*out*/);

/*!
* @fn __vmstatic bool vxengineapi::VXEVolumeGetActualValue(int iObjectID, vmfloat3 f3PosSample, bool bIsVolumeSpacePosition, double* pdSampleValue)
* @brief iObjectID의 VXVObjectVolume에 저장된 Volume에 대해 파라미터로 정의되는 특정 좌표의 Volume 값을 sample 하는 함수
* @param iObjectID [in] \n int \n Source VXVObjectVolume ID
* @param f3PosSample [in] \n vmfloat3 \n WS 또는 VS 에서 정의되는 샘플 좌표
* @param bIsVolumeSpacePosition [in] \n bool \n f3PosSample 의 좌표계가 VS 이면 true, WS 이면 false
* @param pdSampleValue [out] \n double \n 해당 좌표 상에서 sampling 된 값을 저장할 double 값에 대한 포인터
* @return bool \n 성공적으로 함수가 동작하면 true, 그렇지 않으면 false 반환.
* @remarks 볼륨 외의 공간을 샘플하게 되면 최소값을 반환하며, 모든 Sample 값은 Real 값으로 둠.
*/
__vmstatic bool VXEVolumeGetActualValue(int iObjectID, vmfloat3 f3PosSample, bool bIsVolumeSpacePosition, double* pdSampleValue/*out*/);

// Primitive Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXEPrimitiveGenerateNew(int* piObjectID)
 * @brief VXVObjectPrimitive 를 생성하고 이를 Resource Manager에 등록함.
 * @param piObjectID [out] \n int \n Resource Manager에 등록된 VXVObjectPrimitive의 ID를 저장할 int의 포인터
 * @param iForcedObjectID [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
 * @param bIsSafeID [in] \n bool \n Object ID 할당 영역의 마지막에 Safe Zone 에서 ID를 할당하여 ID가 중복되지 않도록 함
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks VXObejct가 생성만 됐을 뿐 아직 내용물이 채워지진 않은 상태.
 * @sa vxobjects::CVXVObjectPrimitive
*/
__vmstatic bool VXEPrimitiveGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0, bool bIsSafeID = false);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeCopy(int iObjectDestID, int iObjectSrcID)
 * @brief iObjectSrcID의 VXVObjectPrimitive에 저장된 Primitive를 iObjectDestID의 VXVObjectPrimitive에 복사
 * @param iObjectDestID [in] \n int \n Source VXVObjectPrimitive ID
 * @param iObjectSrcID [in] \n int \n Destination VXVObjectPrimitive ID
 * @return bool \n Primitive 복사가 성공적으로 수행되면 true, 그렇지 않으면 false 반환
 * @remarks @ref vxobjects::SVXPrimitiveDataArchive의 Primitive정보와 Primitive Data를 저장한 array가 별도로 생성되어 Copy됨.
 * @sa 
 * vxobjects::SVXPrimitiveDataArchive \n
 * vxobjects::CVXVObjectPrimitive
*/
__vmstatic bool VXEPrimitiveCopy(int iObjectDestID, int iObjectSrcID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEPrimitiveGetPrimitiveInfo(int iObjectID, SIVXPrimitiveInfo* psivxPrimitiveInfo)
 * @brief iObjectID의 VXVObjectPrimitive에 저장된 @ref vxobjects::SVXPrimitiveDataArchive 의 정보를 얻음
 * @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
 * @param psivxPrimitiveInfo [out] \n SIVXPrimitiveInfo \n @ref vxobjects::SVXPrimitiveDataArchive 에 대응하는 Primitive 정보를 저장할 자료구조 SIVXPrimitiveInfo 의 포인터
 * @return bool \n Primitive 정보를 성공적으로 얻으면 true, 그렇지 않으면 false 반환
 * @remarks 
 * @sa vxobjects::SVXPrimitiveDataArchive
*/
__vmstatic bool VXEPrimitiveGetPrimitiveInfo(int iObjectID, SIVXPrimitiveInfo* psivxPrimitiveInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEPrimitiveSetFrontFace(int iObjectID, bool bIsFrontCCW)
 * @brief iObjectID의 VXVObjectPrimitive가 정의하는 Front Face 를 설정하는 함수
 * @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
 * @param bIsFrontCCW [in] \n bool \n Front Face 가 시계 방향이면 false, 시계 반대 방향이면 true
 * @return bool \n 성공적으로 설정되면 true, 그렇지 않으면 false 반환
 * @remarks 
 * @ref vxobjects::SVXPrimitiveDataArchive::bIsPrimitiveFrontCCW 를 수정 \n
 * @sa vxobjects::CVXVObjectPrimitive, vxengineapi::VXEPrimitiveGetPrimitiveInfo
*/
__vmstatic bool VXEPrimitiveSetFrontFace(int iObjectID, bool bIsFrontCCW);
/*!
* @fn __vmstatic bool vxengineapi::VXPrimitiveSetColorVisibility(int iObjectID, vmfloat4 f4ColorMesh, bool bIsVisibleMesh, vmfloat4 f4ColorWire, bool bVisibleWire)
* @brief iObjectID의 VXVObjectPrimitive에 대한 Visibility 및 Color 를 설정하는 함수
* @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
* @param f4ColorMesh [in] \n vmfloat4 \n Mesh 의 기본 색상을 정의
* @param bIsVisibleMesh [in] \n bool \n Mesh 에 대한 Visibility
* @param f4ColorWire [in] \n vmfloat4 \n Mesh 의 Wireframe 을 나타내는 색상 정의
* @param bIsVisibleWire [in] \n bool \n Mesh 의 Wireframe 에 대한 Visibility
* @return bool \n 성공적으로 설정되면 true, 그렇지 않으면 false 반환
* @remarks
* @ref vxobjects 의 기본 속성 \n
* @sa vxobjects::CVXVObjectPrimitive, vxengineapi::VXEPrimitiveGetPrimitiveInfo
*/
__vmstatic bool VXPrimitiveSetColorVisibility(int iObjectID, vmfloat4 f4ColorMesh, bool bIsVisibleMesh, vmfloat4 f4ColorWire, bool bIsVisibleWire);
/*!
* @fn __vmstatic bool vxengineapi::VXPrimitiveGetColorVisibility(int iObjectID, vmfloat4* pf4ColorMesh, bool* pbIsVisibleMesh, vmfloat4* pf4ColorWire, bool* pbVisibleWire)
* @brief iObjectID의 VXVObjectPrimitive에 대한 Visibility 및 Color 를 속성을 가져 오는 함수
* @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
* @param pf4ColorMesh [out](optional) \n vmfloat4 \n Mesh 의 기본 색상을 저장할 vmfloat4 포인터, NULL이면 사용 안 함
* @param pbIsVisibleMesh [out](optional) \n bool \n Mesh 에 대한 Visibility를 저장할 bool 포인터, NULL이면 사용 안 함
* @param pf4ColorWire [out](optional) \n vmfloat4 \n Mesh 의 Wireframe 을 나타내는 색상을 저장할 vmfloat4 포인터, NULL이면 사용 안 함
* @param pbIsVisibleWire [out](optional) \n bool \n Mesh 의 Wireframe 에 대한 Visibility를 저장할 bool 포인터, NULL이면 사용 안 함
* @return bool \n 성공적으로 설정되면 true, 그렇지 않으면 false 반환
* @remarks
* @ref vxobjects 의 기본 속성 \n
* @sa vxobjects::CVXVObjectPrimitive, vxengineapi::VXEPrimitiveGetPrimitiveInfo
*/
__vmstatic bool VXPrimitiveGetColorVisibility(int iObjectID, vmfloat4* pf4ColorMesh, bool* pbIsVisibleMesh, vmfloat4* pf4ColorWire, bool* pbIsVisibleWire);

// CustomList Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListGenerateNew(int* piObjectID)
 * @brief VXLObject 를 생성하고 이를 Resource Manager에 등록함.
 * @param piObjectID [out] \n int \n Resource Manager에 등록된 VXLObject의 ID를 저장할 int의 포인터
 * @param iForcedObjectID [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks VXObejct가 생성만 됐을 뿐 아직 내용물이 채워지진 않은 상태.
 * @sa vxobjects::CVXLObject
*/
__vmstatic bool VXECustomListGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListGetList(int iObjectID, string* pstrListName, void* pvOutput)
 * @brief iObjectID의 VXLObject에 저장되어 있는 Custom List를 얻는 함수
 * @param iObjectID [in] \n int \n Source VXLObject ID
 * @param pstrListName [in] \n string \n "_vlist_[type string]_[Name]" 형식으로 주어진 문자열 string의 포인터
 * @param ppvOutput [out] \n void* \n Custom List가 저장될 vector<type> 포인터에 대한 포인터 (이중 포인터)
 * @return bool \n pstrListName으로 저장되어 있는 Custom List가 해당 VXLObject에 있으면 true, 없으면 false 반환
 * @remarks 
 * pstrListName에서 지원하는 [type string]은 VXFramework에서 정의한 문자열(@ref vxhelpers::VXHGetDataTypeFromString)을 따름\n
 * Custom List는 vector container로 정의되어 있음.
 * @sa vxobjects::CVXLObject, vxhelpers::VXHGetDataTypeFromString, vxengineapi::VXECustomListSetList
*/
__vmstatic bool VXECustomListGetList(int iObjectID, string* pstrListName, void** ppvOutput/*out*/, size_t& size_bytes);
__vmstatic bool VXECustomListGetStringObjs(int iObjectID, string* pstrListName, string** ppstrOutput/*out*/, int& num_stringobjs);
/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListSetList(int iObjectID, string* pstrListName, void* pvInput)
 * @brief iObjectID의 VXLObject에 Custom List를 등록하는 함수
 * @param iObjectID [in] \n int \n 저장하고자 하는 VXLObject의 ID
 * @param pstrListName [in] \n string \n "_vlist_[type string]_[Name]" 형식으로 주어진 문자열 string의 포인터
 * @param pvInput [out] \n void \n Custom List로 저장할 vector<type>의 포인터
 * @return bool \n Custom List로 등록이 성공하면 true, 없으면 false 반환
 * @remarks 
 * pstrListName에서 지원하는 [type string]은 VXFramework에서 정의한 문자열(@ref vxhelpers::VXHGetDataTypeFromString)을 따름\n
 * Custom List는 vector container만 지원.
 * @sa vxobjects::CVXLObject, vxhelpers::VXHGetDataTypeFromString, vxengineapi::VXECustomListGetList
*/
__vmstatic bool VXECustomListSetList(int iObjectID, string* pstrListName, void* pvInput, int ele_num, int type_bytes, void* dstInput = NULL);
__vmstatic bool VXECustomListSetStringObjs(int iObjectID, string* pstrListName, string* pvInput, int ele_num);

/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListSetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, double dValue)
 * @brief iTargetObjectID 에 대한 custom parameter 를 저장하고 있는 map 에 값을 등록 \n
 * @param iTargetObjectID [in] \n int \n Custom Parameter 의 Target 이 되는 Object ID \n
 * @param strName [in] \n string \n 해당 iTargetObjectID 에 대해 저장되어 있는 map 의 Key \n
 * @param pvValue [in] \n void \n 해당 iTargetObjectID 에 대해 저장할 있는 map 의 Value \n
 * @return bool \n 성공하면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * 지원 파라미터 종류는 @ref VXHStringGetParameterFromCustomStringMap 와 같음 (단 String 종류는 지원 안 함)
 * @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListSetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, void* pvValue, size_t bytes_obj, void* pvValueDst = NULL);

/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListSetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, double dValue)
 * @brief iTargetObjectID 에 대한 custom parameter 를 저장하고 있는 map 에 값을 등록 \n
 * @param iTargetObjectID [in] \n int \n Custom Parameter 의 Target 이 되는 Object ID \n
 * @param strName [in] \n string \n 해당 iTargetObjectID 에 대해 저장되어 있는 map 의 Key \n
 * @param pvValue [out] \n void \n 해당 iTargetObjectID 에 대해 저장되어 있는 map 의 Value 를 저장하고 있는 void 포인터 \n
 * @return bool \n 성공하면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * 지원 파라미터 종류는 @ref VXHStringGetParameterFromCustomStringMap 와 같음 (단 String 종류는 지원 안 함)
 * @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListGetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, void* pvValue);

/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListCopyCustomValues(int iObjectSrcID, int iObjectDstID)
 * @brief iObjectSrcID 의 VxCLObject 에 있는 custom parameters 를 iObjectDstID 의 VxCLObject 에 있는 custom parameters 로 Copy \n
 * @param iObjectSrcID [in] \n int \n 복사할 내용을 담은 Object ID \n
 * @param iObjectDstID [in] \n int \n 복사될 Object ID \n
 * @return bool \n 성공하면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * 기존 iObjectDstID의 map values 는 모두 새로운 map values 로 대체됨.
 * @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListCopyCustomValues(int iObjectSrcID, int iObjectDstID);

/*!
* @fn __vmstatic bool vxengineapi::VXECustomListRemoveCustomValueGroup(int iObjectID, int iTargetObjectID)
* @brief iObjectID 의 VxCLObject 에 있는 해당 VxObject 에 대한 custom parameters 를 없앰 \n
* @param iObjectID [in] \n int \n VxCLObject 의 Object ID \n
* @param iTargetObjectID [in] \n int \n 대상이 되는 VxObject ID \n
* @return bool \n 성공하면 true, 그렇지 않으면 false 반환.
* @remarks
* @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListRemoveCustomValueGroup(int iObjectID, int iTargetObjectID);

/*!
* @fn __vmstatic bool vxengineapi::VXECustomListRemoveAll(int iObjectID)
* @brief iObjectID 의 VxCLObject 에 있는 모든 내용물을 없앰 \n
* @param iObjectID [in] \n int \n VxCLObject 의 Object ID \n
* @return bool \n 성공하면 true, 그렇지 않으면 false 반환.
* @remarks
* @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListRemoveAll(int iObjectID);

// Transfer Function Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXETransferfunctionGenerateNew(int* piObjectID)
 * @brief VXTObject 를 생성하고 이를 Resource Manager에 등록함.
 * @param piObjectID [out] \n int \n Resource Manager에 등록된 VXTObject의 ID를 저장할 int의 포인터
 * @param iForcedObjectID [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks VXObejct가 생성만 됐을 뿐 아직 내용물이 채워지진 않은 상태.
 * @sa vxobjects::CVXTObject
*/
__vmstatic bool VXETransferfunctionGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
 * @fn __vmstatic bool vxengineapi::VXETransferfunctionGetArchiveInfo(int iObjectID, SIVXTransferfunctionArchiveInfo* psivxTransferfunctionArchiveInfo)
 * @brief iObjectID의 VXTObject에 저장되어 있는 @ref vxobjects::SVXTransferFunctionArchive 의 정보를 얻음
 * @param iObjectID [in] \n int \n Source VXTObject ID
 * @param psivxTransferfunctionArchiveInfo [out] \n SIVXTransferfunctionArchiveInfo \n @ref vxobjects::SVXTransferFunctionArchive 에 대응하는 Primitive 정보를 저장할 자료구조 SIVXTransferfunctionArchiveInfo 의 포인터
 * @return bool \n 정보를 성공적으로 얻으면 true, 그렇지 않으면 false 반환
 * @remarks OTF 값을 저장하고 있는 array는 @ref vxengineapi::VXETransferfunctionGetOpticalValue 로 확인
 * @sa vxobjects::SVXTransferFunctionArchive, vxengineapi::VXETransferfunctionGetOpticalValue
*/
__vmstatic bool VXETransferfunctionGetArchiveInfo(int iObjectID, SIVXTransferfunctionArchiveInfo* psivxTransferfunctionArchiveInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXETransferfunctionGetOpticalValue(int iObjectID, SIVXTransferfunctionOpticalValue* psivxTransferfunctionOpticalValue, int iTfIndexZ)
 * @brief iObjectID의 VXTObject에 저장되어 있는 @ref vxobjects::SVXTransferFunctionArchive 의 OTF array 값을 얻음
 * @param iObjectID [in] \n int \n Source VXTObject ID
 * @param psivxTransferfunctionOpticalValue [out] \n SIVXTransferfunctionOpticalValue \n @ref vxobjects::SVXTransferFunctionArchive 에 정의되어 있는 OTF array 에 대한 정보를 담은 SIVXTransferfunctionOpticalValue 의 포인터
 * @param iTfIndexZ [in](optional) \n int \n OTF dimension에 비해 실제 archive에 저장된 array의 dimension이 큰 경우 (ex. 1D summed preintegrating OTF) 해당 Target metric의 OTF array pointer를 위한 indexing
 * @return bool \n 정보를 성공적으로 얻으면 true, 그렇지 않으면 false 반환
 * @remarks 
 * OTF array 값 이외의 정보는 @ref vxengineapi::VXETransferfunctionGetArchiveInfo 로 확인 \n
 * OTF dimension과 저장된 array의 dimension이 같으면 iTfIndexZ은 항상 0이 사용됨.
 * @sa vxobjects::SVXTransferFunctionArchive, vxengineapi::VXETransferfunctionGetArchiveInfo
*/
__vmstatic bool VXETransferfunctionGetOpticalValue(int iObjectID, SIVXTransferfunctionOpticalValue* psivxTransferfunctionOpticalValue/*out*/, int iTfIndexZ = 0);

// Image Plane Object //
/*!
 * @fn __vmstatic bool VXEImageplaneGenerateNew(vmint2 i2WindowSizePix, int* piObjectID)
 * @brief VXIObject 를 생성하고 이를 Resource Manager에 등록함.
 * @param piObjectID [out] \n int \n Resource Manager에 등록된 VXIObject의 ID를 저장할 int의 포인터
 * @param iForcedObjectID [in] \n int \n 강제로 등록하고자 하는 ID, 0 이 아니면 강제 등록을 시도
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks VXObejct가 생성만 됐을 뿐 아직 내용물이 채워지진 않은 상태.
 * @sa vxobjects::CVXIObject
*/
__vmstatic bool VXEImageplaneGenerateNew(vmint2 i2WindowSizePix, int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
* @fn __vmstatic bool vxengineapi::VXEImageplaneGenerateBmpBindBuffer(int iObjectID, vmint2 i2WindowSizePix)
* @brief iObjectID의 VXIObject에 Bitmap Bind 용 vxenums::EnumVXRDataType::vxrDataTypeBYTE4 을 할당하는 함수
* @param iObjectID [in] \n int \n Source VXIObject ID
* @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
* @remarks
* 기존의 IObject 에 등록된 버퍼 크기에 해당하는 vxenums::EnumVXRFrameBufferUsage::vxrFrameBufferUsageRENDEROUT 용 버퍼를 index 0 에 할당 \n
* 이미 생성되어 있으면 새로 할당하지 않음 \n
* @sa vxobjects::CVXIObject, vxengineapi::VXEImageplaneGenerateNew
*/
__vmstatic bool VXEImageplaneGenerateBmpBindBuffer(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneResize(int iObjectID, vmint2 i2WindowSizePix)
 * @brief iObjectID의 VXIObject에 할당된 Frame Bufffer의 크기를 재설정하는 함수
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param i2WindowSizePix [in] \n vmint2 \n 새로운 Frame Bufffer의 크기 \n 
 * >> i2WindowSizePix = vmint2(width, height);
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * 기존의 Frame Buffer를 메모리 상에서 해제하고 새로 메모리를 할당 \n
 * PS (Projection Space) 와 SS (Screen Space) 에서 SS 에서의 Image Plane 크기가 변하므로 WS (World Space) 와 SS 간 변환 matrix를 다시 계산함\n
 * 이 과정에서 image plane의 비율도 내부적으로 다시 계산됨. (@ref vxobjects::CVXIObject::ResizeFrameBuffer)
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject
*/
__vmstatic bool VXEImageplaneResize(int iObjectID, vmint2 i2WindowSizePix);
/*!
* @fn __vmstatic bool vxengineapi::VXEImageplaneGetOutBufferSize(int iObjectID, EnumVXRFrameBufferUsage eFrameBufferUsage, int iBufferIndex, SIVXOutBufferInfo* psvxBufferInfo)
* @brief iObjectID의 VXIObject에 저장된 Buffer 의 2D Size 의 정보를 얻음
* @param iObjectID [in] \n int \n Source VXIObject ID
* @param pi2BufferSize [out] \n vmint2 \n 해당 VXIObject에 저장된 Buffer 의 정보를 갖는 vmint2 의 포인터
* @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
* @remarks
* VXIObject는 여러개의 @ref vxobjects::SVXFrameBuffer 를 갖으며, 모든 Buffer 의 2D Size 는 같음. 이것의 Size 를 반환함.\n
* @sa vxobjects::CVXIObject
*/
__vmstatic bool VXEImageplaneGetOutBufferSize(int iObjectID, vmint2* pi2BufferSize/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetOutBufferInfo(int iObjectID, EnumVXRFrameBufferUsage eFrameBufferUsage, int iBufferIndex, SIVXOutBufferInfo* psvxBufferInfo)
 * @brief iObjectID의 VXIObject에 저장된 @ref vxobjects::SVXFrameBuffer 의 정보를 얻음
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param eFrameBufferUsage [in] \n EnumVXRFrameBufferUsage \n 해당 VXIObject에 저장된 @ref vxobjects::SVXFrameBuffer 들에 대해 사용 용도(@ref vxenums::EnumVXRFrameBufferUsage)에 따른 Buffer를 지정
 * @param iBufferIndex [in] \n int \n 해당 VXIObject에 저장된 @ref vxobjects::SVXFrameBuffer 들에 대해 같은 사용 용도에서 index에 따른 Buffer 지정
 * @param psvxBufferInfo [out] \n SIVXOutBufferInfo \n 해당 VXIObject에 저장된 @ref vxobjects::SVXFrameBuffer 의 정보를 갖는 SIVXOutBufferInfo 의 포인터
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * VXIObject는 여러개의 @ref vxobjects::SVXFrameBuffer 를 갖으며, eFrameBufferUsage과 iBufferIndex로 Frame Buffer 정보를 indexing 함.\n
 * 일반적으로 Render out 되는 RGBA(4bytes) 단위 pixel의 버퍼는 eFrameBufferUsage = @ref vxenums::vxrFrameBufferUsageRENDEROUT 와 iBufferIndex = 0 에 해당하는 Frame Buffer에 저장되어 있음 \n
 * (실제로는 Module 에서 어떻게 결과를 저장하느냐에 따라 다름) \n
 * @sa vxobjects::CVXIObject, @ref vxobjects::SVXFrameBuffer
*/
__vmstatic bool VXEImageplaneGetOutBufferInfo(int iObjectID, EvmFrameBufferUsage eFrameBufferUsage, int iBufferIndex, SIVXOutBufferInfo* psvxBufferInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneSetCameraState(int iObjectID, const SIVXCameraStateDescription* psivxCameraStateDescriptor)
 * @brief iObjectID의 VXIObject에 포함되어 있는 Camera 관련 @ref vxobjects::CVXCObject 에 Camera States 를 설정하는 함수
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param psivxCameraStateDescriptor [in] \n SIVXCameraStateDescription \n 설정할 Camera States 가 저장되어 있는 SIVXCameraStateDescription의 포인터
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * SIVXCameraStateDescription 의 모든 정보가 다 유효해야 하므로 @ref vxengineapi::VXEImageplaneGetCameraState 에서 얻은 현재 Camera 정보에서 원하는 정보만 수정 후 Setter로 사용함.\n
 * iObjectID의 VXIObject 는 defined 되어(Frame Buffer의 크기가 0 보다 크게 설정) 있어야 WS (World Space) 와 SS (Screen Space) 간 변환 matrix가 제대로 설정됨.
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneGetCameraState
*/
__vmstatic bool VXEImageplaneSetCameraState(int iObjectID, const SIVXCameraStateDescription* psivxCameraStateDescriptor);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetCameraState(int iObjectID, SIVXCameraStateDescription* psivxCameraStateDescriptor)
 * @brief iObjectID의 VXIObject에 저장되어 있는 Camera 관련 @ref vxobjects::CVXCObject 에 설정된 Camera States 를 얻는 함수
 * @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject 를 포함하고 있는 Source VXIObject ID
 * @param psivxCameraStateDescriptor [out] \n SIVXCameraStateDescription \n Camera States가 저장될 SIVXCameraStateDescription의 포인터
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * iObjectID의 VXIObject 는 defined 되어(Frame Buffer의 크기가 0 보다 크게 설정) 있지 않으면 부정확한 결과가 도출될 수 있음
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState
*/
__vmstatic bool VXEImageplaneGetCameraState(int iObjectID, SIVXCameraStateDescription* psivxCameraStateDescriptor/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneSetOrthoStageFitting(int iObjectID, vmdouble3 d3PosOrthoBoxMinWS, vmdouble3 d3PosOrthoBoxMaxWS, EnumVXVStageViewType eStageViewFlag)
 * @brief iObjectID의 VXIObject에 저장되어 있는 Camera 관련 @ref vxobjects::CVXCObject 에 Camera States 를 WS 에서 정의된 axis-aligned box stage 기준으로 초기화하는 함수
 * @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject 를 포함하고 있는 Source VXIObject ID
 * @param d3PosOrthoBoxMinWS [in] \n vmdouble3 \n WS 상의 axis-aligned box stage 를 정의하는 최소점의 위치
 * @param d3PosOrthoBoxMaxWS [in] \n vmdouble3 \n WS 상의 axis-aligned box stage 를 정의하는 최대점의 위치
 * @param eStageViewFlag [in] \n EnumVXVStageViewType \n Camera의 초기화 위치 설정
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks 이것을 통해 설정되는 Camera를 기준으로 zoom ratio 1.0 을 정의.
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState, @ref vxengineapi::VXEImageplaneGetCameraState
*/
__vmstatic bool VXEImageplaneSetOrthoStageFitting(int iObjectID, vmdouble3 d3PosOrthoBoxMinWS, vmdouble3 d3PosOrthoBoxMaxWS, EvmStageViewType eStageViewFlag);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetMatrixBetweenWSandSS(int iObjectID, vmmat44* psvxMatrixWS2SS, vmmat44* psvxMatrixSS2WS)
 * @brief iObjectID의 VXIObject에 저장되어 있는 Camera 관련 @ref vxobjects::CVXCObject 로부터 SS 와 WS 간 변환 matrix를 얻는 함수
 * @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject 를 포함하고 있는 Source VXIObject ID
 * @param psvxMatrixWS2SS [out] \n vmmat44 \n WS 에서 SS 로 변환하는 matrix가 저장될 vmmat44 의 포인터, NULL이면 사용 안 함
 * @param psvxMatrixSS2WS [out] \n vmmat44 \n SS 에서 WS 로 변환하는 matrix가 저장될 vmmat44 의 포인터, NULL이면 사용 안 함
 * @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * psvxMatrixWS2SS 과 psvxMatrixSS2WS 은 역행렬 관계\n
 * RHS, row major matrix로 정의
 * @ref vxengineapi::VXEImageplaneSetCameraState 와 vxengineapi::VXEImageplaneResize 이 호출되면 변환 matrix가 재설정 됨.
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState, @ref vxengineapi::VXEImageplaneResize
*/
__vmstatic bool VXEImageplaneGetMatrixBetweenWSandSS(int iObjectID, vmmat44* psvxMatrixWS2SS/*optional out*/, vmmat44* psvxMatrixSS2WS/*optional out*/);
/*!
* @fn __vmstatic bool vxengineapi::VXEImageplaneGetMatrixThroughWSandSS(int iObjectID, vmmat44* psvxMatrixWS2SS, vmmat44* psvxMatrixSS2WS)
* @brief iObjectID의 VXIObject에 저장되어 있는 Camera 관련 @ref vxobjects::CVXCObject 로부터 WS -> CS -> PS -> SS 로의 변환 matrix를 얻는 함수
* @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject 를 포함하고 있는 Source VXIObject ID
* @param psvxMatrixWS2SS [out] \n vmmat44 \n WS 에서 CS 로 변환하는 matrix가 저장될 vmmat44 의 포인터, NULL이면 사용 안 함
* @param psvxMatrixSS2WS [out] \n vmmat44 \n CS 에서 PS 로 변환하는 matrix가 저장될 vmmat44 의 포인터, NULL이면 사용 안 함
* @param psvxMatrixSS2WS [out] \n vmmat44 \n PS 에서 SS 로 변환하는 matrix가 저장될 vmmat44 의 포인터, NULL이면 사용 안 함
* @return bool \n 성공적으로 수행되면 true, 그렇지 않으면 false 반환.
* @remarks
* psvxMatrixWS2SS 과 psvxMatrixSS2WS 은 역행렬 관계\n
* RHS, row major matrix로 정의
* @ref vxengineapi::VXEImageplaneSetCameraState 와 vxengineapi::VXEImageplaneResize 이 호출되면 변환 matrix가 재설정 됨.
* @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState, @ref vxengineapi::VXEImageplaneResize
*/
__vmstatic bool VXEImageplaneGetMatrixThroughWSandSS(int iObjectID, vmmat44* psvxMatrixWS2CS/*optional out*/, vmmat44* psvxMatrixCS2PS/*optional out*/, vmmat44* psvxMatrixPS2SS/*optional out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetOutRgbaAlongLine(int iObjectID, int iNumSamples, vmint2 i2PosStartSS, vmint2 i2PosEndSS, vector<vmbyte4>* pvtrRGBAValues)
 * @brief iObjectID의 VXIObject에서 Render out 될 Frame Buffer에 저장되어 있는 값을 sampling 하는 함수
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param iNumSamples [in] \n int \n Line 상에서 sampling 할 점의 개수
 * @param i2PosStartSS [in] \n vmint2 \n SS에서 정의되는 Line의 시작점
 * @param i2PosEndSS [in] \n vmint2 \n SS에서 정의되는 Line의 끝점
 * @param pvtrRGBAValues [out] \n vector<vmbyte4> \n Line 상에서 sampling 된 값이 저장될 vector list에 대한 vector<vmbyte4>의 포인터
 * @return bool \n 성공적으로 함수가 동작하면 true, 그렇지 않으면 false 반환.
 * @remarks 
 * 정의되는 Line에서 iNumSamples 개만큼 sampling이 진행될 때 처음과 끝점 기준으로 등간격 sampling이 진행 \n
 * VXIObject에 저장된 @ref vxobjects::SVXFrameBuffer 들 중 eFrameBufferUsage = @ref vxenums::vxrFrameBufferUsageRENDEROUT 와 iBufferIndex = 0 에 해당하는 Frame Buffer 를 대상으로 함
 * @sa vxobjects::CVXIObject, @ref vxobjects::SVXFrameBuffer
*/
__vmstatic bool VXEImageplaneGetOutRgbaAlongLine(int iObjectID, int iNumSamples, vmint2 i2PosStartSS, vmint2 i2PosEndSS, vector<vmbyte4>* pvtrRGBAValues/*out*/);

/////////////
// Modules //
// 1. map<string, vector<CVXObject*>> mapVXObjects;
//		Key String : _[in/out]_[ChildObjectType]_[ValueType]
//		Content List : Registered Object Pointer list
// 2. map<string, void*> mapCustomParamters;
//		Key String : _[ValueType]_[CustomName]
//		Content String : value defined by string
// 3. map<string, void* /*Pointer*/> mapCustomObjects;
//		Key String : _[in/out]_[PointerType]_[CustomName]
//		Content void pointer : Pointer used as custum-datastructed value in/out
/*!
 * @fn __vmstatic EnumVXMModuleType vxengineapi::VXEModuleTypeGet(int iModuleID)
 * @brief iModuleID 모듈의 type을 얻는 함수
 * @param iModuleID [in] \n int \n Source Module ID
 * @return EnumVXMModuleType \n Module Type 을 반환. Type 이 존재하지 않는 Module 일 경우 vxenums::vxmModuleTypeNONE 반환
 * @remarks @ref Module Arbiter에서 관리되며, VXObject ID와 같은 경우도 있으나 별도의 Manager에서 관리되는 ID 이므로 상관 없음
*/
__vmstatic EnumVXMModuleType VXEModuleTypeGet(int iModuleID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEModuleRegister(string strModuleName, EnumVXMModuleType eModuleType, string strModuleSpecifier, int* piModuleID)
 * @brief Module의 File 이름으로 Module Arbiter에 Module 등록
 * @param strModuleName [in] \n string \n Module File의 경로 및 이름
 * @param eModuleType [in] \n EnumVXMModuleType \n 등록할 Module의 Type
 * @param strModuleSpecifier [in] \n string \n 등록할 Module에 대한 Mention, 별도의 형식은 없으며, string 타입의 아무 값을 넣어도 됨.
 * @param piModuleID [out] \n int \n 등록된 Module ID를 저장할 int 포인터
 * @return bool \n 등록이 성공하면 true, 그렇지 않을 경우 false를 반환하고 *piModuleID = 0 으로 처리.
 * @remarks 
*/
__vmstatic bool VXEModuleRegister(string strModuleName, EnumVXMModuleType eModuleType, string strModuleSpecifier, int* piModuleID/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEModuleExecute(int iModuleID, const map<string, int>* pmapObjIDs, const map<string, void*>* pmapCustomParameters, const map<string, void*>* pmapCustomObjects, bool bIsOnlyParameterCheck)
 * @brief iModuleID의 Module을 실행하는 함수
 * @param iModuleID [in] \n int \n 실행하고자 하는 Module ID
 * @param pmapObjIDs [in] \n map<string, int> \n 
 * Module에 사용될 VXObject 들에 대한 ID 들이 등록된 container map에 대한 map<string, int>의 포인터 \n
 * 내부적으로 @ref vxhelpers::VXHStringGetVXObjectFromObjectStringMap 을 통해 ID를 얻을 수 있도록 container를 가공함 \n
 * NULL일 경우 사용 안 함.
 * >> string key 의 형식 \n
 * >> _[in/out]_[VOLUME/PRIMITIVE/IMAGEPLANE/TRANSFERFUNCTION/CUSTOMLIST]_[0/1/2/3... 0 based-index] \n
 * >> ex. 3번째 VXVObjectVolume의 경우 : "_in_VOLUME_2"
 * @param pmapCustomParameters [in] \n map<string, void*> \n 
 * Module에 사용될 VXFramework format으로 정의된 container map에 대한 map<string, void*>의 포인터 \n
 * @ref vxhelpers::VXHStringGetParameterFromCustomStringMap 을 통해 값을 얻을 수 있도록 container를 구성 \n
 * NULL일 경우 사용 안 함.
 * >> string key 와 Value 의 형식 \n
 * >> _[bool/int/double/int2/int3/int4/double2/double3/double4/matrix44/string]_[name] \n
 * @param pmapCustomObjects [in] \n map<string, void*> \n 
 * Module에 사용될 custom pointer로 정의된 container map에 대한 map<string, void*>의 포인터 \n
 * @ref vxhelpers::VXHStringGetCustomObjectFromPointerStringMap 을 통해 포인터를 얻을 수 있도록 container를 구성 \n
 * NULL일 경우 사용 안 함.
 * >> string key 의 형식 \n
 * >> _[in/out]_[class/vector/custom...]_[name] \n
 * >> ex. "_in_class_GPUMANAGER"
 * @param bIsOnlyParameterCheck [in] \n bool \n true이면 Module의 Parameter Checker만 수행하며, false면 Module 작업 전체를 수행
 * @return bool \n Module이 성공적으로 동작하면 true, 그렇지 않으면 false 반환
 * @remarks 각각의 container에 Module 공통이 아닌 특별하게 사용할 container 형식으로 parameter를 제공할 수도 있음
 * @sa 
 * vxhelpers::VXHStringGetVXObjectFromObjectStringMap \n 
 * vxhelpers::VXHStringGetParameterFromCustomStringMap \n
 * vxhelpers::VXHStringGetCustomObjectFromPointerStringMap \n
 * vxobjects::SVXModuleParameters
*/
__vmstatic bool VXEModuleExecute(int iModuleID, 
	const map<string, int>* pmapObjIDs, 
	const map<string, void*>* pmapCustomParameters, 
	const map<string, void*>* pmapCustomObjects, 
	bool bIsOnlyParameterCheck);
/*!
 * @fn __vmstatic bool vxengineapi::VXEModuleClear(int iModuleID, map<string, void*>* pmapCustomParamters)
 * @brief iModuleID의 Module을 Module Arbiter에서 해제
 * @param iModuleID [in] \n int \n 실행하고자 하는 Module ID
 * @param pmapCustomParamters [in] \n map<string, void*> \n 
 * Module 에서 해제 시 사용될 VXFramework format으로 정의된 container map에 대한 map<string, void*>의 포인터 \n
 * @ref vxhelpers::VXHStringGetParameterFromCustomStringMap 을 통해 값을 얻을 수 있도록 container를 구성해야 함 \n
 * NULL일 경우 사용 안 함.
 * @return bool \n Module이 성공적으로 해제되면 true, 그렇지 않으면 false 반환
 * @remarks 별도의 해제 작업이 없어도 @ref vxengineapi::VXEEndEngineLib 에서 모든 Module이 해제됨
 * @sa vxengineapi::VXEEndEngineLib, vxengineapi::VXEModuleExecute
*/
__vmstatic bool VXEModuleClear(int iModuleID,
	map<string, void*>* pmapCustomParamters);
/*!
 * @fn __vmstatic double vxengineapi::VXEModuleGetProgress(int iModuleID)
 * @brief 현재 Module에서 작업 수행 중인 Progress를 반환
 * @param iModuleID [in] \n int \n 실행하고자 하는 Module ID
 * @return double 0.0 ~ 100.0
 * @remarks Module 개발 시 code 단계에서 @ref vxobjects::SVXLocalProgress 에 적용된 progress를 반환
 * @sa vxobjects::SVXLocalProgress
*/
__vmstatic double VXEModuleGetProgress(int iModuleID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEModuleInteropCustomWork(int iModuleID, const map<string, int>* pmapObjIDs, const map<string, string>* pmapCustomParameters, const map<string, void*>* pmapCustomInformation)
 * @brief iModuleID 의 Module과 customized interoperation 을 하기 위한 함수
 * @param iModuleID [in] \n int \n 실행하고자 하는 Module ID
 * @param pmapObjIDs [in] \n map<string, int> \n 
 * Module에 사용될 VXObject 들에 대한 ID 들이 등록된 container map에 대한 map<string, int>의 포인터 \n
 * @ref vxhelpers::VXHStringGetVXObjectFromObjectStringMap 을 통해 ID를 얻을 수 있도록 container를 구성\n
 * NULL일 경우 사용 안 함.
 * @param pmapCustomParameters [in] \n map<string, void*> \n 
 * Module에 사용될 VXFramework format으로 정의된 container map에 대한 map<string, void*>의 포인터 \n
 * NULL일 경우 사용 안 함.
 * @param pmapCustomInformation [in] \n map<string, void*> \n 
 * Module에 사용될 custom pointer로 정의된 container map에 대한 map<string, void*>의 포인터 \n
 * @ref vxhelpers::VXHStringGetCustomObjectFromPointerStringMap 을 통해 포인터를 얻을 수 있도록 container를 구성 \n
 * NULL일 경우 사용 안 함.
 * @return bool \n Module과의 interoperation이 성공적으로 수행되면 true, 그렇지 않으면 false 반환
 * @remarks 
 * 각각의 container에 Module 공통이 아닌 특별하게 사용할 container 형식으로 parameter를 제공할 수도 있음 \n
 * container의 사용법은 @ref vxengineapi::VXEModuleExecute 과 동일
 * @sa 
 * vxengineapi::VXEModuleExecute \n
 * vxhelpers::VXHStringGetVXObjectFromObjectStringMap \n 
 * vxhelpers::VXHStringGetParameterFromCustomStringMap \n
 * vxhelpers::VXHStringGetCustomObjectFromPointerStringMap
*/
__vmstatic bool VXEModuleInteropCustomWork(int iModuleID, 
	const map<string, int>* pmapObjIDs, 
	const map<string, void*>* pmapCustomParameters,
	const map<string, void*>* pmapCustomInformation);
}