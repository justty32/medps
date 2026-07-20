#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <memory>
#include <iostream>
#include <fstream>
#include <unordered_map>

// 地圖生成相關的枚舉和結構
enum class TerrainType {
    DEEP_WATER = 0,
    SHALLOW_WATER,
    BEACH,
    GRASS,
    FOREST,
    HILLS,
    MOUNTAINS,
    SNOW,
    DESERT,
    SWAMP
};

enum class ResourceType {
    NONE = 0,
    FOOD,
    WOOD,
    STONE,
    IRON,
    GOLD,
    GEMS
};

enum class BiomeType {
    OCEAN,
    TEMPERATE,
    FOREST,
    DESERT,
    MOUNTAIN,
    ARCTIC,
    TROPICAL
};

struct TerrainInfo {
    TerrainType type;
    float elevation;
    float temperature;
    float moisture;
    float fertility;
    BiomeType biome;
    SDL_Color color;
    
    TerrainInfo() : type(TerrainType::GRASS), elevation(0.5f), temperature(0.5f), 
                   moisture(0.5f), fertility(0.5f), biome(BiomeType::TEMPERATE) {
        color = {50, 150, 50, 255};
    }
};

struct ResourceNode {
    ResourceType type;
    int abundance;
    float quality;
    SDL_Color color;
    
    ResourceNode() : type(ResourceType::NONE), abundance(0), quality(0.0f) {
        color = {255, 255, 255, 255};
    }
};

// Perlin噪音生成器
class PerlinNoise {
private:
    std::vector<int> permutation;
    
    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }
    
    double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
    
public:
    PerlinNoise(unsigned int seed = 0) {
        permutation.resize(256);
        std::iota(permutation.begin(), permutation.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(permutation.begin(), permutation.end(), engine);
        permutation.insert(permutation.end(), permutation.begin(), permutation.end());
    }
    
    double noise(double x, double y, double z = 0.0) {
        int X = (int)floor(x) & 255;
        int Y = (int)floor(y) & 255;
        int Z = (int)floor(z) & 255;
        
        x -= floor(x);
        y -= floor(y);
        z -= floor(z);
        
        double u = fade(x);
        double v = fade(y);
        double w = fade(z);
        
        int A = permutation[X] + Y;
        int AA = permutation[A] + Z;
        int AB = permutation[A + 1] + Z;
        int B = permutation[X + 1] + Y;
        int BA = permutation[B] + Z;
        int BB = permutation[B + 1] + Z;
        
        return lerp(w, lerp(v, lerp(u, grad(permutation[AA], x, y, z),
                                      grad(permutation[BA], x - 1, y, z)),
                              lerp(u, grad(permutation[AB], x, y - 1, z),
                                     grad(permutation[BB], x - 1, y - 1, z))),
                      lerp(v, lerp(u, grad(permutation[AA + 1], x, y, z - 1),
                                     grad(permutation[BA + 1], x - 1, y, z - 1)),
                             lerp(u, grad(permutation[AB + 1], x, y - 1, z - 1),
                                    grad(permutation[BB + 1], x - 1, y - 1, z - 1))));
    }
    
    // 分形噪音（多層疊加）
    double fractalNoise(double x, double y, int octaves = 4, double persistence = 0.5, double scale = 0.01) {
        double value = 0.0;
        double amplitude = 1.0;
        double frequency = scale;
        double maxValue = 0.0;
        
        for (int i = 0; i < octaves; i++) {
            value += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0;
        }
        
        return value / maxValue;
    }
};

// Simplex噪音生成器（更自然的噪音）
class SimplexNoise {
private:
    static const int grad3[12][3];
    std::vector<int> perm;
    std::vector<int> permMod12;
    
