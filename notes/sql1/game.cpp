#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>
#include <queue>

// 前向聲明
class GameState;
class Player;

// 枚舉定義
enum class TerrainType {
    GRASS = 1,
    FOREST,
    MOUNTAIN,
    WATER,
    DESERT,
    SNOW
};

enum class ResourceType {
    NONE = 0,
    FOOD,
    WOOD,
    STONE,
    IRON,
    GOLD
};

enum class UnitType {
    WARRIOR = 1,
    ARCHER,
    CAVALRY,
    SIEGE,
    WORKER
};

enum class BuildingType {
    NONE = 0,
    CITY_CENTER,
    BARRACKS,
    FARM,
    MINE,
    LUMBER_MILL
};

// 遊戲常數
const int TILE_SIZE = 32;
const int MAP_WIDTH = 80;
const int MAP_HEIGHT = 60;
const int MAX_PLAYERS = 4;

// 工具類別
struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct Color {
    Uint8 r, g, b, a;
    Color(Uint8 r = 255, Uint8 g = 255, Uint8 b = 255, Uint8 a = 255) : r(r), g(g), b(b), a(a) {}
};

// 地形系統
class Terrain {
public:
    TerrainType type;
    int movementCost;
    int defenseBonus;
    Color color;
    
    Terrain(TerrainType t = TerrainType::GRASS) : type(t) {
        switch (type) {
            case TerrainType::GRASS:
                movementCost = 1;
                defenseBonus = 0;
                color = Color(50, 150, 50);
                break;
            case TerrainType::FOREST:
                movementCost = 2;
                defenseBonus = 1;
                color = Color(34, 100, 34);
                break;
            case TerrainType::MOUNTAIN:
                movementCost = 3;
                defenseBonus = 2;
                color = Color(120, 100, 80);
                break;
            case TerrainType::WATER:
                movementCost = 999; // 不可通行
                defenseBonus = 0;
                color = Color(50, 100, 200);
                break;
            case TerrainType::DESERT:
                movementCost = 2;
                defenseBonus = 0;
                color = Color(200, 180, 120);
                break;
            case TerrainType::SNOW:
                movementCost = 2;
                defenseBonus = 0;
                color = Color(240, 240, 255);
                break;
        }
    }
};

// 資源系統
class Resource {
public:
    ResourceType type;
    int amount;
    int maxAmount;
    Color color;
    
    Resource(ResourceType t = ResourceType::NONE, int amt = 0) : type(t), amount(amt), maxAmount(amt) {
        switch (type) {
            case ResourceType::FOOD:
                color = Color(255, 255, 0);
                break;
            case ResourceType::WOOD:
                color = Color(139, 69, 19);
                break;
            case ResourceType::STONE:
                color = Color(128, 128, 128);
                break;
            case ResourceType::IRON:
                color = Color(70, 70, 70);
                break;
            case ResourceType::GOLD:
                color = Color(255, 215, 0);
                break;
            default:
                color = Color(255, 255, 255);
        }
    }
};

// 建築系統
class Building {
public:
    BuildingType type;
    int health;
    int maxHealth;
    int playerId;
    std::vector<ResourceType> production;
    int productionRate;
    Color color;
    
    Building(BuildingType t = BuildingType::NONE, int owner = -1) 
        : type(t), playerId(owner), productionRate(1) {
        switch (type) {
            case BuildingType::CITY_CENTER:
                maxHealth = 200;
                production = {ResourceType::FOOD};
                color = Color(255, 255, 255);
                break;
            case BuildingType::BARRACKS:
                maxHealth = 100;
                color = Color(150, 0, 0);
                break;
            case BuildingType::FARM:
                maxHealth = 50;
                production = {ResourceType::FOOD};
                productionRate = 3;
                color = Color(255, 255, 0);
                break;
            case BuildingType::MINE:
                maxHealth = 75;
                production = {ResourceType::STONE, ResourceType::IRON};
                color = Color(100, 100, 100);
                break;
            case BuildingType::LUMBER_MILL:
                maxHealth = 60;
                production = {ResourceType::WOOD};
                productionRate = 2;
                color = Color(139, 69, 19);
                break;
            default:
                maxHealth = 1;
        }
        health = maxHealth;
    }
    
