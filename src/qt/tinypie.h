// LitecoinCash: Hexagon pie :)

#ifndef RING_QT_TINYPIE_H
#define RING_QT_TINYPIE_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>

class TinyPie : public QWidget
{
public:
    QColor foregroundCol;
    QColor backgroundCol;

    explicit TinyPie(QWidget *parent=0);

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void setValue(const double val) {
        normalisedVal = val;
        update();
    }

private:
    double normalisedVal;
};

#endif