    static double dot(const int g[], double x, double y) {
        return g[0] * x + g[1] * y;
    }
    
public:
    SimplexNoise(unsigned int seed = 0) {
        perm.resize(256);
        permMod12.resize(256);
        
        std::iota(perm.begin(), perm.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(perm.begin(), perm.end(), engine);
        
        for (int i = 0; i < 256; i++) {
            permMod12[i] = perm[i] % 12;
        }
        
        perm.insert(perm.end(), perm.begin(), perm.end());
        permMod12.insert(permMod12.end(), permMod12.begin(), permMod12.end());
    }
    
    double noise(double xin, double yin) {
        double n0, n1, n2;
        const double F2 = 0.5 * (sqrt(3.0) - 1.0);
        double s = (xin + yin) * F2;
        int i = (int)floor(xin + s);
        int j = (int)floor(yin + s);
        
        const double G2 = (3.0 - sqrt(3.0)) / 6.0;
        double t = (i + j) * G2;
        double X0 = i - t;
        double Y0 = j - t;
        double x0 = xin - X0;
        double y0 = yin - Y0;
        
        int i1, j1;
        if (x0 > y0) {
            i1 = 1; j1 = 0;
        } else {
            i1 = 0; j1 = 1;
        }
        
        double x1 = x0 - i1 + G2;
        double y1 = y0 - j1 + G2;
        double x2 = x0 - 1.0 + 2.0 * G2;
        double y2 = y0 - 1.0 + 2.0 * G2;
        
        int ii = i & 255;
        int jj = j & 255;
        int gi0 = permMod12[ii + perm[jj]];
        int gi1 = permMod12[ii + i1 + perm[jj + j1]];
        int gi2 = permMod12[ii + 1 + perm[jj + 1]];
        
        double t0 = 0.5 - x0 * x0 - y0 * y0;
        if (t0 < 0) n0 = 0.0;
        else {
            t0 *= t0;
            n0 = t0 * t0 * dot(grad3[gi0], x0, y0);
        }
        
        double t1 = 0.5 - x1 * x1 - y1 * y1;
        if (t1 < 0) n1 = 0.0;
        else {
            t1 *= t1;
            n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
        }
        
        double t2 = 0.5 - x2 * x2 - y2 * y2;
        if (t2 < 0) n2 = 0.0;
        else {
            t2 *= t2;
            n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
        }
        
        return 70.0 * (n0 + n1 + n2);
    }
};

const int SimplexNoise::grad3[12][3] = {
    {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
    {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
    {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
};

// Voronoi圖生成器（用於生成細胞狀結構）
class VoronoiGenerator {
private:
    struct Point {
        double x, y;
        int id;
        Point(double x = 0, double y = 0, int id = 0) : x(x), y(y), id(id) {}
    };
    
    std::vector<Point> seeds;
    
public:
    void generateSeeds(int width, int height, int numSeeds, unsigned int seed = 0) {
        seeds.clear();
        std::mt19937 gen(seed);
        std::uniform_real_distribution<> xDist(0, width);
        std::uniform_real_distribution<> yDist(0, height);
        
        for (int i = 0; i < numSeeds; i++) {
            seeds.emplace_back(xDist(gen), yDist(gen), i);
        }
    }
    
    int getClosestSeed(double x, double y) {
        if (seeds.empty()) return 0;
        
        double minDist = std::numeric_limits<double>::max();
        int closestId = 0;
        
        for (const auto& seed : seeds) {
            double dist = (x - seed.x) * (x - seed.x) + (y - seed.y) * (y - seed.y);
            if (dist < minDist) {
                minDist = dist;
                closestId = seed.id;
            }
        }
        
        return closestId;
    }
    
    double getDistanceToClosestSeed(double x, double y) {
        if (seeds.empty()) return 0.0;
        
        double minDist = std::numeric_limits<double>::max();
        
        for (const auto& seed : seeds) {
            double dist = sqrt((x - seed.x) * (x - seed.x) + (y - seed.y) * (y - seed.y));
            minDist = std::min(minDist, dist);
        }
        
        return minDist;
    }
};

// 主要地圖生成器類別
class MapGenerator {
private:
    int width, height;
    std::vector<std::vector<TerrainInfo>> terrainMap;
    std::vector<std::vector<ResourceNode>> resourceMap;
    
    PerlinNoise elevationNoise;
    PerlinNoise temperatureNoise;
    PerlinNoise moistureNoise;
    SimplexNoise detailNoise;
    VoronoiGenerator biomeGenerator;
    
    std::mt19937 randomGen;
    
public:
    MapGenerator(int w, int h, unsigned int seed = 0) 
        : width(w), height(h), elevationNoise(seed), temperatureNoise(seed + 1000), 
          moistureNoise(seed + 2000), detailNoise(seed + 3000), randomGen(seed) {
        terrainMap.resize(height, std::vector<TerrainInfo>(width));
        resourceMap.resize(height, std::vector<ResourceNode>(width));
    }
    
    // 生成基礎高度圖
    void generateElevation() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // 多層噪音疊加創造複雜地形
                double elevation = 0.0;
                
                // 大陸形狀（低頻）
                elevation += elevationNoise.fractalNoise(x, y, 3, 0.5, 0.005) * 0.6;
                
                // 山脈和丘陵（中頻）
                elevation += elevationNoise.fractalNoise(x, y, 4, 0.6, 0.02) * 0.3;
                
                // 細節（高頻）
                elevation += detailNoise.noise(x * 0.1, y * 0.1) * 0.1;
                
                // 邊緣衰減（創造島嶼效果）
                double centerX = width / 2.0;
                double centerY = height / 2.0;
                double maxDist = sqrt(centerX * centerX + centerY * centerY);
                double distFromCenter = sqrt((x - centerX) * (x - centerX) + (y - centerY) * (y - centerY));
                double falloff = 1.0 - (distFromCenter / maxDist);
                falloff = std::max(0.0, falloff);
                
                elevation *= falloff * falloff; // 平方衰減更自然
                
                // 正規化到0-1範圍
                elevation = (elevation + 1.0) / 2.0;
                elevation = std::max(0.0, std::min(1.0, elevation));
                
                terrainMap[y][x].elevation = elevation;
            }
        }
    }
    
    // 生成溫度圖
    void generateTemperature() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // 基於緯度的溫度（赤道熱，兩極冷）
                double latitudeTemp = 1.0 - abs(y - height/2.0) / (height/2.0);
                
                // 高度影響溫度（高山較冷）
                double elevationEffect = 1.0 - terrainMap[y][x].elevation * 0.8;
                
                // 隨機變化
                double noiseEffect = temperatureNoise.fractalNoise(x, y, 3, 0.4, 0.01) * 0.3;
                
                double temperature = latitudeTemp * elevationEffect + noiseEffect;
                temperature = std::max(0.0, std::min(1.0, temperature));
                
                terrainMap[y][x].temperature = temperature;
            }
        }
    }
    
    // 生成濕度圖
    void generateMoisture() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                // 基礎濕度噪音
                double moisture = moistureNoise.fractalNoise(x, y, 4, 0.5, 0.008);
                
                // 距離水源的影響
                double waterDistance = getDistanceToWater(x, y);
                double waterEffect = exp(-waterDistance * 0.1); // 指數衰減
                
                moisture = moisture * 0.7 + waterEffect * 0.3;
                moisture = (moisture + 1.0) / 2.0; // 正規化
                moisture = std::max(0.0, std::min(1.0, moisture));
                
                terrainMap[y][x].moisture = moisture;
            }
        }
    }
    
    // 計算到最近水源的距離
    double getDistanceToWater(int x, int y) {
        double minDist = std::numeric_limits<double>::max();
        
        // 搜索附近的水域
        int searchRadius = 20;
        for (int dy = -searchRadius; dy <= searchRadius; dy++) {
            for (int dx = -searchRadius; dx <= searchRadius; dx++) {
                int nx = x + dx;
                int ny = y + dy;
                
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    if (terrainMap[ny][nx].elevation < 0.3) { // 水域
                        double dist = sqrt(dx * dx + dy * dy);
                        minDist = std::min(minDist, dist);
                    }
                }
            }
        }
        
        return minDist == std::numeric_limits<double>::max() ? searchRadius : minDist;
    }
    
    // 決定生物群系
    void generateBiomes() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                TerrainInfo& terrain = terrainMap[y][x];
                
                // 基於溫度、濕度和高度決定生物群系
                if (terrain.elevation < 0.25) {
                    terrain.biome = BiomeType::OCEAN;
                } else if (terrain.temperature < 0.2) {
                    terrain.biome = BiomeType::ARCTIC;
                } else if (terrain.temperature > 0.8 && terrain.moisture < 0.3) {
                    terrain.biome = BiomeType::DESERT;
                } else if (terrain.moisture > 0.7 && terrain.temperature > 0.6) {
                    terrain.biome = BiomeType::TROPICAL;
                } else if (terrain.elevation > 0.7) {
                    terrain.biome = BiomeType::MOUNTAIN;
                } else if (terrain.moisture > 0.5) {
                    terrain.biome = BiomeType::FOREST;
                } else {
                    terrain.biome = BiomeType::TEMPERATE;
                }
            }
        }
    }
    
    // 決定具體地形類型
    void generateTerrainTypes() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                TerrainInfo& terrain = terrainMap[y][x];
                
                // 基於生物群系和具體數值決定地形
                switch (terrain.biome) {
                    case BiomeType::OCEAN:
                        if (terrain.elevation < 0.15) {
                            terrain.type = TerrainType::DEEP_WATER;
                            terrain.color = {0, 50, 150, 255};
                        } else {
                            terrain.type = TerrainType::SHALLOW_WATER;
                            terrain.color = {50, 100, 200, 255};
                        }
                        break;
                        
                    case BiomeType::ARCTIC:
                        terrain.type = TerrainType::SNOW;
                        terrain.color = {240, 240, 255, 255};
                        break;
                        
                    case BiomeType::DESERT:
                        terrain.type = TerrainType::DESERT;
                        terrain.color = {200, 180, 120, 255};
                        break;
                        
                    case BiomeType::TROPICAL:
                        if (terrain.elevation < 0.35) {
                            terrain.type = TerrainType::SWAMP;
                            terrain.color = {80, 120, 60, 255};
                        } else {
                            terrain.type = TerrainType::FOREST;
                            terrain.color = {20, 80, 20, 255};
                        }
                        break;
                        
                    case BiomeType::MOUNTAIN:
                        if (terrain.elevation > 0.85) {
                            terrain.type = TerrainType::MOUNTAINS;
                            terrain.color = {120, 100, 80, 255};
                        } else {
                            terrain.type = TerrainType::HILLS;
                            terrain.color = {140, 120, 90, 255};
                        }
                        break;
                        
                    case BiomeType::FOREST:
                        terrain.type = TerrainType::FOREST;
                        terrain.color = {34, 100, 34, 255};
                        break;
                        
                    case BiomeType::TEMPERATE:
                    default:
                        if (terrain.elevation < 0.3) {
                            terrain.type = TerrainType::BEACH;
                            terrain.color = {230, 220, 170, 255};
                        } else {
                            terrain.type = TerrainType::GRASS;
                            terrain.color = {50, 150, 50, 255};
                        }
                        break;
                }
                
                // 添加一些隨機細節變化
                double detail = detailNoise.noise(x * 0.3, y * 0.3);
                if (detail > 0.3 && terrain.type == TerrainType::GRASS && terrain.moisture > 0.4) {
                    terrain.type = TerrainType::FOREST;
                    terrain.color = {34, 100, 34, 255};
                }
            }
        }
    }
    
    // 生成資源
    void generateResources() {
        std::uniform_real_distribution<> prob(0.0, 1.0);
        std::uniform_int_distribution<> abundance(10, 100);
        std::uniform_real_distribution<> quality(0.5, 1.0);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                TerrainInfo& terrain = terrainMap[y][x];
                ResourceNode& resource = resourceMap[y][x];
                
                // 基於地形類型決定資源機率
                double resourceChance = 0.0;
                ResourceType possibleResources[3] = {ResourceType::NONE, ResourceType::NONE, ResourceType::NONE};
                int numPossible = 0;
                
                switch (terrain.type) {
                    case TerrainType::FOREST:
                        resourceChance = 0.3;
                        possibleResources[0] = ResourceType::WOOD;
                        possibleResources[1] = ResourceType::FOOD;
                        numPossible = 2;
                        break;
                        
                    case TerrainType::MOUNTAINS:
                        resourceChance = 0.4;
                        possibleResources[0] = ResourceType::STONE;
                        possibleResources[1] = ResourceType::IRON;
                        possibleResources[2] = ResourceType::GEMS;
                        numPossible = 3;
                        break;
                        
                    case TerrainType::HILLS:
                        resourceChance = 0.25;
                        possibleResources[0] = ResourceType::STONE;
                        possibleResources[1] = ResourceType::IRON;
                        numPossible = 2;
                        break;
                        
                    case TerrainType::GRASS:
                        resourceChance = 0.2;
                        possibleResources[0] = ResourceType::FOOD;
                        numPossible = 1;
                        break;
                        
                    case TerrainType::DESERT:
                        resourceChance = 0.15;
                        possibleResources[0] = ResourceType::GOLD;
                        numPossible = 1;
                        break;
                        
                    case TerrainType::SWAMP:
                        resourceChance = 0.1;
                        possibleResources[0] = ResourceType::FOOD;
                        numPossible = 1;
                        break;
                }
                
                if (prob(randomGen) < resourceChance && numPossible > 0) {
                    std::uniform_int_distribution<> resDist(0, numPossible - 1);
                    resource.type = possibleResources[resDist(randomGen)];
                    resource.abundance = abundance(randomGen);
                    resource.quality = quality(randomGen);
                    
                    // 設定資源顏色
                    switch (resource.type) {
                        case ResourceType::FOOD:
                            resource.color = {255, 255, 0, 255};
                            break;
                        case ResourceType::WOOD:
                            resource.color = {139, 69, 19, 255};
                            break;
                        case ResourceType::STONE:
                            resource.color = {128, 128, 128, 255};
                            break;
                        case ResourceType::IRON:
                            resource.color = {70, 70, 70, 255};
                            break;
                        case ResourceType::GOLD:
                            resource.color = {255, 215, 0, 255};
                            break;
                        case ResourceType::GEMS:
                            resource.color = {255, 0, 255, 255};
                            break;
                    }
                }
            }
        }
    }
    
    // 地形後處理（平滑化等）
    void postProcess() {
        // 移除孤立的地形塊
        std::vector<std::vector<TerrainInfo>> smoothed = terrainMap;
        
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                std::unordered_map<TerrainType, int> neighbors;
                
                // 計算鄰居地形類型
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        TerrainType type = terrainMap[y + dy][x + dx].type;
                        neighbors[type]++;
                    }
                }
                
                // 找到最常見的鄰居類型
                TerrainType mostCommon = terrainMap[y][x].type;
                int maxCount = 0;
                for (const auto& pair : neighbors) {
                    if (pair.second > maxCount) {
                        maxCount = pair.second;
                        mostCommon = pair.first;
                    }
                }
                
                // 如果當前類型是少數，改變它
                if (neighbors[terrainMap[y][x].type] < 3) {
                    smoothed[y][x].type = mostCommon;
                    // 也要更新顏色
                    updateTerrainColor(smoothed[y][x]);
                }
            }
        }
        
        terrainMap = smoothed;
    }
    
    void updateTerrainColor(TerrainInfo& terrain) {
        switch (terrain.type) {
            case TerrainType::DEEP_WATER:
                terrain.color = {0, 50, 150, 255};
                break;
            case TerrainType::SHALLOW_WATER:
                terrain.color = {50, 100, 200, 255};
                break;
            case TerrainType::BEACH:
                terrain.color = {230, 220, 170, 255};
                break;
            case TerrainType::GRASS:
                terrain.color = {50, 150, 50, 255};
                break;
            case TerrainType::FOREST:
                terrain.color = {34, 100, 34, 255};
                break;
            case TerrainType::HILLS:
                terrain.color = {140, 120, 90, 255};
                break;
            case TerrainType::MOUNTAINS:
                terrain.color = {120, 100, 80, 255};
                break;
            case TerrainType::SNOW:
                terrain.color = {240, 240, 255, 255};
                break;
            case TerrainType::DESERT:
                terrain.color = {200, 180, 120, 255};
                break;
            case TerrainType::SWAMP:
                terrain.color = {80, 120, 60, 255};
                break;
        }
    }
    
    // 完整地圖生成流程
    void generateMap() {
        std::cout << "生成高度圖..." << std::endl;
        generateElevation();
        
        std::cout << "生成溫度圖..." << std::endl;
        generateTemperature();
        
        std::cout << "生成濕度圖..." << std::endl;
        generateMoisture();
        
        std::cout << "決定生物群系..." << std::endl;
        generateBiomes();
        
        std::cout << "生成地形類型..." << std::endl;
        generateTerrainTypes();
        
        std::cout << "生成資源..." << std::endl;
        generateResources();
        
        std::cout << "後處理..." << std::endl;
        postProcess();
        
        std::cout << "地圖生成完成！" << std::endl;
    }
    
    // 取得地形信息
    const TerrainInfo& getTerrain(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return terrainMap[y][x];
        }
        static TerrainInfo defaultTerrain;
        return defaultTerrain;
    }
    
    const ResourceNode& getResource(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return resourceMap[y][x];
        }
        static ResourceNode defaultResource;
        return defaultResource;
    }
    
    // 導