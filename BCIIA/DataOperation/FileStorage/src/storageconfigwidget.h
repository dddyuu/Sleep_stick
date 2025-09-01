#ifndef StorageConfigWidget_H
#define StorageConfigWidget_H

#include <QWidget>
#include <QToolBox>
#include <QFileDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class StorageConfigWidget; }
QT_END_NAMESPACE

class StorageConfigWidget : public QWidget
{
    Q_OBJECT

public:
    StorageConfigWidget(QWidget *parent = nullptr);
    ~StorageConfigWidget();

private slots:
    void on_pieceRadioButton_clicked();

    void on_allRadioButton_clicked();

    void on_chockFAddButton_clicked();

private:
    Ui::StorageConfigWidget *ui;
};
#endif // StorageConfigWidget_H
