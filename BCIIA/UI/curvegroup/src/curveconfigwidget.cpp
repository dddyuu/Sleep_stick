#include "curveconfigwidget.h"
#include "ui_curveconfigwidget.h"

CurveConfigWidget::CurveConfigWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CurveConfigWidget)
{
    ui->setupUi(this);
}

CurveConfigWidget::~CurveConfigWidget()
{
    delete ui;
}

