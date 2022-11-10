#include "trkbutton.h"

#include "mainwindow.h"

TrkButton::TrkButton(const QString &text, const trklist_item_t &trk, MainWindow *parent)
    : QPushButton{text, parent},
      m_trk(trk),
      m_wnd(parent)
{
    setFixedWidth(200);
    connect(this, &QPushButton::clicked, this, &TrkButton::onClicked);
}

void TrkButton::onClicked()
{
    m_wnd->trkView(m_trk);
}