    bool isDestroyed() const { return health <= 0; }
};

// 單位系統
class Unit {
public:
    UnitType type;
    int health;
    int maxHealth;
    int attack;
    int defense;
    int movement;
    int maxMovement;
    int playerId;
    Point position;
    bool hasActed;
    Color color;
    
    Unit(UnitType t, int owner, Point pos) 
        : type(t), playerId(owner), position(pos), hasActed(false) {
        switch (type) {
            case UnitType::WARRIOR:
                maxHealth = 100;
                attack = 20;
                defense = 15;
                maxMovement = 2;
                color = Color(200, 0, 0);
                break;
            case UnitType::ARCHER:
                maxHealth = 70;
                attack = 25;
                defense = 10;
                maxMovement = 2;
                color = Color(0, 150, 0);
                break;
            case UnitType::CAVALRY:
                maxHealth = 120;
                attack = 30;
                defense = 12;
                maxMovement = 4;
                color = Color(100, 50, 0);
                break;
            case UnitType::SIEGE:
                maxHealth = 150;
                attack = 50;
                defense = 5;
                maxMovement = 1;
                color = Color(80, 80, 80);
                break;
            case UnitType::WORKER:
                maxHealth = 50;
                attack = 5;
                defense = 5;
                maxMovement = 2;
                color = Color(255, 255, 0);
                break;
        }
        health = maxHealth;
        movement = maxMovement;
    }
    
    bool canMove() const { return movement > 0 && !hasActed; }
    bool isAlive() const { return health > 0; }
    
    void resetTurn() {
        movement = maxMovement;
        hasActed = false;
    }
};

// 玩家系統
class Player {
public:
    int id;
    std::string name;
    Color color;
    std::unordered_map<ResourceType, int> resources;
    std::vector<std::unique_ptr<Unit>> units;
    std::vector<Point> cities;
    bool isActive;
    
    Player(int playerId, const std::string& playerName, Color playerColor) 
        : id(playerId), name(playerName), color(playerColor), isActive(true) {
        // 初始資源
        resources[ResourceType::FOOD] = 50;
        resources[ResourceType::WOOD] = 30;
        resources[ResourceType::STONE] = 20;
        resources[ResourceType::IRON] = 10;
        resources[ResourceType::GOLD] = 100;
    }
    
    void addResource(ResourceType type, int amount) {
        resources[type] += amount;
    }
    
    bool hasResource(ResourceType type, int amount) const {
        auto it = resources.find(type);
        return it != resources.end() && it->second >= amount;
    }
    
    bool spendResource(ResourceType type, int amount) {
        if (hasResource(type, amount)) {
            resources[type] -= amount;
            return true;
        }
        return false;
    }
    
    void addUnit(std::unique_ptr<Unit> unit) {
        units.push_back(std::move(unit));
    }
    
    void resetTurn() {
        for (auto& unit : units) {
            unit->resetTurn();
        }
    }
};

// 遊戲狀態管理
class GameState {
public:
    std::vector<std::unique_ptr<Player>> players;
    int currentPlayerIndex;
    int turnNumber;
    Point selectedTile;
    Unit* selectedUnit;
    bool showUI;
    
    GameState() : currentPlayerIndex(0), turnNumber(1), selectedTile(-1, -1), selectedUnit(nullptr), showUI(true) {
        // 創建玩家
        players.push_back(std::make_unique<Player>(0, "Player 1", Color(255, 0, 0)));
        players.push_back(std::make_unique<Player>(1, "Player 2", Color(0, 0, 255)));
        players.push_back(std::make_unique<Player>(2, "Player 3", Color(0, 255, 0)));
        players.push_back(std::make_unique<Player>(3, "Player 4", Color(255, 255, 0)));
    }
    
    Player* getCurrentPlayer() {
        if (currentPlayerIndex < players.size()) {
            return players[currentPlayerIndex].get();
        }
        return nullptr;
    }
    
    void nextTurn() {
        currentPlayerIndex = (currentPlayerIndex + 1) % players.size();
        if (currentPlayerIndex == 0) {
            turnNumber++;
            // 新回合開始，重置所有玩家
            for (auto& player : players) {
                player->resetTurn();
            }
        }
        selectedUnit = nullptr;
        selectedTile = Point(-1, -1);
    }
    
