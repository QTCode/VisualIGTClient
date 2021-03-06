﻿#include "VisualBrainLabClient.h"

#include "nlohmann/json.hpp"

// Qt include 
#include <QTimer>
#include <QMap>

// openigtlink include
#include "igtlOSUtil.h"
#include "igtlClientSocket.h"
#include "igtlMessageHeader.h"
#include "igtlTransformMessage.h"
#include "igtlPositionMessage.h"
#include "igtlImageMessage.h"
#include "igtlStatusMessage.h"
#include "igtlImageMetaMessage.h"
#include "igtlLabelMetaMessage.h"

#if OpenIGTLink_PROTOCOL_VERSION >= 2
#include "igtlPointMessage.h"
#include "igtlTrajectoryMessage.h"
#include "igtlStringMessage.h"
#include "igtlTrackingDataMessage.h"
#include "igtlQuaternionTrackingDataMessage.h"
#include "igtlCapabilityMessage.h"
#endif // OpenIGTLink_PROTOCOL_VERSION >= 2

class VisualBrainLabClientPrivate
{
	Q_DECLARE_PUBLIC(VisualBrainLabClient);
protected:
	VisualBrainLabClient* const q_ptr;

public:
	VisualBrainLabClientPrivate::VisualBrainLabClientPrivate(VisualBrainLabClient* parent);
	int SaveImage(igtl::ImageMessage::Pointer& msg, int i);
	int ReceiveTransform(igtl::Socket * socket, igtl::MessageHeader::Pointer& header);
	int ReceiveImage(igtl::Socket* socket, igtl::MessageHeader::Pointer& header);
	int ReceiveTRAJ(igtl::Socket* socket, igtl::MessageHeader::Pointer& header);
	int ReceiveLabelMeta(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);
	int ReceiveImageMeta(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);

//#if OpenIGTLink_PROTOCOL_VERSION >= 2
	int ReceiveTrackingData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header);

	int ReceivePoint(igtl::Socket* socket, igtl::MessageHeader::Pointer& header);

//#endif // OpenIGTLink_PROTOCOL_VERSION >= 2

	igtl::ClientSocket::Pointer m_igtSocket;

	// server info
	QString m_serverAddress = "127.0.0.1";
	int m_serverPort = 18944;

	int m_igtQueryFPS = 30;
	int m_igtQueryInterval = 0;
	QTimer* m_igtQueryTimer = nullptr;
	int m_igtSocketResult = -100;

	QTimer* m_igtConnectTimer = nullptr;
	int m_connectInterval = 5000;		// 5 sec
	int imageFileId = 0;
};

VisualBrainLabClientPrivate::VisualBrainLabClientPrivate(VisualBrainLabClient* parent)
	:q_ptr(parent)
{

}

//------------------------------------------------------------
// Function to read test image data
int VisualBrainLabClientPrivate::SaveImage(igtl::ImageMessage::Pointer& msg, int i)
{
	Q_Q(VisualBrainLabClient);
	//------------------------------------------------------------
	// Check if image index is in the range
	if (i < 0 || i >= 100)
	{
		std::cerr << "Image index is invalid." << std::endl;
		return 0;
	}

	//------------------------------------------------------------
	// Generate path to the raw image file
	char filename[128];
	sprintf(filename, "BrainLabImage%d.dcm", i + 1);
	std::cerr << "Saving " << filename << "...";

	//------------------------------------------------------------
	// Load raw data from the file
	FILE * fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		std::cerr << "File opeining error: " << filename << std::endl;
		return 0;
	}
	int fsize = msg->GetImageSize();
	size_t b = fwrite(msg->GetScalarPointer(), 1, fsize, fp);

	fclose(fp);

	std::cerr << "done." << std::endl;

	return 1;
}

int VisualBrainLabClientPrivate::ReceiveTransform(igtl::Socket * socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualBrainLabClient);

	// Create a message buffer to receive transform data
	igtl::TransformMessage::Pointer transMsg;
	transMsg = igtl::TransformMessage::New();
	transMsg->SetMessageHeader(header);
	transMsg->AllocatePack();

	// Receive transform data from the socket
	socket->Receive(transMsg->GetPackBodyPointer(), transMsg->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = transMsg->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		// Retrive the transform data
		igtl::Matrix4x4 matrix;
		transMsg->GetMatrix(matrix);

		double pos[3] = { matrix[0][3], matrix[1][3], matrix[2][3] };
		float qua[4] = { 0 };
		igtl::MatrixToQuaternion(matrix, qua);

		nlohmann::json sensorJson;
		QString qSensorName(transMsg->GetDeviceName());
		if(qSensorName.contains("ToTracker"))
			qSensorName = qSensorName.remove("ToTracker").toLower();

		QVariant poseVariant;
		std::vector<double> poseVector{ pos[0], pos[1], pos[2],
			qua[0], qua[1], qua[2], qua[3] };
		poseVariant.setValue(poseVector);

		emit q->signal_GetSensor(qSensorName, poseVariant);
		return 1;
	}

	return 0;
}

