#include "IGTLinkClientWidget.h"
#include "ui_IGTLinkClientWidget.h"
#include "VisualOpenIGTLinkClient.h"

class IGTLinkClientWidgetPrivate : public Ui_IGTLinkClientWidget
{
	Q_DECLARE_PUBLIC(IGTLinkClientWidget);
protected:
	IGTLinkClientWidget* const q_ptr;


public:

	IGTLinkClientWidgetPrivate(IGTLinkClientWidget& object);
	int fps = 20;
	int    interval = (int)(1000.0 / fps);
	int m_igtHeadVersion = 1;
	VisualOpenIGTLinkClient* m_IGTClient = nullptr;

};

//-----------------------------------------------------------------------
IGTLinkClientWidgetPrivate::IGTLinkClientWidgetPrivate(IGTLinkClientWidget& object)
	: q_ptr(&object)
{
}
//-----------------------------------------------------------------------
IGTLinkClientWidget::IGTLinkClientWidget(QWidget* parent)
	: d_ptr(new IGTLinkClientWidgetPrivate(*this))
{
	Q_D(IGTLinkClientWidget);
	d->setupUi(this);
	d->m_IGTClient = new VisualOpenIGTLinkClient();
	QObject::connect(d->m_IGTClient, &VisualOpenIGTLinkClient::signal_log, this, &IGTLinkClientWidget::onPrintLog);

	QObject::connect(d->connectBtn, &QPushButton::clicked, this,&IGTLinkClientWidget::onConnectToServer);

}

IGTLinkClientWidget::~IGTLinkClientWidget()
{
	Q_D(IGTLinkClientWidget);
	if (d->m_IGTClient)
	{
		delete d->m_IGTClient;
		d->m_IGTClient = nullptr;
	}
	if (nullptr != d->m_IGTClient)
	{
		d->m_IGTClient->requestInterruption();
		d->m_IGTClient->quit();
		d->m_IGTClient->wait();
		d->m_IGTClient->deleteLater();
	}
}

void IGTLinkClientWidget::onPrintLog(QString logErr)
{
	Q_D(IGTLinkClientWidget);
	d->logEdit->append(logErr);
}

void IGTLinkClientWidget::onConnectToServer()
{
	Q_D(IGTLinkClientWidget);
	QString address = d->ipLEdit->text();
	QString port = d->portLEdit->text();

	d->m_IGTClient->SetDeviceAddress(address.toStdString().c_str(), port.toInt());
	d->m_IGTClient->start();
}
