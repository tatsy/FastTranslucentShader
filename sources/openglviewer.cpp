#include "openglviewer.h"

#include <ctime>
#include <iostream>
#include <fstream>
#include <functional>

#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglextrafunctions.h>

#include <opencv2/opencv.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "settings.h"

// Please activate folloring line to save intermediate results.
//#define DEBUG_MODE 1

static constexpr int SHADER_POSITION_LOC = 0;
static constexpr int SHADER_NORMAL_LOC   = 1;
static constexpr int SHADER_TEXCOORD_LOC = 2;

static constexpr int SAMPLE_POSITION_LOC = 0;
static constexpr int SAMPLE_NORMAL_LOC   = 1;
static constexpr int SAMPLE_TEXCOORD_LOC = 2;
static constexpr int SAMPLE_RADIUS_LOC   = 3;

static const QVector3D lightPos = QVector3D(-3.0f, 4.0f, 5.0f);

static QVector3D sigma_a   = QVector3D(0.0015333, 0.0046, 0.019933);
static QVector3D sigmap_s  = QVector3D(4.5513   , 5.8294, 7.136   );
static float     eta       = 1.3f;
static float     mtrlScale = 50.0f;
static bool      isRenderRefl = true;
static bool      isRenderTrans = true;

struct Sample {
    QVector3D position;
    QVector3D normal;
    QVector2D texcoord;
    float radius;
};

namespace {

void takeFloatImage(QOpenGLFramebufferObject& fbo, cv::Mat* image, int channels, int attachmentIndex) {
    const int width  = fbo.width();
    const int height = fbo.height();

    fbo.bind();
    std::vector<float> pixels(width * height * 4);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, &pixels[0]);

    *image = cv::Mat(width, height, CV_MAKETYPE(CV_32F, channels));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int ch = 0; ch < channels; ch++) {
                image->at<float>(height - y - 1, x * channels + ch) = pixels[(y * width + x) * 4 + ch];
            }
        }
    }
    fbo.release();
}

void saveFloatImage(const std::string& filename, cv::InputArray image) {
    cv::Mat img = image.getMat();    
    cv::Mat img8u;
    img.convertTo(img8u, CV_MAKETYPE(CV_8U, img.channels()), 255.0);
    cv::imwrite(filename, img8u);
}

template <class T>
void pyrDown(cv::InputArray lower, cv::OutputArray upper, const std::function<T(T, T, T, T)>& f) {
    cv::Mat  low = lower.getMat();
    cv::Mat& up  = upper.getMatRef();
    
    const int width  = low.cols;
    const int height = low.rows;
    up = cv::Mat(height / 2, width / 2, CV_MAKETYPE(low.depth(), low.channels()));

    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            T& t0 = low.at<T>(y * 2, x * 2);
            T& t1 = low.at<T>(y * 2, x * 2 + 1);
            T& t2 = low.at<T>(y * 2 + 1, x * 2);
            T& t3 = low.at<T>(y * 2 + 1, x * 2 + 1);
            up.at<T>(y, x) = f(t0, t1, t2, t3);
        }
    }
}

}  // anonymous namespace

OpenGLViewer::OpenGLViewer(QWidget* parent)
    : QOpenGLWidget{ parent} {
    arcball = std::make_unique<ArcballController>(this);

    QMatrix4x4 mMat, vMat;
    mMat.scale(5.0f);
    vMat.lookAt(QVector3D(0.0f, 0.0f, 3.0f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.1f, 0.0f));
    arcball->initModelView(mMat,vMat);

    timer = std::make_unique<QTimer>(this);
    connect(timer.get(), SIGNAL(timeout()), this, SLOT(OnAnimate()));

    timer->start(0);
}

OpenGLViewer::~OpenGLViewer() {
}

void OpenGLViewer::setMaterial(const std::string& mtrlName) {
    if (mtrlName == "Milk") {
        sigma_a  = QVector3D(0.0015333, 0.0046, 0.019933);
        sigmap_s = QVector3D(4.5513   , 5.8294, 7.136   );
        eta      = 1.3f;            
    } else if (mtrlName == "Skin") {
        sigma_a  = QVector3D(0.061, 0.97, 1.45);
        sigmap_s = QVector3D(0.18, 0.07, 0.03);
        eta      = 1.3f;
    }
}

