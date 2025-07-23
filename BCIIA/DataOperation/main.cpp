#include "dataoperation.h"
#include "QApplication"
int main(int args,char **argv)
{
	QApplication app(args, argv);
	DataOperation dataoperation;
	dataoperation.getPluginWidget()->show();
	return app.exec();
}