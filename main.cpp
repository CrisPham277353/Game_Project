#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <string>

const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 800;
const int BLOCK_SIZE = 40;
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
bool running = true;

// Game states
enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

GameState currentState = MENU;

// Menu options
enum MenuOption {
    START_GAME,
    QUIT,
    MENU_OPTION_COUNT
};

MenuOption selectedOption = START_GAME;

// Game variables
int score = 0;
int level = 1;
int linesCleared = 0;

// Tetromino shapes
const int tetromino[7][4] = {
    {1, 3, 5, 7}, // I
    {2, 4, 5, 7}, // Z
    {3, 5, 4, 6}, // S
    {3, 5, 4, 7}, // T
    {2, 3, 5, 7}, // L
    {3, 5, 7, 6}, // J
    {2, 3, 4, 5}, // O
};

struct Point {
    int x, y;
};

Point current[4], backup[4];
int board[BOARD_HEIGHT][BOARD_WIDTH] = {};
int colorIndex = 1;

bool LoadFont() {
    // Try to load different fonts in order of preference
    const char* fontPaths[] = {
        "arial.ttf",                                         // Local directory
        "fonts/arial.ttf",                                   // Local fonts directory
        "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf", // Linux - MS fonts
        "/usr/share/fonts/TTF/arial.ttf",                    // Linux - Some distros
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",   // Linux - DejaVu
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",   // Linux - Free fonts
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", // Linux - Liberation
        "C:/Windows/Fonts/arial.ttf",                        // Windows
        "/Library/Fonts/Arial.ttf",                          // macOS
        "/System/Library/Fonts/SFNSDisplay.ttf"              // macOS - San Francisco
    };

    const int numPaths = sizeof(fontPaths) / sizeof(fontPaths[0]);

    for (int i = 0; i < numPaths; i++) {
        font = TTF_OpenFont(fontPaths[i], 24);
        if (font) {
            std::cout << "Successfully loaded font: " << fontPaths[i] << std::endl;
            return true;
        }
    }

    std::cout << "Failed to load any font: " << TTF_GetError() << std::endl;
    return false;
}

void Init() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Try to load a font
    if (!LoadFont()) {
        std::cout << "Warning: No font loaded. Text won't be displayed." << std::endl;
    }

    srand(static_cast<unsigned int>(time(nullptr)));
}

void Quit() {
    if (font) {
        TTF_CloseFont(font);
    }
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void DrawText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        std::cout << "Failed to render text: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void DrawBlock(int x, int y, int color) {
    SDL_Rect rect = { x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE - 1, BLOCK_SIZE - 1 };
    switch (color) {
    case 1: SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); break;
    case 2: SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); break;
    case 3: SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); break;
    case 4: SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); break;
    case 5: SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255); break;
    case 6: SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); break;
    case 7: SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255); break;
    }
    SDL_RenderFillRect(renderer, &rect);
}

// Forward declaration
void SpawnTetromino();

void ResetGame() {
    // Clear the board
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            board[i][j] = 0;
        }
    }
    score = 0;
    level = 1;
    linesCleared = 0;
    SpawnTetromino();
}

void SpawnTetromino() {
    int r = rand() % 7;
    colorIndex = r + 1;
    for (int i = 0; i < 4; ++i) {
        current[i].x = tetromino[r][i] % 2 + BOARD_WIDTH / 2 - 1;
        current[i].y = tetromino[r][i] / 2;
    }
}

bool CheckCollision() {
    for (int i = 0; i < 4; ++i) {
        if (current[i].x < 0 || current[i].x >= BOARD_WIDTH || current[i].y >= BOARD_HEIGHT)
            return true;
        if (current[i].y >= 0 && board[current[i].y][current[i].x])
            return true;
    }
    return false;
}

void Rotate() {
    Point pivot = current[1];
    for (int i = 0; i < 4; ++i) {
        int x = current[i].y - pivot.y;
        int y = current[i].x - pivot.x;
        current[i].x = pivot.x - x;
        current[i].y = pivot.y + y;
    }
    if (CheckCollision()) {
        for (int i = 0; i < 4; ++i)
            current[i] = backup[i];
    }
}

void Move(int dx) {
    for (int i = 0; i < 4; ++i) {
        backup[i] = current[i];
        current[i].x += dx;
    }
    if (CheckCollision())
        for (int i = 0; i < 4; ++i)
            current[i] = backup[i];
}

