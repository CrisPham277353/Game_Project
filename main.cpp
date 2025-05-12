#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h> 
#include <iostream>
#include <vector>
#include <ctime>
#include <string>
using namespace std;

const int SCREEN_WIDTH = 400;
const int SCREEN_HEIGHT = 800;
const int BLOCK_SIZE = 40;
const int BOARD_WIDTH = 10;
const int BOARD_HEIGHT = 20;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
SDL_Texture* menuBackgroundTexture = nullptr; 
bool running = true;


enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

GameState currentState = MENU;


enum MenuOption {
    START_GAME,
    QUIT,
    MENU_OPTION_COUNT
};

MenuOption selectedOption = START_GAME;


int score = 0;
int level = 1;
int linesCleared = 0;


const int tetromino[7][4] = {
    {1, 3, 5, 7},
    {2, 4, 5, 7},
    {3, 5, 4, 6}, 
    {3, 5, 4, 7}, 
    {2, 3, 5, 7}, 
    {3, 5, 7, 6}, 
    {2, 3, 4, 5}, 
};

struct Point {
    int x, y;
};

Point current[4], backup[4];
int board[BOARD_HEIGHT][BOARD_WIDTH] = {};
int colorIndex = 1;

bool LoadFont() {
    
    const char* fontPaths[] = {
        "arial.ttf",                                         
        "fonts/arial.ttf",                                   
        "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf", 
        "/usr/share/fonts/TTF/arial.ttf",                    
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",   
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",   
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 
        "C:/Windows/Fonts/arial.ttf",                        
        "/Library/Fonts/Arial.ttf",                          
        "/System/Library/Fonts/SFNSDisplay.ttf"              
    };
    const int numPaths = sizeof(fontPaths) / sizeof(fontPaths[0]);

    for (int i = 0; i < numPaths; i++) {
        font = TTF_OpenFont(fontPaths[i], 24);
        if (font) {
            cout << "Successfully loaded font: " << fontPaths[i] << endl;
            return true;
        }
    }

    cout << "Failed to load any font: " << TTF_GetError() << endl;
    return false;
}


bool LoadMenuBackground() {
   
    const char* imagePaths = "/home/cris-pham/Dev/Project_VSC/Game_Project/background.png";

    const int numPaths = sizeof(imagePaths) / sizeof(imagePaths[0]);

    for (int i = 0; i < numPaths; i++) {
        SDL_Surface* loadedSurface = IMG_Load(imagePaths);
        if (loadedSurface != NULL) {
            menuBackgroundTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
            SDL_FreeSurface(loadedSurface);

            if (menuBackgroundTexture != NULL) {
                cout << "Successfully loaded background image: " << imagePaths[i] << endl;
                return true;
            }
        }
    }

    cout << "Failed to load background image. Using default background." << endl;
    return false;
}

void Init() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG); 

    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    
    if (!LoadFont()) {
        cout << "Warning: No font loaded. Text won't be displayed." << endl;
    }

    
    LoadMenuBackground();

    srand(static_cast<unsigned int>(time(nullptr)));
}

void Quit() {
    if (font) {
        TTF_CloseFont(font);
    }
    if (menuBackgroundTexture) {
        SDL_DestroyTexture(menuBackgroundTexture);
    }
    TTF_Quit();
    IMG_Quit(); 
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void DrawText(const string& text, int x, int y, SDL_Color color) {
    if (!font) return;

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        cout << "Failed to render text: " << TTF_GetError() << endl;
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


void SpawnTetromino();

void ResetGame() {
    
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
    
    for (int i = 0; i < 4; ++i) {
        current[i] = backup[i];
    }
    
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

    
    if (linesCleared > 0) {
        
        switch (linesCleared) {
        case 1: score += 100 * level; break;
        case 2: score += 300 * level; break;
        case 3: score += 500 * level; break;
        case 4: score += 800 * level; break; !
        }

        
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
    
    if (menuBackgroundTexture) {
        SDL_RenderCopy(renderer, menuBackgroundTexture, NULL, NULL);
    }
    else {
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }

    
    if (font) {
        
        SDL_Color titleColor = { 255, 255, 255, 255 };

        for (int i = 0; i < MENU_OPTION_COUNT; ++i) {
            SDL_Color textColor = (i == selectedOption) ? SDL_Color{ 255, 255, 0, 255 } : SDL_Color{ 200, 200, 200, 255 };
            string optionText;

            switch (i) {
            case START_GAME: optionText = "Start Game"; break;
            case QUIT: optionText = "Quit"; break;
            }

            DrawText(optionText, SCREEN_WIDTH / 2 - 80, 400 + i * 60, textColor);
        }
    }
    else {
        for (int i = 0; i < MENU_OPTION_COUNT; ++i) {
            SDL_Rect optionRect = { SCREEN_WIDTH / 2 - 100, 400 + i * 60, 200, 40 };

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

    
    for (int i = 0; i < BOARD_HEIGHT; ++i) {
        for (int j = 0; j < BOARD_WIDTH; ++j) {
            if (board[i][j]) DrawBlock(j, i, board[i][j]);
        }
    }

    for (int i = 0; i < 4; ++i) {
        DrawBlock(current[i].x, current[i].y, colorIndex);
    }

    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    for (int i = 0; i <= BOARD_HEIGHT; ++i) {
        SDL_RenderDrawLine(renderer, 0, i * BLOCK_SIZE, BOARD_WIDTH * BLOCK_SIZE, i * BLOCK_SIZE);
    }
    for (int j = 0; j <= BOARD_WIDTH; ++j) {
        SDL_RenderDrawLine(renderer, j * BLOCK_SIZE, 0, j * BLOCK_SIZE, BOARD_HEIGHT * BLOCK_SIZE);
    }

    if (font) {
        SDL_Color textColor = { 255, 255, 255, 255 };
        DrawText("Score: " + to_string(score), SCREEN_WIDTH - 180, 20, textColor);
        DrawText("Level: " + to_string(level), SCREEN_WIDTH - 180, 60, textColor);
    }
}

void DrawPaused() {
    DrawGame(); 

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
        SDL_Rect pauseRect = { SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 30, 160, 60 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &pauseRect);
    }
}

void DrawGameOver() {
    DrawGame();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect overlay = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    SDL_RenderFillRect(renderer, &overlay);

    if (font) {
        SDL_Color textColor = { 255, 0, 0, 255 };
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 30, textColor);

        SDL_Color scoreColor = { 255, 255, 255, 255 };
        DrawText("Final Score: " + to_string(score), SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 + 30, scoreColor);
        DrawText("Press ENTER to continue", SCREEN_WIDTH / 2 - 140, SCREEN_HEIGHT / 2 + 70, scoreColor);
    }
    else {
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
    Uint32 dropInterval = 500; 

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) HandleInput(e);

        if (currentState == PLAYING) {
            Uint32 now = SDL_GetTicks();
            dropInterval = 500 - (level - 1) * 25;
            if (dropInterval < 100) dropInterval = 100;

            if (now - lastTick > dropInterval) {
                Drop();
                ClearLines();
                lastTick = now;
            }
        }

        Draw();
        SDL_Delay(16);
    }

    Quit();
    return 0;
}