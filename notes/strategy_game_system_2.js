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
}

// Spy/Agent - Espionage and intelligence units
struct Spy {
  id;
  name;
  faction;
  cover;                         // Cover identity/profession
  
  // Core spy attributes
  skill;                         // Overall spy skill level
  stealth;                       // Ability to remain undetected
  infiltration;                  // Ability to penetrate organizations
  sabotage;                      // Destructive capabilities
  diplomacy;                     // Social manipulation skills
  
  // Current assignment
  location;                      // Current city/region
  target;                        // Target faction or city
  mission;                       // Current mission type
  missionProgress;               // Progress on current mission
  
  // Status and equipment
  discovered;                    // Has been identified as spy
  equipment;                     // Special spy equipment
  contacts;                      // Network of informants
  safehouse;                     // Safe locations for operations
  
  // Experience and specialization
  experience;                    // Spy experience points
  specialization;                // assassination, theft, diplomacy, etc.
  languages;                     // Known languages for infiltration
  disguises;                     // Available cover identities
  
  // Risk and loyalty
  exposure;                      // Risk of being caught
  loyalty;                       // Loyalty to faction
  turncoat;                      // Chance of switching sides
  burnout;                       // Mental stress from operations
}

// Diplomat - Political and trade representatives
struct Diplomat {
  id;
  name;
  faction;
  rank;                          // Ambassador, envoy, trade representative
  
  // Diplomatic skills
  negotiation;                   // Ability to make deals
  charisma;                      // Personal appeal and influence
  intelligence;                  // Understanding of politics
  languages;                     // Communication abilities
  
  // Current assignment
  assignedTo;                    // Target faction or neutral city
  mission;                       // Current diplomatic mission
  embassy;                       // Has established embassy
  immunity;                      // Diplomatic immunity status
  
  // Relationships and reputation
  reputation;                    // General diplomatic reputation
  personalRelations;             // Relations with specific leaders
  culturalKnowledge;             // Understanding of foreign cultures
  
  // Capabilities and effects
  tradeBonus;                    // Bonus to trade negotiations
  spyResistance;                 // Resistance to enemy espionage
  informationGathering;          // Intelligence gathering ability
  propagandaSkill;               // Ability to influence public opinion
  
  // Resources and support
  retinue;                       // Diplomatic staff and guards
  gifts;                         // Diplomatic gifts and bribes
  budget;                        // Diplomatic expense account
  connections;                   // Network of contacts
}

// Wonder - Unique monumental buildings
struct Wonder {
  id;
  name;
  type;                          // world, national, natural
  era;                           // Time period when buildable
  
  // Construction requirements
  requiredTech;                  // Technologies needed
  requiredResources;             // Resources consumed
  productionCost;                // Total production needed
  specialRequirements;           // Terrain, religion, government, etc.
  
  // Location and ownership
  city;                          // City where built
  faction;                       // Owning faction
  coordinates;                   // Map location
  
  // Effects and bonuses
  globalEffects;                 // Effects for entire faction
  cityEffects;                   // Effects for host city
  regionEffects;                 // Effects for surrounding area
  victoryPoints;                 // Contribution to victory conditions
  
  // Special properties
  unique;                        // Only one can exist in world
  transferable;                  // Can change hands if city captured
  destructible;                  // Can be destroyed by war/disaster
  cultural;                      // Generates cultural influence
  
  // Maintenance and upkeep
  maintenanceCost;               // Ongoing maintenance cost
  specialistSlots;               // Great people slots provided
  obsolescence;                  // When wonder becomes obsolete
}

// Great Person - Exceptional individuals with special abilities
struct GreatPerson {
  id;
  name;
  type;                          // scientist, artist, merchant, prophet, etc.
  faction;
  
  // Core attributes
  skill;                         // Overall skill level
  specialization;                // Specific area of expertise
  era;                           // Historical era
  lifespan;                      // Remaining useful turns
  
  // Abilities and actions
  abilities;                     // Special abilities available
  usesRemaining;                 // Limited use abilities
  permanentEffects;              // Ongoing passive effects
  oneTimeActions;                // Powerful single-use actions
  
  // Location and status
  location;                      // Current city or position
  assignment;                    // Current role or project
  active;                        // Currently providing benefits
  
  // Historical impact
  achievements;                  // Major accomplishments
  legacy;                        // Lasting effects after death
  inspiration;                   // Bonus to great person generation
  
  // Special mechanics
  retirement;                    // Can settle in city permanently
  works;                         // Great works created
  discoveries;                   // Scientific discoveries made
  influence;                     // Cultural/religious influence
}

// Disease/Plague - Biological threats and health crises
struct Disease {
  id;
  name;
  type;                          // bacterial, viral, parasitic, etc.
  
  // Transmission and spread
  contagion;                     // How easily it spreads
  transmission;                  // Methods of transmission
  incubationPeriod;              // Time before symptoms appear
  duration;                      // How long disease lasts
  
  // Effects on population
  mortality;                     // Death rate
  morbidity;                     // Sickness/disability rate
  economicImpact;                // Effect on productivity
  militaryImpact;                // Effect on military units
  
