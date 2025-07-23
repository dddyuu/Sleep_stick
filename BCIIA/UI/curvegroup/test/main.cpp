#include "QApplication"
#include "curvegroup.h"
int main(int args,char **argv)
{
    QApplication app(args,argv);
    CurveGroup w;
    w.resize(640,480);
    w.setMaxPoint(1000);
    w.setCurveNumGroup(4);
    w.setCurveNum(10);
    w.setCurveLabels(QStringList({ "FP","AF","Pf","PF","PG","FP1","AF1","Pf1","PF1","PG1"}));
    w.increaseGroupChannel();
    w.increaseGroupChannel();
    w.test_chart();
    w.show();
    w.appendMark("123");

    return app.exec();
}
