#ifndef ROMBROWSERWIDGET_HPP
#define ROMBROWSERWIDGET_HPP

#include <Thread/RomSearcherThread.hpp>
#include <Mupen/Mupen.hpp>

#include <QTableView> 
#include <QStandardItemModel>
#include <QHeaderView>
#include <QString>
#include <QList>

#define MAX_ROM_INFO 50

class RomBrowserWidget: public  QTableView
{
    Q_OBJECT

public:
    RomBrowserWidget(void);
    ~RomBrowserWidget(void);

    void RefreshRomList(void);

private:
    QStandardItemModel* model_Model;
    void model_Init(void);
    void model_Setup(void);

    void widget_Init(void);

    RomSearcherThread* rom_Searcher_Thread;
    bool rom_Searcher_Recursive;
    bool rom_Searcher_MaxItems;
    bool rom_Searcher_Running;

    m64p_rom_info rom_List[MAX_ROM_INFO];
    int rom_List_Index = 0;

    bool rom_List_Recursive;
    bool rom_List_Fill_Thread_Init;
    bool rom_List_Fill_Thread_Running;
    void rom_List_Init();
    void rom_List_Fill(QString);



public slots:
    void on_RomBrowserThread_Received(m64p_rom_info info);
    void on_RomBrowserThread_Finished(void);
};

#endif // ROMBROWSERWIDGET_HPP
