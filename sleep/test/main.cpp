#include "mathgame.h"
#include "QApplication"
int main(int args,char **argv)
{
    QApplication app(args,argv);
    MathGame w;
    w.show();
    return  app.exec();
}
