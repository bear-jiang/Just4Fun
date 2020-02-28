#include <ncurses.h>
#include <cstdlib>
#include <vector>
#include <deque>
#include <thread>
#include <iostream>
#include <cstring>

using namespace std;

class NCursesWindow
{
public:
    static int win_count;

    NCursesWindow(int height, int width, int start_y, int start_x)
    {
        this->height = height;
        this->width = width;
        win = newwin(height, width, start_y, start_x);
        box(win, 0, 0);
        wrefresh(win);
    }

    virtual ~NCursesWindow()
    {
        delwin(win);
        win_count--;
        if(win_count == 0)
        {
            NCursesWindow::exit();
        }
    }

    virtual void clear()
    {
        wclear(win);
        box(win, 0, 0);
        wrefresh(win);
    }

    static void init()
    {
        initscr();  /* 初始化，进入ncurses模式 */

        raw();      /* 禁止行缓冲，可以立即见到结果 */
        noecho();   /* 不在终端上显示控制字符，比如Ctrl+c */
        keypad(stdscr, TRUE);   /* 允许用户在终端使用键盘 */
        curs_set(0);    /* 设置光标是否可见，0不可见，1可见，2完全可见 */
        refresh();      /* 将虚拟屏幕上的内容写到显示器上，并刷新 */
    }

    static void exit()
    {
        endwin();
    }

protected:
    struct Init
    {
        Init()
        {
            if(win_count == 0)
            {
                NCursesWindow::init();
            }

            win_count++;
        }
    }no_use;

    WINDOW *win;
    int height;
    int width;
};

int NCursesWindow::win_count = 0;

class LogWin: public NCursesWindow
{
public:
    LogWin(int height, int width, int start_y, int start_x): NCursesWindow(height, width, start_y, start_x)
    {
        logbuf_num = height-2;
        logbuf_len = width-2;
        logbuf = (char**)malloc(sizeof(char*)*logbuf_num);
        for(int i = 0; i<logbuf_num; i++)
        {
            logbuf[i] = (char*)malloc(sizeof(char)*logbuf_len);
        }
        mvwprintw(win, 0, 2, " LOG ");
        wrefresh(win);
    }

    ~LogWin()
    {
        for(int i = 0; i<logbuf_num; i++)
        {
            free(logbuf[i]);
        }

        free(logbuf);
    }

    void info(const char* str)
    {
        char empty_line[logbuf_len] = {0};
        memset(empty_line, ' ', logbuf_len-1);

        if(index < logbuf_num)
        {
            memcpy(logbuf[index], str, min(strlen(str)+1, (size_t)logbuf_len));
            logbuf[index][logbuf_len-1] = '\0';
            index++;
        }
        else
        {

            for(int i=0; i<index-1; i++)
            {
                memcpy(logbuf[i], logbuf[i+1], strlen(logbuf[i+1])+1);
                mvwprintw(win, i+1, 1, empty_line);
                mvwprintw(win, i+1, 1, logbuf[i]);
            }
            memcpy(logbuf[index-1], str, min(strlen(str)+1, (size_t)logbuf_len));
            logbuf[index-1][logbuf_len-1] = '\0';
        }

        mvwprintw(win, index, 1, empty_line);
        mvwprintw(win, index, 1, logbuf[index-1]);
        wrefresh(win);
    }

private:
    char** logbuf;
    int logbuf_num;
    int logbuf_len;
    int index{0};
};

enum class Direction
{
    Up,
    Down,
    Left,
    Right
};

class Snake
{
public:
    Snake()
    {
        body.push_back({0, 0});
    }

    Snake(int x, int y)
    {
        body.push_back({x, y});
    }

    ~Snake()
    {
    }

    void reset(int x=0, int y=0)
    {
        body.clear();
        body.push_back({x, y});
    }

    void move(Direction dir, std::pair<int, int> food)
    {
        std::pair<int, int> head = body.back();
        std::pair<int, int> new_head;

        switch (dir)
        {
            case Direction::Up:
            {
                new_head = {head.first, head.second-1};
                break;
            }
            case Direction::Down:
            {
                new_head = {head.first, head.second+1};
                break;
            }
            case Direction::Left:
            {
                new_head = {head.first-1, head.second};
                break;
            }
            case Direction::Right:
            {
                new_head = {head.first+1, head.second};
                break;
            }
            default:
                break;
        }

        if(new_head.second == food.second && new_head.first == food.first)
        {
            body.push_back(new_head);
        }
        else
        {
            body.push_back(new_head);
            last_tail = body.front();
            body.pop_front();
        }
    }

    bool checkBite()
    {
        const auto head = body.back();
        for(int i = 0; i<body.size()-1; i++)
        {
            if(body[i].first == head.first && body[i].second == head.second)
            {
                return true;
            }
        }

        return false;
    }

    const std::pair<int, int>& getHead() const
    {
        return body.back();
    }

    const std::pair<int, int>& getLastTail() const
    {
        return last_tail;
    }

    const std::deque<std::pair<int, int>>& getBody() const
    {
        return body;
    }

private:
    std::deque<std::pair<int, int>> body;
    std::pair<int, int> last_tail;
};

