#include "QApplication"
#include "mishow.h"
int main(int args,char** argv)
{
    QApplication app(args,argv);
    MIShow mishow;
    mishow.show();

    return app.exec();
}
