// Strategy Game System - Core Data Structures

// Tile - Basic map unit
struct Tile {
  x;                             // Grid position X
  y;                             // Grid position Y
  terrain;                       // plains, forest, mountain, water, desert, etc.
  elevation;                     // Height level for gameplay mechanics
  resources;                     // iron, gold, food, stone, etc.
  improvements;                  // roads, farms, mines, etc.
  visibility;                    // faction_id -> visibility level (0=unexplored, 1=explored, 2=visible)
  movementCost;                  // Base movement cost to enter
  defenseBonus;                  // Defensive modifier for units
  owner;                         // Faction that controls this tile
  unitStack;                     // Reference to unit stack on this tile
  city;                          // Reference to city if present
  river;                         // Has river for trade/movement
  climate;                       // temperate, tropical, arctic, arid
  fertility;                     // Soil fertility for agriculture
  pollution;                     // Environmental damage level
  naturalWonder;                 // Special landmark (mountain, lake, etc.)
}

// Unit - Individual military/civilian unit
struct Unit {
  id;
  type;                          // warrior, archer, settler, worker, etc.
  faction;                       // Owner faction
  name;                          // Custom unit name
  
  // Core stats
  health;                        // Current health (0-100)
  maxHealth;                     // Maximum health
  attack;                        // Attack strength
  defense;                       // Defense strength
  movement;                      // Movement points per turn
  currentMovement;               // Remaining movement this turn
  
  // Position and state
  x;                             // Current X position
  y;                             // Current Y position
  level;                         // Experience level
  experience;                    // Current experience points
  promoted;                      // Has received promotion bonuses
  
  // Abilities and status
  abilities;                     // Special abilities (charge, fortify, etc.)
  status;                        // Status effects (poisoned, blessed, etc.)
  fortified;                     // Is fortified for defense bonus
  veteran;                       // Veteran status from combat
  
  // Resource costs and upkeep
  productionCost;                // Cost to produce
  goldUpkeep;                    // Gold per turn maintenance
  population;                    // Population units consumed
  
  // Combat modifiers
  combatBonuses;                 // terrain_type -> bonus
  unitClassBonuses;              // unit_class -> bonus/penalty
  morale;                        // Unit morale (affects combat)
  supply;                        // Supply level (affects effectiveness)
}

// UnitStack - Collection of units on same tile
struct UnitStack {
  x;
  y;
  units;                         // Array of Unit objects
  maxSize;                       // Maximum units in stack
  leader;                        // Hero leading the stack
  formation;                     // Combat formation
  stance;                        // aggressive, defensive, balanced
  orders;                        // Current movement/combat orders
  supply;                        // Supply level (0-100)
  experience;                    // Stack collective experience
  veterancy;                     // Overall veteran status
}

// City - Settlement that produces units and buildings
struct City {
  id;
  name;
  x;
  y;
  faction;
  
  // Core city stats
  population;                    // Current population
  maxPopulation;                 // Population cap
  growthRate;                    // Population growth per turn
  happiness;                     // Happiness level (0-100)
  health;                        // Health level (0-100)
  
  // Production and resources
  production;                    // Current production points
  food;                          // Stored food
  commerce;                      // Gold generation per turn
  science;                       // Research points per turn
  culture;                       // Cultural influence
  borders;                       // Cultural border radius
  
  // Buildings and improvements
  buildings;                     // Built structures
  buildQueue;                    // Production queue
  currentProject;                // What's being built
  turnsToComplete;               // Turns until current project done
  
  // Worked tiles and specialists
  workedTiles;                   // Tiles being worked by citizens
  specialists;                   // Citizens assigned to specialist roles
  
  // Defense and military
  defenseStrength;               // Base city defense
  walls;                         // Has defensive walls
  garrison;                      // Units stationed in city
  underSiege;                    // Currently besieged
  
