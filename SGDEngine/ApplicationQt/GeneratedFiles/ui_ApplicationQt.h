/********************************************************************************
** Form generated from reading UI file 'ApplicationQt.ui'
**
** Created by: Qt User Interface Compiler version 5.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_APPLICATIONQT_H
#define UI_APPLICATIONQT_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ApplicationQtClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *ApplicationQtClass)
    {
        if (ApplicationQtClass->objectName().isEmpty())
            ApplicationQtClass->setObjectName(QStringLiteral("ApplicationQtClass"));
        ApplicationQtClass->resize(600, 400);
        menuBar = new QMenuBar(ApplicationQtClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        ApplicationQtClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(ApplicationQtClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        ApplicationQtClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(ApplicationQtClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        ApplicationQtClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(ApplicationQtClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        ApplicationQtClass->setStatusBar(statusBar);

        retranslateUi(ApplicationQtClass);

        QMetaObject::connectSlotsByName(ApplicationQtClass);
    } // setupUi

    void retranslateUi(QMainWindow *ApplicationQtClass)
    {
        ApplicationQtClass->setWindowTitle(QApplication::translate("ApplicationQtClass", "ApplicationQt", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class ApplicationQtClass: public Ui_ApplicationQtClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_APPLICATIONQT_H
