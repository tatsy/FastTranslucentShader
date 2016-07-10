#ifdef _MSC_VER
#pragma once
#endif

#ifndef _OPENGL_VIEWER_H_
#define _OPENGL_VIEWER_H_

#include <memory>

#include <QtCore/qtimer.h>

#include <QtWidgets/qmenubar.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qaction.h>

#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qopenglshaderprogram.h>
#include <QtGui/qopenglvertexarrayobject.h>
#include <QtGui/qopenglbuffer.h>
#include <QtGui/qopengltexture.h>
#include <QtGui/qopenglframebufferobject.h>

#include "arcballcontroller.h"

class OpenGLViewer : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit OpenGLViewer(QWidget* parent = nullptr);
    virtual ~OpenGLViewer();

    void setMaterial(const std::string& mtrlName);
    void setMaterialScale(double scale);
    void setRenderComponents(bool isRefl, bool isTrans);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void mousePressEvent(QMouseEvent* ev) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;

private slots:
    void OnAnimate();

private:
    void calcGBuffers();

    std::unique_ptr<QOpenGLShaderProgram> shader       = nullptr;
    std::unique_ptr<QOpenGLShaderProgram> dipoleShader = nullptr; 
    std::unique_ptr<QOpenGLShaderProgram> gbufShader   = nullptr;

    std::unique_ptr<QOpenGLVertexArrayObject> vao = nullptr;
    std::unique_ptr<QOpenGLBuffer> vBuffer = nullptr;
    std::unique_ptr<QOpenGLBuffer> iBuffer = nullptr;

    std::unique_ptr<QOpenGLVertexArrayObject> sampleVAO = nullptr;
    std::unique_ptr<QOpenGLBuffer> sampleVBuf = nullptr;
    std::unique_ptr<QOpenGLBuffer> sampleIBuf = nullptr;

    std::unique_ptr<QOpenGLFramebufferObject> dipoleFbo = nullptr;
    std::unique_ptr<QOpenGLFramebufferObject> deferFbo = nullptr;
    std::unique_ptr<QOpenGLFramebufferObject> gbufFbo = nullptr;

    std::unique_ptr<QOpenGLTexture> texture = nullptr;

    std::unique_ptr<QTimer> timer = nullptr;
    std::unique_ptr<ArcballController> arcball = nullptr;
};

#endif  // _OPENGL_VIEWER_H_
