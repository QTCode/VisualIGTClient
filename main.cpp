#include "IGTLinkClinet.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	IGTLinkClinet w;
	w.show();
	return a.exec();
}
