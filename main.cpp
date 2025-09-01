#include "platformwidget.h"
#include "QApplication"
//#include "QBreakpadHandler.h"
int main(int args,char **argv)
{
   // QBreakpadInstance.setDumpPath("E:\\qt5_cmake_pro\\dump_dir"); // 设置生成dump文件路径
	QApplication app(args, argv);
	PlatFormWidget w;
	w.showMaximized();
	return app.exec();
}
