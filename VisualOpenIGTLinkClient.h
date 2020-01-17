#pragma once
#include <QObject>
#include <QThread>
#include <QVariant>
#include <QMetaType>

Q_DECLARE_METATYPE(std::vector<double>);

enum OpenIGTLinkQueryType
{
	TYPE_IMAGE,
	TYPE_LABEL,
	TYPE_POINT,
	//TYPE_GET_IMAGE
};

class VisualOpenIGTLinkClientPrivate;
class  VisualOpenIGTLinkClient : public QThread
{
    Q_OBJECT
public:
	VisualOpenIGTLinkClient(QObject* parent = Q_NULLPTR);
	~VisualOpenIGTLinkClient();

	void SetDeviceAddress(QString address = QString("127.0.0.1"), int port = 18944);

	void QueryTrackingData();
	void StopQueryTrackData();

	void QueryImages();
	void QueryImage(QString imageID);

	void QueryMetadata(OpenIGTLinkQueryType type);
	
	
signals:
	void signal_GetSensor(QString sname, QVariant spose);
	void signal_QueryOpenIGTLink();
	void signal_log(QString logText);

protected slots:
	void onQueryOpenigtLinkServer();

protected:
	QScopedPointer<VisualOpenIGTLinkClientPrivate> d_ptr;
	void run();
	int ConnectDevice();
	void QueryDevice();



private:
	Q_DECLARE_PRIVATE(VisualOpenIGTLinkClient);
	Q_DISABLE_COPY(VisualOpenIGTLinkClient);
};