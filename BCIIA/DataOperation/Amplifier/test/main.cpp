#include "amplifier.h"
#include "QDebug"
#include "QApplication"
int main(int args,char* *argv)
{

    QApplication app(args,argv);
    Amplifier amplifier;
    amplifier.getPluginWidget()->show();
    return app.exec();
}
