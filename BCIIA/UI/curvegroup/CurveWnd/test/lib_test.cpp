#include "curvewnd.h"
#include "QApplication"
int main(int args,char **argv)
{
    QApplication app(args,argv);
    CurveWnd curvewnd;
    //设置图表大小
    curvewnd.resize(640,480);
    //设置采样点
    //设置曲线数目
    curvewnd.setCurveNum(5);
    //设置曲线标签
    curvewnd.setCurveLabels(QStringList({ "FP","AF","Pf","PF","PG"}));
//    curvewnd.appendMark("123");
    curvewnd.show();
    curvewnd.test_chart();
    //测试数据
    return app.exec();
}
