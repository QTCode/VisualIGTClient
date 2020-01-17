#include "VisualOpenIGTLinkClient.h"

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

#if OpenIGTLink_PROTOCOL_VERSION >= 2
#include "igtlPointMessage.h"
#include "igtlTrajectoryMessage.h"
#include "igtlStringMessage.h"
#include "igtlTrackingDataMessage.h"
#include "igtlQuaternionTrackingDataMessage.h"
#include "igtlCapabilityMessage.h"
#endif // OpenIGTLink_PROTOCOL_VERSION >= 2

class VisualOpenIGTLinkClientPrivate
{
	Q_DECLARE_PUBLIC(VisualOpenIGTLinkClient);
protected:
	VisualOpenIGTLinkClient* const q_ptr;

public:
	VisualOpenIGTLinkClientPrivate::VisualOpenIGTLinkClientPrivate(VisualOpenIGTLinkClient* parent);
	int ReceiveTransform(igtl::Socket * socket, igtl::MessageHeader::Pointer& header);

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
};

VisualOpenIGTLinkClientPrivate::VisualOpenIGTLinkClientPrivate(VisualOpenIGTLinkClient* parent)
	:q_ptr(parent)
{

}

int VisualOpenIGTLinkClientPrivate::ReceiveTransform(igtl::Socket * socket, igtl::MessageHeader::Pointer& header)
{
	Q_Q(VisualOpenIGTLinkClient);

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

VisualOpenIGTLinkClient::VisualOpenIGTLinkClient(QObject* parent)
	:QThread(parent), d_ptr(new VisualOpenIGTLinkClientPrivate(this))
{
	Q_D(VisualOpenIGTLinkClient);
    
	d->m_igtSocket = igtl::ClientSocket::New();
	d->m_igtQueryInterval = 1000 / d->m_igtQueryFPS;
}

VisualOpenIGTLinkClient::~VisualOpenIGTLinkClient()
{
	Q_D(VisualOpenIGTLinkClient);

}

void VisualOpenIGTLinkClient::ConnectToServer(QString address, int port)
{
	Q_D(VisualOpenIGTLinkClient);

	d->m_serverAddress = address;
	d->m_serverPort = port;

	if (!d->m_serverAddress.isEmpty() && 0 != d->m_serverPort)
	{
		// 首先尝试连接一次IGTServer，如果没成功，则开始进行计时器，每隔一段时间尝试连接一次
		onConnectIGTServer();
		if (0 != d->m_igtSocketResult)
		{
			if (nullptr != d->m_igtConnectTimer)
			{
				if (d->m_igtSocket->GetConnected())
					d->m_igtSocket->CloseSocket();

				d->m_igtConnectTimer->stop();
				QObject::disconnect(d->m_igtConnectTimer, &QTimer::timeout, this, &VisualOpenIGTLinkClient::onConnectIGTServer);
				delete d->m_igtConnectTimer;
			}

			d->m_igtConnectTimer = new QTimer(this);
			d->m_igtConnectTimer->setInterval(d->m_connectInterval);
			QObject::connect(d->m_igtConnectTimer, &QTimer::timeout, this, &VisualOpenIGTLinkClient::onConnectIGTServer);
			d->m_igtConnectTimer->start();
		}
	}
}

void VisualOpenIGTLinkClient::SetDeviceAddress(QString address /*= QString("127.0.0.1")*/, int port /*= 18944*/)
{
	Q_D(VisualOpenIGTLinkClient);

	d->m_serverAddress = address;
	d->m_serverPort = port;
}

void VisualOpenIGTLinkClient::onQueryOpenigtLinkServer()
{
	Q_D(VisualOpenIGTLinkClient);

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
		else
		{
			std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
			d->m_igtSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
		}
	}
}

void VisualOpenIGTLinkClient::onConnectIGTServer()
{
	Q_D(VisualOpenIGTLinkClient);

	d->m_igtSocketResult = d->m_igtSocket->ConnectToServer(d->m_serverAddress.toLocal8Bit(), d->m_serverPort);
}

void VisualOpenIGTLinkClient::run()
{
	Q_D(VisualOpenIGTLinkClient);

	//while (!QThread::isInterruptionRequested())
	while (!QThread::currentThread()->isInterruptionRequested())
	{
		//std::cout << "VisualOpenIGTLinkClient Work In Thread : " << QThread::currentThread() << std::endl;
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

int VisualOpenIGTLinkClient::ConnectDevice()
{
	Q_D(VisualOpenIGTLinkClient);

	d->m_igtSocketResult = d->m_igtSocket->ConnectToServer(d->m_serverAddress.toLocal8Bit(), d->m_serverPort);
	if (d->m_igtSocketResult != 0)
	{
		emit signal_log(QString("Can't Connect To Device Server! Wait for ") + QString::number(d->m_connectInterval / 1000) + QString("s"));
	}
	else
	{
		emit signal_log(QString("Connection to the server was successful"));
	}
	return d->m_igtSocketResult;
}

void VisualOpenIGTLinkClient::QueryDevice()
{
	Q_D(VisualOpenIGTLinkClient);

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
		else
		{
			std::cerr << "Receiving : " << headerMsg->GetDeviceType() << std::endl;
			d->m_igtSocket->Skip(headerMsg->GetBodySizeToRead(), 0);
		}
	}
}
