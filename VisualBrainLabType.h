#pragma once
#include <iostream>
#include <string>


typedef unsigned char BLBUint8;
typedef char BLBInt8;

typedef struct IMGMetaData
{
	int index;
	std::string DeviceName;
	std::string Name;
	std::string PatientID;
	std::string PatientName;
	std::string Modality;
	std::string Timess;
}IMGMetaData;

typedef struct LBMetaData
{
	int index;
	std::string DeviceName;
	std::string Name;
	std::string Owner;
}LBMetaData;

typedef struct TRAJData
{
	int index;
	std::string Name;
	std::string GroupName;
	BLBUint8 Type;//1:entry,2:target,3:entry&target
	BLBUint8 Color[4] = {0};
	float EntryPoint[3] = { 0 };
	float TargetPoint[3] = { 0 };
	float Diameter = { 0 };//can be 0
	char OwnerImage[20] = { '0' };
}TRAJData;

typedef struct PointData
{
	int index;
	std::string Name;
	std::string GroupName;
	BLBUint8 Color[4] = { 0 };
	float CoordinatePoint[3] = { 0 };
	float Diameter = { 0 };//can be 0
	char OwnerImage[20] = { '0' };
}PointData;