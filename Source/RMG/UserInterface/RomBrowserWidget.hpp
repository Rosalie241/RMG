#ifndef ROMBROWSERWIDGET_HPP
#define ROMBROWSERWIDGET_HPP

#include "../Thread/RomSearcherThread.hpp"
#include "../Globals.hpp"

#include <QTableView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QString>
#include <QList>

#define MAX_ROM_INFO 50

namespace UserInterface
{
    class RomBrowserWidget : public QTableView
    {
        Q_OBJECT

    public:
        RomBrowserWidget(void);
        ~RomBrowserWidget(void);

        void RefreshRomList(void);

    private:
        QStandardItemModel *model_Model;
        QStringList model_LabelList;
        void model_Init(void);
        void model_Setup(void);
        void model_LabelList_Setup(void);

        void widget_Init(void);

        Thread::RomSearcherThread *rom_Searcher_Thread;
        bool rom_Searcher_Recursive;
        bool rom_Searcher_MaxItems;
        bool rom_Searcher_Running;

        void rom_Searcher_Init(void);

        M64P::Wrapper::RomInfo_t rom_List[MAX_ROM_INFO];
        int rom_List_Index = 0;

        bool rom_List_Recursive;
        bool rom_List_Fill_Thread_Running;
        void rom_List_Init();
        void rom_List_Fill(QString);

        void column_SetSize();

    public slots:
        void on_RomBrowserThread_Received(M64P::Wrapper::RomInfo_t info);
        void on_RomBrowserThread_Finished(void);
    };
} // namespace UserInterface

#endif // ROMBROWSERWIDGET_HPP
