#include "VisualBrainLabClientWidget.h"
#include "ui_VisualBrainLabClientWidget.h"
#include "VisualBrainLabClient.h"

#include <QButtonGroup>
#include <QRadioButton>
#include <QDebug>

class VisualBrainLabClientWidgetPrivate : public Ui_VisualBrainLabClientWidget
{
	Q_DECLARE_PUBLIC(VisualBrainLabClientWidget);
protected:
	VisualBrainLabClientWidget* const q_ptr;


public:

	VisualBrainLabClientWidgetPrivate(VisualBrainLabClientWidget& object);
	int fps = 20;
	int    interval = (int)(1000.0 / fps);
	int m_igtHeadVersion = 1;
	VisualBrainLabClient* m_brainLabClient = nullptr;
	QButtonGroup m_TypeButtonGroup;

};

//-----------------------------------------------------------------------
VisualBrainLabClientWidgetPrivate::VisualBrainLabClientWidgetPrivate(VisualBrainLabClientWidget& object)
	: q_ptr(&object)
{
}

//-----------------------------------------------------------------------
VisualBrainLabClientWidget::VisualBrainLabClientWidget(QWidget* parent)
	: d_ptr(new VisualBrainLabClientWidgetPrivate(*this))
{
	Q_D(VisualBrainLabClientWidget);
	d->setupUi(this);
	setStyleSheet("venus--TitleBar {background-color: rgb(0,0,0);color: rgb(255,255,255);}");
	//this->setStyleSheet()
	//this->setWindowOpacity(1);
	//this->setWindowFlags(Qt::FramelessWindowHint);
	//this->setAttribute(Qt::WA_TranslucentBackground);
	d->m_TypeButtonGroup.addButton(d->typeImageRBtn, OpenIGTLinkQueryType::TYPE_IMAGE);
	d->m_TypeButtonGroup.addButton(d->typeLabelRBtn, OpenIGTLinkQueryType::TYPE_LABEL);
	d->m_TypeButtonGroup.addButton(d->typePointRBtn, OpenIGTLinkQueryType::TYPE_POINT);
	d->m_TypeButtonGroup.addButton(d->typeTRAJRBtn, OpenIGTLinkQueryType::TYPE_TRAJ);
	d->m_TypeButtonGroup.addButton(d->typeCAPRBtn, OpenIGTLinkQueryType::TYPE_CAPABIL);
	d->m_TypeButtonGroup.addButton(d->typeColorRBtn, OpenIGTLinkQueryType::TYPE_COLOR);

	QObject::connect(&d->m_TypeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onQueryTypeChanged(int)));
	QObject::connect(d->connectBtn, &QPushButton::clicked, this,&VisualBrainLabClientWidget::onConnectToServer);
	QObject::connect(d->updateBtn, &QPushButton::clicked, this, &VisualBrainLabClientWidget::onQueryRemoteList);
	QObject::connect(d->getSelectedItemBtn, &QPushButton::clicked, this, &VisualBrainLabClientWidget::onGetMetaItem);

	d->m_brainLabClient = new VisualBrainLabClient();
	QObject::connect(d->m_brainLabClient, &VisualBrainLabClient::logErr, this, &VisualBrainLabClientWidget::onPrintLog);
	QObject::connect(this, &VisualBrainLabClientWidget::logErr, this, &VisualBrainLabClientWidget::onPrintLog);
	QObject::connect(d->m_brainLabClient, &VisualBrainLabClient::getIMGMeta, this, &VisualBrainLabClientWidget::onUpdateIMGMetaTabWidget);
	QObject::connect(d->m_brainLabClient, &VisualBrainLabClient::getLBMeta, this, &VisualBrainLabClientWidget::onUpdateLBMetaTabWidget);
	QObject::connect(d->m_brainLabClient, &VisualBrainLabClient::getTRAJ, this, &VisualBrainLabClientWidget::onUpdateTRAJDataTabWidget);
}

VisualBrainLabClientWidget::~VisualBrainLabClientWidget()
{
	Q_D(VisualBrainLabClientWidget);
	if (nullptr != d->m_brainLabClient)
	{
		d->m_brainLabClient->requestInterruption();
		d->m_brainLabClient->quit();
		d->m_brainLabClient->wait();
		d->m_brainLabClient->deleteLater();
	}
}

void VisualBrainLabClientWidget::onPrintLog(QString logErr)
{
	Q_D(VisualBrainLabClientWidget);
	d->logEdit->append(logErr);
}

void VisualBrainLabClientWidget::onQueryRemoteList()
{
	Q_D(VisualBrainLabClientWidget);
	d->m_brainLabClient->QueryMetadata(d->m_TypeButtonGroup.checkedId());
}

