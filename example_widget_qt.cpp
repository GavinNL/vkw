#include <vulkan/vulkan.hpp>


// WINDOW_MANAGER is a definition which is defined in the CMakeLists.txt file
// if WINDOW_MANAGER==0 it will use a qt window
// if WINDOW_MANGER==1 it will use an SDL window.

#define WINDOW_MANAGER 0

#if WINDOW_MANAGER == 0


#include <vkw/QtVulkanWidget.h>
#include <QMainWindow>
#include <QApplication>
#include <QPlainTextEdit>
#include <QVulkanInstance>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QPointer>
#include <QHBoxLayout>
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

#else

#endif



class MyApplication : public Application
{
    // Application interface
public:
    void initResources() override
    {
        std::cout << "ir " << std::endl;
    }
    void releaseResources() override
    {
        std::cout << "rr " << std::endl;
    }
    void initSwapChainResources() override
    {
        std::cout << "is " << std::endl;
    }
    void releaseSwapChainResources() override
    {
        std::cout << "rs " << std::endl;
    }
    void render(Frame &frame) override
    {
        assert( frame.depthImage != VK_NULL_HANDLE);

        frame.clearColor.float32[0] = 0.0f;
        frame.beginRenderPass( frame.commandBuffer );

        frame.endRenderPass(frame.commandBuffer);

        // request the next frame
        // so that this function will be called again
        requestNextFrame();
    }
};


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
    // This is the widget that can be added to a QtWindow or any other
    // widget. The actual  Applicatioof the application
    QtVulkanWidget2 * vulkanWindow = new QtVulkanWidget2();
    vulkanWindow->setVulkanInstance(&inst);

    // create a wrapper for the vulkanWindow
    QWidget *wrapper = QWidget::createWindowContainer(vulkanWindow);


    MyApplication * myApp = new MyApplication();

    vulkanWindow->init( myApp );

    m_window->setLayout( new QHBoxLayout() );
    m_window->layout()->addWidget(wrapper);

        // or we can add it to another widget;
        //MainWindow mainWindow(vulkanWindow, messageLogWidget.data());
        //
        //mainWindow.resize(1024, 768);
        //mainWindow.show();
    m_window->resize(1024,768);
    m_window->show();


    return app.exec();
}
