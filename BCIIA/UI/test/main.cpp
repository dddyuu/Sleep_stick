#include "QApplication"
#include "bcimonitor.h"
int main(int args, char** argv)
{
	QApplication app(args, argv);
	BCIMonitor w;
	w.show();
	return app.exec();
}