#include "IGTLinkClientWidget.h"
#include "ui_IGTLinkClientWidget.h"

//igt
#include "igtlOSUtil.h"
#include "igtlMessageHeader.h"
#include "igtlTransformMessage.h"
#include "igtlPositionMessage.h"
#include "igtlImageMessage.h"
#include "igtlClientSocket.h"
#include "igtlStatusMessage.h"

class IGTLinkClientWidgetPrivate : public Ui_IGTLinkClientWidget
{
	Q_DECLARE_PUBLIC(IGTLinkClientWidget);
protected:
	IGTLinkClientWidget* const q_ptr;


public:

	IGTLinkClientWidgetPrivate(IGTLinkClientWidget& object);
	int fps = 20;
	int    interval = (int)(1000.0 / fps);
	igtl::ClientSocket::Pointer m_socket;
	int m_igtHeadVersion = 1;

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
	d->m_socket = igtl::ClientSocket::New();
	QObject::connect(d->connectBtn, &QPushButton::clicked, this,&IGTLinkClientWidget::ConnectToServer);
}

IGTLinkClientWidget::~IGTLinkClientWidget()
{

}

void IGTLinkClientWidget::ConnectToServer()
{
	Q_D(IGTLinkClientWidget);
	QString address = d->ipLEdit->text();
	QString port = d->portLEdit->text();


	int r = d->m_socket->ConnectToServer(address.toStdString().c_str(), port.toInt());
	if (r != 0)
	{
		d->logEdit->append(QString("Cannot connect to the server."));
	}
	else
	{
		d->logEdit->append(QString("connect to the server:") + address);
	}

}
