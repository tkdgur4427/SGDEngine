#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ApplicationQt.h"

class ApplicationQt : public QMainWindow
{
	Q_OBJECT

public:
	ApplicationQt(QWidget *parent = Q_NULLPTR);

private:
	Ui::ApplicationQtClass ui;
};