    Unit* getUnitAt(Point pos) {
        for (auto& player : players) {
            for (auto& unit : player->units) {
                if (unit->position == pos && unit->isAlive()) {
                    return unit.get();
                }
            }
        }
        return nullptr;
    }
};

// 文字渲染管理器
class TextRenderer {
private:
    TTF_Font* font;
    SDL_Renderer* renderer;

public:
    TextRenderer(SDL_Renderer* r) : renderer(r), font(nullptr) {}
    
    bool initialize() {
        if (TTF_Init() == -1) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
            return false;
        }
        
        // 嘗試載入系統字體
        font = TTF_OpenFont("/System/Library/Fonts/Arial.ttf", 16);
        if (!font) {
            font = TTF_OpenFont("/Windows/Fonts/arial.ttf", 16);
        }
        if (!font) {
            font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
        }
        if (!font) {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
            return false;
        }
        
        return true;
    }
    
    void renderText(const std::string& text, int x, int y, Color color = Color(255, 255, 255)) {
        if (!font) return;
        
        SDL_Color sdlColor = {color.r, color.g, color.b, color.a};
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), sdlColor);
        if (!surface) return;
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_FreeSurface(surface);
            return;
        }
        
        SDL_Rect destRect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &destRect);
        
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }
    
    ~TextRenderer() {
        if (font) {
            TTF_CloseFont(font);
        }
        TTF_Quit();
    }
};

// 攝影機系統
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
    
    Point screenToWorld(int screenX, int screenY) const {
        return Point((screenX + x) / TILE_SIZE, (screenY + y) / TILE_SIZE);
    }
    
    Point worldToScreen(int worldX, int worldY) const {
        return Point(worldX * TILE_SIZE - x, worldY * TILE_SIZE - y);
    }
    
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    bool isVisible(int objX, int objY, int objW = TILE_SIZE, int objH = TILE_SIZE) const {
        return !(objX + objW < x || objX > x + width ||
                objY + objH < y || objY > y + height);
    }
};

// 主要遊戲地圖類別
class StrategyMap {
private:
    std::vector<std::vector<Terrain>> terrain;
    std::vector<std::vector<Resource>> resources;
    std::vector<std::vector<std::unique_ptr<Building>>> buildings;
    int width, height;

public:
    StrategyMap(int w, int h) : width(w), height(h) {
        terrain.resize(height, std::vector<Terrain>(width));
        resources.resize(height, std::vector<Resource>(width));
        buildings.resize(height, std::vector<std::unique_ptr<Building>>(width));
        
        generateMap();
    }
    
    void generateMap() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> terrainDist(1, 6);
        std::uniform_int_distribution<> resourceDist(0, 5);
        std::uniform_int_distribution<> amountDist(10, 50);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // 生成地形
                terrain[y][x] = Terrain(static_cast<TerrainType>(terrainDist(gen)));
                