void OpenGLViewer::setMaterialScale(double scale) {
    mtrlScale = static_cast<float>(scale);
}

void OpenGLViewer::setRenderComponents(bool isRef, bool isTrans) {
    isRenderRefl = isRef;
    isRenderTrans = isTrans;
}

void OpenGLViewer::initializeGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Load .obj file.
    std::string filename = std::string(DATA_DIRECTORY) + "dragon.obj";
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(shapes, materials, err, filename.c_str()), tinyobj::load_flags_t::triangulation) {
        if (!err.empty()) {
            std::cerr << err << std::endl;
        }
    }
    const int nVerts = shapes[0].mesh.positions.size() / 3;

    // Initialize VAO.
    vao = std::make_unique<QOpenGLVertexArrayObject>(this);
    vao->create();
    vao->bind();

    vBuffer = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    vBuffer->create();
    vBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    vBuffer->bind();
    vBuffer->allocate(sizeof(float) * (3 + 3 + 2) * nVerts);
    vBuffer->write(0, &shapes[0].mesh.positions[0], sizeof(float) * 3 * nVerts);
    vBuffer->write(sizeof(float) * 3 * nVerts, &shapes[0].mesh.normals[0],   sizeof(float) * 3 * nVerts);
    vBuffer->write(sizeof(float) * 6 * nVerts, &shapes[0].mesh.texcoords[0], sizeof(float) * 2 * nVerts);

    auto f = QOpenGLContext::currentContext()->extraFunctions();
    f->glEnableVertexAttribArray(SHADER_POSITION_LOC);
    f->glEnableVertexAttribArray(SHADER_NORMAL_LOC);
    f->glEnableVertexAttribArray(SHADER_TEXCOORD_LOC);
    f->glVertexAttribPointer(SHADER_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    f->glVertexAttribPointer(SHADER_NORMAL_LOC,   3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 3 * nVerts));
    f->glVertexAttribPointer(SHADER_TEXCOORD_LOC, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 6 * nVerts));

    iBuffer = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    iBuffer->create();
    iBuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
    iBuffer->bind();
    iBuffer->allocate(&shapes[0].mesh.indices[0], sizeof(unsigned int) * shapes[0].mesh.indices.size());

    vao->release();

    // Initialize texture.
    QImage texImage;
    texImage.load(QString(DATA_DIRECTORY) + "wood.jpg");
    texture = std::make_unique<QOpenGLTexture>(texImage, QOpenGLTexture::MipMapGeneration::GenerateMipMaps);
    texture->setMinificationFilter(QOpenGLTexture::Filter::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Filter::Linear);
    texture->setWrapMode(QOpenGLTexture::CoordinateDirection::DirectionS, QOpenGLTexture::WrapMode::ClampToEdge);
    texture->setWrapMode(QOpenGLTexture::CoordinateDirection::DirectionT, QOpenGLTexture::WrapMode::ClampToEdge);

    // Initialize shaders.
    shader = std::make_unique<QOpenGLShaderProgram>(this);
    shader->addShaderFromSourceFile(QOpenGLShader::Vertex, QString(SHADER_DIRECTORY) + "render.vs");
    shader->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(SHADER_DIRECTORY) + "render.fs");
    shader->link();
    if (!shader->isLinked()) {
        std::cerr << "Failed to link shader files!!" << std::endl;
        std::exit(1);        
    }

    dipoleShader = std::make_unique<QOpenGLShaderProgram>(this);
    dipoleShader->addShaderFromSourceFile(QOpenGLShader::Vertex, QString(SHADER_DIRECTORY) + "dipole.vs");
    dipoleShader->addShaderFromSourceFile(QOpenGLShader::Geometry, QString(SHADER_DIRECTORY) + "dipole.gs");
    dipoleShader->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(SHADER_DIRECTORY) + "dipole.fs");
    dipoleShader->link();
    if (!dipoleShader->isLinked()) {
        std::cerr << "Failed to link shader files!!" << std::endl;
        std::exit(1);
    }

    gbufShader = std::make_unique<QOpenGLShaderProgram>(this);
    gbufShader->addShaderFromSourceFile(QOpenGLShader::Vertex, QString(SHADER_DIRECTORY) + "gbuffers.vs");
    gbufShader->addShaderFromSourceFile(QOpenGLShader::Fragment, QString(SHADER_DIRECTORY) + "gbuffers.fs");
    gbufShader->link();
    if (!gbufShader->isLinked()) {
        std::cerr << "Failed to link shader files!!" << std::endl;
        std::exit(1);
    }

    // Compute hierarchical irradiance samples.
    calcGBuffers();
}