  // Trade and diplomacy
  tradeRoutes;                   // Active trade connections
  wonders;                       // World wonders built here
  religion;                      // Dominant religion
  founded;                       // Turn when city was founded
  age;                           // City age category
  corruption;                    // Distance/administration penalty
  pollution;                     // Industrial pollution level
}

// Faction - Player or AI civilization
struct Faction {
  id;
  name;
  color;                         // Display color
  isPlayer;                      // Human vs AI
  isAlive;                       // Still in game
  
  // Resources
  gold;                          // Treasury
  food;                          // Global food storage
  production;                    // Global production
  science;                       // Research points
  culture;                       // Cultural points
  faith;                         // Religious points
  
  // Technology and progression
  technologies;                  // Researched technologies
  currentResearch;               // Technology being researched
  researchProgress;              // Progress on current research
  
  // Government and policies
  government;                    // Government type
  policies;                      // Active social policies
  civics;                        // Unlocked civic concepts
  
  // Military and diplomacy
  unitUpkeep;                    // Total military upkeep cost
  cities;                        // Owned cities
  units;                         // All faction units
  heroes;                        // Faction heroes
  
  // Diplomacy relations (faction_id -> relation_value)
  relations;                     // -100 to +100 diplomatic relations
  wars;                          // Factions at war with
  allies;                        // Allied factions
  tradePacts;                    // Trade agreement partners
  
  // Victory conditions progress
  victoryProgress;               // Progress toward different victory types
  
  // AI behavior (for AI factions)
  personality;                   // AI behavioral traits
  expansionTendency;             // Desire to expand territory
  militaryFocus;                 // Military vs peaceful focus
}

// Hero - Special leader units with unique abilities
struct Hero {
  id;
  name;
  faction;
  class;                         // warrior, mage, diplomat, explorer, etc.
  
  // Enhanced unit stats
  level;
  experience;
  health;
  maxHealth;
  attack;
  defense;
  movement;
  currentMovement;
  
  // Position
  x;
  y;
  stackId;                       // UnitStack they're leading
  
  // Hero-specific attributes
  skills;                        // skill_name -> level
  traits;                        // Permanent character traits
  equipment;                     // Equipped items
  inventory;                     // Carried items
  
  // Leadership abilities
  leadershipRange;               // Tiles affected by leadership
  armyBonuses;                   // Bonuses to led units
  
  // Special abilities and spells
  abilities;                     // Active abilities
  spells;                        // Available spells (if magical)
  mana;                          // Spell casting resource
  maxMana;
  
  // Quests and story
  quests;                        // Active quests
  questsCompleted;               // Completed quests
  reputation;                    // General reputation (-100 to +100)
  alignment;                     // good, neutral, evil
  
  // Recruitment and loyalty
  recruitmentBonus;              // Bonus to unit recruitment
  loyalty;                       // Loyalty to faction (0-100)
  birthTurn;                     // When hero was recruited
  immortal;                      // Cannot die permanently
  age;                           // Hero age
  background;                    // Origin story/background
}

// BigMap - Main game world
struct BigMap {
  width;
  height;
  tiles;                         // 2D array of Tile objects
  cities;                        // city_id -> City
  unitStacks;                    // position_key -> UnitStack
  factions;                      // faction_id -> Faction
  heroes;                        // hero_id -> Hero
  
  // World generation settings
  seed;                          // World generation seed
  worldAge;                      // young, standard, old
  climate;                       // Overall world climate
  seaLevel;                      // low, medium, high
  
  // Game state
  currentTurn;
  currentFaction;                // Active faction's turn
  gamePhase;                     // ancient, classical, medieval, etc.
  worldEvents;                   // Active world events
  season;                        // Spring, summer, autumn, winter
  weather;                       // Current weather patterns
}

// Civilization - Cultural and technological identity
struct Civilization {
  id;
  name;                          // Civilization name (Romans, Greeks, etc.)
  adjective;                     // Roman, Greek, etc.
  
