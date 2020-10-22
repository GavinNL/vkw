#include <vulkan/vulkan.hpp>

#include <vkw/QtVulkanWidget.h>
#include <QMainWindow>
#include <QApplication>
#include <QPlainTextEdit>
#include <QVulkanInstance>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QPointer>
#include <QHBoxLayout>
#include <QPushButton>
#include <iostream>

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")


static QPointer<QPlainTextEdit> messageLogWidget;
static QtMessageHandler oldMessageHandler = nullptr;

static void messageHandler(QtMsgType msgType, const QMessageLogContext &logContext, const QString &text)
{
    if (!messageLogWidget.isNull())
        messageLogWidget->appendPlainText(text);
    if (oldMessageHandler)
        oldMessageHandler(msgType, logContext, text);
}


// This header file contains the actual
// rendering code for vulkan. It can be used
// by both the SDLWidget and the Qt widget.
// see example_widget_sdl.cpp
// and example_widget_qt.cpp
#include "example_myApplication.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    messageLogWidget = new QPlainTextEdit(QLatin1String(QLibraryInfo::build()) + QLatin1Char('\n'));
    messageLogWidget->setReadOnly(true);

    oldMessageHandler = qInstallMessageHandler(messageHandler);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;

    inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());


    QMainWindow * m_window = new QMainWindow();


    //=============================================================
    // This the main VulkanWidget
    //=============================================================

    // create a QtVulkanWidget and
    // set the vulkan instance
    vkw::QtVulkanWidget2 * vulkanWindow = new vkw::QtVulkanWidget2();
    vulkanWindow->setVulkanInstance(&inst);

    // create a wrapper for the vulkanWindow
    QWidget *wrapper = QWidget::createWindowContainer(vulkanWindow);

    // Create an instance of our application.
    // this is the actual app that will be
    // performing all the rendering
    MyApplication * myApp = new MyApplication();

    vulkanWindow->init( myApp );

    // create a very simple user interface;

//    +------------------------------------+
//    |                                    |
//    +--------------------------+---------+
//    |                          |         |
//    |                          |         |
//    |                          |         |
//    |                          |         |
//    |   vulkan widget          |  other  |
//    |                          |         |
//    |                          |         |
//    |                          |         |
//    |                          |         |
//    +--------------------------+---------+
    auto l = new QHBoxLayout();

    auto centralWidget = new QWidget();
    centralWidget->setLayout(l);

    m_window->setCentralWidget(centralWidget);

        l->addWidget(wrapper, 5);
        l->addWidget(new QPushButton(m_window), 1);

    m_window->resize(1024,768);
    m_window->show();


    return app.exec();
}
