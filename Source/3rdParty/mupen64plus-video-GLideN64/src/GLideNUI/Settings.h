#ifndef SETTINGS_H
#define SETTINGS_H

void loadSettings(const QString & _strIniFolder, const QString & _strSharedIniFolder);
void writeSettings(const QString & _strIniFolder);
void resetSettings(const QString & _strIniFolder);
void loadCustomRomSettings(const QString & _strIniFolder, const QString & _strSharedIniFolder, const char * _strRomName);
void saveCustomRomSettings(const QString & _strIniFolder, const QString & _strSharedIniFolder, const char * _strRomName);
QString getTranslationFile();
QStringList getProfiles(const QString & _strIniFolder);
QString getCurrentProfile(const QString & _strIniFolder);
void changeProfile(const QString & _strIniFolder, const QString & _strSharedIniFolder, const QString & _strProfile);
void addProfile(const QString & _strIniFolder, const QString & _strProfile);
void removeProfile(const QString & _strIniFolder, const QString & _strProfile);

#endif // SETTINGS_H