int VisualBrainLabClientPrivate::ReceiveTrackingData(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualBrainLabClient);
	std::cerr << "Receiving TDATA data type." << std::endl;

	// Create a message buffer to receive transform data
	igtl::TrackingDataMessage::Pointer trackingData;
	trackingData = igtl::TrackingDataMessage::New();
	trackingData->SetMessageHeader(header);
	trackingData->AllocatePack();

	// Receive body from the socket
	socket->Receive(trackingData->GetPackBodyPointer(), trackingData->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = trackingData->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		int nElements = trackingData->GetNumberOfTrackingDataElements();
		for (int i = 0; i < nElements; i++)
		{
			igtl::TrackingDataElement::Pointer trackingElement;
			trackingData->GetTrackingDataElement(i, trackingElement);

			igtl::Matrix4x4 matrix;
			trackingElement->GetMatrix(matrix);


			std::cerr << "========== Element #" << i << " ==========" << std::endl;
			std::cerr << " Name       : " << trackingElement->GetName() << std::endl;
			std::cerr << " Type       : " << (int)trackingElement->GetType() << std::endl;
			std::cerr << " Matrix : " << std::endl;
			igtl::PrintMatrix(matrix);
			std::cerr << "================================" << std::endl << std::endl;
		}
		return 1;
	}
	return 0;
}

int VisualBrainLabClientPrivate::ReceivePoint(igtl::Socket* socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualBrainLabClient);
	std::cerr << "Receiving POINT data type." << std::endl;

	// Create a message buffer to receive transform data
	igtl::PointMessage::Pointer pointMsg;
	pointMsg = igtl::PointMessage::New();
	pointMsg->SetMessageHeader(header);
	pointMsg->AllocatePack();

	// Receive transform data from the socket
	socket->Receive(pointMsg->GetPackBodyPointer(), pointMsg->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = pointMsg->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		int nElements = pointMsg->GetNumberOfPointElement();
		for (int i = 0; i < nElements; i++)
		{
			igtl::PointElement::Pointer pointElement;
			pointMsg->GetPointElement(i, pointElement);

			igtlUint8 rgba[4];
			pointElement->GetRGBA(rgba);

			igtlFloat32 pos[3];
			pointElement->GetPosition(pos);
			PointData pData;
			pData.index = i;
			pData.Name = pointElement->GetName();
			pData.GroupName = pointElement->GetGroupName();
			pointElement->GetRGBA(pData.Color);
			pData.Diameter = pointElement->GetRadius() * 2.0f;
			strcpy(pData.OwnerImage, pointElement->GetOwner());
			emit q->getPoint(pData);

			std::cerr << "========== Element #" << i << " ==========" << std::endl;
			std::cerr << " Name      : " << pointElement->GetName() << std::endl;
			std::cerr << " GroupName : " << pointElement->GetGroupName() << std::endl;
			std::cerr << " RGBA      : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
			std::cerr << " Position  : ( " << std::fixed << pos[0] << ", " << pos[1] << ", " << pos[2] << " )" << std::endl;
			std::cerr << " Radius    : " << std::fixed << pointElement->GetRadius() << std::endl;
			std::cerr << " Owner     : " << pointElement->GetOwner() << std::endl;
			std::cerr << "================================" << std::endl << std::endl;
		}
	}

	return 1;
}

