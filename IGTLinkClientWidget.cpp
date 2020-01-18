#include "IGTLinkClientWidget.h"
#include "ui_IGTLinkClientWidget.h"
#include "VisualOpenIGTLinkClient.h"

#include <QButtonGroup>
#include <QRadioButton>
#include <QDebug>

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
	QButtonGroup m_TypeButtonGroup;

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

	d->m_TypeButtonGroup.addButton(d->typeImageRBtn, OpenIGTLinkQueryType::TYPE_IMAGE);
	d->m_TypeButtonGroup.addButton(d->typeLabelRBtn, OpenIGTLinkQueryType::TYPE_LABEL);
	d->m_TypeButtonGroup.addButton(d->typePointRBtn, OpenIGTLinkQueryType::TYPE_POINT);

	QObject::connect(&d->m_TypeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onQueryTypeChanged(int)));
	QObject::connect(d->connectBtn, &QPushButton::clicked, this,&IGTLinkClientWidget::onConnectToServer);
	QObject::connect(d->updateBtn, &QPushButton::clicked, this, &IGTLinkClientWidget::onQueryRemoteList);

	d->m_IGTClient = new VisualOpenIGTLinkClient();
	QObject::connect(d->m_IGTClient, &VisualOpenIGTLinkClient::signal_log, this, &IGTLinkClientWidget::onPrintLog);
	QObject::connect(d->m_IGTClient, &VisualOpenIGTLinkClient::getIMGMeta, this, &IGTLinkClientWidget::onUpdateIMGMetaTabWidget);
	QObject::connect(d->m_IGTClient, &VisualOpenIGTLinkClient::getLBMeta, this, &IGTLinkClientWidget::onUpdateLBMetatabWidget);
}

IGTLinkClientWidget::~IGTLinkClientWidget()
{
	Q_D(IGTLinkClientWidget);
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

void IGTLinkClientWidget::onQueryRemoteList()
{
	Q_D(IGTLinkClientWidget);
	d->m_IGTClient->QueryMetadata(d->m_TypeButtonGroup.checkedId());
}

void IGTLinkClientWidget::onConnectToServer()
{
	Q_D(IGTLinkClientWidget);
	QString address = d->ipLEdit->text();
	QString port = d->portLEdit->text();

	d->m_IGTClient->SetDeviceAddress(address.toStdString().c_str(), port.toInt());
	d->m_IGTClient->start();
}

//------------------------------------------------------------------------------
void IGTLinkClientWidget::onQueryTypeChanged(int id)
{
	Q_D(IGTLinkClientWidget);
	qDebug() << "onQueryTypeChanged:" << id;
	d->tableWidget->clearContents();
	d->tableWidget->setRowCount(0);
	QStringList list;
	switch (id)
	{
	case OpenIGTLinkQueryType::TYPE_IMAGE:
		list << QObject::tr("Image ID") << QObject::tr("Image Name")
			<< QObject::tr("Patient ID") << QObject::tr("Patient Name")
			<< QObject::tr("Modality") << QObject::tr("Time");
		break;
	case OpenIGTLinkQueryType::TYPE_LABEL:
		list << QObject::tr("Image ID") << QObject::tr("Image Name")
			<< QObject::tr("Owner Image") << QObject::tr("")
			<< QObject::tr("");
		break;
	case OpenIGTLinkQueryType::TYPE_POINT:
		list << QObject::tr("Group ID") << QObject::tr("")
			<< QObject::tr("") << QObject::tr("")
			<< QObject::tr("");
		break;
	}
	d->tableWidget->setColumnCount(list.size());
	d->tableWidget->setHorizontalHeaderLabels(list);
}

void IGTLinkClientWidget::onGetMetaItem()
{
	Q_D(IGTLinkClientWidget);

	QList<QTableWidgetSelectionRange> selectRange(d->tableWidget->selectedRanges());
	for (int selectionIndex = 0; selectionIndex < selectRange.size(); selectionIndex++)
	{
		int topRowIndex = selectRange.at(selectionIndex).topRow();
		// Get the item identifier from the table
		QTableWidgetItem* selectedItem = d->tableWidget->item(topRowIndex, 0);
		if (!selectedItem)
		{
			qCritical() << Q_FUNC_INFO << " failed: selected item is empty";
			continue;
		}
		std::string dataId(selectedItem->text().toLatin1());
		switch (d->m_TypeButtonGroup.checkedId())
		{
		case OpenIGTLinkQueryType::TYPE_IMAGE:
		case OpenIGTLinkQueryType::TYPE_LABEL:
			d->m_IGTClient->QueryImage(dataId);
			break;
		case OpenIGTLinkQueryType::TYPE_POINT:
			//this->getPointList(dataId);
			break;
		default:
			qCritical() << Q_FUNC_INFO << " failed: unknown item type selected";
		}
	}
}

void IGTLinkClientWidget::onUpdateIMGMetaTabWidget(IMGMetaData metaData)
{
	Q_D(IGTLinkClientWidget);
	d->tableWidget->setRowCount(metaData.index + 1);
	qDebug() <<"onUpdateIMGMetaTabWidget:"<< metaData.DeviceName.c_str();
	QTableWidgetItem* deviceItem = new QTableWidgetItem(metaData.DeviceName.c_str());
	QTableWidgetItem* nameItem = new QTableWidgetItem(metaData.Name.c_str());
	QTableWidgetItem* patientIdItem = new QTableWidgetItem(metaData.PatientID.c_str());
	QTableWidgetItem* patientNameItem = new QTableWidgetItem(metaData.PatientName.c_str());
	QTableWidgetItem* modalityItem = new QTableWidgetItem(metaData.Modality.c_str());
	QTableWidgetItem* timeItem = new QTableWidgetItem(metaData.Timess.c_str());

	d->tableWidget->setItem(metaData.index, 0, deviceItem);
	d->tableWidget->setItem(metaData.index, 1, nameItem);
	d->tableWidget->setItem(metaData.index, 2, patientIdItem);
	d->tableWidget->setItem(metaData.index, 3, patientNameItem);
	d->tableWidget->setItem(metaData.index, 4, modalityItem);
	d->tableWidget->setItem(metaData.index, 5, timeItem);
}

void IGTLinkClientWidget::onUpdateLBMetatabWidget(LBMetaData metaData)
{
	Q_D(IGTLinkClientWidget);
	qDebug() << "onUpdateLBMetatabWidget:" << metaData.DeviceName.c_str();
	d->tableWidget->setRowCount(metaData.index + 1);
	QTableWidgetItem* deviceItem = new QTableWidgetItem(metaData.DeviceName.c_str());
	QTableWidgetItem* nameItem = new QTableWidgetItem(metaData.Name.c_str());
	QTableWidgetItem* ownerItem = new QTableWidgetItem(metaData.Owner.c_str());

	d->tableWidget->setItem(metaData.index, 0, deviceItem);
	d->tableWidget->setItem(metaData.index, 1, nameItem);
	d->tableWidget->setItem(metaData.index, 2, ownerItem);
}