class GameWindow: public NCursesWindow
{
public:
    GameWindow(int height, int width, int start_y, int start_x): NCursesWindow(height, width, start_y, start_x)
    {
    }

    void printInfo(int food_count, int speed)
    {
        mvwprintw(win, 0, 2, " GAME ");
        mvwprintw(win, height-1, 2, " Foods: %d ", food_count);
        mvwprintw(win, height-1, 30, " Speed: %d ", speed);
        wrefresh(win);
    }

    void drawSnake(const Snake& snake)
    {
        mvwprintw(win, snake.getLastTail().second+1, snake.getLastTail().first+1, " ");
        mvwprintw(win, snake.getHead().second+1, snake.getHead().first+1, "#");
        wrefresh(win);
    }

    void drawFood(const std::pair<int, int>& food)
    {
        mvwprintw(win, food.second+1, food.first+1, "@");
        wrefresh(win);
    }

    bool outBoundary(const Snake& snake)
    {
        const auto head = snake.getHead();
        if(head.first<0 or head.first>width-3 or head.second<0 or head.second>height-3)
        {
            return true;
        }

        return false;
    }

    void gameOver(const char* s)
    {
        mvwprintw(win, (int)(height/2), (width/2 - strlen(s)/2), s);
        wrefresh(win);
    }

private:

};

enum class GameState
{
    Running,
    Over,
    Pause
};

class SnakeGame
{
public:
    SnakeGame(int height, int width)
    :game_window_height{height*3/4},
    game_window_width{width},
    game_window{game_window_height, game_window_width, 1, 3},
    log_window{height/4, width, game_window_height+3, 3}
    {
    }

    ~SnakeGame()
    {
    }

    void run()
    {
        bool should_stop = false;
        timeout(200);

        log_window.info("  Press 'q' to quit, 'r' to reset, 'p' to pause.");
        log_window.info("  Press 'w/s/a/d' to move the snake.");
        log_window.info("  Press 'z/x' to change the speed.");

        while(!should_stop)
        {
            int key = -1;
            key = getch();

            if(state == GameState::Running)
            {
                switch (key)
                {
                    case 'q':
                    {
                        should_stop = true;
                        break;
                    }
                    case 'w':
                    {
                        dir = Direction::Up;
                        break;
                    }
                    case 'a':
                    {
                        dir = Direction::Left;
                        break;
                    }
                    case 's':
                    {
                        dir = Direction::Down;
                        break;
                    }
                    case 'd':
                    {
                        dir = Direction::Right;
                        break;
                    }
                    case 'p':
                    {
                        state = GameState::Pause;
                        continue;
                    }
                    case 'r':
                    {
                        reset();
                        continue;
                    }
                    case 'x':
                    {
                        if(speed<10)
                        {
                            speed++;
                        }

                        timeout(200-(speed-1)*20);
                        break;
                    }
                    case 'z':
                    {
                        if(speed>1)
                        {
                            speed--;
                        }

                        timeout(200-(speed-1)*20);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                game_window.printInfo(snake.getBody().size()-1, speed);

                updateFood();
                game_window.drawFood(food);

                snake.move(dir, food);

                if(snake.checkBite() || game_window.outBoundary(snake))
                {
                    game_window.gameOver("Game over!");
                    log_window.info("  Game over.");
                    state = GameState::Over;
                    continue;
                }
                game_window.drawSnake(snake);
            }
            else if(state == GameState::Pause)
            {
                switch (key)
                {
                    case 'p':
                    {
                        state = GameState::Running;
                        break;
                    }
                    case 'q':
                    {
                        should_stop = true;
                        break;
                    }
                    case 'r':
                    {
                        reset();
                        continue;
                    }
                    default:
                    {
                        break;
                    }

                }
            }
            else if(state == GameState::Over)
            {
                switch (key)
                {
                    case 'r':
                    {
                        reset();
                        break;
                    }
                    case 'q':
                    {
                        should_stop = true;
                        break;
                    }
                    default:
                    {
                        break;
                    }

                }
            }
        }

        if(should_stop)
        {
            return;
        }
        timeout(-1);
        getch();
    }


private:

    Snake snake;
    int speed{1};
    std::pair<int, int> food;
    int game_window_height;
    int game_window_width;
    GameWindow game_window;
    LogWin log_window;

    Direction dir{Direction::Right};
    GameState state{GameState::Running};

    bool isFoodValid()
    {
        for(const auto item: snake.getBody())
        {
            if(food.second == item.second && food.first == item.first)
            {
                return false;
            }
        }

        return true;
    }

    void updateFood()
    {
        while(!isFoodValid())
        {
            food.second = random() % (game_window_height - 2);
            food.first = random() % (game_window_width - 2);
        }
    }

    void reset()
    {
        game_window.clear();
        state = GameState::Running;
        snake.reset();
        speed = 1;
        dir = Direction::Right;
        log_window.info("  Game reset.");
    }
};

int main()
{
    SnakeGame g{30, 60};
    g.run();
}