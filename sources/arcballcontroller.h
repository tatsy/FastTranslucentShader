#ifndef _ARCBALL_CONTROLLER_H_
#define _ARCBALL_CONTROLLER_H_

#include <QtWidgets/qwidget.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qmatrix4x4.h>

enum class ArcballMode : int {
    None = 0x00,
    Translate = 0x01,
    Rotate = 0x02,
    Scale = 0x04
};

class ArcballController {
public:
    // Public methods
    ArcballController(QWidget* parent);

    void initModelView(const QMatrix4x4& mMat, const QMatrix4x4& vMat);
    
    void update();
    
    inline QMatrix4x4 modelMat() const { return modelMat_; }
    inline QMatrix4x4 viewMat() const { return viewMat_; }
    inline QMatrix4x4 modelViewMat() const { return viewMat_ * modelMat_; }
    
    inline double scroll() const { return scroll_; }
    
    inline void setMode(ArcballMode mode) { mode_ = mode; }
    inline void setOldPoint(const QPoint& pos) { oldPoint_ = pos; }
    inline void setNewPoint(const QPoint& pos) { newPoint_ = pos; }
    inline void setScroll(double scroll) { scroll_ = scroll; }
    
private:
    // Private methods
    QVector3D getVector(int x, int y) const;
    void updateTranslate();
    void updateRotate();
    void updateScale();
    
    // Private parameters
    QWidget* parent_;
    QMatrix4x4 modelMat_;
    QMatrix4x4 viewMat_;
    double scroll_ = 0.0;
    QPoint oldPoint_ = QPoint(0, 0);
    QPoint newPoint_ = QPoint(0, 0);
    
    ArcballMode mode_ = ArcballMode::None;
    QVector3D translate_ = QVector3D(0.0f, 0.0f, 0.0f);
    QMatrix4x4 lookMat_;
    QMatrix4x4 rotMat_;
};

#endif  // _ARCBALL_CONTROLLER_H_
