#pragma once

#include <QtWidgets/QWidget>
#include "ui_IGTLinkClinet.h"

class IGTLinkClinet : public QWidget
{
	Q_OBJECT

public:
	IGTLinkClinet(QWidget *parent = Q_NULLPTR);

private:
	Ui::IGTLinkClinetClass ui;
};
