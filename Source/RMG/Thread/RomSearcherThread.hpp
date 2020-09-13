#ifndef ROMSEARCHERTHREAD_HPP
#define ROMSEARCHERTHREAD_HPP

#include <Mupen/Mupen.hpp>

#include <QThread>
#include <QString>

struct m64p_rom_info
{
    QString file;
    m64p_rom_settings m64p_settings;
    m64p_rom_header m64p_header;
};

class RomSearcherThread : public QThread
{
    Q_OBJECT

public:
    RomSearcherThread(void);

    void SetDirectory(QString);
    void SetRecursive(bool);
    void SetMaximumFiles(int);

    void run() override;

private:
    QString rom_Directory;
    bool rom_Search_Recursive;
    int rom_Search_MaxItems;
    int rom_Search_Count;

    void rom_Search(QString);
    bool rom_Get_Info(QString, m64p_rom_info*);
    
signals:
    void on_Rom_Found(m64p_rom_info);
};


#endif // ROMSEARCHERTHREAD_HPP