void Drop() {
    for (int i = 0; i < 4; ++i) {
        backup[i] = current[i];
        current[i].y += 1;
    }
    if (CheckCollision()) {
        for (int i = 0; i < 4; ++i)
            board[backup[i].y][backup[i].x] = colorIndex;
        SpawnTetromino();
        if (CheckCollision()) {
            currentState = GAME_OVER;
        }
    }
}

void HardDrop() {
    while (!CheckCollision()) {
        for (int i = 0; i < 4; ++i) {
            backup[i] = current[i];
            current[i].y += 1;
        }
    }
    // Move back one step
    for (int i = 0; i < 4; ++i) {
        current[i] = backup[i];
    }
    // Lock the piece
    for (int i = 0; i < 4; ++i) {
        board[current[i].y][current[i].x] = colorIndex;
    }
    SpawnTetromino();
    if (CheckCollision()) {
        currentState = GAME_OVER;
    }
}

void ClearLines() {
    int linesCleared = 0;
    for (int i = BOARD_HEIGHT - 1; i >= 0; --i) {
        bool full = true;
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (!board[i][j]) {
                full = false;
                break;
            }
        }
        if (full) {
            linesCleared++;
            for (int k = i; k > 0; --k)
                for (int j = 0; j < BOARD_WIDTH; ++j)
                    board[k][j] = board[k - 1][j];
            ++i;
        }
    }

    // Update score based on lines cleared
    if (linesCleared > 0) {
        // Classic Tetris scoring
        switch (linesCleared) {
        case 1: score += 100 * level; break;
        case 2: score += 300 * level; break;
        case 3: score += 500 * level; break;
        case 4: score += 800 * level; break; // Tetris!
        }

        // Update total lines and level
        ::linesCleared += linesCleared;
        level = 1 + (::linesCleared / 10);
    }
}

void HandleMenuInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_UP:
            selectedOption = static_cast<MenuOption>((selectedOption - 1 + MENU_OPTION_COUNT) % MENU_OPTION_COUNT);
            break;
        case SDLK_DOWN:
            selectedOption = static_cast<MenuOption>((selectedOption + 1) % MENU_OPTION_COUNT);
            break;
        case SDLK_RETURN:
            switch (selectedOption) {
            case START_GAME:
                ResetGame();
                currentState = PLAYING;
                break;
            case QUIT:
                running = false;
                break;
            }
            break;
        }
    }
}

void HandleGameInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_LEFT:
            Move(-1);
            break;
        case SDLK_RIGHT:
            Move(1);
            break;
        case SDLK_UP:
            for (int i = 0; i < 4; ++i) backup[i] = current[i];
            Rotate();
            break;
        case SDLK_DOWN:
            Drop();
            break;
        case SDLK_SPACE:
            HardDrop();
            break;
        case SDLK_p:
            currentState = PAUSED;
            break;
        case SDLK_ESCAPE:
            currentState = MENU;
            break;
        }
    }
}

void HandlePausedInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_p:
        case SDLK_RETURN:
            currentState = PLAYING;
            break;
        case SDLK_ESCAPE:
            currentState = MENU;
            break;
        }
    }
}

void HandleGameOverInput(SDL_Event& e) {
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_RETURN:
            currentState = MENU;
            break;
        case SDLK_ESCAPE:
            running = false;
            break;
        }
    }
}

void HandleInput(SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        running = false;
        return;
    }

    switch (currentState) {
    case MENU:
        HandleMenuInput(e);
        break;
    case PLAYING:
        HandleGameInput(e);
        break;
    case PAUSED:
        HandlePausedInput(e);
        break;
    case GAME_OVER:
        HandleGameOverInput(e);
        break;
    }
}

