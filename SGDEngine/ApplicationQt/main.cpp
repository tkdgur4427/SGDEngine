#include "stdafx.h"
#include "ApplicationQt.h"
#include <QtWidgets/QApplication>

#include "H1LaunchEngineLoop.h"

int main(int argc, char *argv[])
{
	// for testing
	Init();
	Run();
	Destroy();

	QApplication a(argc, argv);
	ApplicationQt w;
	w.show();
	return a.exec();
}
