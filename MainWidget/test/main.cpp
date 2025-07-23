#include "mainwidget.h"
#include "QApplication"
int main(int args,char** argv)
{
    QApplication app(args,argv);
    MainWidget w;
    w.show();
    app.exec();
}