void DrawMenu() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw title even if no font is available
    if (font) {
        SDL_Color titleColor = { 255, 255, 255, 255 };
        DrawText("TETRIS", SCREEN_WIDTH / 2 - 60, 150, titleColor);

        // Draw menu options
        for (int i = 0; i < MENU_OPTION_COUNT; ++i) {
            SDL_Color textColor = (i == selectedOption) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 200, 200, 200, 255 };
            std::string optionText;

            switch (i) {
            case START_GAME: optionText = "Start Game"; break;
            case QUIT: optionText = "Quit"; break;
            }

            DrawText(optionText, SCREEN_WIDTH / 2 - 80, 300 + i * 60, textColor);
        }

        // Draw controls
        SDL_Color controlsColor = { 150, 150, 150, 255 };
        DrawText("Controls:", 50, 550, controlsColor);
        DrawText("Arrow Keys - Move/Rotate", 50, 590, controlsColor);
        DrawText("Space - Hard Drop", 50, 630, controlsColor);
        DrawText("P - Pause", 50, 670, controlsColor);
        DrawText("ESC - Menu", 50, 710, controlsColor);
    }
    else {
        // Fallback for when no font is available - draw colored rectangles for menu options
        SDL_Rect titleRect = { SCREEN_WIDTH / 2 - 100, 150, 200, 40 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &titleRect);

        for (int i = 0; i < MENU_OPTION_COUNT; ++i) {
            SDL_Rect optionRect = { SCREEN_WIDTH / 2 - 100, 300 + i * 60, 200, 40 };

            if (i == selectedOption) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
            }

            SDL_RenderFillRect(renderer, &optionRect);
        }
    }
}

void DrawGame() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw board
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j]) DrawBlock(j, i, board[i][j]);
        }
    }

    // Draw current tetromino
    for (int i = 0; i < 4; ++i) {
        DrawBlock(current[i].x, current[i].y, colorIndex);
    }

    // Draw grid
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    for (int i = 0; i <= BOARD_HEIGHT; ++i) {
        SDL_RenderDrawLine(renderer, 0, i * BLOCK_SIZE, BOARD_WIDTH * BLOCK_SIZE, i * BLOCK_SIZE);
    }
    for (int j = 0; j <= BOARD_WIDTH; ++j) {
        SDL_RenderDrawLine(renderer, j * BLOCK_SIZE, 0, j * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE);
    }

    // Draw score and level
    if (font) {
        SDL_Color textColor = { 255, 255, 255, 255 };
        DrawText("Score: " + std::to_string(score), SCREEN_WIDTH - 180, 20, textColor);
        DrawText("Level: " + std::to_string(level), SCREEN_WIDTH - 180, 60, textColor);
    }
}

void DrawPaused() {
    DrawGame(); // Draw game in background

    // Semi-transparent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &overlay);

    if (font) {
        SDL_Color textColor = { 255, 255, 255, 255 };
        DrawText("PAUSED", SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 30, textColor);
        DrawText("Press P to resume", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 + 30, textColor);
        DrawText("Press ESC for menu", SCREEN_WIDTH / 2 - 120, SCREEN_HEIGHT / 2 + 70, textColor);
    }
    else {
        // Fallback visual indicator for PAUSED
        SDL_Rect pauseRect = { SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 30, 160, 60 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &pauseRect);
    }
}

void DrawGameOver() {
    DrawGame(); // Draw game in background

    // Semi-transparent overlay
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &overlay);

    if (font) {
        SDL_Color textColor = { 255, 0, 0, 255 };
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 30, textColor);

        SDL_Color scoreColor = { 255, 255, 255, 255 };
        DrawText("Final Score: " + std::to_string(score), SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 30, scoreColor);
        DrawText("Press ENTER to continue", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 + 70, scoreColor);
    }
    else {
        // Fallback visual indicator for GAME OVER
        SDL_Rect gameOverRect = { SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 30, 200, 60 };
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &gameOverRect);
    }
}

void Draw() {
    switch (currentState) {
    case MENU:
        DrawMenu();
        break;
    case PLAYING:
        DrawGame();
        break;
    case PAUSED:
        DrawPaused();
        break;
    case GAME_OVER:
        DrawGameOver();
        break;
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    Init();

    Uint32 lastTick = SDL_GetTicks();
    Uint32 dropInterval = 500; // ms - will decrease with level

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) HandleInput(e);

        if (currentState == PLAYING) {
            Uint32 now = SDL_GetTicks();
            // Decrease interval as level increases
            dropInterval = 500 - (level - 1) * 25;
            if (dropInterval < 100) dropInterval = 100; // Minimum speed cap

            if (now - lastTick > dropInterval) {
                Drop();
                ClearLines();
                lastTick = now;
            }
        }

        Draw();
        SDL_Delay(16); // ~60 FPS
    }

    Quit();
    return 0;
}