void OpenGLViewer::resizeGL(int width, int height) {
    glViewport(0, 0, this->width(), this->height());

    // FBOs for G-buffers.
    deferFbo = std::make_unique<QOpenGLFramebufferObject>(this->width(), this->height(),
        QOpenGLFramebufferObject::Attachment::Depth, GL_TEXTURE_2D, GL_RGBA32F);
    deferFbo->addColorAttachment(this->width(), this->height(), GL_RGBA32F);
    deferFbo->addColorAttachment(this->width(), this->height(), GL_RGBA32F);
    deferFbo->addColorAttachment(this->width(), this->height(), GL_RGBA32F);

    // FBO for translucent component.
    dipoleFbo = std::make_unique<QOpenGLFramebufferObject>(this->width(), this->height(),
        QOpenGLFramebufferObject::Attachment::Depth, GL_TEXTURE_2D, GL_RGBA32F);
}

void OpenGLViewer::paintGL() {
    QMatrix4x4 mMat, vMat, pMat;
    mMat = arcball->modelMat();
    vMat = arcball->viewMat();
    pMat.perspective(45.0f, (float)width() / height(), 1.0f, 1000.0f);
    QMatrix4x4 mvMat = vMat * mMat;
    QMatrix4x4 mvpMat = pMat * mvMat;

    // Compute deferred shading buffers.
    gbufShader->bind();
    deferFbo->bind();
    vao->bind();

    gbufShader->setUniformValue("uMVPMat", mvpMat);
    gbufShader->setUniformValue("isMaxDepth", 0);

    auto f = QOpenGLContext::currentContext()->extraFunctions();
    GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    f->glDrawBuffers(4, bufs);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawElements(GL_TRIANGLES, iBuffer->size() / sizeof(unsigned int), GL_UNSIGNED_INT, 0);

    gbufShader->release();
    deferFbo->release();
    vao->release();

    #if DEBUG_MODE
    deferFbo->toImage(true, 1).save(QString(OUTPUT_DIRECTORY) + "position.png");
    deferFbo->toImage(true, 2).save(QString(OUTPUT_DIRECTORY) + "normal.png");
    deferFbo->toImage(true, 3).save(QString(OUTPUT_DIRECTORY) + "texcoord.png");
    #endif

    // Translucent part.
    dipoleShader->bind();
    dipoleFbo->bind();
    sampleVAO->bind();

    f->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deferFbo->textures()[1]);
    f->glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, deferFbo->textures()[2]);
    f->glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, deferFbo->textures()[3]);

    dipoleShader->setUniformValue("uPositionMap", 0);
    dipoleShader->setUniformValue("uNormalMap",   1);
    dipoleShader->setUniformValue("uTexCoordMap", 2);

    dipoleShader->setUniformValue("uMVPMat", mvpMat);
    dipoleShader->setUniformValue("uMVMat", mvMat);
    dipoleShader->setUniformValue("uLightPos", lightPos);

    dipoleShader->setUniformValue("sigma_a", sigma_a * mtrlScale);
    dipoleShader->setUniformValue("sigmap_s", sigmap_s * mtrlScale);
    dipoleShader->setUniformValue("eta", eta);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDrawElements(GL_POINTS, sampleIBuf->size() / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_DEPTH_TEST);

    dipoleShader->release();
    dipoleFbo->release();
    sampleVAO->release();

    #if DEBUG_MODE
    dipoleFbo->toImage().save(QString(OUTPUT_DIRECTORY) + "dipole.png");
    #endif

    // Main rendering.
    shader->bind();
    vao->bind();

    f->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dipoleFbo->texture());
    shader->setUniformValue("uTransMap", 0);

    shader->setUniformValue("uMVPMat", mvpMat);
    shader->setUniformValue("uMVMat", mvMat);
    shader->setUniformValue("uLightPos", lightPos);

    shader->setUniformValue("refFactor", isRenderRefl ? 1.0f : 0.0f);
    shader->setUniformValue("transFactor", isRenderTrans ? 1.0f : 0.0f);

    glDrawElements(GL_TRIANGLES, iBuffer->size() / sizeof(unsigned int), GL_UNSIGNED_INT, 0);

    shader->release();
    vao->release();
}