int VisualBrainLabClientPrivate::ReceiveImage(igtl::Socket* socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualBrainLabClient);

	std::cerr << "Receiving IMAGE data type." << std::endl;

	// Create a message buffer to receive transform data
	igtl::ImageMessage::Pointer imgMsg;
	imgMsg = igtl::ImageMessage::New();
	imgMsg->SetMessageHeader(header);
	imgMsg->AllocatePack();

	// Receive transform data from the socket
	socket->Receive(imgMsg->GetPackBodyPointer(), imgMsg->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = imgMsg->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		// Retrive the image data
		int   size[3];          // image dimension
		float spacing[3];       // spacing (mm/pixel)
		int   svsize[3];        // sub-volume size
		int   svoffset[3];      // sub-volume offset
		int   scalarType;       // scalar type
		int   endian;           // endian

		scalarType = imgMsg->GetScalarType();
		endian = imgMsg->GetEndian();
		imgMsg->GetDimensions(size);
		imgMsg->GetSpacing(spacing);
		imgMsg->GetSubVolume(svsize, svoffset);


		std::cerr << "Device Name           : " << imgMsg->GetDeviceName() << std::endl;
		std::cerr << "Scalar Type           : " << scalarType << std::endl;
		std::cerr << "Endian                : " << endian << std::endl;
		std::cerr << "Dimensions            : ("
			<< size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
		std::cerr << "Spacing               : ("
			<< spacing[0] << ", " << spacing[1] << ", " << spacing[2] << ")" << std::endl;
		std::cerr << "Sub-Volume dimensions : ("
			<< svsize[0] << ", " << svsize[1] << ", " << svsize[2] << ")" << std::endl;
		std::cerr << "Sub-Volume offset     : ("
			<< svoffset[0] << ", " << svoffset[1] << ", " << svoffset[2] << ")" << std::endl << std::endl;
		SaveImage(imgMsg, imageFileId++);
		return 1;
	}
	return 0;
}

int VisualBrainLabClientPrivate::ReceiveTRAJ(igtl::Socket* socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualBrainLabClient);

	std::cerr << "Receiving TRAJ data type." << std::endl;

	// Create a message buffer to receive transform data
	igtl::TrajectoryMessage::Pointer trajMsg;
	trajMsg = igtl::TrajectoryMessage::New();
	trajMsg->SetMessageHeader(header);
	trajMsg->AllocatePack();

	// Receive transform data from the socket
	socket->Receive(trajMsg->GetPackBodyPointer(), trajMsg->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = trajMsg->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		int nElements = trajMsg->GetNumberOfTrajectoryElement();
		for (int i = 0; i < nElements; i++)
		{
			igtl::TrajectoryElement::Pointer trajElement;
			trajMsg->GetTrajectoryElement(i, trajElement);
			// Retrive the traj data
			TRAJData trajData;
			trajData.index = i;
			trajData.Name = trajElement->GetName();
			trajData.GroupName = trajElement->GetGroupName();
			trajData.Type = trajElement->GetType();
			trajData.Diameter = trajElement->GetRadius()*2.0f;
			trajElement->GetEntryPosition(trajData.EntryPoint[0], trajData.EntryPoint[1], trajData.EntryPoint[2]);
			trajElement->GetTargetPosition(trajData.TargetPoint[0], trajData.TargetPoint[1], trajData.TargetPoint[2]);
			strcpy(trajData.OwnerImage, trajElement->GetOwner());
			emit q->getTRAJ(trajData);
		}
		return 1;
	}

	return 0;

}

int VisualBrainLabClientPrivate::ReceiveLabelMeta(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualBrainLabClient);
	std::cerr << "Receiving LBMETA data type." << std::endl;

	// Create a message buffer to receive transform data
	igtl::LabelMetaMessage::Pointer lbMeta;
	lbMeta = igtl::LabelMetaMessage::New();
	lbMeta->SetMessageHeader(header);
	lbMeta->AllocatePack();

	// Receive transform data from the socket
	socket->Receive(lbMeta->GetPackBodyPointer(), lbMeta->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = lbMeta->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		int nElements = lbMeta->GetNumberOfLabelMetaElement();
		for (int i = 0; i < nElements; i++)
		{
			igtl::LabelMetaElement::Pointer lbMetaElement;
			lbMeta->GetLabelMetaElement(i, lbMetaElement);

			igtlUint8 rgba[4];
			lbMetaElement->GetRGBA(rgba);

			igtlUint16 size[3];
			lbMetaElement->GetSize(size);

			std::cerr << "========== Element #" << i << " ==========" << std::endl;
			std::cerr << " Name       : " << lbMetaElement->GetName() << std::endl;
			std::cerr << " DeviceName : " << lbMetaElement->GetDeviceName() << std::endl;
			std::cerr << " Label      : " << (int)lbMetaElement->GetLabel() << std::endl;
			std::cerr << " RGBA       : ( " << (int)rgba[0] << ", " << (int)rgba[1] << ", " << (int)rgba[2] << ", " << (int)rgba[3] << " )" << std::endl;
			std::cerr << " Size       : ( " << size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
			std::cerr << " Owner      : " << lbMetaElement->GetOwner() << std::endl;
			std::cerr << "================================" << std::endl;
			
			LBMetaData metaData;
			metaData.index = i;
			metaData.DeviceName = lbMetaElement->GetDeviceName();
			metaData.Name = lbMetaElement->GetName();
			metaData.Owner = lbMetaElement->GetOwner();
			emit q->getLBMeta(metaData);
		}
		return 1;
	}

	return 0;

}

