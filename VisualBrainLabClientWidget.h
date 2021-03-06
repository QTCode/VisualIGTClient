#pragma once
#include <QWidget>
#include "ui_VisualBrainLabClientWidget.h"
#include "VisualBrainLabType.h"

class VisualBrainLabClientWidgetPrivate;
class VisualBrainLabClientWidget : public QWidget
{
	Q_OBJECT

public:
	VisualBrainLabClientWidget(QWidget *parent = Q_NULLPTR);
	~VisualBrainLabClientWidget();
signals:
	void logErr(QString log);
protected:
	QScopedPointer<VisualBrainLabClientWidgetPrivate> d_ptr;
protected slots:
	void onConnectToServer();
	void onPrintLog(QString logErr);
	void onQueryRemoteList();
	void onQueryTypeChanged(int id);
	void onGetMetaItem();

	void onUpdateIMGMetaTabWidget(IMGMetaData metaData);
	void onUpdateLBMetaTabWidget(LBMetaData metaData);

	void onUpdatePointDataTabWidget(PointData pData);

	void onUpdateTRAJDataTabWidget(TRAJData trajData);


private:

	Q_DECLARE_PRIVATE(VisualBrainLabClientWidget);
	Q_DISABLE_COPY(VisualBrainLabClientWidget);
};
