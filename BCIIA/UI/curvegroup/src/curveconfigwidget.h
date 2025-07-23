#ifndef CurveConfigWidget_H
#define CurveConfigWidget_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class CurveConfigWidget; }
QT_END_NAMESPACE

class CurveConfigWidget : public QWidget
{
    Q_OBJECT

public:
    CurveConfigWidget(QWidget *parent = nullptr);
    ~CurveConfigWidget();

private:
    Ui::CurveConfigWidget *ui;
};
#endif // CurveConfigWidget_H
