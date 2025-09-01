#include <QCoreApplication>
#include "matread.h"
#include "selfamplifer.h"
#include "QDebug"
#include "QObject"
#include "QApplication"
#include <QTimer>
#include "matio.h"
class Test :public QObject
{
public:
    Test() {
        selfamplifer.start();
        startTimer(200);
        //brain_heart.getLocalData();
    };
    ~Test() {};
    void show()
    {
        selfamplifer.getConnectWidget()->show();
    };
    void Test::timerEvent(QTimerEvent* event) override
    {


        QList<QList<double>> EEG_ = selfamplifer.getChartData();
        if (!EEG_.isEmpty())
        {
            qDebug() << EEG_;
        }
        else
        {
            qDebug() << "no data";
        }
        selfamplifer.incrementB();
    }

private:
    SelfAmplifer selfamplifer;
    MatRead matread;

};
int main(int args, char** argv)
{
    QApplication app(args, argv);
    Test test;
    test.show();
    return app.exec();
}
