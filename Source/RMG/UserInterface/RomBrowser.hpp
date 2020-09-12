#ifndef ROMBROWSER_HPP
#define ROMBROWSER_HPP

#include <QTableView> 
#include <QStandardItemModel>
#include <QHeaderView>
#include "../Mupen/Mupen.hpp"
#include <QString>
#include <QList>

struct m64p_rom_info
{
    QString file;
    m64p_rom_settings m64p_settings;
    m64p_rom_header m64p_header;
};

#define MAX_ROM_INFO 50

class RomBrowser: public  QTableView
{
    Q_OBJECT

public:
    RomBrowser(Mupen*);
    ~RomBrowser();

    void RefreshRomList(void);

private:
    Mupen* m64p; 

    QStandardItemModel* model_Model;
    void model_Init(void);
    void model_Setup(void);

    void widget_Init(void);

    m64p_rom_info rom_List[MAX_ROM_INFO];
    int rom_List_Index = 0;

    bool rom_List_Recursive;
    void rom_List_Fill(QString);
    bool rom_Get_Info(QString, m64p_rom_info*);
};

#endif // ROMBROWSER_HPP
