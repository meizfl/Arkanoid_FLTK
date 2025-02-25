#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

class Ball {
public:
    int x, y;
    float dx, dy;
    static const int SIZE = 10;

    Ball(int startX, int startY) : x(startX), y(startY) {
        dx = 3;
        dy = -3;
    }

    void move() {
        x += dx;
        y += dy;
    }

    bool collidesWith(int rx, int ry, int rw, int rh) {
        int closestX = std::max(rx, std::min(x, rx + rw));
        int closestY = std::max(ry, std::min(y, ry + rh));
        int distX = x - closestX;
        int distY = y - closestY;
        return (distX * distX + distY * distY) <= (SIZE * SIZE);
    }

    bool isVerticalCollision(int rx, int ry, int rw, int rh) {
        int prevY = y - dy;
        return (prevY + SIZE < ry && y + SIZE >= ry) ||
        (prevY - SIZE > ry + rh && y - SIZE <= ry + rh);
    }
};

class Brick {
public:
    int x, y;
    bool active;
    static const int HEIGHT = 20;

    Brick(int startX, int startY, int width) : x(startX), y(startY), active(true), WIDTH(width) {}
    int WIDTH;
};

class ArkanoidGame : public Fl_Double_Window {
    Ball ball;
    std::vector<Brick> bricks;
    int paddleX;
    float paddleSpeed;
    static const int PADDLE_WIDTH = 80;
    static const int PADDLE_HEIGHT = 10;
    static const int PADDLE_Y = 550;
    static const int SCORE_AREA_HEIGHT = 40;
    bool gameOver;
    int score;
    static const int BRICK_ROWS = 5;
    static const int BRICK_COLS = 10;
    bool paused;
    std::string pauseStatus;

public:
    ArkanoidGame(int difficulty) :
    Fl_Double_Window(800, 600, "Arkanoid"),
    ball(400, 500),
    paddleX(360),
    gameOver(false),
    score(0),
    paused(false) {

        setDifficulty(difficulty);
        initializeBricks();
        Fl::add_timeout(1.0 / 60.0, Timer_CB, this);
        pauseStatus = "Active";
    }

    void setDifficulty(int difficulty) {
        switch (difficulty) {
            case 0:
                ball.dx = ball.dy = 2;
                paddleSpeed = 15;
                break;
            case 1:
                ball.dx = ball.dy = 3;
                paddleSpeed = 20;
                break;
            case 2:
                ball.dx = ball.dy = 5;
                paddleSpeed = 25;
                break;
        }
    }

    void initializeBricks() {
        bricks.clear();
        int windowWidth = w();
        int brickWidth = windowWidth / BRICK_COLS;

        for (int j = 0; j < BRICK_ROWS; j++) {
            for (int i = 0; i < BRICK_COLS; i++) {
                int actualWidth = (i == BRICK_COLS - 1) ?
                windowWidth - (brickWidth * (BRICK_COLS - 1)) : brickWidth;
                bricks.emplace_back(i * brickWidth, j * (Brick::HEIGHT + 5) + SCORE_AREA_HEIGHT, actualWidth);
            }
        }
    }

    static void Timer_CB(void *v) {
        ((ArkanoidGame *) v)->onTimer();
        Fl::repeat_timeout(1.0 / 60.0, Timer_CB, v);
    }

