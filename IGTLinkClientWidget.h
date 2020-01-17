#pragma once
#include <QWidget>
#include "ui_IGTLinkClientWidget.h"

class IGTLinkClientWidgetPrivate;
class IGTLinkClientWidget : public QWidget
{
	Q_OBJECT

public:
	IGTLinkClientWidget(QWidget *parent = Q_NULLPTR);
	~IGTLinkClientWidget();

protected:
	QScopedPointer<IGTLinkClientWidgetPrivate> d_ptr;
protected slots:
	void ConnectToServer();

private:

	Q_DECLARE_PRIVATE(IGTLinkClientWidget);
	Q_DISABLE_COPY(IGTLinkClientWidget);
};