int VisualBrainLabClientPrivate::ReceiveImageMeta(igtl::ClientSocket::Pointer& socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualBrainLabClient);
	std::cerr << "Receiving IMGMETA data type." << std::endl;

	// Create a message buffer to receive transform data
	igtl::ImageMetaMessage::Pointer imgMeta;
	imgMeta = igtl::ImageMetaMessage::New();
	imgMeta->SetMessageHeader(header);
	imgMeta->AllocatePack();

	// Receive transform data from the socket
	socket->Receive(imgMeta->GetPackBodyPointer(), imgMeta->GetPackBodySize());

	// Deserialize the transform data
	// If you want to skip CRC check, call Unpack() without argument.
	int c = imgMeta->Unpack(1);

	if (c & igtl::MessageHeader::UNPACK_BODY) // if CRC check is OK
	{
		int nElements = imgMeta->GetNumberOfImageMetaElement();
		for (int i = 0; i < nElements; i++)
		{
			igtl::ImageMetaElement::Pointer imgMetaElement;
			imgMeta->GetImageMetaElement(i, imgMetaElement);
			igtlUint16 size[3];
			imgMetaElement->GetSize(size);

			igtl::TimeStamp::Pointer ts;
			imgMetaElement->GetTimeStamp(ts);
			double time = ts->GetTimeStamp();

			std::cerr << "========== Element #" << i << " ==========" << std::endl;
			std::cerr << " Name       : " << imgMetaElement->GetName() << std::endl;
			std::cerr << " DeviceName : " << imgMetaElement->GetDeviceName() << std::endl;
			std::cerr << " Modality   : " << imgMetaElement->GetModality() << std::endl;
			std::cerr << " PatientName: " << imgMetaElement->GetPatientName() << std::endl;
			std::cerr << " PatientID  : " << imgMetaElement->GetPatientID() << std::endl;
			std::cerr << " TimeStamp  : " << std::fixed << time << std::endl;
			std::cerr << " Size       : ( " << size[0] << ", " << size[1] << ", " << size[2] << ")" << std::endl;
			std::cerr << " ScalarType : " << (int)imgMetaElement->GetScalarType() << std::endl;
			std::cerr << "================================" << std::endl;

			IMGMetaData metaData;
			metaData.index = i;
			metaData.DeviceName = imgMetaElement->GetDeviceName();
			metaData.Name = imgMetaElement->GetName();
			metaData.Modality = imgMetaElement->GetModality();
			metaData.PatientID = imgMetaElement->GetPatientID();
			metaData.PatientName = imgMetaElement->GetPatientName();

			//更新tableWidget 显示
			emit q->getIMGMeta(metaData);
		 

		}
		return 1;
	}

	return 0;

}

VisualBrainLabClient::VisualBrainLabClient(QObject* parent)
	:QThread(parent), d_ptr(new VisualBrainLabClientPrivate(this))
{
	Q_D(VisualBrainLabClient);
	qRegisterMetaType<IMGMetaData>("IMGMetaData");
	qRegisterMetaType<LBMetaData>("LBMetaData");
	qRegisterMetaType<TRAJData>("TRAJData");
	qRegisterMetaType<PointData>("PointData");

    
	d->m_igtSocket = igtl::ClientSocket::New();
	d->m_igtQueryInterval = 1000 / d->m_igtQueryFPS;
}

VisualBrainLabClient::~VisualBrainLabClient()
{
	Q_D(VisualBrainLabClient);

}

void VisualBrainLabClient::SetDeviceAddress(QString address /*= QString("127.0.0.1")*/, int port /*= 18944*/)
{
	Q_D(VisualBrainLabClient);

	d->m_serverAddress = address;
	d->m_serverPort = port;
}

void VisualBrainLabClient::QueryTrackingData()
{
	Q_D(VisualBrainLabClient);
	emit logErr("start QueryTrackData");
	// Send request data
	igtl::StartTrackingDataMessage::Pointer startTrackingDataMsg;
	startTrackingDataMsg = igtl::StartTrackingDataMessage::New();
	startTrackingDataMsg->SetDeviceName("");
	d->m_igtSocket->Send(startTrackingDataMsg->GetPackPointer(), startTrackingDataMsg->GetPackSize());
	emit logErr(QString::fromStdString(startTrackingDataMsg->GetDeviceType()));

}