                // 生成資源（30%機率）
                if (resourceDist(gen) == 0) {
                    ResourceType resType = static_cast<ResourceType>(resourceDist(gen));
                    if (resType != ResourceType::NONE) {
                        resources[y][x] = Resource(resType, amountDist(gen));
                    }
                }
            }
        }
        
        // 生成起始城市
        placeStartingCities();
    }
    
    void placeStartingCities() {
        std::vector<Point> startPositions = {
            Point(10, 10), Point(width-15, 10), 
            Point(10, height-15), Point(width-15, height-15)
        };
        
        for (size_t i = 0; i < startPositions.size() && i < MAX_PLAYERS; ++i) {
            Point pos = startPositions[i];
            if (isValidPosition(pos.x, pos.y)) {
                buildings[pos.y][pos.x] = std::make_unique<Building>(BuildingType::CITY_CENTER, i);
                terrain[pos.y][pos.x] = Terrain(TerrainType::GRASS);
            }
        }
    }
    
    bool isValidPosition(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }
    
    Terrain& getTerrain(int x, int y) {
        if (isValidPosition(x, y)) {
            return terrain[y][x];
        }
        static Terrain invalid;
        return invalid;
    }
    
    Resource& getResource(int x, int y) {
        if (isValidPosition(x, y)) {
            return resources[y][x];
        }
        static Resource invalid;
        return invalid;
    }
    
    Building* getBuilding(int x, int y) {
        if (isValidPosition(x, y)) {
            return buildings[y][x].get();
        }
        return nullptr;
    }
    
    void setBuilding(int x, int y, std::unique_ptr<Building> building) {
        if (isValidPosition(x, y)) {
            buildings[y][x] = std::move(building);
        }
    }
    
    void render(SDL_Renderer* renderer, const Camera& camera, const GameState& gameState) {
        int startX = camera.getX() / TILE_SIZE;
        int startY = camera.getY() / TILE_SIZE;
        int endX = (camera.getX() + camera.getWidth()) / TILE_SIZE + 2;
        int endY = (camera.getY() + camera.getHeight()) / TILE_SIZE + 2;
        
        startX = std::max(0, startX);
        startY = std::max(0, startY);
        endX = std::min(width, endX);
        endY = std::min(height, endY);
        
        // 渲染地形
        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                Point screenPos = camera.worldToScreen(x, y);
                SDL_Rect rect = {screenPos.x, screenPos.y, TILE_SIZE, TILE_SIZE};
                
                // 地形
                Color terrainColor = terrain[y][x].color;
                SDL_SetRenderDrawColor(renderer, terrainColor.r, terrainColor.g, terrainColor.b, terrainColor.a);
                SDL_RenderFillRect(renderer, &rect);
                
                // 資源
                if (resources[y][x].type != ResourceType::NONE && resources[y][x].amount > 0) {
                    Color resColor = resources[y][x].color;
                    SDL_Rect resRect = {screenPos.x + 2, screenPos.y + 2, 6, 6};
                    SDL_SetRenderDrawColor(renderer, resColor.r, resColor.g, resColor.b, resColor.a);
                    SDL_RenderFillRect(renderer, &resRect);
                }
                
                // 建築
                if (buildings[y][x]) {
                    Color buildColor = buildings[y][x]->color;
                    SDL_Rect buildRect = {screenPos.x + 8, screenPos.y + 8, TILE_SIZE - 16, TILE_SIZE - 16};
                    SDL_SetRenderDrawColor(renderer, buildColor.r, buildColor.g, buildColor.b, buildColor.a);
                    SDL_RenderFillRect(renderer, &buildRect);
                }
                
                // 選中的格子
                if (gameState.selectedTile.x == x && gameState.selectedTile.y == y) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                    SDL_RenderDrawRect(renderer, &SDL_Rect{rect.x+1, rect.y+1, rect.w-2, rect.h-2});
                }
                
                // 格線
                SDL_SetRenderDrawColor(renderer, 64, 64, 64, 128);
                SDL_RenderDrawRect(renderer, &rect);
            }
        }
        
        // 渲染單位
        for (const auto& player : gameState.players) {
            for (const auto& unit : player->units) {
                if (unit->isAlive()) {
                    Point screenPos = camera.worldToScreen(unit->position.x, unit->position.y);
                    if (camera.isVisible(screenPos.x, screenPos.y)) {
                        SDL_Rect unitRect = {screenPos.x + 4, screenPos.y + 4, TILE_SIZE - 8, TILE_SIZE - 8};
                        
                        // 單位顏色（玩家顏色）
                        Color playerColor = player->color;
                        SDL_SetRenderDrawColor(renderer, playerColor.r, playerColor.g, playerColor.b, playerColor.a);
                        SDL_RenderFillRect(renderer, &unitRect);
                        
                        // 單位類型指示
                        Color unitColor = unit->color;
                        SDL_Rect typeRect = {screenPos.x + 12, screenPos.y + 12, 8, 8};
                        SDL_SetRenderDrawColor(renderer, unitColor.r, unitColor.g, unitColor.b, unitColor.a);
                        SDL_RenderFillRect(renderer, &typeRect);
                        
                        // 選中的單位
                        if (gameState.selectedUnit == unit.get()) {
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                            SDL_RenderDrawRect(renderer, &unitRect);
                        }
                        
                        // 血量條
                        if (unit->health < unit->maxHealth) {
                            int barWidth = (TILE_SIZE - 8) * unit->health / unit->maxHealth;
                            SDL_Rect healthBg = {screenPos.x + 4, screenPos.y + 2, TILE_SIZE - 8, 2};
                            SDL_Rect healthBar = {screenPos.x + 4, screenPos.y + 2, barWidth, 2};
                            
                            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                            SDL_RenderFillRect(renderer, &healthBg);
                            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                            SDL_RenderFillRect(renderer, &healthBar);
                        }
                    }
                }
            }
        }
    }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

