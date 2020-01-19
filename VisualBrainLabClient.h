#pragma once
#include "VisualBrainLabType.h"

#include <QObject>
#include <QThread>
#include <QVariant>
#include <QMetaType>


Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(IMGMetaData);
Q_DECLARE_METATYPE(LBMetaData);
Q_DECLARE_METATYPE(TRAJData);


enum OpenIGTLinkQueryType
{
	TYPE_IMAGE,
	TYPE_LABEL,
	TYPE_POINT,
	TYPE_TRAJ,
	TYPE_CAPABIL,
	TYPE_COLOR,
	TYPE_DEFAULT


	//TYPE_GET_IMAGE
};

class VisualBrainLabClientPrivate;
class  VisualBrainLabClient : public QThread
{
    Q_OBJECT
public:
	VisualBrainLabClient(QObject* parent = Q_NULLPTR);
	~VisualBrainLabClient();

	void SetDeviceAddress(QString address = QString("127.0.0.1"), int port = 18944);

	void QueryTrackingData();
	void StopQueryTrackData();

	void QueryImages();
	void QueryImage(std::string imageID);

	void QueryMetadata(int id);
	
	
signals:
	void signal_GetSensor(QString sname, QVariant spose);
	void signal_QueryOpenIGTLink();
	void logErr(QString logText);
	void getIMGMeta(IMGMetaData);
	void getLBMeta(LBMetaData);
	void getTRAJ(TRAJData);
	void getPoint(PointData);
protected slots:
	void onQueryOpenigtLinkServer();

protected:
	QScopedPointer<VisualBrainLabClientPrivate> d_ptr;
	void run();
	int ConnectDevice();
	void QueryDevice();



private:
	Q_DECLARE_PRIVATE(VisualBrainLabClient);
	Q_DISABLE_COPY(VisualBrainLabClient);
};