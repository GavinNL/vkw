#ifndef VKA_QT_VULKAN_WIDGET_H
#define VKA_QT_VULKAN_WIDGET_H

#include <QVulkanWindow>
#include <QWidget>


#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>

#include <iostream>

#include "VulkanApplication.h"

namespace vkw {


/**
 * @brief The QTRenderer struct
 *
 * This is how Qt would like to use
 * Vulkan within their windowing system
 */
class QTRenderer : public QVulkanWindowRenderer
{
    public:

    QTRenderer(QVulkanWindow * w, bool msaa) : m_window(w)
    {
        msaa= false;
        if (msaa) {
            const QVector<int> counts = w->supportedSampleCounts();
            qDebug() << "Supported sample counts:" << counts;
            for (int s = 16; s >= 4; s /= 2) {
                if (counts.contains(s)) {
                    qDebug("Requesting sample count %d", s);
                    m_window->setSampleCount(s);
                    break;
                }
            }
        }
    }

    ~QTRenderer()
    {
    }


    /**
     * @brief startNextFrame
     *
     * Override the QVulkanWindowRenderer::startNextFrame to call the
     * render() method instead.
     */
    virtual void startNextFrame() override
    {
        auto i = m_window->currentSwapChainImageIndex();

        Frame _frame;

        m_application->m_currentSwapchainIndex = static_cast<uint32_t>(m_window->currentSwapChainImageIndex() );

        QSize sz = m_window->swapChainImageSize();

        _frame.renderPass           = m_application->m_defaultRenderPass;

        _frame.swapchainSize.width  = static_cast<uint32_t>(sz.width() );
        _frame.swapchainSize.height = static_cast<uint32_t>(sz.height());
        _frame.swapchainIndex     = static_cast<uint32_t>(m_window->currentSwapChainImageIndex() );
        _frame.swapchainFormat      = m_window->colorFormat();
        _frame.swapchainImage     = m_window->swapChainImage(i);
        _frame.swapchainImageView = m_window->swapChainImageView(i);

        _frame.depthImage = m_window->depthStencilImage();
        _frame.depthImageView = m_window->depthStencilImageView();
        _frame.depthFormat          = m_window->depthStencilFormat();

        _frame.framebuffer        = m_window->currentFramebuffer();
        _frame.commandPool        =  m_window->graphicsCommandPool();
        _frame.commandBuffer      = m_window->currentCommandBuffer();


        _frame.clearColor.float32[0] = 1.0f;
        _frame.clearColor.float32[1] = 1.0f;
        _frame.clearColor.float32[2] = 1.0f;
        _frame.clearColor.float32[3] = 1.0f;

        _frame.clearDepth.depth = 1.0f;
        _frame.clearDepth.stencil = 0;
        m_application->m_renderNextFrame = false;
        m_application->render(_frame);

        m_window->frameReady();

        if( m_application->shouldRender())
        {
            m_window->requestUpdate();
        }
    }

    void initResources() override
    {
        m_application->m_concurrentFrameCount = static_cast<uint32_t>( m_window->concurrentFrameCount() );
        m_application->m_defaultRenderPass = m_window->defaultRenderPass();
        m_application->m_device            = m_window->device();
        m_application->m_physicalDevice    = m_window->physicalDevice();
        m_application->m_instance          = m_window->vulkanInstance()->vkInstance();
        m_application->initResources();

    }

    void initSwapChainResources() override
    {
        auto s = m_window->swapChainImageSize();

        m_application->m_concurrentFrameCount = static_cast<uint32_t>( m_window->concurrentFrameCount() );
        m_application->m_swapChainSize        = VkExtent2D{ static_cast<uint32_t>(s.width()), static_cast<uint32_t>(s.height()) };
        m_application->m_swapChainFormat      = m_window->colorFormat();
        m_application->m_swapChainDepthFormat = m_window->depthStencilFormat();

        m_application->m_swapchainImages.clear();
        m_application->m_swapchainImageViews.clear();

        for(int i=0;i<m_window->swapChainImageCount();i++)
        {
            m_application->m_swapchainImages.push_back( m_window->swapChainImage(i));
            m_application->m_swapchainImageViews.push_back( m_window->swapChainImageView(i));
        }
        m_application->initSwapChainResources();
    }

