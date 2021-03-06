// Copyright (c) 2019 The Ring Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Ring-fork: Pop

#ifndef RING_QT_POP_BOARD_H
#define RING_QT_POP_BOARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

#include <crypto/pop/popgame.h>
#include <crypto/pop/game0/game0.h>

#define TILE_THUMB_SIZE 80                      // Thumb size for tile preview pane

class Game0Board : public QWidget
{
	Q_OBJECT
public:
	Game0Board(QWidget* parent = 0);
    bool newGame(int currentTargetScore, const CAvailableGame *newGame, bool updateTime = true, bool restart = false);
    CAvailableGame *getCurrentGame() { return currentGame; }
    void updateCurrentGameInfo(int currentTargetScore, int blocks, bool playTimeWarn = true);

Q_SIGNALS:
	void solutionReady(CAvailableGame *g, uint8_t gameType, std::vector<unsigned char> solution);

protected:
    virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void paintEvent(QPaintEvent*);
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void resizeEvent(QResizeEvent* event);

private:
	void gameOver();
    bool placeTile();
    void rotateTile();
	bool endGame();
    bool winCheck();
    void clampHighlightedTileAndRedraw();   // Helper for keypress handler

    CAvailableGame *currentGame = NULL;

    QLabel *preview;
	QLabel *scoreLabel;
    QLabel *scoreMessage;
    QLabel *gameHashLabel;
    QLabel *timeLeftLabel = NULL;
    QLabel *scoreTargetLabel = NULL;
    QLabel *timeLeftCaptionLabel;
    QPushButton *restartButton;
    QPushButton *submitButton;

    Game0 game;
	QRect background;
    int tileSize;
	int currentTileRotation;
    int highlightedTileX, highlightedTileY;
    int score, targetScore;
	bool done;
    QCheckBox *autoSubmitToggle, *soundToggle, *keysToggle;
    bool autoSubmit, soundsEnabled, keysEnabled;
    QPixmap tileImages[GAME0_NUM_TILES * 4];
    QPixmap tileImageThumbs[GAME0_NUM_TILES];
    QPixmap mouseOverImageOK, mouseOverImageBad, candleImage;

private Q_SLOTS:
    void on_restartButton_clicked();
    void on_submitButton_clicked();
    void on_soundToggle_clicked();
    void on_keysToggle_clicked();
    void on_autoSubmitToggle_clicked();

Q_SIGNALS:
    void hideMessage();
	void showMessage(const QString& message);
};

#endif // RING_QT_POP_BOARD_H
