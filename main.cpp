#include "VisualBrainLabClientWidget.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	VisualBrainLabClientWidget w;
	w.show();
	return a.exec();
}
