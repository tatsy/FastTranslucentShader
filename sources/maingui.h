#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MAIN_GUI_H_
#define _MAIN_GUI_H_

#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qboxlayout.h>

#include "openglviewer.h"

class MainGui : public QMainWindow {
    Q_OBJECT

public:
    explicit MainGui(QWidget* parent = nullptr);
    virtual ~MainGui();

private slots:
    void OnRadioToggled(bool);
    void OnScaleChanged();
    void OnCheckStateChanged(int);
    void OnFrameSwapped();

private:
    QHBoxLayout*  mainLayout = nullptr;
    QWidget*      mainWidget = nullptr;
    OpenGLViewer* viewer = nullptr;

    class Ui;
    Ui* ui = nullptr;
};

#endif  // _MAIN_GUI_H_
