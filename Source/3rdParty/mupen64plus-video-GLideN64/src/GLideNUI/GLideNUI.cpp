#include <thread>
#include <QApplication>
#include <QTranslator>

#include "GLideNUI.h"
#include "AboutDialog.h"
#include "ConfigDialog.h"
#include "Settings.h"
#include "../Config.h"

#ifdef QT_STATICPLUGIN
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
#endif

//#define RUN_DIALOG_IN_THREAD

inline void initMyResource() { Q_INIT_RESOURCE(icon); }
inline void cleanMyResource() { Q_CLEANUP_RESOURCE(icon); }

static
int openConfigDialog(const wchar_t * _strFileName, const wchar_t * _strSharedFileName, const char * _romName, unsigned int _maxMSAALevel, float _maxAnisotropy, bool & _accepted)
{
	cleanMyResource();
	initMyResource();
	QString strIniFileName = QString::fromWCharArray(_strFileName);
	QString strSharedIniFileName = QString::fromWCharArray(_strSharedFileName);
	loadSettings(strIniFileName, strSharedIniFileName);
	if (config.generalEmulation.enableCustomSettings != 0 && _romName != nullptr && strlen(_romName) != 0)
		loadCustomRomSettings(strIniFileName, strSharedIniFileName, _romName);

	int argc = 1;
	char argv0[] = "GLideN64";
	char * argv[] = { argv0 };
	std::unique_ptr<QApplication> pQApp;
	QCoreApplication* pApp = QCoreApplication::instance();

	if (pApp == nullptr) {
		pQApp.reset(new QApplication(argc, argv));
		pApp = pQApp.get();
	}

	QTranslator translator;
	if (translator.load(getTranslationFile(), strSharedIniFileName))
		pApp->installTranslator(&translator);

	ConfigDialog w(Q_NULLPTR, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint, _maxMSAALevel, _maxAnisotropy);

	w.setIniPath(strIniFileName, strSharedIniFileName);
	w.setRomName(_romName);
	w.setTitle();
	w.show();

	int res = pQApp ? pQApp->exec() : w.exec();
	_accepted = w.isAccepted();
	return res;
}

static
int openAboutDialog(const wchar_t * _strFileName)
{
	cleanMyResource();
	initMyResource();

	int argc = 1;
	char argv0[] = "GLideN64";
	char * argv[] = { argv0 };
	QApplication a(argc, argv);

	QTranslator translator;
	if (translator.load(getTranslationFile(), QString::fromWCharArray(_strFileName)))
		a.installTranslator(&translator);

	AboutDialog w(Q_NULLPTR, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
	w.show();
	return a.exec();
}

bool runConfigThread(const wchar_t * _strFileName, const wchar_t * _strSharedFileName, const char * _romName, unsigned int _maxMSAALevel, unsigned int _maxAnisotropy) {
	bool accepted = false;
#ifdef RUN_DIALOG_IN_THREAD
	std::thread configThread(openConfigDialog, _strFileName, _strSharedFileName, _maxMSAALevel, std::ref(accepted));
	configThread.join();
#else
	openConfigDialog(_strFileName, _strSharedFileName, _romName, _maxMSAALevel, _maxAnisotropy, accepted);
#endif
	return accepted;

}

int runAboutThread(const wchar_t * _strFileName) {
#ifdef RUN_DIALOG_IN_THREAD
	std::thread aboutThread(openAboutDialog, _strFileName);
	aboutThread.join();
#else
	openAboutDialog(_strFileName);
#endif
	return 0;
}

EXPORT bool CALL RunConfig(const wchar_t * _strFileName, const wchar_t * _strUserFileName, const char * _romName, unsigned int _maxMSAALevel, unsigned int _maxAnisotropy)
{
	return runConfigThread(_strFileName, _strUserFileName, _romName, _maxMSAALevel, _maxAnisotropy);
}

EXPORT int CALL RunAbout(const wchar_t * _strFileName)
{
	return runAboutThread(_strFileName);
}

EXPORT void CALL LoadConfig(const wchar_t * _strFileName, const wchar_t * _strSharedFileName)
{
	loadSettings(QString::fromWCharArray(_strFileName), QString::fromWCharArray(_strSharedFileName));
}

EXPORT void CALL LoadCustomRomSettings(const wchar_t * _strFileName, const wchar_t * _strSharedFileName, const char * _romName)
{
	loadCustomRomSettings(QString::fromWCharArray(_strFileName), QString::fromWCharArray(_strSharedFileName), _romName);
}