  // Cultural identity
  cultureGroup;                  // Western, Eastern, Nomadic, etc.
  language;                      // Primary language
  script;                        // Writing system
  artStyle;                      // Architectural/artistic style
  
  // Unique traits and bonuses
  traits;                        // Unique civilization traits
  uniqueUnits;                   // Special units only this civ can build
  uniqueBuildings;               // Special buildings
  uniqueTechnologies;            // Exclusive tech paths
  
  // Starting bonuses
  startingTech;                  // Technologies known at start
  startingUnits;                 // Initial military units
  startingResources;             // Bonus starting resources
  
  // Preferred terrain and climate
  preferredTerrain;              // Best terrain types for this civ
  climateAdaptation;             // Climate bonuses/penalties
  
  // Victory condition modifiers
  victoryBonuses;                // Bonuses toward specific victory types
  
  // Historical context
  historicalPeriod;              // Time period this civ represents
  realWorldRegion;               // Geographic origin
  leaderNames;                   // Pool of potential leader names
  cityNames;                     // Pool of city names
}

// Culture - Social and artistic development
struct Culture {
  id;
  name;                          // Culture name
  civilization;                  // Parent civilization
  
  // Cultural attributes
  values;                        // Core cultural values
  traditions;                    // Cultural practices and customs
  artForms;                      // Music, literature, visual arts
  philosophy;                    // Dominant philosophical schools
  social_structure;              // Class system, social hierarchy
  
  // Cultural spread and influence
  influence;                     // Cultural influence strength
  spread_rate;                   // How fast culture spreads
  resistance;                    // Resistance to foreign culture
  absorption;                    // Ability to absorb other cultures
  
  // Cultural buildings and wonders
  culturalBuildings;             // Buildings that boost culture
  monuments;                     // Cultural monuments and landmarks
  festivals;                     // Cultural celebrations
  
  // Technology and knowledge
  scholarship;                   // Academic traditions
  innovation;                    // Technological innovation rate
  preservation;                  // Knowledge preservation methods
  
  // Cultural conflicts and compatibility
  compatibility;                 // culture_id -> compatibility rating
  conflicts;                     // Historical cultural conflicts
  assimilation;                  // How easily others adopt this culture
}

// Climate - Environmental conditions
struct Climate {
  id;
  name;                          // Climate name
  
  // Temperature and precipitation
  temperature;                   // Average temperature
  temperatureVariation;          // Seasonal temperature change
  precipitation;                 // Annual rainfall
  humidity;                      // Moisture levels
  
  // Seasonal patterns
  seasons;                       // Number and intensity of seasons
  growingSeason;                 // Length of agricultural growing season
  storms;                        // Frequency of severe weather
  
  // Effects on gameplay
  agricultureModifier;           // Food production modifier
  healthModifier;                // Population health effects
  movementModifier;              // Unit movement effects
  constructionModifier;          // Building construction effects
  
  // Climate-specific resources
  nativeResources;               // Resources common in this climate
  rarities;                      // Rare resources possible here
  
  // Adaptation and technology
  requiredTech;                  // Technologies needed to thrive
  adaptationBonus;               // Bonuses for adapted civilizations
  
  // Climate change and events
  stability;                     // How stable this climate is
  changeRate;                    // Rate of climate evolution
  extremeEvents;                 // Possible extreme weather events
}

// Religion - Spiritual and organizational belief system
struct Religion {
  id;
  name;                          // Religion name
  type;                          // monotheistic, polytheistic, animistic, etc.
  
  // Core beliefs and doctrine
  deities;                       // Gods, spirits, or divine figures
  beliefs;                       // Core religious beliefs
  practices;                     // Rituals and ceremonies
  morality;                      // Moral code and ethics
  afterlife;                     // Beliefs about death and afterlife
  