  // Environmental factors
  climatePreference;             // Climates where disease thrives
  seasonality;                   // Seasonal patterns
  animalVectors;                 // Animal carriers
  
  // Spread mechanics
  currentLocations;              // Cities/regions affected
  spreadRate;                    // Speed of geographic spread
  immunity;                      // Population immunity levels
  
  // Medical response
  treatment;                     // Available treatments
  prevention;                    // Preventive measures
  quarantine;                    // Isolation effectiveness
  research;                      // Medical research progress
}

// Mercenary Company - Professional military units for hire
struct MercenaryCompany {
  id;
  name;
  reputation;                    // Military reputation and reliability
  
  // Company composition
  units;                         // Available units for hire
  commander;                     // Company leader
  size;                          // Total company size
  specialty;                     // Military specialization
  
  // Contract terms
  currentEmployer;               // Currently hired by which faction
  contractLength;                // Duration of current contract
  payment;                       // Required payment per turn
  loyalty;                       // Likelihood of breaking contract
  
  // Location and availability
  location;                      // Current position
  available;                     // Available for hire
  exclusions;                    // Factions they won't work for
  
  // Experience and equipment
  veterancy;                     // Company experience level
  equipment;                     // Quality of arms and armor
  morale;                        // Company morale and cohesion
  
  // Special capabilities
  siege;                         // Siege warfare capabilities
  naval;                         // Naval operations ability
  reconnaissance;                // Scouting and intelligence
  engineering;                   // Military engineering skills
}

// Barbarian Tribe - Nomadic or primitive groups
struct BarbarianTribe {
  id;
  name;
  type;                          // raiders, nomads, primitives, etc.
  
  // Tribal characteristics
  culture;                       // Tribal culture and customs
  technology;                    // Technological level
  population;                    // Tribe size
  aggressiveness;                // Tendency toward violence
  
  // Location and movement
  territory;                     // Tribal lands or range
  mobility;                      // Movement patterns
  camps;                         // Temporary settlements
  migration;                     // Seasonal migration routes
  
  // Military capabilities
  warriors;                      // Available fighting force
  tactics;                       // Preferred combat methods
  equipment;                     // Weapons and armor quality
  
  // Relations with civilizations
  hostility;                     // faction_id -> hostility level
  tribute;                       // Tribute payments received
  alliances;                     // Temporary alliances
  trade;                         // Trade relationships
  
  // Development potential
  settleProbability;             // Chance of settling down
  assimilation;                  // Chance of joining civilization
  evolution;                     // Potential to become faction
  
  // Threats and challenges
  infighting;                    // Internal tribal conflicts
  resources;                     // Access to resources
  survival;                      // Survival challenges faced
}

// Natural Disaster - Environmental catastrophes
struct NaturalDisaster {
  id;
  type;                          // earthquake, flood, volcano, drought, etc.
  name;                          // Specific disaster instance
  
  // Disaster characteristics
  severity;                      // Intensity of disaster
  duration;                      // How long effects last
  predictability;                // How much warning given
  frequency;                     // How often this type occurs
  
  // Affected area
  epicenter;                     // Central point of disaster
  radius;                        // Area of effect
  affectedTiles;                 // Specific tiles impacted
  affectedCities;                // Cities in disaster zone
  
  // Immediate effects
  destruction;                   // Buildings/infrastructure destroyed
  casualties;                    // Population killed or injured
  displacement;                  // People forced to relocate
  economicLoss;                  // Immediate economic damage
  
  // Ongoing consequences
  reconstruction;                // Time/cost to rebuild
  diseaseRisk;                   // Increased disease probability
  refugee;                       // Refugee population created
  psychological;                 // Morale and happiness effects
  
  // Recovery and adaptation
  reliefEfforts;                 // Disaster relief measures
  adaptations;                   // Long-term adaptations made
  preparedness;                  // Future disaster preparedness
  lessons;                       // Technological/social learning
}

// Artifact - Ancient or magical items with special powers
struct Artifact {
  id;
  name;
  type;                          // weapon, tool, relic, treasure
  origin;                        // Civilization or era of creation
  
  // Discovery and ownership
  location;                      // Current location
  owner;                         // Current possessor
  discovered;                    // Has been found
  discoveryMethod;               // How it was discovered
  
  // Powers and effects
  abilities;                     // Special abilities granted
  bonuses;                       // Stat bonuses provided
  curses;                        // Negative effects
  activation;                    // How abilities are triggered
  
  // Historical significance
  age;                           // Approximate age
  creators;                      // Who made it
  history;                       // Previous owners and events
  legend;                        // Myths and stories about it
  
  // Physical properties
  durability;                    // Resistance to damage
  size;                          // Physical dimensions
  weight;                        // Portability
  materials;                     // What it's made from
  
  // Game mechanics
  questItem;                     // Required for specific quests
  tradeable;                     // Can be traded or sold
  destructible;                  // Can be destroyed
  replication;                   // Can copies be made
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