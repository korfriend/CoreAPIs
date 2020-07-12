#pragma once

#include "../CommonUnits/VimCommon.h"

/**
 * @file VXEngineFrame.h
 * @brief Platform ������ ���� VXFramework�� manager frame�� �ڷᱸ�� �� interface�� ������ API�� ���� ��� ����
 * @section Include & Link ����
 *		- Include : VXEngineFrame.h, VXEngineGlobalUnit.h
 *		- Library : VXEngineFrame.lib, VXEngineGlobalUnit.lib
 *		- Linking Binary : VXEngineFrame.dll, VXEngineGlobalUnit.dll
 */

/**
 * @package vxengineapi
 * @brief Platform ������ ���� Native Level VXFramework Engine API�� ���� namespace
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
		vxmModuleTypeTMAP,/*!< OTF setting ���� module */
		vxmModuleTypeVGENERATION,/*!< VObject ����/ó��/�м� ���� module */
		vxmModuleTypeETC/*!< Customized-Define Module */
	};
/**
 * @class SIVXCameraStateDescription
 * @brief Camera ���� states�� [Get/Set] �ϱ� ���� �ڷᱸ��
 * @sa
 * vxobjects::CVXCObject \n
 * vxengineapi::VXEImageplaneSetCameraState, vxengineapi::VXEImageplaneGetCameraState
 */
struct SIVXCameraStateDescription{
	/// Camera ��ġ
	vmfloat3 f3PosCamera; 
	/// Camera View Vector
	vmfloat3 f3VecView;
	/// Camera Up Vector
	vmfloat3 f3VecUp;
	/// Camera Space ���� Image Plane ũ�� (���� ������ Projection Plane�� ũ��)
	vmfloat2 f2ImageplaneSize; 
	/// Camera ��ġ ���� Near Plane, ���� ������ Projection Plane
	float fNearPlaneDistFromCamera;	
	/// Camera ��ġ ���� Far Plane
	float fFarPlaneDistFromCamera;	
	/// Perspective ���(bIsPerspective == true)���� Up Vector ���� ���� Field of View �� ����, radian.
	float fFovY;	
	/// Perspective ����, true : Perspective, false : Orthogonal
	bool bIsPerspective;	
	/// Image Plane �� �ػ� ��ȭ�� ���� ���� ratio �� ������Ű�� ���� Fitting ������, Orthogonal Projection, Stage Setting ���� ������
	vmfloat2 f2FittingSize;
	/// Image Plane �� �ػ� ��ȭ�� ���� ���� ratio �� ������Ű�� ���� Fitting ������, Perspective Projection, Stage Setting ���� ������
	float fFittingFovY;
	/// constructor, ��� 0 (NULL or false)���� �ʱ�ȭ
	SIVXCameraStateDescription(){
		memset(&f3PosCamera, 0, sizeof(vmfloat3)); memset(&f3VecView, 0, sizeof(vmfloat3));
		memset(&f3VecUp, 0, sizeof(vmfloat3)); memset(&f2ImageplaneSize, 0, sizeof(vmfloat2));
		fNearPlaneDistFromCamera = fFarPlaneDistFromCamera = fFovY = 0;
		bIsPerspective = false;
	}
};

/**
 * @class SIVXTransferfunctionArchiveInfo
 * @brief VXTObject�� @ref vxobjects::SVXTransferFunctionArchive ������ [Get] �ϱ� ���� �ڷᱸ��
 * @sa
 * vxobjects::SVXTransferFunctionArchive, vxobjects::CVXTObject \n
 * vxengineapi::VXETransferfunctionGetArchiveInfo
 */
struct SIVXTransferfunctionArchiveInfo{
	/// TObject�� �����ؼ� ���� VXVObjectVolume�� ID
	int iRefObjectID;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive �� �Ҵ�Ǿ� �ִ� OTF array �� pointer dimension
	 * @details iNumDims = 1 or 2 or 3
	 */
	int iNumDims;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive �� �Ҵ�Ǿ� �ִ� OTF array �� �� dimension �� ���� array ũ��
	 * @details 
	 * i3DimSizes.x : 1st dimension �� array ũ�� \n
	 * i3DimSizes.y : 2nd dimension �� array ũ�� \n
	 * i3DimSizes.z : 3rd dimension �� array ũ�� \n
	 * Valid dimension�� ���Ͽ� i3DimSizes.xyz > 0, Invalid demension�� ���Ͽ� i3DimSizes.xyz <= 0
	 */
	vmint3 i3DimSizes;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive �� �Ҵ�Ǿ� �ִ� ������ dimension�� ���� OTF array �� �� �ּҰ�
	 * @details
	 * i3ValidMinIndex.x : 1st dimension �� array �� �ּҰ� \n
	 * i3ValidMinIndex.y : 2nd dimension �� array �� �ּҰ� \n
	 * i3ValidMinIndex.z : 3rd dimension �� array �� �ּҰ�
	 */
	vmint3 i3ValidMinIndex;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive �� �Ҵ�Ǿ� �ִ� ������ dimension�� ���� OTF array �� �� �ִ밪
	 * @details 
	 * i3ValidMaxIndex.x : 1st dimension �� array �� �ִ밪 \n
	 * i3ValidMaxIndex.y : 2nd dimension �� array �� �ִ밪 \n
	 * i3ValidMaxIndex.z : 3rd dimension �� array �� �ִ밪
	 */
	vmint3 i3ValidMaxIndex;
	/// constructor, ��� 0 (NULL or false)���� �ʱ�ȭ
	SIVXTransferfunctionArchiveInfo(){
		iRefObjectID = 0; iNumDims = 0; 
		memset(&i3DimSizes, 0, sizeof(i3DimSizes));
		memset(&i3ValidMinIndex, 0, sizeof(i3ValidMinIndex));
		memset(&i3ValidMaxIndex, 0, sizeof(i3ValidMaxIndex));
	}
};
/**
 * @class SIVXTransferfunctionOpticalValue
 * @brief VXTObject�� @ref vxobjects::SVXTransferFunctionArchive �� array �� [Get/Set] �ϱ� ���� �ڷᱸ��
 * @sa
 * vxengineapi::SIVXTransferfunctionArchiveInfo, vxengineapi::VXETransferfunctionGetOpticalValue \n
 * vxobjects::SVXTransferFunctionArchive, vxobjects::CVXTObject 
 * 
 */
struct SIVXTransferfunctionOpticalValue{
	/**
	 * @brief array pf4OpticalValue �� ũ��
	 */
	int iSizeArray;
	/**
	 * @brief @ref vxobjects::SVXTransferFunctionArchive �� �Ҵ�Ǿ� �ִ� OTF array �� ���� array
	 * @details
	 * pf4OpticalValue[0 ~ (iSizeArray - 1)].xyzw : normalized RGBA �� ���� \n
	 * 1D OTF �� ���ǵǾ� (dimension == 1 or 2 && 2nd dimension metric == vxrMetricTypeINDEX) ���� �� @ref vxengineapi::SIVXTransferfunctionArchiveInfo �� i3DimSizes.x ���� ���� \n
	 * 2D OTF �� ���ǵǾ� ���� �� @ref vxengineapi::SIVXTransferfunctionArchiveInfo �� i3DimSizes.x*i3DimSizes.y �� ���� \n
	 * 3D OTF �� ���ǵǾ� ���� �� @ref vxengineapi::SIVXTransferfunctionArchiveInfo �� i3DimSizes.x*i3DimSizes.y*i3DimSizes.z �� ����
	 */
	vmbyte4* py4OpticalValue;	// xyzw : rgba
	/// constructor, ��� 0 (NULL or false)���� �ʱ�ȭ
	SIVXTransferfunctionOpticalValue(){
		iSizeArray = 0; py4OpticalValue = NULL;
	}
};
/**
 * @class SIVXOutBufferInfo
 * @brief VXIObject�� Frame Buffer�� [Get]�ϱ� ���� �ڷᱸ��
 * @sa
 * vxobjects::CVXIObject \n
 * vxengineapi::VXEImageplaneGetOutBufferInfo
 */
