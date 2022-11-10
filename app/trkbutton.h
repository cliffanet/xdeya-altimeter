#ifndef TRKBUTTON_H
#define TRKBUTTON_H

#include <QPushButton>
#include "nettypes.h"

class MainWindow;

class TrkButton : public QPushButton
{
    Q_OBJECT
public:
    explicit TrkButton(const QString &text, const trklist_item_t &trk, MainWindow *parent = nullptr);

private slots:
    void onClicked();

private:
    trklist_item_t m_trk;
    MainWindow *m_wnd;
};

#endif // TRKBUTTON_H