    void releaseSwapChainResources() override
    {
        m_application->releaseSwapChainResources();
    }

    void releaseResources() override
    {
        m_application->releaseResources();
    }


protected:
    QVulkanWindow               *m_window;
    vkw::Application            *m_application = nullptr;
    bool                         m_SystemCreated=false;
    friend class QtVulkanWidget2;
};




/**
 * @brief The VulkanWidget class
 *
 * This simple class just needs to override the createRenderer() method
 * and return the the TriangleRenderer*
 *
 * If you do not want to use the VKA event methods, You can
 * make a copy of this class and replace the qt events
 *
 *
 */
class QtVulkanWidget2 : public QVulkanWindow
{
  //  Q_OBJECT

public:

    void init(Application * app)
    {
        this->setFlags( QVulkanWindow::PersistentResources);
        m_application = app;
    }

    //=========================================================
    // These two functions are needed to interact iwth
    // Qt.
    //=========================================================
    QVulkanWindowRenderer* createRenderer() override
    {
        auto * t = new QTRenderer(this, true);
        assert(m_application != nullptr);
        t->m_application = m_application;
        return t;
    }
    //=========================================================
#if 0
    void mousePressEvent(QMouseEvent * e) override
    {
        EvtInputMouseButton E;
        E.x      = e->x();//event.button.x;
        E.y      = e->y();//event.button.y;
        switch( e->button() )
        {
           case Qt::NoButton         : E.button = vka::MouseButton::NONE;  break;//= 0x00000000,
           case Qt::LeftButton       : E.button = vka::MouseButton::LEFT;  break;//= 0x00000001,
           case Qt::RightButton      : E.button = vka::MouseButton::RIGHT;  break;//= 0x00000002,
           //case Qt::MidButton      : E.button = ;  break;//= 0x00000004, // ### Qt 6: remove me
           case Qt::MiddleButton     : E.button = vka::MouseButton::MIDDLE;  break;//= MidButton,
           //case Qt::BackButton     : E.button = ;  break;//= 0x00000008,
           case Qt::XButton1         : E.button = vka::MouseButton::X1;  break;//= BackButton,
           //case Qt::ExtraButton1   : E.button = ;  break;//= XButton1,
           //case Qt::ForwardButton  : E.button = ;  break;//= 0x00000010,
           case Qt::XButton2         : E.button = vka::MouseButton::X2;  break;//= ForwardButton,
        default:
            E.button = vka::MouseButton::NONE;
         break;
        }
        E.state =  1;//e->buttons() & e->button() ? 1 : 0;

        E.clicks = 1;//event.button.clicks;
        m_application->mousePressEvent(&E);
    }

    void mouseReleaseEvent(QMouseEvent * e) override
    {
        EvtInputMouseButton E;
        E.x      = e->x();//event.button.x;
        E.y      = e->y();//event.button.y;
        switch( e->button() )
        {
           case Qt::NoButton         : E.button = vka::MouseButton::NONE;  break;//= 0x00000000,
           case Qt::LeftButton       : E.button = vka::MouseButton::LEFT;  break;//= 0x00000001,
           case Qt::RightButton      : E.button = vka::MouseButton::RIGHT;  break;//= 0x00000002,
           //case Qt::MidButton      : E.button = ;  break;//= 0x00000004, // ### Qt 6: remove me
           case Qt::MiddleButton     : E.button = vka::MouseButton::MIDDLE;  break;//= MidButton,
           //case Qt::BackButton     : E.button = ;  break;//= 0x00000008,
           case Qt::XButton1         : E.button = vka::MouseButton::X1;  break;//= BackButton,
           //case Qt::ExtraButton1   : E.button = ;  break;//= XButton1,
           //case Qt::ForwardButton  : E.button = ;  break;//= 0x00000010,
           case Qt::XButton2         : E.button = vka::MouseButton::X2;  break;//= ForwardButton,
           default:
               E.button = vka::MouseButton::NONE;
            break;
        }
        E.state =  0;//e->buttons() & e->button() ? 1 : 0;

        E.clicks = 1;//event.button.clicks;

        m_application->mouseReleaseEvent(&E);
    }

