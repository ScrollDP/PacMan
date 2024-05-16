#include <QApplication>
#include <QWidget>
#include <QKeyEvent>
#include <QPainter>
#include <QStringList>
#include <QTimer>
#include <QFontMetrics>
#include <QRandomGenerator>
#include <cmath>
#include <iostream>
#include <queue>

class PacMan : public QWidget {
public:
    explicit PacMan(QWidget *parent = nullptr) : QWidget(parent), speed(250), direction(0, 0), gameStarted(false) {
        setFixedSize(860, 360); //Nastaví fixnú veľkosť okna
        timer = new QTimer(this); //Vytvorí časovač pre plynulý pohyb
        connect(timer, &QTimer::timeout, this, &PacMan::movePacMan); //Pripojí signál timeout časovača na slot movePacMan
        pacmanX = 11; // Nastaví počiatočnú pozíciu X (riadok) Pacmana
        pacmanY = 21; // Nastaví počiatočnú pozíciu Y (stĺpec) Pacmana

        //Inicializuje duchov
        for (int i = 0; i < numGhosts; ++i) {
            ghosts.append(Ghost());
            ghosts[i].row = 7;
            ghosts[i].col = 19 + i; // Umiestni každého ducha horizontálne oddelene
            ghosts[i].color = QColor::fromHsv(i * 60 + 180, 255, 255); // Priradí každému duchovi inú farbu (okrem žltej)
            ghosts[i].direction = QPoint(1, 0); // Nastaví počiatočný smer pre každého ducha
            ghosts[i].target = QPoint(-1, -1); // Inicializuje cieľovú pozíciu ako neplatnú
        }
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        switch (event->key()) {
            case Qt::Key_W:
                direction = {-1, 0}; //Nastaví smer nahor
                break;
            case Qt::Key_S:
                direction = {1, 0}; //Nastaví smer dolu
                break;
            case Qt::Key_A:
                direction = {0, -1}; //Nastaví smer doľava
                break;
            case Qt::Key_D:
                direction = {0, 1}; //Nastaví smer doprava
                break;
            case Qt::Key_X:
                startGame();    //Spustí hru
                break;
            default:
                QWidget::keyPressEvent(event);
        }
    }

    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Nakreslí steny
        for (int i = 0; i < map.size(); ++i) {
            for (int j = 0; j < map[i].size(); ++j) {
                if (map[i][j] == '#') {
                    painter.fillRect(j * blockSize, i * blockSize, blockSize, blockSize, Qt::lightGray);
                }
            }
        }

        painter.setBrush(Qt::yellow); // Nastaví farbu Pacmana
        painter.drawEllipse(pacmanY * blockSize, pacmanX * blockSize, blockSize, blockSize); //Nakreslí PacMan

        // Nakreslí mince
        painter.setBrush(Qt::yellow); // Nastaví farbu mince
        for (const QPoint& coin : coins) {
            painter.drawEllipse(coin.y() * blockSize + blockSize / 4, coin.x() * blockSize + blockSize / 4, blockSize / 2, blockSize / 2);
        }

        // Nakreslí duchov
        for (const Ghost& ghost : ghosts) {
            painter.setBrush(ghost.color); // Nastaví farbu ducha
            painter.drawEllipse(ghost.col * blockSize, ghost.row * blockSize, blockSize, blockSize);
        }

        //Nakreslí "Stlačte X pre začiatok", ak hra ešte nezačala
        if (!gameStarted) {
            QFont font = painter.font();
            font.setPixelSize(20);
            painter.setFont(font);
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance("Press X to start");
            int textHeight = fm.height();
            painter.drawText((width() - textWidth) / 2, (height() + textHeight) / 2, "Press X to start");
        }
        // Nakreslí správu "You won!", ak boli všetky mince zbierané
        if(gameStarted){

            if (coins.isEmpty()) {
                qDebug()<<"Game won over";
                qDebug() <<coins.empty();
                QFont font = painter.font();
                font.setPixelSize(20);
                painter.setFont(font);
                QFontMetrics fm(font);
                int textWidth = fm.horizontalAdvance("You won!");
                int textHeight = fm.height();
                painter.drawText((width() - textWidth) / 2, ((height() + textHeight) / 2)+1, "You won!");
                qDebug() << "You won!";
                qDebug() << coins.count();
                resetGame(); // Resetuje hru, ak boli všetky mince zbierané
            }
        }

    }

