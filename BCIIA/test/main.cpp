#include "QApplication"
#include "bciia.h"
int main(int args, char** argv)
{
	QApplication app(args, argv);
	BCIIA bciia;
	bciia.getMonitorWidget()->show();
	bciia.reciveSubName("123");
	return app.exec();
}