    void mouseDoubleClickEvent(QMouseEvent * e) override
    {
        EvtInputMouseButton E;
        E.x      = e->x();//event.button.x;
        E.y      = e->y();//event.button.y;
        switch( e->button() )
        {
           case Qt::NoButton         : E.button = vka::MouseButton::NONE;  break;//= 0x00000000,
           case Qt::LeftButton       : E.button = vka::MouseButton::LEFT;  break;//= 0x00000001,
           case Qt::RightButton      : E.button = vka::MouseButton::RIGHT;  break;//= 0x00000002,
           //case Qt::MidButton      : E.button = ;  break;//= 0x00000004, // ### Qt 6: remove me
           case Qt::MiddleButton     : E.button = vka::MouseButton::MIDDLE;  break;//= MidButton,
           //case Qt::BackButton     : E.button = ;  break;//= 0x00000008,
           case Qt::XButton1         : E.button = vka::MouseButton::X1;  break;//= BackButton,
           //case Qt::ExtraButton1   : E.button = ;  break;//= XButton1,
           //case Qt::ForwardButton  : E.button = ;  break;//= 0x00000010,
           case Qt::XButton2         : E.button = vka::MouseButton::X2;  break;//= ForwardButton,
           default:
               E.button = vka::MouseButton::NONE;
            break;
        }
        E.state =  0;//e->buttons() & e->button() ? 1 : 0;

        E.clicks = 2;//event.button.clicks;

        m_application->mouseReleaseEvent(&E);
    }

    void mouseMoveEvent(QMouseEvent * e) override
    {
        EvtInputMouseMotion M;
        M.x    = e->x();//event.motion.x;
        M.y    = e->y();//event.motion.y;
        M.xrel = M.x - m_oldMouseX;//event.motion.xrel;
        M.yrel = M.y - m_oldMouseY;//event.motion.yrel;

        m_oldMouseX = M.x;
        m_oldMouseY = M.y;
        //
        m_application->mouseMoveEvent(&M);
    }

    void keyPressEvent(QKeyEvent * e)       override
    {
        EvtInputKey M;

        M.timestamp      = std::chrono::system_clock::now();
        M.down           = true;
        M.repeat         = static_cast<uint8_t>(e->count());
        auto f    = m_QT_to_Key.find( e->key() );
        M.keycode = KeyCode::UNKNOWN;
        if( f != m_QT_to_Key.end() )
           M.keycode = f->second;

        M.windowKeyCode  = static_cast<uint32_t>( e->key() );
        M.windowScanCode = static_cast<uint32_t>( e->nativeScanCode() );

        M.windowEvent = e;

        //std::cout << "QtScan " << M.windowScanCode << " --> vkaScan " << to_string(M.scancode) << std::endl;
        //std::cout << "QtKey  " << M.windowKeyCode << " --> vkaKey  " << to_string(M.keycode) << std::endl;
        //std::cout << "VulkanWidgetQt: Key: " << e->nativeVirtualKey() << "  Scan  " << e->nativeScanCode() << std::endl;
        m_application->keyPressEvent(&M);

    }

    void keyReleaseEvent(QKeyEvent * e)     override
    {
        EvtInputKey M;

        M.timestamp      = std::chrono::system_clock::now();
        M.down           = false;
        M.repeat         = static_cast<uint8_t>( e->count() );
        auto f    = m_QT_to_Key.find( e->key() );
        M.keycode = KeyCode::UNKNOWN;
        if( f != m_QT_to_Key.end() )
           M.keycode = f->second;

        M.windowKeyCode  = static_cast<uint32_t>( e->key() );
        M.windowScanCode = e->nativeScanCode();

        M.windowEvent = e;

        //std::cout << "QtScan " << M.windowScanCode << " --> vkaScan " << to_string(M.scancode) << std::endl;
        //std::cout << "QtKey  " << M.windowKeyCode << " --> vkaKey  " << to_string(M.keycode) << std::endl;
        //std::cout << "VulkanWidgetQt: Key: " << e->nativeVirtualKey() << "  Scan  " << e->nativeScanCode() << std::endl;
        m_application->keyReleaseEvent(&M);

    }

    void wheelEvent(QWheelEvent * e) override
    {
        EvtInputMouseWheel E;
        E.x = e->pixelDelta().x();
        E.y = e->pixelDelta().y();
        m_application->mouseWheelEvent(&E);
    }
#endif
protected:
    Application * m_application = nullptr;


};

}

#endif