void VisualBrainLabClient::StopQueryTrackData()
{
	Q_D(VisualBrainLabClient);
	emit logErr("StopQueryTrackData");
	// Send request data
	igtl::StopTrackingDataMessage::Pointer stopTrackingDataMsg;
	stopTrackingDataMsg = igtl::StopTrackingDataMessage::New();
	stopTrackingDataMsg->SetDeviceName("");
	d->m_igtSocket->Send(stopTrackingDataMsg->GetPackPointer(), stopTrackingDataMsg->GetPackSize());
	emit logErr(QString::fromStdString(stopTrackingDataMsg->GetDeviceType()));
}

void VisualBrainLabClient::QueryImages()
{
	Q_D(VisualBrainLabClient);
	emit logErr("QueryImages");
	igtl::ImageMessage::Pointer imageMsg;
	imageMsg = igtl::ImageMessage::New();
	imageMsg->SetDeviceName("");
	d->m_igtSocket->Send(imageMsg->GetPackPointer(), imageMsg->GetPackSize());
}

void VisualBrainLabClient::QueryImage(std::string imageID)
{
	Q_D(VisualBrainLabClient);
	emit logErr("QueryImage");
	igtl::GetImageMessage::Pointer getImageMsg;
	getImageMsg = igtl::GetImageMessage::New();
	getImageMsg->SetDeviceName(imageID);
	d->m_igtSocket->Send(getImageMsg->GetPackPointer(), getImageMsg->GetPackSize());
}

void VisualBrainLabClient::QueryMetadata(int id)
{
	Q_D(VisualBrainLabClient);
	if (id == OpenIGTLinkQueryType::TYPE_IMAGE)
	{
		emit logErr("QueryImageMate");
		// Send request data
		igtl::GetImageMetaMessage::Pointer getImageMetaMsg;
		getImageMetaMsg = igtl::GetImageMetaMessage::New();
		getImageMetaMsg->SetDeviceName("");
		getImageMetaMsg->Pack();
		d->m_igtSocket->Send(getImageMetaMsg->GetPackPointer(), getImageMetaMsg->GetPackSize());
	}
	else if(id == OpenIGTLinkQueryType::TYPE_LABEL)
	{
		igtl::GetLabelMetaMessage::Pointer getLableMetaMsg;

		getLableMetaMsg = igtl::GetLabelMetaMessage::New();
		getLableMetaMsg->SetDeviceName("");
		getLableMetaMsg->Pack();
		d->m_igtSocket->Send(getLableMetaMsg->GetPackPointer(), getLableMetaMsg->GetPackSize());
	}
	else if (id == OpenIGTLinkQueryType::TYPE_POINT)
	{
		igtl::GetPointMessage::Pointer getPointMsg;
		getPointMsg = igtl::GetPointMessage::New();
		getPointMsg->Pack();
		d->m_igtSocket->Send(getPointMsg->GetPackPointer(), getPointMsg->GetPackSize());
	}
	else if (id == OpenIGTLinkQueryType::TYPE_TRAJ)
	{
		igtl::GetTrajectoryMessage::Pointer getTRAJMsg;
		getTRAJMsg = igtl::GetTrajectoryMessage::New();
		getTRAJMsg->Pack();
		d->m_igtSocket->Send(getTRAJMsg->GetPackPointer(), getTRAJMsg->GetPackSize());

	}
	else if (id == OpenIGTLinkQueryType::TYPE_CAPABIL)
	{
		igtl::CapabilityMessage::Pointer capabilityMsg;
		capabilityMsg = igtl::CapabilityMessage::New();
		capabilityMsg->SetDeviceName("Device");
		capabilityMsg->Pack();
		d->m_igtSocket->Send(capabilityMsg->GetPackPointer(), capabilityMsg->GetPackSize());
	}
	else if (id == OpenIGTLinkQueryType::TYPE_COLOR)
	{
		//igtl::GetColor
	}
}

