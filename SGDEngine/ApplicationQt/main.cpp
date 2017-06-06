#include "stdafx.h"
#include "ApplicationQt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	// testing
	for (int32 Index = 0; Index < 1000; ++Index)
		SGD::Memory::H1Log<char>::CreateFormattedLog("test %s %d", "test0", Index);

	QApplication a(argc, argv);
	ApplicationQt w;
	w.show();
	return a.exec();
}
