// Ring-fork: Hexagon pie :)

#include <qt/tinypie.h>
#include <qt/uicolours.h>

TinyPie::TinyPie(QWidget *parent) : QWidget(parent) {
    normalisedVal = 0;
    foregroundCol = QColor(247, 213, 33);
    backgroundCol = QColor(42, 182, 67);
}

void TinyPie::paintEvent(QPaintEvent *event) {
    QPainter painter;
    painter.begin(this);
    painter.setRenderHints(QPainter::Antialiasing, true);

    // Background
    painter.setPen(QPen(backgroundCol, 1));
    painter.setBrush(backgroundCol);
    painter.drawEllipse(3, 3, width() - 6, height() - 6);

    // Filled slice
    if (normalisedVal > 0) {
        double localVal = normalisedVal;
        if (localVal > 1.0)
            localVal = 1.0;
        painter.setPen(QPen(foregroundCol, 1));
        painter.setBrush(foregroundCol);
        painter.drawPie(3, 3, width() - 6, height() - 6, 90 * 16, -localVal * 16 * 360);
    }

    QWidget::paintEvent(event);
    painter.end();
}
