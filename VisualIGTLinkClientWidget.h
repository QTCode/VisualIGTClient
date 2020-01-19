#pragma once
#include <QWidget>
#include "ui_VisualIGTLinkClientWidget.h"
#include "VisualBrainLabClient.h"

class VisualIGTLinkClientWidgetPrivate;
class VisualIGTLinkClientWidget : public QWidget
{
	Q_OBJECT

public:
	VisualIGTLinkClientWidget(QWidget *parent = Q_NULLPTR);
	~VisualIGTLinkClientWidget();

protected:
	QScopedPointer<VisualIGTLinkClientWidgetPrivate> d_ptr;
protected slots:
	void onConnectToServer();
	void onPrintLog(QString logErr);
	void onQueryRemoteList();
	void onQueryTypeChanged(int id);
	void onGetMetaItem();

	void onUpdateIMGMetaTabWidget(IMGMetaData metaData);
	void onUpdateLBMetatabWidget(LBMetaData metaData);


private:

	Q_DECLARE_PRIVATE(VisualIGTLinkClientWidget);
	Q_DISABLE_COPY(VisualIGTLinkClientWidget);
};
