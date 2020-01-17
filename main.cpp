#include "IGTLinkClientWidget.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	IGTLinkClientWidget w;
	w.show();
	return a.exec();
}
