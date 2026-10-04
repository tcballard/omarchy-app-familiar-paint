#include "window.h"
#include "theme.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QThread>
#include <QTextStream>
int main(int argc,char **argv) {
 QApplication app(argc,argv); app.setStyle("Fusion"); applyTheme(&app);
 if(argc!=3) return 2;
 Window window; bool failed=false;
 window.reportError=[&](QString e){QTextStream(stderr)<<e; failed=true;};
 window.resize(1280,850); window.show(); app.processEvents();
 window.open(QString::fromLocal8Bit(argv[1]));
 QElapsedTimer wait; wait.start();
 while(window.busy() && wait.elapsed()<10000) {app.processEvents(); QThread::msleep(5);}
 if(failed || window.busy()) return 1;
 window.canvas()->setZoom(1); window.refresh(); app.processEvents();
 window.capture(QString::fromLocal8Bit(argv[2])); return 0;
}