void VisualBrainLabClient::onQueryOpenigtLinkServer()
{
	Q_D(VisualBrainLabClient);

	emit signal_QueryOpenIGTLink();
	for (int i = 0; i < d->m_igtQueryFPS; i++)
	{
		igtl::MessageHeader::Pointer headerMsg = igtl::MessageHeader::New();
		igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();

		headerMsg->InitPack();
		int r = d->m_igtSocket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
		if (r == 0)
		{
			d->m_igtSocket->CloseSocket();
			return;
		}
		if (r != headerMsg->GetPackSize())
			return;

		// Deserialize the header
		headerMsg->Unpack();

		// Get time stamp
		igtlUint32 sec;
		igtlUint32 nanosec;

		headerMsg->GetTimeStamp(ts);
		ts->GetTimeStamp(&sec, &nanosec);

		// Check data type and receive data body
		if (strcmp(headerMsg->GetDeviceType(), "TRANSFORM") == 0)
		{
			d->ReceiveTransform(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "TDATA") == 0)
		{
			d->ReceiveTrackingData(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "IMGMETA") == 0)
		{

		}
		else if (strcmp(headerMsg->GetDeviceType(), "LBMETA") == 0)
		{

		}
		else if (strcmp(headerMsg->GetDeviceType(), "POINT") == 0)
		{

		}

		else
		{
			std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
			d->m_igtSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
		}
	}
}

void VisualBrainLabClient::run()
{
	Q_D(VisualBrainLabClient);

	//while (!QThread::isInterruptionRequested())
	while (!QThread::currentThread()->isInterruptionRequested())
	{
		//std::cout << "VisualBrainLabClient Work In Thread : " << QThread::currentThread() << std::endl;
		//QThread::sleep(1);

		if (!d->m_serverAddress.isEmpty() && 0 != d->m_serverPort)
		{
			if (0 != d->m_igtSocketResult)
			{
				// try to connect device
				if (0 != ConnectDevice())
				{ 
					QThread::msleep(d->m_connectInterval);
				}
			}
			else
			{
				// try to query device
				QueryDevice();
				QThread::msleep(d->m_igtQueryInterval);
			}
		}
		else
		{
			std::cout << "Device Server Address Is Invalid !" << std::endl;
			std::cout << "Wait For Set Valid Addres !" << std::endl;
			QThread::msleep(d->m_igtQueryInterval);
		}
	}

	exec();
}

int VisualBrainLabClient::ConnectDevice()
{
	Q_D(VisualBrainLabClient);

	d->m_igtSocketResult = d->m_igtSocket->ConnectToServer(d->m_serverAddress.toLocal8Bit(), d->m_serverPort);
	if (d->m_igtSocketResult != 0)
	{
		emit logErr(QString("Can't Connect To Device Server! Wait for ") + QString::number(d->m_connectInterval / 1000) + QString("s"));
	}
	else
	{
		emit logErr(QString("Connection to the server was successful"));
	}
	return d->m_igtSocketResult;
}

void VisualBrainLabClient::QueryDevice()
{
	Q_D(VisualBrainLabClient);

	//emit signal_QueryOpenIGTLink();
	for (int i = 0; i < d->m_igtQueryFPS; i++)
	{
		igtl::MessageHeader::Pointer headerMsg = igtl::MessageHeader::New();
		igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();

		headerMsg->InitPack();
		int r = d->m_igtSocket->Receive(headerMsg->GetPackPointer(), headerMsg->GetPackSize());
		if (r == 0)
		{
			d->m_igtSocket->CloseSocket();
			return;
		}
		if (r != headerMsg->GetPackSize())
			return;

		// Deserialize the header
		headerMsg->Unpack();

		// Get time stamp
		igtlUint32 sec;
		igtlUint32 nanosec;

		headerMsg->GetTimeStamp(ts);
		ts->GetTimeStamp(&sec, &nanosec);

		// Check data type and receive data body
		if (strcmp(headerMsg->GetDeviceType(), "TRANSFORM") == 0)
		{
			d->ReceiveTransform(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "TDATA") == 0)
		{
			d->ReceiveTrackingData(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "IMAGE") == 0)
		{
			d->ReceiveImage(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "IMGMETA") == 0)
		{
			d->ReceiveImageMeta(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "LBMETA") == 0)
		{
			d->ReceiveLabelMeta(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "POINT") == 0)
		{
			d->ReceivePoint(d->m_igtSocket, headerMsg);
		}
		else if (strcmp(headerMsg->GetDeviceType(), "CAPABIL") == 0)
		{

		}
		else if (strcmp(headerMsg->GetDeviceType(), "TRAJ") == 0)
		{
			d->ReceiveTRAJ(d->m_igtSocket, headerMsg);
		}
		else
		{
			std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
			d->m_igtSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
		}
	}
}
