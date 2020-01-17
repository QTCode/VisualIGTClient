#pragma once
#include <QWidget>
#include "ui_IGTLinkClinet.h"


class IGTLinkClinetPrivate;
class IGTLinkClinet : public QWidget
{
	Q_OBJECT

public:
	IGTLinkClinet(QWidget *parent = Q_NULLPTR);
	~IGTLinkClinet();

protected:
	QScopedPointer<IGTLinkClinetPrivate> d_ptr;

private:

	Q_DECLARE_PRIVATE(IGTLinkClinet);
	Q_DISABLE_COPY(IGTLinkClinet);
};