    void onTimer() {
        if (gameOver || paused) return;

        ball.move();

        // Отскок от стен
        if (ball.x <= 0 || ball.x >= w() - Ball::SIZE) {
            ball.dx = -ball.dx;
            ball.x = (ball.x <= 0) ? 0 : w() - Ball::SIZE;
        }

        // Отскок от верхней границы области счета
        if (ball.y <= SCORE_AREA_HEIGHT) {
            ball.dy = std::abs(ball.dy);
            ball.y = SCORE_AREA_HEIGHT;
        }

        if (ball.y >= h()) {
            gameOver = true;
            redraw();
            return;
        }

        if (ball.collidesWith(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
            ball.dy = -std::abs(ball.dy);
            float hitPos = (ball.x - paddleX) / (float) PADDLE_WIDTH;
            ball.dx = (hitPos - 0.5f) * 8;
        }

        for (auto &brick : bricks) {
            if (!brick.active) continue;

            if (ball.collidesWith(brick.x, brick.y, brick.WIDTH, Brick::HEIGHT)) {
                brick.active = false;
                score += 10;

                if (ball.isVerticalCollision(brick.x, brick.y, brick.WIDTH, Brick::HEIGHT)) {
                    ball.dy = -ball.dy;
                } else {
                    ball.dx = -ball.dx;
                }
                break;
            }
        }

        redraw();
    }

    int handle(int event) {
        switch (event) {
            case FL_KEYDOWN:
                switch (Fl::event_key()) {
                    case FL_Left:
                        if (!paused && paddleX > 0) paddleX -= paddleSpeed;
                        break;
                    case FL_Right:
                        if (!paused && paddleX < w() - PADDLE_WIDTH) paddleX += paddleSpeed;
                        break;
                    case FL_Enter:
                        if (gameOver) {
                            resetGame();
                        }
                        break;
                    case ' ':
                        paused = !paused;  // Переключаем состояние паузы
                        pauseStatus = paused ? "Paused" : "Active";  // Обновляем статус
                        redraw();  // Обновляем окно, чтобы отобразить новый статус
                        break;
                }
                redraw();  // Обновляем окно после обработки клавиши
                return 1;
        }
        return Fl_Double_Window::handle(event);
    }


    void resetGame() {
        ball = Ball(400, 500);
        paddleX = 360;
        gameOver = false;
        score = 0;
        initializeBricks();
        paused = false;
        pauseStatus = "Active";
    }

    void draw() {
        Fl_Double_Window::draw();
        fl_color(FL_BLACK);
        fl_rectf(0, 0, w(), h());

        // Отрисовка области счета
        fl_color(FL_DARK_BLUE);
        fl_rectf(0, 0, w(), SCORE_AREA_HEIGHT);

        // Отрисовка счета
        fl_color(FL_WHITE);
        fl_font(FL_HELVETICA, 20);
        std::string scoreText = "Score: " + std::to_string(score);
        fl_draw(scoreText.c_str(), 20, 30);

        // Отрисовка статуса паузы
        int pauseStatusWidth, pauseStatusHeight;
        fl_measure(pauseStatus.c_str(), pauseStatusWidth, pauseStatusHeight);
        fl_draw(pauseStatus.c_str(), w() - pauseStatusWidth - 20, 30);

        // Отрисовка мяча
        fl_color(FL_WHITE);
        fl_begin_polygon();
        fl_circle(ball.x, ball.y, Ball::SIZE);
        fl_end_polygon();

        // Отрисовка ракетки
        fl_color(FL_CYAN);
        fl_rectf(paddleX, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT);

        // Отрисовка кирпичей
        for (const auto &brick : bricks) {
            if (!brick.active) continue;
            fl_color(FL_RED);
            fl_rectf(brick.x, brick.y, brick.WIDTH, Brick::HEIGHT);
            fl_color(FL_WHITE);
            fl_rect(brick.x, brick.y, brick.WIDTH, Brick::HEIGHT);
        }

        if (gameOver) {
            fl_color(FL_WHITE);
            fl_font(FL_HELVETICA, 30);
            std::string gameOverText = "Game Over! Score: " + std::to_string(score) +
            " Press Enter to restart";
        fl_draw(gameOverText.c_str(), 20, h() - 50);
        }
    }
};

class DifficultySelector : public Fl_Window {
    Fl_Choice *difficultyChoice;
    Fl_Button *startButton;
    int selectedDifficulty;

    static void Start_CB(Fl_Widget *w, void *data) {
        DifficultySelector *selector = (DifficultySelector *) data;
        selector->selectedDifficulty = selector->difficultyChoice->value();
        selector->hide();
        ArkanoidGame *game = new ArkanoidGame(selector->selectedDifficulty);
        game->show();
    }

public:
    DifficultySelector() : Fl_Window(200, 100, "Select Difficulty"), selectedDifficulty(1) {
        difficultyChoice = new Fl_Choice(50, 15, 100, 30,"");
        difficultyChoice->add("Easy");
        difficultyChoice->add("Normal");
        difficultyChoice->add("Hard");
        difficultyChoice->value(1);

        startButton = new Fl_Button(50, 50, 100, 30, "Start");
        startButton->callback(Start_CB, this);
        end();
    }
};

int main(int argc, char *argv[]) {
    DifficultySelector selector;
    selector.show();
    return Fl::run();
}
