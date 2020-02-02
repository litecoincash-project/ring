// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Rialto

#include <qt/rialtodialog.h>
#include <qt/forms/ui_rialtodialog.h>

RialtoDialog::RialtoDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RialtoDialog),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);
}

RialtoDialog::~RialtoDialog() {
    delete ui;
}

void RialtoDialog::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}
