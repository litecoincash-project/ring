// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RING_QT_RINGADDRESSVALIDATOR_H
#define RING_QT_RINGADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class RingAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RingAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Ring address widget validator, checks for a valid ring address.
 */
class RingAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit RingAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // RING_QT_RINGADDRESSVALIDATOR_H