  // Organization and hierarchy
  clergy;                        // Religious leadership structure
  temples;                       // Religious buildings and sites
  holy_sites;                    // Sacred locations
  scripture;                     // Religious texts and laws
  
  // Spread and influence
  missionaries;                  // Ability to spread religion
  conversion;                    // Conversion mechanics
  tolerance;                     // Tolerance of other religions
  persecution;                   // Tendency to persecute others
  
  // Gameplay effects
  happiness_bonus;               // Happiness from following religion
  science_modifier;              // Effect on scientific research
  culture_bonus;                 // Cultural influence bonus
  military_bonus;                // Military bonuses (crusades, etc.)
  
  // Religious conflicts and relationships
  schisms;                       // Internal religious splits
  holy_wars;                     // Religious conflicts with others
  syncretism;                    // Ability to merge with other faiths
  
  // Special mechanics
  prophecies;                    // Religious prophecies and events
  miracles;                      // Special religious events
  martyrdom;                     // Martyrdom and sacrifice mechanics
  pilgrimage;                    // Pilgrimage sites and benefits
}

// Politics - Government and power structures
struct Politics {
  id;
  name;                          // Political system name
  type;                          // democracy, monarchy, republic, etc.
  
  // Government structure
  headOfState;                   // Who leads the government
  succession;                    // How leadership changes
  administration;                // Government departments/ministries
  bureaucracy;                   // Administrative efficiency
  
  // Power distribution
  centralization;                // Central vs local power
  separation;                    // Separation of powers
  checks;                        // Checks and balances
  representation;                // How citizens are represented
  
  // Political participation
  voting;                        // Voting systems and rights
  parties;                       // Political parties or factions
  opposition;                    // How opposition is handled
  dissent;                       // Tolerance for dissent
  
  // Laws and justice
  legal_system;                  // Court system and laws
  law_enforcement;               // Police and military
  punishment;                    // Criminal justice system
  rights;                        // Citizen rights and freedoms
  
  // Economic policy
  taxation;                      // Tax system and rates
  trade_policy;                  // Trade regulations
  economic_control;              // Government economic intervention
  property_rights;               // Private property protections
  
  // Foreign policy
  diplomacy;                     // Diplomatic capabilities
  war_powers;                    // Authority to declare war
  treaties;                      // Treaty-making authority
  intelligence;                  // Espionage capabilities
  
  // Political stability
  legitimacy;                    // Government legitimacy
  stability;                     // Political stability
  revolution_risk;               // Risk of political upheaval
  reform_capacity;               // Ability to change peacefully
  
  // Special mechanics
  corruption;                    // Government corruption levels
  propaganda;                    // State propaganda capabilities
  censorship;                    // Information control
  political_events;              // Special political events/crises
}

// Additional Supporting Structures

// Technology - Research and innovation
struct Technology {
  id;
  name;                          // Technology name
  category;                      // Military, civic, economic, religious, etc.
  era;                           // Stone age, bronze age, iron age, etc.
  
  // Research requirements
  prerequisites;                 // Required technologies
  cost;                          // Research points needed
  difficulty;                    // Research difficulty modifier
  
  // Unlocks and benefits
  unlocksUnits;                  // New unit types
  unlocksBuildings;              // New building types
  unlocksResources;              // Access to new resources
  unlocksGovernments;            // New government types
  
  // Gameplay modifiers
  economicBonus;                 // Economic benefits
  militaryBonus;                 // Military improvements
  culturalBonus;                 // Cultural benefits
  scienceBonus;                  // Research speed improvements
  
  // Special effects
  obsoletes;                     // Technologies/units made obsolete
  reveals;                       // Map features revealed
  enables;                       // Special game mechanics enabled
}

// Resource - Natural and strategic materials
struct Resource {
  id;
  name;                          // Resource name
  type;                          // food, luxury, strategic, bonus
  
  // Availability and distribution
  rarity;                        // How rare this resource is
  terrain;                       // Terrain types where found
  climate;                       // Climate requirements
  
