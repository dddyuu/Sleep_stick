# CurveGroup

## 测试

```
#include "QApplication"
#include "curvegroup.h"
int main(int args,char **argv)
{
    QApplication app(args,argv);
    CurveGroup w;
    w.resize(640,480);
    w.setMaxPoint(150);//设置显示的采样点数目
    w.setMinMax(-3,3);//设置数据的范围
    w.setCurveNumGroup(5);//设置每页的显示曲线数目
    w.setCurveNum(10);//设置总曲线数目
    //设置标签
    w.setCurveLabels(QStringList({ "FP","AF","Pf","PF","PG","FP1","AF1","Pf1","PF1","PG1"}));
    w.test_chart();
    w.show();

    return app.exec();
}
```

next()：下一页

last()：上一页

## 2023.2.27修改日志

1.添加上下页切换按钮

2.修改切换页面导致放大曲线出现错误的bug

## 2023.3.14修改日志

1.增加修改放大曲线的纵坐标范围

2.修改曲线的对于y的自适应
