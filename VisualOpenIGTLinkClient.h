#pragma once
#include <QObject>
#include <QThread>
#include <QVariant>
#include <QMetaType>

Q_DECLARE_METATYPE(std::vector<double>);

class VisualOpenIGTLinkClientPrivate;
class  VisualOpenIGTLinkClient : public QThread
{
    Q_OBJECT
public:
	VisualOpenIGTLinkClient(QObject* parent = Q_NULLPTR);
	~VisualOpenIGTLinkClient();

	void ConnectToServer(QString address = QString("127.0.0.1"), int port = 18944);
	void SetDeviceAddress(QString address = QString("127.0.0.1"), int port = 18944);
	
signals:
	void signal_GetSensor(QString sname, QVariant spose);
	void signal_QueryOpenIGTLink();
	void signal_log(QString logText);

protected slots:
	void onQueryOpenigtLinkServer();
	void onConnectIGTServer();

protected:
	QScopedPointer<VisualOpenIGTLinkClientPrivate> d_ptr;
	void run();
	int ConnectDevice();
	void QueryDevice();

private:
	Q_DECLARE_PRIVATE(VisualOpenIGTLinkClient);
	Q_DISABLE_COPY(VisualOpenIGTLinkClient);
};