void VisualBrainLabClientWidget::onConnectToServer()
{
	Q_D(VisualBrainLabClientWidget);
	QString address = d->ipLEdit->text();
	QString port = d->portLEdit->text();

	d->m_brainLabClient->SetDeviceAddress(address.toStdString().c_str(), port.toInt());
	d->m_brainLabClient->start();
}

//------------------------------------------------------------------------------
void VisualBrainLabClientWidget::onQueryTypeChanged(int id)
{
	Q_D(VisualBrainLabClientWidget);
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
	case OpenIGTLinkQueryType::TYPE_TRAJ:
		list << QObject::tr("Name") << QObject::tr("Group Name")
			<< QObject::tr("Type") << QObject::tr("Entry")
			<< QObject::tr("Target");
	}
	d->tableWidget->setColumnCount(list.size());
	d->tableWidget->setHorizontalHeaderLabels(list);
	d->tableWidget->verticalHeader()->hide();//隐藏行号方法 
	d->tableWidget->setStyleSheet("selection-background-color:lightblue;"); //设置选中背景色
	//d->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;}"); //设置表头背景色
	d->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section{background:#646464;}"); //设置表头背景色
}

void VisualBrainLabClientWidget::onGetMetaItem()
{
	Q_D(VisualBrainLabClientWidget);

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
		emit logErr(QString::fromStdString(dataId));
		switch (d->m_TypeButtonGroup.checkedId())
		{
		case OpenIGTLinkQueryType::TYPE_IMAGE:
		case OpenIGTLinkQueryType::TYPE_LABEL:
			d->m_brainLabClient->QueryImage(dataId);
			break;
		case OpenIGTLinkQueryType::TYPE_POINT:
			//this->getPointList(dataId);
			break;
		case OpenIGTLinkQueryType::TYPE_TRAJ:
			break;
		case OpenIGTLinkQueryType::TYPE_CAPABIL:
			break;
		default:
			qCritical() << Q_FUNC_INFO << " failed: unknown item type selected";
		}
	}
}

void VisualBrainLabClientWidget::onUpdateIMGMetaTabWidget(IMGMetaData metaData)
{
	Q_D(VisualBrainLabClientWidget);
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

void VisualBrainLabClientWidget::onUpdateLBMetaTabWidget(LBMetaData metaData)
{
	Q_D(VisualBrainLabClientWidget);
	qDebug() << "onUpdateLBMetatabWidget:" << metaData.DeviceName.c_str();
	d->tableWidget->setRowCount(metaData.index + 1);
	QTableWidgetItem* deviceItem = new QTableWidgetItem(metaData.DeviceName.c_str());
	QTableWidgetItem* nameItem = new QTableWidgetItem(metaData.Name.c_str());
	QTableWidgetItem* ownerItem = new QTableWidgetItem(metaData.Owner.c_str());

	d->tableWidget->setItem(metaData.index, 0, deviceItem);
	d->tableWidget->setItem(metaData.index, 1, nameItem);
	d->tableWidget->setItem(metaData.index, 2, ownerItem);
}

void VisualBrainLabClientWidget::onUpdateTRAJDataTabWidget(TRAJData trajData)
{
	Q_D(VisualBrainLabClientWidget);

	qDebug() << "onUpdateTRAJDataTabWidget:" << trajData.Name.c_str();
	d->tableWidget->setRowCount(trajData.index + 1);
	
	QTableWidgetItem * nameItem = new QTableWidgetItem(trajData.Name.c_str());
	QTableWidgetItem* groupNameItem = new QTableWidgetItem(trajData.GroupName.c_str());

	int type = static_cast<int>(trajData.Type);
	QTableWidgetItem* typeItem = new QTableWidgetItem(QString::number(type));

	QString entry = QString("(%1,%2,%3)").arg(trajData.EntryPoint[0]).arg(trajData.EntryPoint[1]).arg(trajData.EntryPoint[2]);
	QTableWidgetItem* entryItem = new QTableWidgetItem(entry);

	QString target = QString("(%1,%2,%3)").arg(trajData.TargetPoint[0]).arg(trajData.TargetPoint[1]).arg(trajData.TargetPoint[2]);
	QTableWidgetItem* targetItem = new QTableWidgetItem(entry);

	d->tableWidget->setItem(trajData.index, 0, nameItem);
	d->tableWidget->setItem(trajData.index, 1, groupNameItem);
	d->tableWidget->setItem(trajData.index, 2, typeItem);
	d->tableWidget->setItem(trajData.index, 3, entryItem);
	d->tableWidget->setItem(trajData.index, 4, targetItem);
}