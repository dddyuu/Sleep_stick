#include "mediawidget.h"
#include "QApplication"
int main(int args,char** argv)
{
    QApplication app(args,argv);
    MediaWidget w;
    w.show();
    return app.exec();
}