private:
    struct Ghost {
        int row{};
        int col{};
        QColor color;
        QPoint direction; // Smer pohybu ducha
        QPoint target; //Cieľová pozícia pre ducha

    };

    void movePacMan() {
        int newX = pacmanX + direction.x();
        int newY = pacmanY + direction.y();

        //Kontroluje, či nová pozícia je v hraniciach mapy a nie je to stena
        if (newX >= 0 && newX < map.size() && newY >= 0 && newY < map[0].size() && map[newX][newY] != '#') {
            // Kontroluje, či PacMan zbiera mincu
            for (int i = 0; i < coins.size(); ++i) {
                if (coins[i] == QPoint(newX, newY)) {
                    coins.removeAt(i); //Odstráni zbieranú mincu
                    break;
                }
            }
            pacmanX = newX;
            pacmanY = newY;
        } else {
            //Ak je nová pozícia stena alebo mimo hraníc mapy, PacMan sa nepohne
            qDebug()<<"Game Over";
            timer->stop(); // Zastaví hru
            resetGame();
        }

        moveGhosts(); // Pohne duchmi po pohybe Pacmana
        checkCollision(); // Skontroluje zrážku po pohyboch Pacmana a duchov
        update();
    }
// Define the moveInDirection function to allow ghosts to move randomly
    void moveInDirection(Ghost& ghost) {
        int randDir = QRandomGenerator::global()->bounded(4); // Generate random direction (0 - up, 1 - down, 2 - left, 3 - right)
        switch (randDir) {
            case 0:
                ghost.direction = {-1, 0}; // Up
                break;
            case 1:
                ghost.direction = {1, 0}; // Down
                break;
            case 2:
                ghost.direction = {0, -1}; // Left
                break;
            case 3:
                ghost.direction = {0, 1}; // Right
                break;
        }
        // Move ghost in the selected direction
        int newRow = ghost.row + ghost.direction.x();
        int newCol = ghost.col + ghost.direction.y();
        if (map[newRow][newCol] != '#') {
            ghost.row = newRow;
            ghost.col = newCol;
        }
    }
    void checkCollision() {
        // Check collision between PacMan and ghosts
        for (const Ghost& ghost : ghosts) {
            if (pacmanX == ghost.row && pacmanY == ghost.col) {
                timer->stop(); // Stop the game
                qDebug() << "pacmanX and pacmanY" << pacmanX <<pacmanY;
                qDebug() << "ghostrow and ghostcol" << ghost.row <<ghost.col;
                qDebug() << "Game Over - PacMan caught by a ghost!";
                resetGame();
            }
        }

    }


    void startGame() {
        if (!gameStarted) {
            gameStarted = true;
            // Set a default direction for PacMan to start moving right
            direction = {0, 1};
            timer->start(speed); // Start the timer with initial speed
            update(); // Redraw the window to remove the "Press X to start" text

            generateCoins();
        }
    }

    void resetGame() {
        // Reset PacMan position
        pacmanX = 11;
        pacmanY = 21;

        // Reset game state
        gameStarted = false;

        // Stop the timer if running
        if (timer->isActive()) {
            timer->stop();
        }

        // Reset ghosts
        for (Ghost& ghost : ghosts) {
            ghost.row = 7;
            ghost.col = 19 + (&ghost - &ghosts[0]); // Place each ghost horizontally separated
            ghost.direction = QPoint(1, 0); // Set initial direction for each ghost
            ghost.target = QPoint(-1, -1); // Initialize target as invalid
        }
    }

    void generateCoins() {
        // Clear previous coins
        coins.clear();

        // Generate coins randomly on the map
        for (int i = 0; i < map.size(); ++i) {
            for (int j = 0; j < map[i].size(); ++j) {
                if (map[i][j] != '#' && QRandomGenerator::global()->bounded(100) < 3) {
                    // Check if this position should have a coin (34% chance)
                    // If so, add a coin at this position
                    coins.append(QPoint(i, j));

                    // Optionally, add more coins side by side
                    int numCoins = QRandomGenerator::global()->bounded(4, 7); // Generate between 4 and 6 coins side by side
                    for (int k = 1; k < numCoins; ++k) {
                        if (j + k < map[i].size() && map[i][j + k] != '#' && QRandomGenerator::global()->bounded(100) < 34) {
                            coins.append(QPoint(i, j + k));
                        } else {
                            break; // Stop adding more coins if we encounter a wall or reach the map boundary
                        }
                    }

                    // Skip to the next position after adding coins side by side
                    j += numCoins - 1;
                }
            }
        }
    }

    //Funkcia na pohyb duchov
    void moveGhosts() {
        //Nájde pozíciu Pacmana
        QPoint pacmanPos(pacmanX, pacmanY);

        //Skontroluje, či niektorý duch vidí Pacmana a aktualizuje ich cieľové pozície
        for (Ghost& ghost : ghosts) {
            if (canSeePacMan(ghost)) {
                ghost.target = pacmanPos;
            }
        }

        //Pohne duchmi smerom k ich cieľovým pozíciám
        for (Ghost& ghost : ghosts) {
            moveToTarget(ghost, ghost.target);
        }

        update(); //Aktualizuje widget a prekreslí duchov na nové pozície
    }


    QPoint findClosestPosition(const Ghost& ghost, const QPoint& target) const {
        QPoint closestPosition = ghost.target; // Initialize with current position (invalid if target is a wall)

        int minDistance = INT_MAX; // Initialize minimum distance as maximum integer value

        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                int newRow = ghost.row + i;
                int newCol = ghost.col + j;

                // Check if the new position is valid (not a wall or occupied by another ghost)
                if (newRow >= 0 && newRow < map.size() && newCol >= 0 && newCol < map[0].size() &&
                    map[newRow][newCol] != '#' && !isGhostAtPosition(newRow, newCol)) {
                    // Calculate the distance to the target
                    int distance = std::abs(newRow - target.x()) + std::abs(newCol - target.y());

                    // Update the closest position if this position is closer to the target
                    if (distance < minDistance) {
                        minDistance = distance;
                        closestPosition = QPoint(newRow, newCol);
                    }
                }
            }
        }

        return closestPosition;
    }

    bool isGhostAtPosition(int row, int col) const {
        for (const Ghost& ghost : ghosts) {
            if (ghost.row == row && ghost.col == col) {
                return true; // Ghost is at the given position
            }
        }
        return false; // No ghost at the given position
    }


    bool canSeePacMan(const Ghost& ghost) const {
        int dx = pacmanX - ghost.row;
        int dy = pacmanY - ghost.col;
        int steps = std::max(std::abs(dx), std::abs(dy));
        if (steps == 0) return true; // PacMan is at the ghost's position
        for (int i = 1; i <= steps; ++i) {
            int x = ghost.row + dx * i / steps;
            int y = ghost.col + dy * i / steps;
            if (map[x][y] == '#') return false; // Obstacle (wall) blocks the view
        }
        return true; // PacMan is visible
    }

    void moveToTarget(Ghost& ghost, const QPoint& target) {
        // Check if the ghost can see Pacman
        if (canSeePacMan(ghost)) {
            // Move directly towards Pacman
            // Calculate direction towards the target
            int dx = target.x() - ghost.row;
            int dy = target.y() - ghost.col;

            // Choose the primary axis to follow Pacman
            if (std::abs(dx) > std::abs(dy)) {
                ghost.direction = QPoint(dx > 0 ? 1 : -1, 0); // Move horizontally
            } else {
                ghost.direction = QPoint(0, dy > 0 ? 1 : -1); // Move vertically
            }

            // Move ghost in the selected direction
            int newRow = ghost.row + ghost.direction.x();
            int newCol = ghost.col + ghost.direction.y();
            if (map[newRow][newCol] != '#') {
                ghost.row = newRow;
                ghost.col = newCol;
            } else {
                // If there's a wall in the way, set the target position to be the current position
                ghost.target = QPoint(ghost.row, ghost.col);
            }
        } else {
            // Determine whether the ghost will explore or move randomly
            constexpr int exploreChance = 95; // Percentage chance to explore instead of targeting Pacman
            if (QRandomGenerator::global()->bounded(100) < exploreChance) {
                // Explore: Move randomly
                moveInDirection(ghost);
            } else {
                // Move towards Pacman
                // Calculate direction towards the target
                int dx = target.x() - ghost.row;
                int dy = target.y() - ghost.col;

                // Choose the primary axis to follow Pacman
                if (std::abs(dx) > std::abs(dy)) {
                    ghost.direction = QPoint(dx > 0 ? 1 : -1, 0); // Move horizontally
                } else {
                    ghost.direction = QPoint(0, dy > 0 ? 1 : -1); // Move vertically
                }

                // Move ghost in the selected direction
                int newRow = ghost.row + ghost.direction.x();
                int newCol = ghost.col + ghost.direction.y();
                if (map[newRow][newCol] != '#') {
                    ghost.row = newRow;
                    ghost.col = newCol;
                } else {
                    // If there's a wall in the way, set the target position to be the current position
                    ghost.target = QPoint(ghost.row, ghost.col);
                }
            }
        }
    }


    QStringList map = {
            "###########################################",
            "#                                         #",
            "#          #                   #          #",
            "#   ###    #   #############   #    ###   #",
            "#   #                                 #   #",
            "#   #                                 #   #",
            "#   #    ###   ###       ###   ###    #   #",
            "#              #           #              #",
            "#              #           #              #",
            "#              #           #              #",
            "#              #############              #",
            "#   #    ###                   ###    #   #",
            "#   #                                 #   #",
            "#   #                                 #   #",
            "#   ###    #   #############   #    ###   #",
            "#          #                   #          #",
            "#                                         #",
            "###########################################"
    };

    const int blockSize = 20; //Veľkosť každého bloku v mape
    int pacmanX;
    int pacmanY;
    int speed; //Rychlosť pacmana
    QPoint direction; //Smer pohybu
    bool gameStarted;
    QTimer *timer;

    QList<Ghost> ghosts;
    static constexpr int numGhosts = 4; // Number of ghosts
    QList<QPoint> coins;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    PacMan window;
    window.show();
    return QApplication::exec();
}