struct SIVXOutBufferInfo{
	/**
	 * @brief Frame Buffer �� i2FrameBufferSize.xy : (width, height)
	 */
	vmint2 i2FrameBufferSize;
	/**
	 * @brief array�� ���ǵ� Frame Buffer
	 */
	void* pvBuffer;
#ifdef __WINDOWS
	/**
	 * @brief win32���� File Memory�� ���� buffer interopaeration�� ���� handle
	 */
	HANDLE hFileMap;
#endif
	/**
	 * @brief Frame Buffer �� Data Type
	 */
	data_type dtype;
	/// constructor, ��� 0 (NULL or false)���� �ʱ�ȭ
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
 * @brief @ref vxobjects::SVXPrimitiveDataArchive �� ���ǵ� Primitive ������ [Get]�ϱ� ���� �ڷᱸ��
 * @sa
 * vxobjects::SVXPrimitiveDataArchive, vxobjects::CVXVObjectPrimitive \n
 * vxengineapi::VXEPrimitiveGetPrimitiveInfo
 */
struct SIVXPrimitiveInfo{
	/// Primitive�� ���� �� ��ü�� Vertex ����
	uint uiNumVertice;
	/// Primitive ����
	uint uiNumPrimitives;
	/// @ref vxobjects::CVXVObjectPrimitive �� ���ǵǾ� �ִ� ��ü���� OS ���� Axis-aligned bounding box�� ���� �ִ� ����
	vmfloat3 f3PosMaxBox;
	/// @ref vxobjects::CVXVObjectPrimitive �� ���ǵǾ� �ִ� ��ü���� OS ���� Axis-aligned bounding box�� ���� �ּ� ����
	vmfloat3 f3PosMinBox;
	/// Primitive�� ���� �� ��ü�� Polygon �� ���� Topology ���� ����
	bool bIsTopologyConnected;
};
/**
 * @class SIVXVolumeInfo
 * @brief @ref vxobjects::SVXVolumeDataArchive �� ���ǵ� Volume ���� �� ���� [Get/Set]�ϱ� ���� �ڷᱸ��
 * @sa
 * vxobjects::SVXVolumeDataArchive, @ref vxobjects::CVXVObjectVolume \n
 * vxhelpers::VXHVolumeFillHistogramBasedOnVolumeValues \n
 * vxengineapi::VXEVolumeGetVolumeInfo, vxengineapi::VXEVolumeSetVolumeInfo
 */
struct SIVXVolumeInfo{
	/**
	 * @brief Volume�� ���� Histogram �� �����ϴ� array
	 * @details 
	 * array ũ��� uint(d2MinMaxValue.y - d2MinMaxValue.x + 1.5) \n
	 * pullHistogram[volume value] = # of voxels
	 */
	ullong* pullHistogram;
	/**
	 * @brief Volume���� �����(ppvVolumeSlices)�� �ּҰ� d2MinMaxValue.x, �ִ밪 d2MinMaxValue.y
	 */
	vmdouble2 d2MinMaxValue;
	/**
	 * @brief Volume���� ����Ǳ� ���� ���ǵ� �ּҰ� d2ActualMinMaxValue.x, �ִ밪 d2ActualMinMaxValue.y
	 * @par ex. 
	 * file format float���� -1.5 ~ 2.5 ����� ������ ushort ���� ������ ��� 
	 * @par 
	 * >> d2MinMaxValue = vmdouble(0, 65535), d2ActualMinMaxValue = vmdouble(-1.5, 2.5);
	 */
	vmdouble2 d2ActualMinMaxValue;
	/**
	 * @brief Volume �� ũ�� i3VolumeSize = (width, height, depth or slices) \n
	 * i3SizeExtraBoundary �� ���Ե��� ����
	 */
	vmint3 i3VolumeSize;
	/**
	 * @brief CPU Memory Access Violation�� ���ϱ� ���� System Memory ���� Extra Boundary ������ ���ʸ� ũ��
	 * @details i3SizeExtraBoundary = (���� x�� ���� ũ��, ���� y�� ���� ũ��, ���� z�� ���� ũ��)
	 */
	vmint3 i3SizeExtraBoundary;
	
	/**
	 * @brief ���� Voxel�� ���� OS ���� cell edge�� WS ���� ũ��
	 * @details d3VoxelPitch = (x edge ũ��, y edge ũ��, z edge ũ��)
	 */
	vmdouble3 d3VoxelPitch;
	/**
	 * @brief Volume�� ������ 2D array
	 * @details 
	 * ���� �Ҵ�� x �� ���� ũ�� = i3VolumeSize.x + i3SizeExtraBoundary.x*2 \n
	 * ���� �Ҵ�� y �� ���� ũ�� = i3VolumeSize.y + i3SizeExtraBoundary.y*2 \n
	 * ���� �Ҵ�� z �� ���� ũ�� = i3VolumeSize.z + i3SizeExtraBoundary.z*2 \n
	 * @par ex. 
	 * ushort 512x512x512 Volume���� (100, 120, 150) index �� sample \n
	 * @par
	 * >> int iSamplePosX = 100 + i3SizeExtraBoundary.x; \n
	 * >> int iSamplePosY = 120 + i3SizeExtraBoundary.y; \n
	 * >> int iSamplePosZ = 150 + i3SizeExtraBoundary.z; \n
	 * >> ushort usValue = ((ushort**)ppvVolumeSlices)[iSamplePosZ][iSamplePosX + iSamplePosY*(i3VolumeSize.x + i3SizeExtraBoundary.x*2)];
	 */
	void** ppvVolumeSlices;
	/**
	 * @brief Volume array �� data type
	 */
	data_type stored_dtype;
	/**
	 * @brief �޸𸮿� ����Ǳ� �� Volume Original data type
	 * @par ex. 
	 * file format float ����� ������ ushort ���� ������ ���
	 * @par
	 * >> eDataType = vxrDataTypeUNSIGNEDSHORT, eDataTypeOriginal = vxrDataTypeFLOAT;
	 */
	data_type origin_dtype;
	
	/**
	 * @brief memory �� ����� volume space (���� ��ǥ)�� �ʱ� world space �� ��ġ�Ǵ� ��ȯ matrix ����
	 */
	AxisInfoOS2WS svxAlignAxisOS2WS;

	/// constructor, ��� 0 (NULL or false)���� �ʱ�ȭ
	SIVXVolumeInfo(){ pullHistogram = NULL; ppvVolumeSlices = NULL;
		d2MinMaxValue = d2ActualMinMaxValue = vmdouble2(0, 0);
		i3VolumeSize = i3SizeExtraBoundary = vmint3(0, 0, 0);
		d3VoxelPitch = vmdouble3(0, 0, 0);
	}
};
/**
 * @class SIVXVolumeSliceInfo
 * @brief @ref vxobjects::SVXVolumeDataArchive �� ���ǵ� Volume �� z�� Slice ���� [Get]�ϱ� ���� �ڷᱸ��
 * @sa
 * vxobjects::SVXVolumeDataArchive, vxobjects::CVXVObjectVolume \n
 * ****** ���� �Լ� �� �ڷ� ���� ���� ��ȹ! ******* \n
 * ****** ���� �Լ� ������� ������!!!!! ********** \n
 * vxengineapi::VXEVolumeGetVolumeZaxisSliceInfo
 */
struct SIVXVolumeSliceInfo{
	/**
	 * @brief Volume�� z�� ���� �ܸ�(���簢��)�� �����ϱ� ���� double �� array
	 * @details
	 * ��� data type�� Volume Value�� double ������ casting �Ǿ� ���� \n
	 * ���������� ...
	 */
	double* pdValues;
	/**
	 * @brief pdValues�� ����� ���� ���� �ּ�/�ִ밪
	 * @details d2MinMaxValue = (�ּҰ�, �ִ밪)
	 */
	vmdouble2 d2MinMaxValue;
	
	/**
	 * @brief Volume�� z�� ���� �ܸ�(���簢��)�� pixel ���� width, height ũ��(or ����)
	 * @details i2DimSize = (width, height)
	 */
	vmint2 i2DimSize;

