// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Hive

#ifndef RING_QT_RIALTODIALOG_H
#define RING_QT_RIALTODIALOG_H

#include <qt/guiutil.h>

#include <QDialog>

class PlatformStyle;

namespace Ui {
    class RialtoDialog;
}

class RialtoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RialtoDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~RialtoDialog();

public Q_SLOTS:
Q_SIGNALS:

private:
    Ui::RialtoDialog *ui;
    const PlatformStyle *platformStyle;
    virtual void resizeEvent(QResizeEvent *event);

private Q_SLOTS:
};

#endif // RING_QT_RIALTODIALOG_H
