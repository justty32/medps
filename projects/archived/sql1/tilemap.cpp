#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class TextureManager {
private:
    std::unordered_map<std::string, SDL_Texture*> textures;
    SDL_Renderer* renderer;

public:
    TextureManager(SDL_Renderer* r) : renderer(r) {}
    
    ~TextureManager() {
        for (auto& pair : textures) {
            SDL_DestroyTexture(pair.second);
        }
    }
    
    SDL_Texture* loadTexture(const std::string& path) {
        if (textures.find(path) != textures.end()) {
            return textures[path];
        }
        
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
            return nullptr;
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        
        if (!texture) {
            std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        textures[path] = texture;
        return texture;
    }
};

class TileSet {
private:
    SDL_Texture* texture;
    int tileWidth, tileHeight;
    int tilesPerRow, tilesPerCol;
    int totalTiles;

public:
    TileSet(SDL_Texture* tex, int tw, int th) 
        : texture(tex), tileWidth(tw), tileHeight(th) {
        int texWidth, texHeight;
        SDL_QueryTexture(texture, nullptr, nullptr, &texWidth, &texHeight);
        tilesPerRow = texWidth / tileWidth;
        tilesPerCol = texHeight / tileHeight;
        totalTiles = tilesPerRow * tilesPerCol;
    }
    
    void renderTile(SDL_Renderer* renderer, int tileId, int x, int y) {
        if (tileId >= totalTiles || tileId < 0) return;
        
        int srcX = (tileId % tilesPerRow) * tileWidth;
        int srcY = (tileId / tilesPerRow) * tileHeight;
        
        SDL_Rect srcRect = {srcX, srcY, tileWidth, tileHeight};
        SDL_Rect destRect = {x, y, tileWidth, tileHeight};
        
        SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
    }
    
    int getTileWidth() const { return tileWidth; }
    int getTileHeight() const { return tileHeight; }
};

class Camera {
private:
    int x, y;
    int width, height;
    int worldWidth, worldHeight;

public:
    Camera(int w, int h, int worldW, int worldH) 
        : x(0), y(0), width(w), height(h), worldWidth(worldW), worldHeight(worldH) {}
    
    void setPosition(int newX, int newY) {
        x = std::max(0, std::min(newX, worldWidth - width));
        y = std::max(0, std::min(newY, worldHeight - height));
    }
    
    void move(int dx, int dy) {
        setPosition(x + dx, y + dy);
    }
    
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    bool isVisible(int objX, int objY, int objW, int objH) const {
        return !(objX + objW < x || objX > x + width ||
                objY + objH < y || objY > y + height);
    }
};

class TileMap {
private:
    std::vector<std::vector<int>> tiles;
    std::vector<std::unique_ptr<TileSet>> tilesets;
    std::vector<int> tilesetMapping; // 哪個tile屬於哪個tileset
    int mapWidth, mapHeight;
    int tileWidth, tileHeight;
    
    // 優化：批次渲染
    struct RenderBatch {
        SDL_Texture* texture;
        std::vector<SDL_Rect> srcRects;
        std::vector<SDL_Rect> destRects;
    };

public:
    TileMap(int w, int h, int tw, int th) 
        : mapWidth(w), mapHeight(h), tileWidth(tw), tileHeight(th) {
        tiles.resize(mapHeight, std::vector<int>(mapWidth, 0));
        tilesetMapping.resize(10000, 0); // 預設支援10000個不同的tile
    }
    
    void addTileSet(std::unique_ptr<TileSet> tileset) {
        tilesets.push_back(std::move(tileset));
    }
    
    void setTile(int x, int y, int tileId) {
        if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
            tiles[y][x] = tileId;
        }
    }
    
    int getTile(int x, int y) const {
        if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
            return tiles[y][x];
        }
        return 0;
    }
    
    void render(SDL_Renderer* renderer, const Camera& camera) {
        // 計算可見區域
        int startX = camera.getX() / tileWidth;
        int startY = camera.getY() / tileHeight;
        int endX = (camera.getX() + camera.getWidth()) / tileWidth + 1;
        int endY = (camera.getY() + camera.getHeight()) / tileHeight + 1;
        
        // 限制在地圖範圍內
        startX = std::max(0, startX);
        startY = std::max(0, startY);
        endX = std::min(mapWidth, endX);
        endY = std::min(mapHeight, endY);
        
        // 渲染可見tile
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                int tileId = tiles[y][x];
                if (tileId > 0 && tileId < tilesets.size() * 1000) { // 假設每個tileset最多1000個tile
                    int tilesetIndex = tilesetMapping[tileId];
                    if (tilesetIndex < tilesets.size()) {
                        int screenX = x * tileWidth - camera.getX();
                        int screenY = y * tileHeight - camera.getY();
                        tilesets[tilesetIndex]->renderTile(renderer, tileId % 1000, screenX, screenY);
                    }
                }
            }
        }
    }
    
    // 從檔案載入地圖
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open map file: " << filename << std::endl;
            return false;
        }
        
        std::string line;
        int row = 0;
        
        while (std::getline(file, line) && row < mapHeight) {
            std::stringstream ss(line);
            std::string cell;
            int col = 0;
            
            while (std::getline(ss, cell, ',') && col < mapWidth) {
                tiles[row][col] = std::stoi(cell);
                col++;
            }
            row++;
        }
        
        file.close();
        return true;
    }
    
    // 儲存地圖到檔案
    bool saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to create map file: " << filename << std::endl;
            return false;
        }
        
        for (int y = 0; y < mapHeight; ++y) {
            for (int x = 0; x < mapWidth; ++x) {
                file << tiles[y][x];
                if (x < mapWidth - 1) file << ",";
            }
            file << "\n";
        }
        
        file.close();
        return true;
    }
    
    int getMapWidth() const { return mapWidth; }
    int getMapHeight() const { return mapHeight; }
    int getTileWidth() const { return tileWidth; }
    int getTileHeight() const { return tileHeight; }
};

