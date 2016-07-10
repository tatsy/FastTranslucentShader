#include "arcballcontroller.h"

#include <cmath>

#include <qdebug.h>

static const double PI = 4.0 * std::atan(1.0);

ArcballController::ArcballController(QWidget* parent)
    : parent_(parent) {
}

void ArcballController::initModelView(const QMatrix4x4 &mMat, const QMatrix4x4 &vMat) {
    rotMat_ = mMat;
    lookMat_ = vMat;
    update();
}

void ArcballController::update() {
    switch (mode_) {
    case ArcballMode::Translate:
        updateTranslate();
        break;
    
    case ArcballMode::Rotate:
        updateRotate();
        break;
        
    case ArcballMode::Scale:
        updateScale();
        break;

    default:
        break;
    }
    
    modelMat_ = rotMat_;
    modelMat_.scale(1.0 - scroll_ * 0.1);
    
    viewMat_ = lookMat_;
    viewMat_.translate(translate_);
}

void ArcballController::updateTranslate() {
    const QVector4D u(1.0f, 0.0f, 0.0f, 0.0f);
    const QVector4D v(0.0f, 1.0f, 0.0f, 0.0f);
    const QMatrix4x4 camera2objMat = lookMat_.inverted();
    
    const QVector3D objspaceU = (camera2objMat * u).toVector3D().normalized();
    const QVector3D objspaceV = (camera2objMat * v).toVector3D().normalized();

    const double dx = 10.0 * (newPoint_.x() - oldPoint_.x()) / parent_->width();
    const double dy = 10.0 * (newPoint_.y() - oldPoint_.y()) / parent_->height();

    translate_ += (objspaceU * dx - objspaceV * dy);
}

void ArcballController::updateRotate() {
    const QVector3D u = getVector(newPoint_.x(), newPoint_.y());
    const QVector3D v = getVector(oldPoint_.x(), oldPoint_.y());
    
    const double angle = std::acos(std::min(1.0f, QVector3D::dotProduct(u, v)));
    
    const QVector3D rotAxis = QVector3D::crossProduct(v, u);
    const QMatrix4x4 camera2objMat = rotMat_.inverted();
    
    const QVector3D objSpaceRotAxis = camera2objMat * rotAxis;
    
    QMatrix4x4 temp;
    double angleByDegree = 180.0 * angle / PI;
    temp.rotate(4.0 * angleByDegree, objSpaceRotAxis);
    
    rotMat_ = rotMat_ * temp;
}

void ArcballController::updateScale() {
    const double dy = 20.0 * (newPoint_.y() - oldPoint_.y()) / parent_->height();
    scroll_ += dy;
}

QVector3D ArcballController::getVector(int x, int y) const {
    QVector3D pt( 2.0 * x / parent_->width()  - 1.0,
                 -2.0 * y / parent_->height() + 1.0,
                  0.0);

    const double xySquared = pt.x() * pt.x() + pt.y() * pt.y();
    if (xySquared) {
        pt.setZ(std::sqrt(1.0 - xySquared));
    } else {
        pt.normalized();
    }
    
    return pt;
}
