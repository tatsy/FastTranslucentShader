#include "maingui.h"

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qgroupbox.h>
#include <QtWidgets/qradiobutton.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>
#include <QtWidgets/qcheckbox.h>
#include <QtCore/qelapsedtimer.h>

class MainGui::Ui : public QWidget {
public:
    Ui(QWidget* parent = nullptr)
        : QWidget{ parent } {
        layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignTop);
        setLayout(layout);

        mtrlGroup = new QGroupBox(this);
        mtrlGroup->setTitle("Materials");

        groupLayout = new QVBoxLayout(this);
        mtrlGroup->setLayout(groupLayout);
        milkRadio = new QRadioButton(this);
        milkRadio->setText("Milk");
        milkRadio->setChecked(true);
        skinRadio = new QRadioButton(this);
        skinRadio->setText("Skin");

        groupLayout->addWidget(milkRadio);
        groupLayout->addWidget(skinRadio);
        layout->addWidget(mtrlGroup);

        scaleLabel = new QLabel("Scale", this);
        layout->addWidget(scaleLabel);
        scaleEdit = new QLineEdit(this);
        scaleEdit->setText("50.0");
        layout->addWidget(scaleEdit);

        reflCheckBox = new QCheckBox("Reflection", this);
        reflCheckBox->setChecked(true);
        layout->addWidget(reflCheckBox);
        transCheckBox = new QCheckBox("Transmission", this);
        transCheckBox->setChecked(true);
        layout->addWidget(transCheckBox);
    }

    ~Ui() {
        delete milkRadio;
        delete skinRadio;
        delete groupLayout;
        delete scaleLabel;
        delete scaleEdit;
        delete mtrlGroup;
        delete reflCheckBox;
        delete transCheckBox;
        delete layout;
    }

    QRadioButton* milkRadio = nullptr;
    QRadioButton* skinRadio = nullptr;
    QGroupBox*    mtrlGroup = nullptr;
    QVBoxLayout*  groupLayout = nullptr;
    QLabel*       scaleLabel = nullptr;
    QLineEdit*    scaleEdit = nullptr;
    QCheckBox*    reflCheckBox  = nullptr;
    QCheckBox*    transCheckBox = nullptr;
    QVBoxLayout*  layout = nullptr;
};

MainGui::MainGui(QWidget* parent)
    : QMainWindow{ parent } {
    setFont(QFont("Meiryo UI"));

    mainLayout = new QHBoxLayout();
    mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);
    setCentralWidget(mainWidget);
    
    viewer = new OpenGLViewer(this);
    QSizePolicy viewPolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    viewPolicy.setHorizontalStretch(3);
    viewer->setSizePolicy(viewPolicy);
    mainLayout->addWidget(viewer);

    ui = new Ui(this);
    QSizePolicy uiPolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    uiPolicy.setHorizontalStretch(1);
    ui->setSizePolicy(uiPolicy);
    mainLayout->addWidget(ui);

    connect(ui->milkRadio, SIGNAL(toggled(bool)), this, SLOT(OnRadioToggled(bool)));
    connect(ui->skinRadio, SIGNAL(toggled(bool)), this, SLOT(OnRadioToggled(bool)));
    connect(ui->scaleEdit, SIGNAL(editingFinished()), this, SLOT(OnScaleChanged()));

    connect(ui->reflCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnCheckStateChanged(int)));
    connect(ui->transCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnCheckStateChanged(int)));
    connect(viewer, SIGNAL(frameSwapped()), this, SLOT(OnFrameSwapped()));
}

MainGui::~MainGui() {
    delete viewer;
    delete ui;
    delete mainLayout;
    delete mainWidget;
}

void MainGui::OnRadioToggled(bool state) {
    if (ui->milkRadio->isChecked()) {
        viewer->setMaterial("Milk");
    } else if (ui->skinRadio->isChecked()) {
        viewer->setMaterial("Skin");
    }
}

void MainGui::OnScaleChanged() {
    viewer->setMaterialScale(ui->scaleEdit->text().toDouble());
}

void MainGui::OnCheckStateChanged(int state) {
    bool isRefl  = ui->reflCheckBox->isChecked();
    bool isTrans = ui->transCheckBox->isChecked();
    viewer->setRenderComponents(isRefl, isTrans);
}

void MainGui::OnFrameSwapped() {
    static bool isStarted = false;
    static QElapsedTimer timer;
    static long long lastTime;
    if (!isStarted) {
        isStarted = true;
        timer.start();
        lastTime = timer.elapsed();
    } else {
        long long currentTime = timer.elapsed();
        double fps = 1000.0 / (currentTime - lastTime);
        setWindowTitle(QString("FPS: %1").arg(QString::number(fps, 'f', 2)));
        lastTime = currentTime;
    }
}