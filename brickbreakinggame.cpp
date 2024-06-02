#include <SDL.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int PADDLE_WIDTH = 100;
const int PADDLE_HEIGHT = 20;
const int BALL_SIZE = 15;
const int BRICK_WIDTH = 60;
const int BRICK_HEIGHT = 30;
const int BRICK_ROWS = 5;
const int BRICK_COLS = 10;
const int BALL_SPEED = 5;
const int ITEM_DROP_PROBABILITY = 10; // 아이템 드랍 확률 (10%)

enum ItemType { NONE, EXTRA_BALL, EXPAND_PADDLE };

struct Brick {
    SDL_Rect rect;
    bool destroyed;
    SDL_Color color;
};

struct Ball {
    SDL_Rect rect;
    int speedX;
    int speedY;
};

struct Item {
    SDL_Rect rect;
    ItemType type;
    bool active;
};

void initializeBricks(std::vector<Brick>& bricks) {
    bricks.clear();
    SDL_Color colors[BRICK_ROWS] = {
        {255, 0, 0, 255}, // Red
        {255, 165, 0, 255}, // Orange
        {255, 255, 0, 255}, // Yellow
        {0, 128, 0, 255}, // Green
        {0, 0, 255, 255} // Blue
    };
    for (int i = 0; i < BRICK_ROWS; ++i) {
        for (int j = 0; j < BRICK_COLS; ++j) {
            Brick brick = { { j * (BRICK_WIDTH + 10) + 35, i * (BRICK_HEIGHT + 10) + 50, BRICK_WIDTH, BRICK_HEIGHT }, false, colors[i] };
            bricks.push_back(brick);
        }
    }
}

void drawCircle(SDL_Renderer* renderer, int x, int y, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

void drawItems(SDL_Renderer* renderer, const std::vector<Item>& items) {
    for (const auto& item : items) {
        if (item.active) {
            switch (item.type) {
            case EXTRA_BALL:
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
                break;
            case EXPAND_PADDLE:
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue
                break;
            default:
                continue;
            }
            SDL_RenderFillRect(renderer, &item.rect);
        }
    }
}

void handleBallCollision(Ball& ball, std::vector<Brick>& bricks, std::vector<Item>& items) {
    for (auto& brick : bricks) {
        if (!brick.destroyed && SDL_HasIntersection(&ball.rect, &brick.rect)) {
            brick.destroyed = true;
            ball.speedY = -ball.speedY;

            // 아이템 드랍 로직
            if (std::rand() % 100 < ITEM_DROP_PROBABILITY) {
                ItemType type = (std::rand() % 2 == 0) ? EXTRA_BALL : EXPAND_PADDLE;
                items.push_back({ { brick.rect.x + BRICK_WIDTH / 2, brick.rect.y, 20, 20 }, type, true });
            }
            break;
        }
    }
}

bool allBricksDestroyed(const std::vector<Brick>& bricks) {
    for (const auto& brick : bricks) {
        if (!brick.destroyed) {
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Brick Breaker", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        SDL_DestroyWindow(window);
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Rect paddle = { (WINDOW_WIDTH - PADDLE_WIDTH) / 2, WINDOW_HEIGHT - 40, PADDLE_WIDTH, PADDLE_HEIGHT };
    std::vector<Ball> balls = { { { (WINDOW_WIDTH - BALL_SIZE) / 2, WINDOW_HEIGHT / 2, BALL_SIZE, BALL_SIZE }, BALL_SPEED, -BALL_SPEED } };
    int paddleSpeed = 0;

    std::vector<Brick> bricks;
    initializeBricks(bricks);

    std::vector<Item> items;

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        const Uint8* state = SDL_GetKeyboardState(NULL);
        paddleSpeed = 0;
        if (state[SDL_SCANCODE_LEFT]) {
            paddle.x -= 10;
            paddleSpeed = -10;
            if (paddle.x < 0) paddle.x = 0;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            paddle.x += 10;
            paddleSpeed = 10;
            if (paddle.x + paddle.w > WINDOW_WIDTH) paddle.x = WINDOW_WIDTH - paddle.w;
        }

        for (auto& ball : balls) {
            ball.rect.x += ball.speedX;
            ball.rect.y += ball.speedY;

            if (ball.rect.x <= 0 || ball.rect.x + BALL_SIZE >= WINDOW_WIDTH) {
                ball.speedX = -ball.speedX;
            }
            if (ball.rect.y <= 0) {
                ball.speedY = -ball.speedY;
            }
            if (ball.rect.y + BALL_SIZE >= WINDOW_HEIGHT) {
                // Ball hit the bottom edge, remove ball
                ball = balls.back();  // Copy the last ball to the current position
                balls.pop_back();     // Remove the last ball
                if (balls.empty()) {
                    // No balls left, end game
                    quit = true;
                }
                break;
            }

            if (SDL_HasIntersection(&ball.rect, &paddle)) {
                ball.speedY = -ball.speedY;
                // 공이 패들과 충돌할 때 패들의 속도에 따라 공의 방향을 변경합니다.
                if (paddleSpeed != 0) {
                    ball.speedX = paddleSpeed > 0 ? BALL_SPEED : -BALL_SPEED;
                }
            }

            handleBallCollision(ball, bricks, items);
        }

        if (allBricksDestroyed(bricks)) {
            quit = true;
        }

        for (auto& item : items) {
            if (item.active) {
                item.rect.y += 5;
                if (SDL_HasIntersection(&item.rect, &paddle)) {
                    item.active = false;
                    switch (item.type) {
                    case EXTRA_BALL:
                        balls.push_back({ { paddle.x + paddle.w / 2, paddle.y - BALL_SIZE, BALL_SIZE, BALL_SIZE }, BALL_SPEED, -BALL_SPEED });
                        break;
                    case EXPAND_PADDLE:
                        paddle.w += 50;
                        if (paddle.w > WINDOW_WIDTH) {
                            paddle.w = WINDOW_WIDTH;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else if (item.rect.y >= WINDOW_HEIGHT) {
                    item.active = false;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &paddle);

        for (const auto& ball : balls) {
            drawCircle(renderer, ball.rect.x + BALL_SIZE / 2, ball.rect.y + BALL_SIZE / 2, BALL_SIZE / 2);
        }

        for (const auto& brick : bricks) {
            if (!brick.destroyed) {
                SDL_SetRenderDrawColor(renderer, brick.color.r, brick.color.g, brick.color.b, 255);
                SDL_RenderFillRect(renderer, &brick.rect);
            }
        }

        drawItems(renderer, items);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