	/// constructor, ��� 0 (NULL or false)���� �ʱ�ȭ
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
 * @brief ���� �����ǰ� �ִ� OS�� ���� System Memory ���¸� ����
 * @param pullFreeMemoryBytes [out] \n ullong \n ���� ��� ������ �޸� ũ�� (bytes)
 * @param pullAvailablePhysicalMemoryBytes [out] \n ullong \n ���� System �� �νĵǴ� ���� �޸� ũ�� (bytes)
 * @remarks x86 �Ǵ� x64, ���� ���� OS�� ���¿� ���� ���� �޸𸮿� �ٸ��� ���� �� ����.
 * @sa
 * @ref vxhelpers::VXHGetSystemMemoryInfo
*/
__vmstatic void VXEGetSystemMemoryInfo(ullong* pullFreeMemoryBytes/*out*/, ullong* pullAvailablePhysicalMemoryBytes/*out*/);


/*!
* @brief �ش� ID�� VXVObject�� ��� �Լ�
* @param puiSizeOfVolumesKB [out] \n uint \n Volume ����� �����ϴ� uint �����ͷ� KB ����
* @param puiSizeOfMeshesKB [out] \n uint \n Mesh ����� �����ϴ� uint �����ͷ� KB ����
* @param puiSizeOfIObjectsKB [out] \n uint \n Frame buffer ����� �����ϴ� uint �����ͷ� KB ����
* @param puiSizeOfEtcKB [out] \n uint \n TObject �� LObject ����� �����ϴ� uint �����ͷ� KB ����
* @remarks �����Ͱ� NULL �̸� ���� �������� ����
*/
__vmstatic void VXEGetAllocatedResources(uint* puiSizeOfVolumesKB /*out*/, 
	uint* puiSizeOfMeshesKB /*out*/, 
	uint* puiSizeOfIObjectsKB /*out*/, 
	uint* puiSizeOfEtcKB /*out*/);

// Pair
/*!
 * @fn __vmstatic bool vxengineapi::VXEBeginEngineLib()
 * @brief VXEngine (Manager or Engine Frame of VXFramework) �� ������
 * @return bool \n VXFramework�� ���� ������ ȯ���̸� true ��ȯ, �׷��� ������ ���� Class Instance���� ���� �ʰ� false ��ȯ
 * @remarks 
 * @ref vxengineapi::VXEEndEngineLib �� pair �� �̷�� ���Ǿ� �� \n
 * ���������� ���� Manager Classes ���� \n 
 * Singleton ���� ������� ������ Instance�� ��� ������ �� ����.
 * @sa vxengineapi::VXEEndEngineLib
*/
__vmstatic bool VXEBeginEngineLib();
/*!
 * @fn __vmstatic bool vxengineapi::VXEEndEngineLib()
 * @brief VXEngine (Manager or Engine Frame of VXFramework) �� ������
 * @return bool \n ���������� �����ϸ� true ��ȯ
 * @remarks 
 * @ref vxengineapi::VXEBeginEngineLib �� pair �� �̷�� ���Ǿ� �� \n 
 * ���������� ���� Manager Classes �� Resources ��� ���� �� ���� \n 
 * @sa vxengineapi::VXEBeginEngineLib
*/
__vmstatic bool VXEEndEngineLib();

// Common Object
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectIsGenerated(int iObjectID)
 * @brief �ش� ID�� VXObject�� �����Ǿ� �ִ��� Ȯ���ϴ� �Լ�
 * @param iObjectID \n int \n VXObject ID
 * @return bool \n Resource Manager�� ��ϵǾ� ������ true ��ȯ, �׷��� ������ false ��ȯ
 * @remarks 
*/
__vmstatic bool VXEObjectIsGenerated(int iObjectID);


// Common Object
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectRemove(int iObjectID)
 * @brief �ش� ID�� VXObject�� Resource���� �����ϴ� �Լ�
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return bool \n ������ �Ϸ�Ǹ� true ��ȯ, �׷��� ������ (ex. �ش� VXObject�� ���� ���) false ��ȯ
 * @remarks 
*/
__vmstatic bool VXEObjectRemove(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectSetMent(int iObjectID, string strHelpMent)
 * @brief �ش� VXObject�� Custom Mention�� �ۼ��ϴ� �Լ�
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param strHelpMent [in] \n string \n Custom Mention
 * @return bool Mention�� ��� �Ϸ�Ǹ� true ��ȯ, �׷��� ������ (ex. �ش� VXObject�� ���� ���) false ��ȯ
 * @remarks �ַ� VXObject ������ ��ó �� ��� ���� ���� ���ԵǸ�, Mention�� ���� ��� ""�� ��ȯ
 * @sa vxobjects::CVXObject, vxengineapi::VXEObjectGetMent
*/
__vmstatic bool VXEObjectSetMent(int iObjectID, string strHelpMent);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectGetMent(int iObjectID, string* pstrHelpMent)
 * @brief �ش� VXObject�� ��ϵǾ� �ִ� Custom Mention�� ����
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param pstrHelpMent [out] \n string \n �ش� VXObject�� ��ϵǾ� �ִ� Custom Mention�� ����Ǿ� �ִ� string�� ������
 * @return bool \n Mention�� ������ true ��ȯ, �׷��� ������ (ex. �ش� VXObject�� ���� ���) false ��ȯ
 * @remarks Mention�� ���� ��� ""�� ��ȯ
 * @sa vxobjects::CVXObject, vxengineapi::VXEObjectSetMent
*/
__vmstatic bool VXEObjectGetMent(int iObjectID, string* pstrHelpMent/*out*/);
/*!
 * @fn __vmstatic int vxengineapi::VXEObjectGetRelatedObjectID(int iObjectID)
 * @brief �ش� VXObject�� ���������� ������ �ֿ켱 ���� VXObject ID�� ��� �Լ�
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return int \n ������ �ֿ켱 ���� VXObject ID
 * @remarks ������ VXObject�� ���� ��� 0 ��ȯ
 * @sa vxobjects::CVXObject
*/
__vmstatic int VXEObjectGetRelatedObjectID(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectIsDefined(int iObjectID)
 * @brief �����Ǿ� �ִ� �ش� ID�� VXObject�� contents�� ���ǵǾ� �ִ��� Ȯ��
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return bool \n VXObject�� �����Ǿ� Resource�� ��ϵǾ� �ְ�, contents�� ���ǵǾ� ������ true ��ȯ, �׷��� ������ false ��ȯ
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
 * @brief �����Ǿ� �ִ� �ش� ID�� VXObject�� ���� @ref vxenums::EnumVXRObjectType �� ��� �Լ�
 * @param iObjectID [in] \n int \n VXObject ID 
 * @return EnumVXRObjectType \n �ش� VXObject�� �����Ǿ� ���� ������ @ref vxenums::vxrObjectTypeNONE ��ȯ
 * @remarks 
 * @sa
 * vxengineapi::VXEImageplaneGenerateNew, vxengineapi::VXETransferfunctionGenerateNew \n
 * vxengineapi::VXEVolumeGenerateNew, vxengineapi::VXEPrimitiveGenerateNew \n
 * vxengineapi::VXECustomListGenerateNew
*/
__vmstatic EvmObjectType VXEObjectGetTypeFromID(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectSetCustomParameter(int iObjectID, string strParameterName, string strParameterValue)
 * @brief �����Ǿ� �ִ� �ش� ID�� �ֻ��� Object @ref vxobjects::CVXObject �� ����� ���� parameter �� ����
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param strParameterName [in] \n string \n @ref �Ķ���� �̸�
 * @param eDataType [in] \n EnumVXRDataType \n @ref �Ķ���� Ÿ�� [string, bool, int, vmint[2, 3, 4], double, vmdouble[2, 3, 4], vmmat44]
 * @param *pvValue [in] \n void \n @ref ���� �����ϴ� ������
 * @return bool \n �ش� �۾��� ���������� ����Ǹ� true ��ȯ, �׷��� ������ false ��ȯ
 * @remarks 
 * @sa
 * vxobjects::CVXObject:RegisterCustomParameter \n
 */
__vmstatic bool VXEObjectSetCustomParameter(int iObjectID, string strParameterName, EnumVXRDataType eDataType, void* pvValue);
/*!
 * @fn __vmstatic bool vxengineapi::VXEObjectGetCustomParameter(int iObjectID, string strParameterName, string* pstrParameterValue/)
 * @brief �����Ǿ� �ִ� �ش� ID�� �ֻ��� Object @ref vxobjects::CVXObject �� ����Ǿ� �ִ� ����� ���� parameter �� ����
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param strParameterName [in] \n string \n @ref �Ķ���� �̸�
 * @param eDataType [in] \n EnumVXRDataType \n @ref �Ķ���� Ÿ�� [string, bool, int, vmint[2, 3, 4], double, vmdouble[2, 3, 4], vmmat44]
 * @param *pvValue [in] \n void \n @ref ���� �����ϴ� ������
 * @return bool \n �ش� �۾��� ���������� ����Ǹ� true ��ȯ, �׷��� ������ false ��ȯ
 * @remarks 
 * @sa
 * vxobjects::CVXObject:GetCustomParameter \n
 */
__vmstatic bool VXEObjectGetCustomParameter(int iObjectID, string strParameterName, EnumVXRDataType eDataType, void* pvValue/*out*/);

/*!
 * @fn __vmstatic double vxengineapi::VXEFrameWorkGetProgress()
 * @brief namespace @ref vxengineapi ���� ����Ǵ� static progress �� ��ȯ
 * @return double \n 0.0 ~ 100.0
 * @remarks �۾� �ε尡 ū function �ܿ� ���� 0.0 ��ȯ
*/
__vmstatic double VXEFrameWorkGetProgress();

// GPU Interface
/*!
 * @fn __vmstatic void vxengineapi::VXEGpuCleanAllResources()
 * @brief ���� GPU Memory�� ��ϵ� ��� resource�� ������
 * @remarks @ref vxgpuinterface::EnumVXGSdkType �� ��ϵ� ��� GPU SDK�� ���� �۾� ����
 * @sa @ref vxgpuinterface::EnumVXGSdkType, vxgpuinterface::CVXGPUManager
*/
__vmstatic void VXEGpuCleanAllResources();
/*!
 * @fn __vmstatic void vxengineapi::VXEGpuRemoveResource(int iIObectID)
 * @brief �ش� ID�� VXObject���κ��� ������ ��� GPU resource�� GPU Memory���� ����
 * @param iObectID [in] \n int \n VXObject ID 
 * @remarks 
 * @sa vxgpuinterface::CVXGPUManager
*/
__vmstatic void VXEGpuRemoveResource(int iObectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEGpuGetMemoryBytes(uint* puiDedicatedGpuMemoryBytes, uint* puiFreeMemoryBytes)
 * @brief CUDA API�� ���� GPU �޸� ���¸� ����
 * @param puiDedicatedGpuMemoryBytes [out] \n uint \n GPU ���� Memory ũ�⸦ ������ uint�� ������, bytes
 * @param puiFreeMemoryBytes [out] \n uint \n ���� ��� ������ GPU Memory ũ�⸦ ������ uint�� ������, bytes
 * @return bool \n CUDA API�� ���� GPU �޸� ���¸� ��µ� �����ϸ� true ��ȯ, �׷��� ������ false ��ȯ
 * @sa vxgpuinterface::CVXGPUManager
*/
__vmstatic bool VXEGpuGetMemoryBytes(uint* puiDedicatedGpuMemoryBytes/*out*/, uint* puiFreeMemoryBytes/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEGpuGetMemoryBytesUsedInVXFramework(uint* puiGpuAllMemoryBytes, uint* puiGpuVObjectMemoryBytes)
 * @brief ���� Ȱ��ȭ�� VXFramework ���� ��� ���� GPU mamory size�� ����
 * @param puiGpuAllMemoryBytes [out] \n uint \n GPU ���� Memory ũ�⸦ ������ uint�� ������, bytes
 * @param puiGpuVObjectMemoryBytes [out] \n uint \n 
 * GPU �� �Ҵ�Ǿ� �ִ� VObject�� ũ�⸦ ������ uint�� ������, bytes \n
 * Default �����Ͱ� NULL�� ��� �����Ϳ� ���� ���� ����
 * @return bool \n GPU �޸� ���¸� ��µ� �����ϸ� true ��ȯ, �׷��� ������ false ��ȯ
 * @remarks
 * @sa vxgpuinterface::CVXGPUManager
*/
__vmstatic bool VXEGpuGetMemoryBytesUsedInVXFramework(uint* puiGpuAllMemoryBytes/*out*/, uint* puiGpuVObjectMemoryBytes/*optional out*/ = NULL);

// Common VObject
/*!
 * @fn __vmstatic bool vxengineapi::VXEVObjectGetBoundingOrthoBox(int iObjectID, bool bIsBoxDefinedInOS, vmdouble3* pd3PosOrthoBoxMin, vmdouble3* pd3PosOrthoBoxMax)
 * @brief VXVObject�� ���� Bounding Box�� WS ��ġ�� ����
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param bIsBoxDefinedInOS [in] \n bool \n 
 * true�� OS ���� Bounding Box�� �ּ�/�ִ����� WS ������ ��ȯ�� ��ġ���� ���� \n
 * false�� WS ���� Bounding Box�� �ּ�/�ִ����� ���� \n
 * @param pd3PosOrthoBoxMin [out] \n vmdouble3 \n (OS or WS����) Bounding Box�� ���� WS ���� �ּ����� ������ vmdouble�� ������  
 * @param pd3PosOrthoBoxMax [out] \n vmdouble3 \n (OS or WS����) Bounding Box�� ���� WS ���� �ִ����� ������ vmdouble�� ������
 * @return bool \n �Լ��� ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks �ش� VXObject�� @ref vxobjects::CVXVObject �̾�� ��.
 * @sa vxobjects::CVXVObject
*/
__vmstatic bool VXEVObjectGetBoundingOrthoBox(int iObjectID, bool bIsBoxDefinedInOS /*false : WS*/, vmdouble3* pd3PosOrthoBoxMin/*out*/, vmdouble3* pd3PosOrthoBoxMax/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVObjectGetMatrixBetweenOSandWS(int iObjectID, vmmat44* pmatOS2WS, vmmat44* pmatWS2OS)
 * @brief VXVObject�� ���Ͽ� OS �� WS �� ��ȯ matrix�� ����
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param pmatOS2WS [out] \n vmmat44 \n OS���� WS���� ��ȯ�ϴ� matrix�� ������ vmmat44 ������, NULL�̸� ��� �� ��
 * @param pmatWS2OS [out] \n vmmat44 \n WS���� OS���� ��ȯ�ϴ� matrix�� ������ vmmat44 ������, NULL�̸� ��� �� ��
 * @return bool \n �۾��� ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks 
 * RHS, row major�� ���ǵ� vmmat44 ���� \n
 * OS���� ���ǵǴ� VXVObject�� WS�� ��ġ�Ǵ� ��ȯ�� ����
 * @sa 
 * vxobjects::CVXVObject \n
 * vxengineapi::VXEVObjectSetMatrixOS2WS
*/
__vmstatic bool VXEVObjectGetMatrixBetweenOSandWS(int iObjectID, vmmat44* pmatOS2WS/*optional out*/, vmmat44* pmatWS2OS/*optional out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVObjectSetMatrixOS2WS(int iObjectID, const vmmat44* pmatOS2WS)
 * @brief VXVObject�� ���Ͽ� OS �� WS �� ��ȯ matrix�� ����
 * @param iObjectID [in] \n int \n VXObject ID 
 * @param pmatOS2WS [in] \n vmmat44 \n OS���� WS���� ��ȯ�ϴ� matrix�� ���� vmmat44 ������
 * @return bool \n �۾��� ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks 
 * VXVObject�� WS�� ��ġ�ϴ� �Լ��� ����. \n
 * RHS, row major�� ���ǵǾ� ��.
 * @sa
 * vxobjects::CVXVObject \n
 * vxengineapi::VXEVObjectGetMatrixBetweenOSandWS
*/
__vmstatic bool VXEVObjectSetMatrixOS2WS(int iObjectID, const vmmat44* pmatOS2WS);
/*!
* @fn __vmstatic bool vxengineapi::VXEVObjectDeleteCustomObjects(int iObjectID)
* @brief VXVObject�� ��ϵǾ� �ִ� vxobjects::SVXVObjectBaseCustomData �� ��� ����
* @param iObjectID [in] \n int \n VXObject ID
* @return bool \n �۾��� ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
* @sa
* vxobjects::SVXVObjectBaseCustomData
*/
__vmstatic bool VXEVObjectRemoveCustomObjects(int iObjectID);


// Volume Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGenerateNew(int* piObjectID)
 * @brief VXVObjectVolume �� �����ϰ� �̸� Resource Manager�� �����.
 * @param piObjectID [out] \n int \n Resource Manager�� ��ϵ� VXVObjectVolume�� ID�� ������ int�� ������
 * @param iForcedObjectID [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks VXObejct�� ������ ���� �� ���� ���빰�� ä������ ���� ����.
 * @sa vxobjects::CVXVObjectVolume
*/
__vmstatic bool VXEVolumeGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeCopy(int iObjectDestID, int iObjectSrcID)
 * @brief iObjectSrcID�� VXVObjectVolume�� ����� Volume�� iObjectDestID�� VXVObjectVolume�� ����
 * @param iObjectDestID [in] \n int \n Source VXVObjectVolume ID
 * @param iObjectSrcID [in] \n int \n Destination VXVObjectVolume ID
 * @return bool \n Volume ���簡 ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks @ref vxobjects::SVXVolumeDataArchive�� Volume������ Volume Data�� ������ array�� ������ �����Ǿ� Copy��.
 * @sa 
 * vxobjects::SVXVolumeDataArchive \n
 * vxobjects::CVXVObjectVolume
*/
__vmstatic bool VXEVolumeCopy(int iObjectDestID, int iObjectSrcID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo)
 * @brief iObjectID�� VXVObjectVolume�� ����� @ref vxobjects::SVXVolumeDataArchive�� ������ ����
 * @param iObjectID [in] \n int \n Source VXVObjectVolume ID
 * @param psivxVolumeInfo [out] \n SIVXVolumeInfo \n @ref vxobjects::SVXVolumeDataArchive �� �����ϴ� Volume ������ ������ �ڷᱸ�� SIVXVolumeInfo �� ������
 * @return bool \n Volume ������ ���������� ������ true, �׷��� ������ false ��ȯ
 * @remarks 
 * @sa vxobjects::SVXVolumeDataArchive
*/
__vmstatic bool VXEVolumeGetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeSetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo)
 * @brief �޸𸮿� ����Ǿ� �ִ� volume���� �ش� ID�� VXVObjectVolume�� ����.
 * @param iObjectID [in] \n int \n volume�� ���ǵ� Resource Manager�� ��ϵ� VXVObjectVolume�� ID
 * @param psivxVolumeInfo [in] \n SIVXVolumeInfo \n volume�� �����ϰ� �ִ� SIVXVolumeInfo �� ������
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * SIVXVolumeInfo::pullHistogram �� ���ǵǾ� ������ �״�� VXVObjectVolume�� ���ǵǸ�, NULL �̸� ���ο��� Histogram �� �����Ͽ� ���� \n
 * SIVXVolumeInfo���� ���ǵ� ������(pullHistogram, ppvVolumeSlices)�� ����Ű�� �Ҵ�� �޸𸮴� Set�� ���ÿ� Resource Manager�� ��ϵǾ�, VXFramework���� �����ϰ� ��.
 * @sa vxengineapi::VXEVolumeGenerateNew, vxobjects::CVXVObjectVolume
*/
__vmstatic bool VXEVolumeSetVolumeInfo(int iObjectID, SIVXVolumeInfo* psivxVolumeInfo);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGetVolumeZaxisSliceInfo(int iObjectID, int iSliceIndex, SIVXVolumeSliceInfo* psivxVolumeSliceInfo)
 * @brief ������� ������!!! ���� �ڷᱸ�� �� ������ refactoring �� ����!
 * @param iObjectID 
 * @param iSliceIndex 
 * @param psivxVolumeSliceInfo 
 * @return __vmstatic bool 
 * @remarks 
*/
__vmstatic bool VXEVolumeGetVolumeZaxisSliceInfo(int iObjectID, int iSliceIndex, SIVXVolumeSliceInfo* psivxVolumeSliceInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeGetActualValuesAlongLine(int iObjectID, vmfloat3 f3PosStartWS, vmfloat3 f3PosEndWS, int iNumSamples, vector<double>* pvtrSampleValues)
 * @brief iObjectID�� VXVObjectVolume�� ����� Volume�� ���� �Ķ���ͷ� ���ǵǴ� Line ���� Volume ���� sample �ϴ� �Լ�
 * @param iObjectID [in] \n int \n Source VXVObjectVolume ID
 * @param f3PosStartWS [in] \n vmfloat3 \n WS���� ���ǵǴ� Line�� ������
 * @param f3PosEndWS [in] \n vmfloat3 \n WS���� ���ǵǴ� Line�� ����
 * @param iNumSamples [in] \n int \n Line �󿡼� sampling �� ���� ����
 * @param pvtrSampleValues [out] \n vector<double> \n Line �󿡼� sampling �� ���� ����� vector list�� ���� ������
 * @return bool \n ���������� �Լ��� �����ϸ� true, �׷��� ������ false ��ȯ.
 * @remarks ���ǵǴ� Line���� iNumSamples ����ŭ sampling�� ����� �� ó���� ���� �������� ��� sampling�� ����
*/
__vmstatic bool VXEVolumeGetActualValuesAlongLine(int iObjectID, vmfloat3 f3PosStartWS, vmfloat3 f3PosEndWS, int iNumSamples, vector<double>* pvtrSampleValues/*out*/);

/*!
* @fn __vmstatic bool vxengineapi::VXEVolumeGetActualValue(int iObjectID, vmfloat3 f3PosSample, bool bIsVolumeSpacePosition, double* pdSampleValue)
* @brief iObjectID�� VXVObjectVolume�� ����� Volume�� ���� �Ķ���ͷ� ���ǵǴ� Ư�� ��ǥ�� Volume ���� sample �ϴ� �Լ�
* @param iObjectID [in] \n int \n Source VXVObjectVolume ID
* @param f3PosSample [in] \n vmfloat3 \n WS �Ǵ� VS ���� ���ǵǴ� ���� ��ǥ
* @param bIsVolumeSpacePosition [in] \n bool \n f3PosSample �� ��ǥ�谡 VS �̸� true, WS �̸� false
* @param pdSampleValue [out] \n double \n �ش� ��ǥ �󿡼� sampling �� ���� ������ double ���� ���� ������
* @return bool \n ���������� �Լ��� �����ϸ� true, �׷��� ������ false ��ȯ.
* @remarks ���� ���� ������ �����ϰ� �Ǹ� �ּҰ��� ��ȯ�ϸ�, ��� Sample ���� Real ������ ��.
*/
__vmstatic bool VXEVolumeGetActualValue(int iObjectID, vmfloat3 f3PosSample, bool bIsVolumeSpacePosition, double* pdSampleValue/*out*/);

// Primitive Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXEPrimitiveGenerateNew(int* piObjectID)
 * @brief VXVObjectPrimitive �� �����ϰ� �̸� Resource Manager�� �����.
 * @param piObjectID [out] \n int \n Resource Manager�� ��ϵ� VXVObjectPrimitive�� ID�� ������ int�� ������
 * @param iForcedObjectID [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
 * @param bIsSafeID [in] \n bool \n Object ID �Ҵ� ������ �������� Safe Zone ���� ID�� �Ҵ��Ͽ� ID�� �ߺ����� �ʵ��� ��
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks VXObejct�� ������ ���� �� ���� ���빰�� ä������ ���� ����.
 * @sa vxobjects::CVXVObjectPrimitive
*/
__vmstatic bool VXEPrimitiveGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0, bool bIsSafeID = false);
/*!
 * @fn __vmstatic bool vxengineapi::VXEVolumeCopy(int iObjectDestID, int iObjectSrcID)
 * @brief iObjectSrcID�� VXVObjectPrimitive�� ����� Primitive�� iObjectDestID�� VXVObjectPrimitive�� ����
 * @param iObjectDestID [in] \n int \n Source VXVObjectPrimitive ID
 * @param iObjectSrcID [in] \n int \n Destination VXVObjectPrimitive ID
 * @return bool \n Primitive ���簡 ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks @ref vxobjects::SVXPrimitiveDataArchive�� Primitive������ Primitive Data�� ������ array�� ������ �����Ǿ� Copy��.
 * @sa 
 * vxobjects::SVXPrimitiveDataArchive \n
 * vxobjects::CVXVObjectPrimitive
*/
__vmstatic bool VXEPrimitiveCopy(int iObjectDestID, int iObjectSrcID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEPrimitiveGetPrimitiveInfo(int iObjectID, SIVXPrimitiveInfo* psivxPrimitiveInfo)
 * @brief iObjectID�� VXVObjectPrimitive�� ����� @ref vxobjects::SVXPrimitiveDataArchive �� ������ ����
 * @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
 * @param psivxPrimitiveInfo [out] \n SIVXPrimitiveInfo \n @ref vxobjects::SVXPrimitiveDataArchive �� �����ϴ� Primitive ������ ������ �ڷᱸ�� SIVXPrimitiveInfo �� ������
 * @return bool \n Primitive ������ ���������� ������ true, �׷��� ������ false ��ȯ
 * @remarks 
 * @sa vxobjects::SVXPrimitiveDataArchive
*/
__vmstatic bool VXEPrimitiveGetPrimitiveInfo(int iObjectID, SIVXPrimitiveInfo* psivxPrimitiveInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEPrimitiveSetFrontFace(int iObjectID, bool bIsFrontCCW)
 * @brief iObjectID�� VXVObjectPrimitive�� �����ϴ� Front Face �� �����ϴ� �Լ�
 * @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
 * @param bIsFrontCCW [in] \n bool \n Front Face �� �ð� �����̸� false, �ð� �ݴ� �����̸� true
 * @return bool \n ���������� �����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks 
 * @ref vxobjects::SVXPrimitiveDataArchive::bIsPrimitiveFrontCCW �� ���� \n
 * @sa vxobjects::CVXVObjectPrimitive, vxengineapi::VXEPrimitiveGetPrimitiveInfo
*/
__vmstatic bool VXEPrimitiveSetFrontFace(int iObjectID, bool bIsFrontCCW);
/*!
* @fn __vmstatic bool vxengineapi::VXPrimitiveSetColorVisibility(int iObjectID, vmfloat4 f4ColorMesh, bool bIsVisibleMesh, vmfloat4 f4ColorWire, bool bVisibleWire)
* @brief iObjectID�� VXVObjectPrimitive�� ���� Visibility �� Color �� �����ϴ� �Լ�
* @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
* @param f4ColorMesh [in] \n vmfloat4 \n Mesh �� �⺻ ������ ����
* @param bIsVisibleMesh [in] \n bool \n Mesh �� ���� Visibility
* @param f4ColorWire [in] \n vmfloat4 \n Mesh �� Wireframe �� ��Ÿ���� ���� ����
* @param bIsVisibleWire [in] \n bool \n Mesh �� Wireframe �� ���� Visibility
* @return bool \n ���������� �����Ǹ� true, �׷��� ������ false ��ȯ
* @remarks
* @ref vxobjects �� �⺻ �Ӽ� \n
* @sa vxobjects::CVXVObjectPrimitive, vxengineapi::VXEPrimitiveGetPrimitiveInfo
*/
__vmstatic bool VXPrimitiveSetColorVisibility(int iObjectID, vmfloat4 f4ColorMesh, bool bIsVisibleMesh, vmfloat4 f4ColorWire, bool bIsVisibleWire);
/*!
* @fn __vmstatic bool vxengineapi::VXPrimitiveGetColorVisibility(int iObjectID, vmfloat4* pf4ColorMesh, bool* pbIsVisibleMesh, vmfloat4* pf4ColorWire, bool* pbVisibleWire)
* @brief iObjectID�� VXVObjectPrimitive�� ���� Visibility �� Color �� �Ӽ��� ���� ���� �Լ�
* @param iObjectID [in] \n int \n Source VXVObjectPrimitive ID
* @param pf4ColorMesh [out](optional) \n vmfloat4 \n Mesh �� �⺻ ������ ������ vmfloat4 ������, NULL�̸� ��� �� ��
* @param pbIsVisibleMesh [out](optional) \n bool \n Mesh �� ���� Visibility�� ������ bool ������, NULL�̸� ��� �� ��
* @param pf4ColorWire [out](optional) \n vmfloat4 \n Mesh �� Wireframe �� ��Ÿ���� ������ ������ vmfloat4 ������, NULL�̸� ��� �� ��
* @param pbIsVisibleWire [out](optional) \n bool \n Mesh �� Wireframe �� ���� Visibility�� ������ bool ������, NULL�̸� ��� �� ��
* @return bool \n ���������� �����Ǹ� true, �׷��� ������ false ��ȯ
* @remarks
* @ref vxobjects �� �⺻ �Ӽ� \n
* @sa vxobjects::CVXVObjectPrimitive, vxengineapi::VXEPrimitiveGetPrimitiveInfo
*/
__vmstatic bool VXPrimitiveGetColorVisibility(int iObjectID, vmfloat4* pf4ColorMesh, bool* pbIsVisibleMesh, vmfloat4* pf4ColorWire, bool* pbIsVisibleWire);

// CustomList Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListGenerateNew(int* piObjectID)
 * @brief VXLObject �� �����ϰ� �̸� Resource Manager�� �����.
 * @param piObjectID [out] \n int \n Resource Manager�� ��ϵ� VXLObject�� ID�� ������ int�� ������
 * @param iForcedObjectID [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks VXObejct�� ������ ���� �� ���� ���빰�� ä������ ���� ����.
 * @sa vxobjects::CVXLObject
*/
__vmstatic bool VXECustomListGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListGetList(int iObjectID, string* pstrListName, void* pvOutput)
 * @brief iObjectID�� VXLObject�� ����Ǿ� �ִ� Custom List�� ��� �Լ�
 * @param iObjectID [in] \n int \n Source VXLObject ID
 * @param pstrListName [in] \n string \n "_vlist_[type string]_[Name]" �������� �־��� ���ڿ� string�� ������
 * @param ppvOutput [out] \n void* \n Custom List�� ����� vector<type> �����Ϳ� ���� ������ (���� ������)
 * @return bool \n pstrListName���� ����Ǿ� �ִ� Custom List�� �ش� VXLObject�� ������ true, ������ false ��ȯ
 * @remarks 
 * pstrListName���� �����ϴ� [type string]�� VXFramework���� ������ ���ڿ�(@ref vxhelpers::VXHGetDataTypeFromString)�� ����\n
 * Custom List�� vector container�� ���ǵǾ� ����.
 * @sa vxobjects::CVXLObject, vxhelpers::VXHGetDataTypeFromString, vxengineapi::VXECustomListSetList
*/
__vmstatic bool VXECustomListGetList(int iObjectID, string* pstrListName, void** ppvOutput/*out*/, size_t& size_bytes);
__vmstatic bool VXECustomListGetStringObjs(int iObjectID, string* pstrListName, string** ppstrOutput/*out*/, int& num_stringobjs);
/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListSetList(int iObjectID, string* pstrListName, void* pvInput)
 * @brief iObjectID�� VXLObject�� Custom List�� ����ϴ� �Լ�
 * @param iObjectID [in] \n int \n �����ϰ��� �ϴ� VXLObject�� ID
 * @param pstrListName [in] \n string \n "_vlist_[type string]_[Name]" �������� �־��� ���ڿ� string�� ������
 * @param pvInput [out] \n void \n Custom List�� ������ vector<type>�� ������
 * @return bool \n Custom List�� ����� �����ϸ� true, ������ false ��ȯ
 * @remarks 
 * pstrListName���� �����ϴ� [type string]�� VXFramework���� ������ ���ڿ�(@ref vxhelpers::VXHGetDataTypeFromString)�� ����\n
 * Custom List�� vector container�� ����.
 * @sa vxobjects::CVXLObject, vxhelpers::VXHGetDataTypeFromString, vxengineapi::VXECustomListGetList
*/
__vmstatic bool VXECustomListSetList(int iObjectID, string* pstrListName, void* pvInput, int ele_num, int type_bytes, void* dstInput = NULL);
__vmstatic bool VXECustomListSetStringObjs(int iObjectID, string* pstrListName, string* pvInput, int ele_num);

/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListSetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, double dValue)
 * @brief iTargetObjectID �� ���� custom parameter �� �����ϰ� �ִ� map �� ���� ��� \n
 * @param iTargetObjectID [in] \n int \n Custom Parameter �� Target �� �Ǵ� Object ID \n
 * @param strName [in] \n string \n �ش� iTargetObjectID �� ���� ����Ǿ� �ִ� map �� Key \n
 * @param pvValue [in] \n void \n �ش� iTargetObjectID �� ���� ������ �ִ� map �� Value \n
 * @return bool \n �����ϸ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * ���� �Ķ���� ������ @ref VXHStringGetParameterFromCustomStringMap �� ���� (�� String ������ ���� �� ��)
 * @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListSetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, void* pvValue, size_t bytes_obj, void* pvValueDst = NULL);

/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListSetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, double dValue)
 * @brief iTargetObjectID �� ���� custom parameter �� �����ϰ� �ִ� map �� ���� ��� \n
 * @param iTargetObjectID [in] \n int \n Custom Parameter �� Target �� �Ǵ� Object ID \n
 * @param strName [in] \n string \n �ش� iTargetObjectID �� ���� ����Ǿ� �ִ� map �� Key \n
 * @param pvValue [out] \n void \n �ش� iTargetObjectID �� ���� ����Ǿ� �ִ� map �� Value �� �����ϰ� �ִ� void ������ \n
 * @return bool \n �����ϸ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * ���� �Ķ���� ������ @ref VXHStringGetParameterFromCustomStringMap �� ���� (�� String ������ ���� �� ��)
 * @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListGetCustomValue(int iObjectID, int iTargetObjectID, string* pstrListName, void* pvValue);

/*!
 * @fn __vmstatic bool vxengineapi::VXECustomListCopyCustomValues(int iObjectSrcID, int iObjectDstID)
 * @brief iObjectSrcID �� VxCLObject �� �ִ� custom parameters �� iObjectDstID �� VxCLObject �� �ִ� custom parameters �� Copy \n
 * @param iObjectSrcID [in] \n int \n ������ ������ ���� Object ID \n
 * @param iObjectDstID [in] \n int \n ����� Object ID \n
 * @return bool \n �����ϸ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * ���� iObjectDstID�� map values �� ��� ���ο� map values �� ��ü��.
 * @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListCopyCustomValues(int iObjectSrcID, int iObjectDstID);

/*!
* @fn __vmstatic bool vxengineapi::VXECustomListRemoveCustomValueGroup(int iObjectID, int iTargetObjectID)
* @brief iObjectID �� VxCLObject �� �ִ� �ش� VxObject �� ���� custom parameters �� ���� \n
* @param iObjectID [in] \n int \n VxCLObject �� Object ID \n
* @param iTargetObjectID [in] \n int \n ����� �Ǵ� VxObject ID \n
* @return bool \n �����ϸ� true, �׷��� ������ false ��ȯ.
* @remarks
* @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListRemoveCustomValueGroup(int iObjectID, int iTargetObjectID);

/*!
* @fn __vmstatic bool vxengineapi::VXECustomListRemoveAll(int iObjectID)
* @brief iObjectID �� VxCLObject �� �ִ� ��� ���빰�� ���� \n
* @param iObjectID [in] \n int \n VxCLObject �� Object ID \n
* @return bool \n �����ϸ� true, �׷��� ������ false ��ȯ.
* @remarks
* @sa CVXLObject::RegisterList
*/
__vmstatic bool VXECustomListRemoveAll(int iObjectID);

// Transfer Function Object //
/*!
 * @fn __vmstatic bool vxengineapi::VXETransferfunctionGenerateNew(int* piObjectID)
 * @brief VXTObject �� �����ϰ� �̸� Resource Manager�� �����.
 * @param piObjectID [out] \n int \n Resource Manager�� ��ϵ� VXTObject�� ID�� ������ int�� ������
 * @param iForcedObjectID [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks VXObejct�� ������ ���� �� ���� ���빰�� ä������ ���� ����.
 * @sa vxobjects::CVXTObject
*/
__vmstatic bool VXETransferfunctionGenerateNew(int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
 * @fn __vmstatic bool vxengineapi::VXETransferfunctionGetArchiveInfo(int iObjectID, SIVXTransferfunctionArchiveInfo* psivxTransferfunctionArchiveInfo)
 * @brief iObjectID�� VXTObject�� ����Ǿ� �ִ� @ref vxobjects::SVXTransferFunctionArchive �� ������ ����
 * @param iObjectID [in] \n int \n Source VXTObject ID
 * @param psivxTransferfunctionArchiveInfo [out] \n SIVXTransferfunctionArchiveInfo \n @ref vxobjects::SVXTransferFunctionArchive �� �����ϴ� Primitive ������ ������ �ڷᱸ�� SIVXTransferfunctionArchiveInfo �� ������
 * @return bool \n ������ ���������� ������ true, �׷��� ������ false ��ȯ
 * @remarks OTF ���� �����ϰ� �ִ� array�� @ref vxengineapi::VXETransferfunctionGetOpticalValue �� Ȯ��
 * @sa vxobjects::SVXTransferFunctionArchive, vxengineapi::VXETransferfunctionGetOpticalValue
*/
__vmstatic bool VXETransferfunctionGetArchiveInfo(int iObjectID, SIVXTransferfunctionArchiveInfo* psivxTransferfunctionArchiveInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXETransferfunctionGetOpticalValue(int iObjectID, SIVXTransferfunctionOpticalValue* psivxTransferfunctionOpticalValue, int iTfIndexZ)
 * @brief iObjectID�� VXTObject�� ����Ǿ� �ִ� @ref vxobjects::SVXTransferFunctionArchive �� OTF array ���� ����
 * @param iObjectID [in] \n int \n Source VXTObject ID
 * @param psivxTransferfunctionOpticalValue [out] \n SIVXTransferfunctionOpticalValue \n @ref vxobjects::SVXTransferFunctionArchive �� ���ǵǾ� �ִ� OTF array �� ���� ������ ���� SIVXTransferfunctionOpticalValue �� ������
 * @param iTfIndexZ [in](optional) \n int \n OTF dimension�� ���� ���� archive�� ����� array�� dimension�� ū ��� (ex. 1D summed preintegrating OTF) �ش� Target metric�� OTF array pointer�� ���� indexing
 * @return bool \n ������ ���������� ������ true, �׷��� ������ false ��ȯ
 * @remarks 
 * OTF array �� �̿��� ������ @ref vxengineapi::VXETransferfunctionGetArchiveInfo �� Ȯ�� \n
 * OTF dimension�� ����� array�� dimension�� ������ iTfIndexZ�� �׻� 0�� ����.
 * @sa vxobjects::SVXTransferFunctionArchive, vxengineapi::VXETransferfunctionGetArchiveInfo
*/
__vmstatic bool VXETransferfunctionGetOpticalValue(int iObjectID, SIVXTransferfunctionOpticalValue* psivxTransferfunctionOpticalValue/*out*/, int iTfIndexZ = 0);

// Image Plane Object //
/*!
 * @fn __vmstatic bool VXEImageplaneGenerateNew(vmint2 i2WindowSizePix, int* piObjectID)
 * @brief VXIObject �� �����ϰ� �̸� Resource Manager�� �����.
 * @param piObjectID [out] \n int \n Resource Manager�� ��ϵ� VXIObject�� ID�� ������ int�� ������
 * @param iForcedObjectID [in] \n int \n ������ ����ϰ��� �ϴ� ID, 0 �� �ƴϸ� ���� ����� �õ�
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks VXObejct�� ������ ���� �� ���� ���빰�� ä������ ���� ����.
 * @sa vxobjects::CVXIObject
*/
__vmstatic bool VXEImageplaneGenerateNew(vmint2 i2WindowSizePix, int* piObjectID/*out*/, int iForcedObjectID = 0);
/*!
* @fn __vmstatic bool vxengineapi::VXEImageplaneGenerateBmpBindBuffer(int iObjectID, vmint2 i2WindowSizePix)
* @brief iObjectID�� VXIObject�� Bitmap Bind �� vxenums::EnumVXRDataType::vxrDataTypeBYTE4 �� �Ҵ��ϴ� �Լ�
* @param iObjectID [in] \n int \n Source VXIObject ID
* @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
* @remarks
* ������ IObject �� ��ϵ� ���� ũ�⿡ �ش��ϴ� vxenums::EnumVXRFrameBufferUsage::vxrFrameBufferUsageRENDEROUT �� ���۸� index 0 �� �Ҵ� \n
* �̹� �����Ǿ� ������ ���� �Ҵ����� ���� \n
* @sa vxobjects::CVXIObject, vxengineapi::VXEImageplaneGenerateNew
*/
__vmstatic bool VXEImageplaneGenerateBmpBindBuffer(int iObjectID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneResize(int iObjectID, vmint2 i2WindowSizePix)
 * @brief iObjectID�� VXIObject�� �Ҵ�� Frame Bufffer�� ũ�⸦ �缳���ϴ� �Լ�
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param i2WindowSizePix [in] \n vmint2 \n ���ο� Frame Bufffer�� ũ�� \n 
 * >> i2WindowSizePix = vmint2(width, height);
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * ������ Frame Buffer�� �޸� �󿡼� �����ϰ� ���� �޸𸮸� �Ҵ� \n
 * PS (Projection Space) �� SS (Screen Space) ���� SS ������ Image Plane ũ�Ⱑ ���ϹǷ� WS (World Space) �� SS �� ��ȯ matrix�� �ٽ� �����\n
 * �� �������� image plane�� ������ ���������� �ٽ� ����. (@ref vxobjects::CVXIObject::ResizeFrameBuffer)
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject
*/
__vmstatic bool VXEImageplaneResize(int iObjectID, vmint2 i2WindowSizePix);
/*!
* @fn __vmstatic bool vxengineapi::VXEImageplaneGetOutBufferSize(int iObjectID, EnumVXRFrameBufferUsage eFrameBufferUsage, int iBufferIndex, SIVXOutBufferInfo* psvxBufferInfo)
* @brief iObjectID�� VXIObject�� ����� Buffer �� 2D Size �� ������ ����
* @param iObjectID [in] \n int \n Source VXIObject ID
* @param pi2BufferSize [out] \n vmint2 \n �ش� VXIObject�� ����� Buffer �� ������ ���� vmint2 �� ������
* @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
* @remarks
* VXIObject�� �������� @ref vxobjects::SVXFrameBuffer �� ������, ��� Buffer �� 2D Size �� ����. �̰��� Size �� ��ȯ��.\n
* @sa vxobjects::CVXIObject
*/
__vmstatic bool VXEImageplaneGetOutBufferSize(int iObjectID, vmint2* pi2BufferSize/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetOutBufferInfo(int iObjectID, EnumVXRFrameBufferUsage eFrameBufferUsage, int iBufferIndex, SIVXOutBufferInfo* psvxBufferInfo)
 * @brief iObjectID�� VXIObject�� ����� @ref vxobjects::SVXFrameBuffer �� ������ ����
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param eFrameBufferUsage [in] \n EnumVXRFrameBufferUsage \n �ش� VXIObject�� ����� @ref vxobjects::SVXFrameBuffer �鿡 ���� ��� �뵵(@ref vxenums::EnumVXRFrameBufferUsage)�� ���� Buffer�� ����
 * @param iBufferIndex [in] \n int \n �ش� VXIObject�� ����� @ref vxobjects::SVXFrameBuffer �鿡 ���� ���� ��� �뵵���� index�� ���� Buffer ����
 * @param psvxBufferInfo [out] \n SIVXOutBufferInfo \n �ش� VXIObject�� ����� @ref vxobjects::SVXFrameBuffer �� ������ ���� SIVXOutBufferInfo �� ������
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * VXIObject�� �������� @ref vxobjects::SVXFrameBuffer �� ������, eFrameBufferUsage�� iBufferIndex�� Frame Buffer ������ indexing ��.\n
 * �Ϲ������� Render out �Ǵ� RGBA(4bytes) ���� pixel�� ���۴� eFrameBufferUsage = @ref vxenums::vxrFrameBufferUsageRENDEROUT �� iBufferIndex = 0 �� �ش��ϴ� Frame Buffer�� ����Ǿ� ���� \n
 * (�����δ� Module ���� ��� ����� �����ϴ��Ŀ� ���� �ٸ�) \n
 * @sa vxobjects::CVXIObject, @ref vxobjects::SVXFrameBuffer
*/
__vmstatic bool VXEImageplaneGetOutBufferInfo(int iObjectID, EvmFrameBufferUsage eFrameBufferUsage, int iBufferIndex, SIVXOutBufferInfo* psvxBufferInfo/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneSetCameraState(int iObjectID, const SIVXCameraStateDescription* psivxCameraStateDescriptor)
 * @brief iObjectID�� VXIObject�� ���ԵǾ� �ִ� Camera ���� @ref vxobjects::CVXCObject �� Camera States �� �����ϴ� �Լ�
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param psivxCameraStateDescriptor [in] \n SIVXCameraStateDescription \n ������ Camera States �� ����Ǿ� �ִ� SIVXCameraStateDescription�� ������
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * SIVXCameraStateDescription �� ��� ������ �� ��ȿ�ؾ� �ϹǷ� @ref vxengineapi::VXEImageplaneGetCameraState ���� ���� ���� Camera �������� ���ϴ� ������ ���� �� Setter�� �����.\n
 * iObjectID�� VXIObject �� defined �Ǿ�(Frame Buffer�� ũ�Ⱑ 0 ���� ũ�� ����) �־�� WS (World Space) �� SS (Screen Space) �� ��ȯ matrix�� ����� ������.
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneGetCameraState
*/
__vmstatic bool VXEImageplaneSetCameraState(int iObjectID, const SIVXCameraStateDescription* psivxCameraStateDescriptor);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetCameraState(int iObjectID, SIVXCameraStateDescription* psivxCameraStateDescriptor)
 * @brief iObjectID�� VXIObject�� ����Ǿ� �ִ� Camera ���� @ref vxobjects::CVXCObject �� ������ Camera States �� ��� �Լ�
 * @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject �� �����ϰ� �ִ� Source VXIObject ID
 * @param psivxCameraStateDescriptor [out] \n SIVXCameraStateDescription \n Camera States�� ����� SIVXCameraStateDescription�� ������
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * iObjectID�� VXIObject �� defined �Ǿ�(Frame Buffer�� ũ�Ⱑ 0 ���� ũ�� ����) ���� ������ ����Ȯ�� ����� ����� �� ����
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState
*/
__vmstatic bool VXEImageplaneGetCameraState(int iObjectID, SIVXCameraStateDescription* psivxCameraStateDescriptor/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneSetOrthoStageFitting(int iObjectID, vmdouble3 d3PosOrthoBoxMinWS, vmdouble3 d3PosOrthoBoxMaxWS, EnumVXVStageViewType eStageViewFlag)
 * @brief iObjectID�� VXIObject�� ����Ǿ� �ִ� Camera ���� @ref vxobjects::CVXCObject �� Camera States �� WS ���� ���ǵ� axis-aligned box stage �������� �ʱ�ȭ�ϴ� �Լ�
 * @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject �� �����ϰ� �ִ� Source VXIObject ID
 * @param d3PosOrthoBoxMinWS [in] \n vmdouble3 \n WS ���� axis-aligned box stage �� �����ϴ� �ּ����� ��ġ
 * @param d3PosOrthoBoxMaxWS [in] \n vmdouble3 \n WS ���� axis-aligned box stage �� �����ϴ� �ִ����� ��ġ
 * @param eStageViewFlag [in] \n EnumVXVStageViewType \n Camera�� �ʱ�ȭ ��ġ ����
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks �̰��� ���� �����Ǵ� Camera�� �������� zoom ratio 1.0 �� ����.
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState, @ref vxengineapi::VXEImageplaneGetCameraState
*/
__vmstatic bool VXEImageplaneSetOrthoStageFitting(int iObjectID, vmdouble3 d3PosOrthoBoxMinWS, vmdouble3 d3PosOrthoBoxMaxWS, EvmStageViewType eStageViewFlag);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetMatrixBetweenWSandSS(int iObjectID, vmmat44* psvxMatrixWS2SS, vmmat44* psvxMatrixSS2WS)
 * @brief iObjectID�� VXIObject�� ����Ǿ� �ִ� Camera ���� @ref vxobjects::CVXCObject �κ��� SS �� WS �� ��ȯ matrix�� ��� �Լ�
 * @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject �� �����ϰ� �ִ� Source VXIObject ID
 * @param psvxMatrixWS2SS [out] \n vmmat44 \n WS ���� SS �� ��ȯ�ϴ� matrix�� ����� vmmat44 �� ������, NULL�̸� ��� �� ��
 * @param psvxMatrixSS2WS [out] \n vmmat44 \n SS ���� WS �� ��ȯ�ϴ� matrix�� ����� vmmat44 �� ������, NULL�̸� ��� �� ��
 * @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * psvxMatrixWS2SS �� psvxMatrixSS2WS �� ����� ����\n
 * RHS, row major matrix�� ����
 * @ref vxengineapi::VXEImageplaneSetCameraState �� vxengineapi::VXEImageplaneResize �� ȣ��Ǹ� ��ȯ matrix�� �缳�� ��.
 * @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState, @ref vxengineapi::VXEImageplaneResize
*/
__vmstatic bool VXEImageplaneGetMatrixBetweenWSandSS(int iObjectID, vmmat44* psvxMatrixWS2SS/*optional out*/, vmmat44* psvxMatrixSS2WS/*optional out*/);
/*!
* @fn __vmstatic bool vxengineapi::VXEImageplaneGetMatrixThroughWSandSS(int iObjectID, vmmat44* psvxMatrixWS2SS, vmmat44* psvxMatrixSS2WS)
* @brief iObjectID�� VXIObject�� ����Ǿ� �ִ� Camera ���� @ref vxobjects::CVXCObject �κ��� WS -> CS -> PS -> SS ���� ��ȯ matrix�� ��� �Լ�
* @param iObjectID [in] \n int \n @ref vxobjects::CVXCObject �� �����ϰ� �ִ� Source VXIObject ID
* @param psvxMatrixWS2SS [out] \n vmmat44 \n WS ���� CS �� ��ȯ�ϴ� matrix�� ����� vmmat44 �� ������, NULL�̸� ��� �� ��
* @param psvxMatrixSS2WS [out] \n vmmat44 \n CS ���� PS �� ��ȯ�ϴ� matrix�� ����� vmmat44 �� ������, NULL�̸� ��� �� ��
* @param psvxMatrixSS2WS [out] \n vmmat44 \n PS ���� SS �� ��ȯ�ϴ� matrix�� ����� vmmat44 �� ������, NULL�̸� ��� �� ��
* @return bool \n ���������� ����Ǹ� true, �׷��� ������ false ��ȯ.
* @remarks
* psvxMatrixWS2SS �� psvxMatrixSS2WS �� ����� ����\n
* RHS, row major matrix�� ����
* @ref vxengineapi::VXEImageplaneSetCameraState �� vxengineapi::VXEImageplaneResize �� ȣ��Ǹ� ��ȯ matrix�� �缳�� ��.
* @sa vxobjects::CVXIObject, vxobjects::CVXCObject, @ref vxengineapi::VXEImageplaneSetCameraState, @ref vxengineapi::VXEImageplaneResize
*/
__vmstatic bool VXEImageplaneGetMatrixThroughWSandSS(int iObjectID, vmmat44* psvxMatrixWS2CS/*optional out*/, vmmat44* psvxMatrixCS2PS/*optional out*/, vmmat44* psvxMatrixPS2SS/*optional out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEImageplaneGetOutRgbaAlongLine(int iObjectID, int iNumSamples, vmint2 i2PosStartSS, vmint2 i2PosEndSS, vector<vmbyte4>* pvtrRGBAValues)
 * @brief iObjectID�� VXIObject���� Render out �� Frame Buffer�� ����Ǿ� �ִ� ���� sampling �ϴ� �Լ�
 * @param iObjectID [in] \n int \n Source VXIObject ID
 * @param iNumSamples [in] \n int \n Line �󿡼� sampling �� ���� ����
 * @param i2PosStartSS [in] \n vmint2 \n SS���� ���ǵǴ� Line�� ������
 * @param i2PosEndSS [in] \n vmint2 \n SS���� ���ǵǴ� Line�� ����
 * @param pvtrRGBAValues [out] \n vector<vmbyte4> \n Line �󿡼� sampling �� ���� ����� vector list�� ���� vector<vmbyte4>�� ������
 * @return bool \n ���������� �Լ��� �����ϸ� true, �׷��� ������ false ��ȯ.
 * @remarks 
 * ���ǵǴ� Line���� iNumSamples ����ŭ sampling�� ����� �� ó���� ���� �������� ��� sampling�� ���� \n
 * VXIObject�� ����� @ref vxobjects::SVXFrameBuffer �� �� eFrameBufferUsage = @ref vxenums::vxrFrameBufferUsageRENDEROUT �� iBufferIndex = 0 �� �ش��ϴ� Frame Buffer �� ������� ��
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
 * @brief iModuleID ����� type�� ��� �Լ�
 * @param iModuleID [in] \n int \n Source Module ID
 * @return EnumVXMModuleType \n Module Type �� ��ȯ. Type �� �������� �ʴ� Module �� ��� vxenums::vxmModuleTypeNONE ��ȯ
 * @remarks @ref Module Arbiter���� �����Ǹ�, VXObject ID�� ���� ��쵵 ������ ������ Manager���� �����Ǵ� ID �̹Ƿ� ��� ����
*/
__vmstatic EnumVXMModuleType VXEModuleTypeGet(int iModuleID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEModuleRegister(string strModuleName, EnumVXMModuleType eModuleType, string strModuleSpecifier, int* piModuleID)
 * @brief Module�� File �̸����� Module Arbiter�� Module ���
 * @param strModuleName [in] \n string \n Module File�� ��� �� �̸�
 * @param eModuleType [in] \n EnumVXMModuleType \n ����� Module�� Type
 * @param strModuleSpecifier [in] \n string \n ����� Module�� ���� Mention, ������ ������ ������, string Ÿ���� �ƹ� ���� �־ ��.
 * @param piModuleID [out] \n int \n ��ϵ� Module ID�� ������ int ������
 * @return bool \n ����� �����ϸ� true, �׷��� ���� ��� false�� ��ȯ�ϰ� *piModuleID = 0 ���� ó��.
 * @remarks 
*/
__vmstatic bool VXEModuleRegister(string strModuleName, EnumVXMModuleType eModuleType, string strModuleSpecifier, int* piModuleID/*out*/);
/*!
 * @fn __vmstatic bool vxengineapi::VXEModuleExecute(int iModuleID, const map<string, int>* pmapObjIDs, const map<string, void*>* pmapCustomParameters, const map<string, void*>* pmapCustomObjects, bool bIsOnlyParameterCheck)
 * @brief iModuleID�� Module�� �����ϴ� �Լ�
 * @param iModuleID [in] \n int \n �����ϰ��� �ϴ� Module ID
 * @param pmapObjIDs [in] \n map<string, int> \n 
 * Module�� ���� VXObject �鿡 ���� ID ���� ��ϵ� container map�� ���� map<string, int>�� ������ \n
 * ���������� @ref vxhelpers::VXHStringGetVXObjectFromObjectStringMap �� ���� ID�� ���� �� �ֵ��� container�� ������ \n
 * NULL�� ��� ��� �� ��.
 * >> string key �� ���� \n
 * >> _[in/out]_[VOLUME/PRIMITIVE/IMAGEPLANE/TRANSFERFUNCTION/CUSTOMLIST]_[0/1/2/3... 0 based-index] \n
 * >> ex. 3��° VXVObjectVolume�� ��� : "_in_VOLUME_2"
 * @param pmapCustomParameters [in] \n map<string, void*> \n 
 * Module�� ���� VXFramework format���� ���ǵ� container map�� ���� map<string, void*>�� ������ \n
 * @ref vxhelpers::VXHStringGetParameterFromCustomStringMap �� ���� ���� ���� �� �ֵ��� container�� ���� \n
 * NULL�� ��� ��� �� ��.
 * >> string key �� Value �� ���� \n
 * >> _[bool/int/double/int2/int3/int4/double2/double3/double4/matrix44/string]_[name] \n
 * @param pmapCustomObjects [in] \n map<string, void*> \n 
 * Module�� ���� custom pointer�� ���ǵ� container map�� ���� map<string, void*>�� ������ \n
 * @ref vxhelpers::VXHStringGetCustomObjectFromPointerStringMap �� ���� �����͸� ���� �� �ֵ��� container�� ���� \n
 * NULL�� ��� ��� �� ��.
 * >> string key �� ���� \n
 * >> _[in/out]_[class/vector/custom...]_[name] \n
 * >> ex. "_in_class_GPUMANAGER"
 * @param bIsOnlyParameterCheck [in] \n bool \n true�̸� Module�� Parameter Checker�� �����ϸ�, false�� Module �۾� ��ü�� ����
 * @return bool \n Module�� ���������� �����ϸ� true, �׷��� ������ false ��ȯ
 * @remarks ������ container�� Module ������ �ƴ� Ư���ϰ� ����� container �������� parameter�� ������ ���� ����
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
 * @brief iModuleID�� Module�� Module Arbiter���� ����
 * @param iModuleID [in] \n int \n �����ϰ��� �ϴ� Module ID
 * @param pmapCustomParamters [in] \n map<string, void*> \n 
 * Module ���� ���� �� ���� VXFramework format���� ���ǵ� container map�� ���� map<string, void*>�� ������ \n
 * @ref vxhelpers::VXHStringGetParameterFromCustomStringMap �� ���� ���� ���� �� �ֵ��� container�� �����ؾ� �� \n
 * NULL�� ��� ��� �� ��.
 * @return bool \n Module�� ���������� �����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks ������ ���� �۾��� ��� @ref vxengineapi::VXEEndEngineLib ���� ��� Module�� ������
 * @sa vxengineapi::VXEEndEngineLib, vxengineapi::VXEModuleExecute
*/
__vmstatic bool VXEModuleClear(int iModuleID,
	map<string, void*>* pmapCustomParamters);
/*!
 * @fn __vmstatic double vxengineapi::VXEModuleGetProgress(int iModuleID)
 * @brief ���� Module���� �۾� ���� ���� Progress�� ��ȯ
 * @param iModuleID [in] \n int \n �����ϰ��� �ϴ� Module ID
 * @return double 0.0 ~ 100.0
 * @remarks Module ���� �� code �ܰ迡�� @ref vxobjects::SVXLocalProgress �� ����� progress�� ��ȯ
 * @sa vxobjects::SVXLocalProgress
*/
__vmstatic double VXEModuleGetProgress(int iModuleID);
/*!
 * @fn __vmstatic bool vxengineapi::VXEModuleInteropCustomWork(int iModuleID, const map<string, int>* pmapObjIDs, const map<string, string>* pmapCustomParameters, const map<string, void*>* pmapCustomInformation)
 * @brief iModuleID �� Module�� customized interoperation �� �ϱ� ���� �Լ�
 * @param iModuleID [in] \n int \n �����ϰ��� �ϴ� Module ID
 * @param pmapObjIDs [in] \n map<string, int> \n 
 * Module�� ���� VXObject �鿡 ���� ID ���� ��ϵ� container map�� ���� map<string, int>�� ������ \n
 * @ref vxhelpers::VXHStringGetVXObjectFromObjectStringMap �� ���� ID�� ���� �� �ֵ��� container�� ����\n
 * NULL�� ��� ��� �� ��.
 * @param pmapCustomParameters [in] \n map<string, void*> \n 
 * Module�� ���� VXFramework format���� ���ǵ� container map�� ���� map<string, void*>�� ������ \n
 * NULL�� ��� ��� �� ��.
 * @param pmapCustomInformation [in] \n map<string, void*> \n 
 * Module�� ���� custom pointer�� ���ǵ� container map�� ���� map<string, void*>�� ������ \n
 * @ref vxhelpers::VXHStringGetCustomObjectFromPointerStringMap �� ���� �����͸� ���� �� �ֵ��� container�� ���� \n
 * NULL�� ��� ��� �� ��.
 * @return bool \n Module���� interoperation�� ���������� ����Ǹ� true, �׷��� ������ false ��ȯ
 * @remarks 
 * ������ container�� Module ������ �ƴ� Ư���ϰ� ����� container �������� parameter�� ������ ���� ���� \n
 * container�� ������ @ref vxengineapi::VXEModuleExecute �� ����
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