void OpenGLViewer::calcGBuffers() {
    static const int bufSize = 1024;
    if (!gbufFbo) {
        gbufFbo = std::make_unique<QOpenGLFramebufferObject>(bufSize, bufSize,
            QOpenGLFramebufferObject::Attachment::Depth, GL_TEXTURE_2D, GL_RGBA32F);
        gbufFbo->addColorAttachment(bufSize, bufSize, GL_RGBA32F);
        gbufFbo->addColorAttachment(bufSize, bufSize, GL_RGBA32F);
        gbufFbo->addColorAttachment(bufSize, bufSize, GL_RGBA32F);
    }

    // Compute G-buffers from the light source. In the following part, 
    // G-buffers except for "Maximum depth" are computed.
    cv::Mat minDepthImage, posImage, normalImage, texcoordImage;
    {
        glViewport(0, 0, bufSize, bufSize);

        gbufShader->bind();
        gbufFbo->bind();
        vao->bind();

        QMatrix4x4 pMat, vMat, mMat;
        pMat.perspective(45.0f, 1.0f, 0.1f, 100.0f);
        vMat.lookAt(lightPos, QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f));
        mMat.scale(7.0f);

        QMatrix4x4 mvpMat = pMat * vMat * mMat;
        gbufShader->setUniformValue("uMVPMat", mvpMat);
        gbufShader->setUniformValue("isMaxDepth", 0);

        auto f = QOpenGLContext::currentContext()->extraFunctions();
        GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                          GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        f->glDrawBuffers(4, bufs);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        f->glClearBufferfv(GL_COLOR, 0, white);

        glDrawElements(GL_TRIANGLES, iBuffer->size() / sizeof(unsigned int), GL_UNSIGNED_INT, 0);

        gbufShader->release();
        gbufFbo->release();
        vao->release();

        takeFloatImage(*gbufFbo.get(), &minDepthImage, 1, 0);
        takeFloatImage(*gbufFbo.get(), &posImage, 3, 1);
        takeFloatImage(*gbufFbo.get(), &normalImage, 3, 2);
        takeFloatImage(*gbufFbo.get(), &texcoordImage, 3, 3);
    }

    // Compute the maximum depth image from the light source.
    cv::Mat maxDepthImage;
    {
        gbufShader->bind();
        gbufFbo->bind();
        vao->bind();

        gbufShader->setUniformValue("isMaxDepth", 1);

        auto f = QOpenGLContext::currentContext()->extraFunctions();
        GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };

        f->glDrawBuffers(1, bufs);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        f->glClearBufferfv(GL_COLOR, 0, white);

        glDrawElements(GL_TRIANGLES, iBuffer->size() / sizeof(unsigned int), GL_UNSIGNED_INT, 0);

        gbufShader->release();
        gbufFbo->release();
        vao->release();

        takeFloatImage(*gbufFbo.get(), &maxDepthImage, 1, 0);
    }

    // Revert viewport.
     glViewport(0, 0, width(), height());

    #if DEBUG_MODE
    saveFloatImage(std::string(OUTPUT_DIRECTORY) + "gbuf_mindepth.png", depthImage);
    saveFloatImage(std::string(OUTPUT_DIRECTORY) + "gbuf_maxdepth.png", maxDepthImage);
    saveFloatImage(std::string(OUTPUT_DIRECTORY) + "gbuf_position.png", posImage);
    saveFloatImage(std::string(OUTPUT_DIRECTORY) + "gbuf_normal.png", normalImage);
    saveFloatImage(std::string(OUTPUT_DIRECTORY) + "gbuf_texcoord.png", texcoordImage);
    #endif
    
    // Compute sample point hierarchy.
    static const int maxPyrLevels = 3;

    std::vector<cv::Mat> minDepthPyr(maxPyrLevels);
    std::vector<cv::Mat> maxDepthPyr(maxPyrLevels);
    std::vector<cv::Mat> positionPyr(maxPyrLevels);
    std::vector<cv::Mat> normalPyr(maxPyrLevels);
    std::vector<cv::Mat> texCoordPyr(maxPyrLevels);

    minDepthPyr[maxPyrLevels - 1] = minDepthImage;
    maxDepthPyr[maxPyrLevels - 1] = maxDepthImage;
    positionPyr[maxPyrLevels - 1] = posImage;
    normalPyr[maxPyrLevels - 1] = normalImage;
    texCoordPyr[maxPyrLevels - 1] = texcoordImage;

    std::function<float(float,float,float,float)> fTakeMin = 
        [&](float f1, float f2, float f3, float f4) {
            return std::min(std::min(f1, f2), std::min(f3, f4));
        };
    std::function<float(float,float,float,float)> fTakeMax =
        [&](float f1, float f2, float f3, float f4) {
            return std::max(std::max(f1, f2), std::max(f3, f4));
        };
    std::function<cv::Vec3f(cv::Vec3f,cv::Vec3f,cv::Vec3f,cv::Vec3f)> fTakeAvg =
        [&](cv::Vec3f v1, cv::Vec3f v2, cv::Vec3f v3, cv::Vec3f v4) {
            return (v1 + v2 + v3 + v4) / 4.0f;
        };

    for (int i = maxPyrLevels - 1; i >= 1; i--) {
        pyrDown(minDepthPyr[i], minDepthPyr[i - 1], fTakeMin);
        pyrDown(maxDepthPyr[i], maxDepthPyr[i - 1], fTakeMax);
        pyrDown(positionPyr[i], positionPyr[i - 1], fTakeAvg);
        pyrDown(normalPyr[i],   normalPyr[i - 1],   fTakeAvg);
        pyrDown(texCoordPyr[i], texCoordPyr[i - 1], fTakeAvg);
    }

    static const double alpha = 30.0;
    static const double Rw = 1.0;
    static const double RPx = 0.1;
    static const double z0 = 0.03;
    double T = 256.0;

    std::vector<cv::Mat> samplePyr(maxPyrLevels);
    for (int l = 0; l < maxPyrLevels; l++) {
        const int subRows = minDepthPyr[l].rows;
        const int subCols = minDepthPyr[l].cols;
        samplePyr[l] = cv::Mat(subRows, subCols, CV_8UC1, cv::Scalar(0, 0, 0));
        for (int i = 0; i < subRows; i++) {
            for (int j = 0; j < subCols; j++) {
                float depthGap = (maxDepthPyr[l].at<float>(i, j) - minDepthPyr[l].at<float>(i, j)) * 10.0f;

                cv::Vec3f pos = positionPyr[l].at<cv::Vec3f>(i, j);
                QVector3D L = QVector3D(lightPos.x() - pos[0], lightPos.y() - pos[1], lightPos.z() - pos[2]).normalized();
                cv::Vec3f nrm = normalPyr[l].at<cv::Vec3f>(i, j);
                QVector3D N = QVector3D(nrm[0], nrm[1], nrm[2]);

                double Mx = alpha * Rw / (RPx * std::abs(QVector3D::dotProduct(N, L)));    

                if (depthGap < z0 && T > Mx) {
                    samplePyr[l].at<uchar>(i, j) = 255;
                } else {
                    samplePyr[l].at<uchar>(i, j) = 0;                                        
                }
            }
        }
        T *= 2.0;
    }

    for (int l = maxPyrLevels - 1; l >= 1; l--) {
        for (int y = 0; y < samplePyr[l].rows; y++) {
            for (int x = 0; x < samplePyr[l].cols; x++) {
                if (samplePyr[l - 1].at<uchar>(y / 2, x / 2) != 0) {
                    samplePyr[l].at<uchar>(y, x) = 0;
                }
            }
        }    
    }

    #if DEBUG_MODE
    cv::imwrite(std::string(OUTPUT_DIRECTORY) + "sample0.png", samplePyr[0]);
    cv::imwrite(std::string(OUTPUT_DIRECTORY) + "sample1.png", samplePyr[1]);
    cv::imwrite(std::string(OUTPUT_DIRECTORY) + "sample2.png", samplePyr[2]); 
    #endif
    
    std::vector<Sample> samples;
    std::vector<unsigned int> sampleIds;
    for (int l = 0; l < maxPyrLevels; l++) {
        for (int y = 0; y < samplePyr[l].rows; y++) {
            for (int x = 0; x < samplePyr[l].cols; x++) {
                if (samplePyr[l].at<uchar>(y, x) != 0) {
                    Sample samp;

                    cv::Vec3f pos = positionPyr[l].at<cv::Vec3f>(y, x);
                    samp.position = QVector3D(pos[0], pos[1], pos[2]);
                    cv::Vec3f nrm = normalPyr[l].at<cv::Vec3f>(y, x);
                    samp.normal   = QVector3D(nrm[0], nrm[1], nrm[2]);
                    cv::Vec3f crd = texCoordPyr[l].at<cv::Vec3f>(y, x);
                    samp.texcoord = QVector2D(crd[0], crd[1]);
                    samp.radius = std::pow(0.5, l);
                    samples.push_back(samp);
                    sampleIds.push_back(sampleIds.size());
                }
            }        
        }
    }

    #if DEBUG_MODE
    std::ofstream ofs((std::string(SLF_OUTPUT_DIRECTORY) + "samples.obj").c_str(), std::ios::out);
    for (const auto& s : samples) {
        ofs << "v ";
        ofs << s.position.x() << " ";
        ofs << s.position.y() << " ";
        ofs << s.position.z() << std::endl;
    }
    ofs.close();
    #endif

    // Prepare sample VAO.
    sampleVAO = std::make_unique<QOpenGLVertexArrayObject>(this);
    sampleVAO->create();
    sampleVAO->bind();

    sampleVBuf = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    sampleVBuf->create();
    sampleVBuf->setUsagePattern(QOpenGLBuffer::StaticDraw);
    sampleVBuf->bind();
    sampleVBuf->allocate(&samples[0], sizeof(Sample) * samples.size());

    auto f = QOpenGLContext::currentContext()->extraFunctions();
    f->glEnableVertexAttribArray(SAMPLE_POSITION_LOC);
    f->glEnableVertexAttribArray(SAMPLE_NORMAL_LOC);
    f->glEnableVertexAttribArray(SAMPLE_TEXCOORD_LOC);
    f->glEnableVertexAttribArray(SAMPLE_RADIUS_LOC);
    f->glVertexAttribPointer(SAMPLE_POSITION_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(Sample), (void*)0);
    f->glVertexAttribPointer(SAMPLE_NORMAL_LOC,   3, GL_FLOAT, GL_FALSE, sizeof(Sample), (void*)(sizeof(float) * 3));
    f->glVertexAttribPointer(SAMPLE_TEXCOORD_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Sample), (void*)(sizeof(float) * 6));
    f->glVertexAttribPointer(SAMPLE_RADIUS_LOC,   1, GL_FLOAT, GL_FALSE, sizeof(Sample), (void*)(sizeof(float) * 8));

    sampleIBuf = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    sampleIBuf->create();
    sampleIBuf->setUsagePattern(QOpenGLBuffer::StaticDraw);
    sampleIBuf->bind();
    sampleIBuf->allocate(&sampleIds[0], sampleIds.size() * sizeof(unsigned int));

    sampleVAO->release();
}

void OpenGLViewer::mousePressEvent(QMouseEvent* ev) {
    // Arcball
    arcball->setOldPoint(ev->pos());
    arcball->setNewPoint(ev->pos());
    if (ev->button() == Qt::LeftButton) {
        arcball->setMode(ArcballMode::Rotate);
    } else if (ev->button() == Qt::RightButton) {
        arcball->setMode(ArcballMode::Translate);
    } else if (ev->button() == Qt::MiddleButton) {
        arcball->setMode(ArcballMode::Scale);
    }    
}

void OpenGLViewer::mouseMoveEvent(QMouseEvent* ev) {
    // Arcball
    arcball->setNewPoint(ev->pos());
    arcball->update();
    arcball->setOldPoint(ev->pos());    
}

void OpenGLViewer::mouseReleaseEvent(QMouseEvent* ev) {
    // Arcball
    arcball->setMode(ArcballMode::None);
}


void OpenGLViewer::wheelEvent(QWheelEvent* ev) {
    arcball->setScroll(arcball->scroll() + ev->delta() / 1000.0);
    arcball->update();
}

void OpenGLViewer::OnAnimate() {
    update();
}