// 主遊戲引擎
class StrategyGameEngine {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    std::unique_ptr<StrategyMap> map;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<GameState> gameState;
    std::unique_ptr<TextRenderer> textRenderer;
    bool isRunning;
    
    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;

public:
    StrategyGameEngine() : window(nullptr), renderer(nullptr), isRunning(false) {}
    
    ~StrategyGameEngine() {
        cleanup();
    }
    
    bool initialize() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
            return false;
        }
        
        window = SDL_CreateWindow("戰略遊戲引擎",
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
        
        // 初始化文字渲染
        textRenderer = std::make_unique<TextRenderer>(renderer);
        textRenderer->initialize();
        
        // 創建遊戲世界
        map = std::make_unique<StrategyMap>(MAP_WIDTH, MAP_HEIGHT);
        camera = std::make_unique<Camera>(SCREEN_WIDTH, SCREEN_HEIGHT, 
                                        MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE);
        gameState = std::make_unique<GameState>();
        
        // 為每個玩家添加初始單位
        initializeStartingUnits();
        
        isRunning = true;
        return true;
    }
    
    void initializeStartingUnits() {
        std::vector<Point> startPositions = {
            Point(10, 10), Point(MAP_WIDTH-15, 10), 
            Point(10, MAP_HEIGHT-15), Point(MAP_WIDTH-15, MAP_HEIGHT-15)
        };
        
        for (size_t i = 0; i < gameState->players.size(); ++i) {
            Point startPos = startPositions[i];
            
            // 每個玩家開始時有：2個戰士、1個弓箭手、1個工人
            gameState->players[i]->addUnit(std::make_unique<Unit>(UnitType::WARRIOR, i, Point(startPos.x-1, startPos.y)));
            gameState->players[i]->addUnit(std::make_unique<Unit>(UnitType::WARRIOR, i, Point(startPos.x+1, startPos.y)));
            gameState->players[i]->addUnit(std::make_unique<Unit>(UnitType::ARCHER, i, Point(startPos.x, startPos.y-1)));
            gameState->players[i]->addUnit(std::make_unique<Unit>(UnitType::WORKER, i, Point(startPos.x, startPos.y+1)));
        }
    }
    
    void handleEvents() {
        SDL_Event e;
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                isRunning = false;
            }
            
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    handleMouseClick(e.button.x, e.button.y);
                }
            }
            
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_SPACE:
                        gameState->nextTurn();
                        break;
                    case SDLK_h:
                        gameState->showUI = !gameState->showUI;
                        break;
                    case SDLK_1:
                        if (gameState->selectedUnit && gameState->selectedUnit->type == UnitType::WORKER) {
                            tryBuildBuilding(BuildingType::FARM);
                        }
                        break;
                    case SDLK_2:
                        if (gameState->selectedUnit && gameState->selectedUnit->type == UnitType::WORKER) {
                            tryBuildBuilding(BuildingType::BARRACKS);
                        }
                        break;
                }
            }
        }
        
        // 相機移動
        const int CAMERA_SPEED = 8;
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
    
    void handleMouseClick(int mouseX, int mouseY) {
        Point worldPos = camera->screenToWorld(mouseX, mouseY);
        
        if (!map->isValidPosition(worldPos.x, worldPos.y)) return;
        
        gameState->selectedTile = worldPos;
        
        Unit* clickedUnit = gameState->getUnitAt(worldPos);
        Player* currentPlayer = gameState->getCurrentPlayer();
        
        if (clickedUnit && clickedUnit->playerId == currentPlayer->id) {
            // 選中自己的單位
            gameState->selectedUnit = clickedUnit;
        } else if (gameState->selectedUnit) {
            // 有選中的單位，嘗試移動或攻擊
            if (clickedUnit && clickedUnit->playerId != currentPlayer->id) {
                // 攻擊敵人單位
                tryAttack(gameState->selectedUnit, clickedUnit);
            } else {
                // 嘗試移動到目標位置
                tryMoveUnit(gameState->selectedUnit, worldPos);
            }
        } else {
            // 沒有選中單位，清除選擇
            gameState->selectedUnit = nullptr;
        }
    }
    
    void tryMoveUnit(Unit* unit, Point target) {
        if (!unit || !unit->canMove()) return;
        
        int distance = abs(target.x - unit->position.x) + abs(target.y - unit->position.y);
        if (distance > unit->movement) return;
        
        // 檢查目標位置是否有其他單位
        if (gameState->getUnitAt(target)) return;
        
        // 檢查地形是否可通行
        Terrain& terrain = map->getTerrain(target.x, target.y);
        if (terrain.movementCost >= 999) return;
        
        // 移動單位
        unit->position = target;
        unit->movement -= distance;
        unit->hasActed = true;
        
        // 如果是工人，嘗試收集資源
        if (unit->type == UnitType::WORKER) {
            Resource& resource = map->getResource(target.x, target.y);
            if (resource.type != ResourceType::NONE && resource.amount > 0) {
                Player* player = gameState->players[unit->playerId].get();
                int collected = std::min(10, resource.amount);
                resource.amount -= collected;
                player->addResource(resource.type, collected);
            }
        }
    }
    
    void tryAttack(Unit* attacker, Unit* defender) {
        if (!attacker || !defender || attacker->hasActed) return;
        
        int distance = abs(defender->position.x - attacker->position.x) + 
                      abs(defender->position.y - attacker->position.y);
        
        // 檢查攻擊範圍（大部分單位需要相鄰）
        int attackRange = (attacker->type == UnitType::ARCHER) ? 2 : 1;
        if (distance > attackRange) return;
        
        // 計算傷害
        Terrain& defenderTerrain = map->getTerrain(defender->position.x, defender->position.y);
        int damage = std::max(1, attacker->attack - defender->defense - defenderTerrain.defenseBonus);
        
        defender->health -= damage;
        attacker->hasActed = true;
        
        // 反擊（如果防守方還活著且在攻擊範圍內）
        if (defender->isAlive() && distance == 1) {
            int counterDamage = std::max(1, defender->attack - attacker->defense);
            attacker->health -= counterDamage;
        }
        
        // 移除死亡單位
        if (!defender->isAlive()) {
            removeDeadUnits();
        }
        if (!attacker->isAlive()) {
            removeDeadUnits();
            gameState->selectedUnit = nullptr;
        }
    }
    
    void removeDeadUnits() {
        for (auto& player : gameState->players) {
            player->units.erase(
                std::remove_if(player->units.begin(), player->units.end(),
                    [](const std::unique_ptr<Unit>& unit) { return !unit->isAlive(); }),
                player->units.end()
            );
        }
    }
    
    void tryBuildBuilding(BuildingType buildingType) {
        if (!gameState->selectedUnit || gameState->selectedUnit->type != UnitType::WORKER) return;
        
        Player* player = gameState->players[gameState->selectedUnit->playerId].get();
        Point pos = gameState->selectedUnit->position;
        
        // 檢查是否已有建築
        if (map->getBuilding(pos.x, pos.y)) return;
        
        // 檢查資源需求
        bool canBuild = false;
        switch (buildingType) {
            case BuildingType::FARM:
                canBuild = player->hasResource(ResourceType::WOOD, 20) && 
                          player->hasResource(ResourceType::FOOD, 10);
                if (canBuild) {
                    player->spendResource(ResourceType::WOOD, 20);
                    player->spendResource(ResourceType::FOOD, 10);
                }
                break;
            case BuildingType::BARRACKS:
                canBuild = player->hasResource(ResourceType::WOOD, 50) && 
                          player->hasResource(ResourceType::STONE, 30);
                if (canBuild) {
                    player->spendResource(ResourceType::WOOD, 50);
                    player->spendResource(ResourceType::STONE, 30);
                }
                break;
            case BuildingType::MINE:
                canBuild = player->hasResource(ResourceType::WOOD, 30) && 
                          player->hasResource(ResourceType::STONE, 20);
                if (canBuild) {
                    player->spendResource(ResourceType::WOOD, 30);
                    player->spendResource(ResourceType::STONE, 20);
                }
                break;
            case BuildingType::LUMBER_MILL:
                canBuild = player->hasResource(ResourceType::WOOD, 40);
                if (canBuild) {
                    player->spendResource(ResourceType::WOOD, 40);
                }
                break;
        }
        
        if (canBuild) {
            map->setBuilding(pos.x, pos.y, 
                std::make_unique<Building>(buildingType, gameState->selectedUnit->playerId));
            gameState->selectedUnit->hasActed = true;
        }
    }
    
    void processTurn() {
        // 處理建築生產
        for (int y = 0; y < map->getHeight(); ++y) {
            for (int x = 0; x < map->getWidth(); ++x) {
                Building* building = map->getBuilding(x, y);
                if (building && building->playerId >= 0) {
                    Player* owner = gameState->players[building->playerId].get();
                    
                    for (ResourceType resType : building->production) {
                        owner->addResource(resType, building->productionRate);
                    }
                }
            }
        }
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_RenderClear(renderer);
        
        // 渲染地圖
        map->render(renderer, *camera, *gameState);
        
        // 渲染UI
        if (gameState->showUI) {
            renderUI();
        }
        
        SDL_RenderPresent(renderer);
    }
    
    void renderUI() {
        Player* currentPlayer = gameState->getCurrentPlayer();
        if (!currentPlayer) return;
        
        // 背景面板
        SDL_Rect uiPanel = {10, 10, 300, 200};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &uiPanel);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &uiPanel);
        
        int yOffset = 25;
        
        // 玩家信息
        std::string playerInfo = "Player: " + currentPlayer->name + " | Turn: " + std::to_string(gameState->turnNumber);
        textRenderer->renderText(playerInfo, 20, yOffset, Color(255, 255, 255));
        yOffset += 20;
        
        // 資源信息
        textRenderer->renderText("Resources:", 20, yOffset, Color(200, 200, 200));
        yOffset += 15;
        
        std::vector<std::pair<ResourceType, std::string>> resourceNames = {
            {ResourceType::FOOD, "Food"}, {ResourceType::WOOD, "Wood"},
            {ResourceType::STONE, "Stone"}, {ResourceType::IRON, "Iron"},
            {ResourceType::GOLD, "Gold"}
        };
        
        for (const auto& res : resourceNames) {
            std::string resText = res.second + ": " + std::to_string(currentPlayer->resources[res.first]);
            textRenderer->renderText(resText, 30, yOffset, Color(180, 180, 180));
            yOffset += 15;
        }
        
        // 選中單位信息
        if (gameState->selectedUnit) {
            yOffset += 10;
            textRenderer->renderText("Selected Unit:", 20, yOffset, Color(255, 255, 0));
            yOffset += 15;
            
            std::string unitType;
            switch (gameState->selectedUnit->type) {
                case UnitType::WARRIOR: unitType = "Warrior"; break;
                case UnitType::ARCHER: unitType = "Archer"; break;
                case UnitType::CAVALRY: unitType = "Cavalry"; break;
                case UnitType::SIEGE: unitType = "Siege Engine"; break;
                case UnitType::WORKER: unitType = "Worker"; break;
            }
            
            textRenderer->renderText(unitType, 30, yOffset, Color(200, 200, 200));
            yOffset += 15;
            
            std::string healthText = "HP: " + std::to_string(gameState->selectedUnit->health) + 
                                   "/" + std::to_string(gameState->selectedUnit->maxHealth);
            textRenderer->renderText(healthText, 30, yOffset, Color(200, 200, 200));
            yOffset += 15;
            
            std::string moveText = "Movement: " + std::to_string(gameState->selectedUnit->movement);
            textRenderer->renderText(moveText, 30, yOffset, Color(200, 200, 200));
        }
        
        // 控制說明
        SDL_Rect helpPanel = {10, SCREEN_HEIGHT - 150, 400, 140};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(renderer, &helpPanel);
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &helpPanel);
        
        yOffset = SCREEN_HEIGHT - 135;
        textRenderer->renderText("Controls:", 20, yOffset, Color(255, 255, 0));
        yOffset += 20;
        textRenderer->renderText("WASD/Arrows: Move camera", 20, yOffset, Color(180, 180, 180));
        yOffset += 15;
        textRenderer->renderText("Mouse: Select units/tiles", 20, yOffset, Color(180, 180, 180));
        yOffset += 15;
        textRenderer->renderText("Space: End turn", 20, yOffset, Color(180, 180, 180));
        yOffset += 15;
        textRenderer->renderText("1: Build Farm (Worker)  2: Build Barracks", 20, yOffset, Color(180, 180, 180));
        yOffset += 15;
        textRenderer->renderText("H: Toggle UI", 20, yOffset, Color(180, 180, 180));
        
        // 地形信息
        if (gameState->selectedTile.x >= 0) {
            SDL_Rect terrainPanel = {SCREEN_WIDTH - 220, 10, 200, 120};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_RenderFillRect(renderer, &terrainPanel);
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
            SDL_RenderDrawRect(renderer, &terrainPanel);
            
            yOffset = 25;
            textRenderer->renderText("Tile Info:", SCREEN_WIDTH - 210, yOffset, Color(255, 255, 0));
            yOffset += 20;
            
            Terrain& terrain = map->getTerrain(gameState->selectedTile.x, gameState->selectedTile.y);
            std::string terrainType;
            switch (terrain.type) {
                case TerrainType::GRASS: terrainType = "Grass"; break;
                case TerrainType::FOREST: terrainType = "Forest"; break;
                case TerrainType::MOUNTAIN: terrainType = "Mountain"; break;
                case TerrainType::WATER: terrainType = "Water"; break;
                case TerrainType::DESERT: terrainType = "Desert"; break;
                case TerrainType::SNOW: terrainType = "Snow"; break;
            }
            textRenderer->renderText("Terrain: " + terrainType, SCREEN_WIDTH - 200, yOffset, Color(180, 180, 180));
            yOffset += 15;
            textRenderer->renderText("Move Cost: " + std::to_string(terrain.movementCost), SCREEN_WIDTH - 200, yOffset, Color(180, 180, 180));
            yOffset += 15;
            textRenderer->renderText("Defense: +" + std::to_string(terrain.defenseBonus), SCREEN_WIDTH - 200, yOffset, Color(180, 180, 180));
            
            Resource& resource = map->getResource(gameState->selectedTile.x, gameState->selectedTile.y);
            if (resource.type != ResourceType::NONE && resource.amount > 0) {
                yOffset += 20;
                std::string resName;
                switch (resource.type) {
                    case ResourceType::FOOD: resName = "Food"; break;
                    case ResourceType::WOOD: resName = "Wood"; break;
                    case ResourceType::STONE: resName = "Stone"; break;
                    case ResourceType::IRON: resName = "Iron"; break;
                    case ResourceType::GOLD: resName = "Gold"; break;
                }
                textRenderer->renderText("Resource: " + resName, SCREEN_WIDTH - 200, yOffset, Color(255, 215, 0));
                yOffset += 15;
                textRenderer->renderText("Amount: " + std::to_string(resource.amount), SCREEN_WIDTH - 200, yOffset, Color(255, 215, 0));
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
            processTurn();
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
        SDL_Quit();
    }
};

// 主程式入口
int main(int argc, char* argv[]) {
    StrategyGameEngine engine;
    engine.run();
    return 0;
}

/*
編譯指令:
Linux/Mac: g++ -std=c++14 strategy_game.cpp -lSDL2 -lSDL2_image -lSDL2_ttf -o strategy_game
Windows: g++ -std=c++14 strategy_game.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o strategy_game.exe

遊戲特色功能:
1. 地形系統 - 6種不同地形，影響移動和防禦
2. 資源系統 - 5種資源（食物、木材、石頭、鐵、金幣）
3. 建築系統 - 城市中心、兵營、農場、礦場、伐木場
4. 單位系統 - 戰士、弓箭手、騎兵、攻城器、工人
5. 玩家系統 - 最多4名玩家，回合制
6. 戰鬥系統 - 攻擊、反擊、地形加成
7. 經濟系統 - 資源收集、建築生產
8. UI系統 - 完整的遊戲界面和信息顯示

操作說明:
- WASD/方向鍵: 移動攝影機
- 滑鼠點選: 選擇單位/地格
- 空白鍵: 結束回合
- 1鍵: 建造農場（工人）
- 2鍵: 建造兵營（工人）
- H鍵: 切換UI顯示
*/