class TileMapEngine {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<TileMap> tileMap;
    std::unique_ptr<Camera> camera;
    bool isRunning;
    
    const int SCREEN_WIDTH = 1024;
    const int SCREEN_HEIGHT = 768;
    const int TILE_SIZE = 32;

public:
    TileMapEngine() : window(nullptr), renderer(nullptr), isRunning(false) {}
    
    ~TileMapEngine() {
        cleanup();
    }
    
    bool initialize() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            std::cerr << "SDL_image could not initialize! IMG_Error: " << IMG_GetError() << std::endl;
            return false;
        }
        
        window = SDL_CreateWindow("高可用性 2D Tilemap",
                                 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                 SCREEN_WIDTH, SCREEN_HEIGHT,
                                 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        
        if (!window) {
            std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        textureManager = std::make_unique<TextureManager>(renderer);
        
        // 創建50x50的測試地圖
        tileMap = std::make_unique<TileMap>(50, 50, TILE_SIZE, TILE_SIZE);
        camera = std::make_unique<Camera>(SCREEN_WIDTH, SCREEN_HEIGHT, 
                                        50 * TILE_SIZE, 50 * TILE_SIZE);
        
        // 創建簡單的測試圖案
        createTestMap();
        
        isRunning = true;
        return true;
    }
    
    void createTestMap() {
        // 創建簡單的測試圖案
        for (int y = 0; y < 50; ++y) {
            for (int x = 0; x < 50; ++x) {
                if ((x + y) % 2 == 0) {
                    tileMap->setTile(x, y, 1);
                } else {
                    tileMap->setTile(x, y, 2);
                }
                
                // 添加一些邊界
                if (x == 0 || x == 49 || y == 0 || y == 49) {
                    tileMap->setTile(x, y, 3);
                }
            }
        }
    }
    
    void handleEvents() {
        SDL_Event e;
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                isRunning = false;
            }
            
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
                // 處理視窗大小調整
                int newWidth, newHeight;
                SDL_GetWindowSize(window, &newWidth, &newHeight);
                camera = std::make_unique<Camera>(newWidth, newHeight,
                                                50 * TILE_SIZE, 50 * TILE_SIZE);
            }
        }
        
        // 相機移動控制
        const int CAMERA_SPEED = 5;
        if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) {
            camera->move(0, -CAMERA_SPEED);
        }
        if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) {
            camera->move(0, CAMERA_SPEED);
        }
        if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT]) {
            camera->move(-CAMERA_SPEED, 0);
        }
        if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) {
            camera->move(CAMERA_SPEED, 0);
        }
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);
        
        // 渲染tilemap
        if (tileMap) {
            renderSimpleTiles();
        }
        
        SDL_RenderPresent(renderer);
    }
    
    void renderSimpleTiles() {
        // 簡單的彩色方塊渲染（不需要圖片檔案）
        int startX = camera->getX() / TILE_SIZE;
        int startY = camera->getY() / TILE_SIZE;
        int endX = (camera->getX() + camera->getWidth()) / TILE_SIZE + 1;
        int endY = (camera->getY() + camera->getHeight()) / TILE_SIZE + 1;
        
        startX = std::max(0, startX);
        startY = std::max(0, startY);
        endX = std::min(50, endX);
        endY = std::min(50, endY);
        
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                int tileId = tileMap->getTile(x, y);
                if (tileId > 0) {
                    SDL_Rect rect = {
                        x * TILE_SIZE - camera->getX(),
                        y * TILE_SIZE - camera->getY(),
                        TILE_SIZE,
                        TILE_SIZE
                    };
                    
                    // 根據tile ID設定不同顏色
                    switch (tileId) {
                        case 1:
                            SDL_SetRenderDrawColor(renderer, 100, 150, 100, 255);
                            break;
                        case 2:
                            SDL_SetRenderDrawColor(renderer, 150, 100, 100, 255);
                            break;
                        case 3:
                            SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
                            break;
                        default:
                            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
                    }
                    
                    SDL_RenderFillRect(renderer, &rect);
                    
                    // 繪製邊框
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                }
            }
        }
    }
    
    void run() {
        if (!initialize()) {
            return;
        }
        
        Uint32 frameStart;
        int frameTime;
        const int FPS = 60;
        const int frameDelay = 1000 / FPS;
        
        while (isRunning) {
            frameStart = SDL_GetTicks();
            
            handleEvents();
            render();
            
            frameTime = SDL_GetTicks() - frameStart;
            if (frameDelay > frameTime) {
                SDL_Delay(frameDelay - frameTime);
            }
        }
        
        cleanup();
    }
    
    void cleanup() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        IMG_Quit();
        SDL_Quit();
    }
};

// 使用範例
int main(int argc, char* argv[]) {
    TileMapEngine engine;
    engine.run();
    return 0;
}

// 編譯指令 (Linux/Mac):
// g++ -std=c++14 tilemap.cpp -lSDL2 -lSDL2_image -o tilemap

// 編譯指令 (Windows with MinGW):
// g++ -std=c++14 tilemap.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -o tilemap.exe