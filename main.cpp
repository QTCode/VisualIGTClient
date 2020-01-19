#include "VisualIGTLinkClientWidget.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	VisualIGTLinkClientWidget w;
	w.show();
	return a.exec();
}