  // Extraction and processing
  extractionTech;                // Technology needed to extract
  processingTech;                // Technology to process/refine
  improvementRequired;           // Tile improvement needed
  
  // Gameplay effects
  foodValue;                     // Food production bonus
  productionValue;               // Production bonus
  commerceValue;                 // Commerce/gold bonus
  scienceValue;                  // Research bonus
  cultureValue;                  // Culture bonus
  
  // Strategic uses
  unitRequirements;              // Units that require this resource
  buildingRequirements;          // Buildings that need this resource
  tradeValue;                    // Value in trade agreements
  
  // Special properties
  perishable;                    // Can spoil or be depleted
  renewable;                     // Regenerates over time
  monopolizable;                 // Can be controlled exclusively
  tradable;                      // Can be traded between factions
}

// Event - Dynamic occurrences that affect gameplay
struct Event {
  id;
  name;                          // Event name
  type;                          // natural, political, military, economic, etc.
  
  // Trigger conditions
  triggers;                      // Conditions that cause this event
  probability;                   // Chance of occurring
  prerequisites;                 // Requirements for event to be possible
  
  // Scope and duration
  scope;                         // global, regional, faction, city
  duration;                      // How long effects last
  repeatable;                    // Can occur multiple times
  
  // Effects and consequences
  immediate;                     // Immediate effects
  ongoing;                       // Continuing effects while active
  choices;                       // Player choices and their outcomes
  
  // Targeting
  targetType;                    // What can be affected
  targetCriteria;                // How targets are selected
  affectedFactions;              // Which factions are impacted
  
  // Narrative elements
  description;                   // Event description text
  flavor;                        // Atmospheric text
  historical;                    // Based on historical events
}

// Trade Route - Economic connections between cities
struct TradeRoute {
  id;
  origin;                        // Starting city
  destination;                   // Ending city
  type;                          // land, sea, river, air
  
  // Route properties
  distance;                      // Length of trade route
  safety;                        // Security from bandits/pirates
  infrastructure;                // Road/port quality
  
  // Economic effects
  goldPerTurn;                   // Gold generated per turn
  resourcesTraded;               // Specific resources exchanged
  culturalExchange;              // Cultural influence spread
  technologySpread;              // Technology transfer rate
  
  // Requirements and maintenance
  requiredTech;                  // Technology needed for this route
  maintenanceCost;               // Upkeep cost per turn
  requiredUnits;                 // Military protection needed
  
  // Status and threats
  active;                        // Currently operating
  disrupted;                     // Temporarily blocked
  threatenedBy;                  // Hostile forces threatening route
}Roman Empire", "#ff0000", true);
  const aiFaction = new Faction("Barbarians", "#0000ff", false);
  
  gameMap.factions.set(playerFaction.id, playerFaction);
  gameMap.factions.set(aiFaction.id, aiFaction);
  
  // Create starting city
  const capitalCity = new City("Rome", 25, 25, playerFaction.id);
  gameMap.addCity(capitalCity);
  playerFaction.cities.push(capitalCity.id);
  
  // Create starting hero
  const hero = new Hero("Marcus Aurelius", playerFaction.id, "warrior");
  hero.x = 25;
  hero.y = 25;
  gameMap.heroes.set(hero.id, hero);
  playerFaction.heroes.push(hero.id);
  
  // Create starting units
  const warrior = new Unit("warrior", playerFaction.id);
  const settler = new Unit("settler", playerFaction.id);
  
  // Create unit stack
  const startingStack = new UnitStack(26, 25);
  startingStack.addUnit(warrior);
  startingStack.addUnit(settler);
  startingStack.leader = hero.id;
  
  gameMap.addUnitStack(startingStack);
  
  return gameMap;
}

// Export the classes for use in your game
export { Tile, Unit, UnitStack, City, Faction, Hero, BigMap, initializeGame };