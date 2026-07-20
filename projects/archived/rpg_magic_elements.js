// RPG and Magic Elements for Strategy Games
// Mount & Blade style individual character focus with fantasy elements

// Player Character - The main character (like Mount & Blade protagonist)
struct PlayerCharacter {
  id;
  name;
  title;                         // Lord, Duke, Archmage, etc.
  portrait;                      // Character appearance
  
  // Core RPG attributes
  level;
  experience;
  attributePoints;               // Unspent attribute points
  skillPoints;                   // Unspent skill points
  
  // Primary attributes
  strength;                      // Physical power, melee damage
  agility;                       // Speed, ranged accuracy, dodge
  intelligence;                  // Mana, spell power, learning
  charisma;                      // Leadership, diplomacy, trade
  constitution;                  // Health, stamina, disease resistance
  wisdom;                        // Mana regeneration, spell resistance
  
  // Secondary stats (derived from primaries)
  health;
  maxHealth;
  mana;
  maxMana;
  stamina;
  maxStamina;
  carryWeight;
  movementSpeed;
  
  // Skills (0-100 scale)
  skills;                        // Map of skill_name -> skill_level
  // Combat skills: one_handed, two_handed, polearm, archery, crossbow, throwing
  // Magic schools: destruction, restoration, illusion, conjuration, enchantment
  // Leadership: leadership, tactics, persuasion, prisoner_management
  // Roguery: stealth, lockpicking, pickpocket, sneaking
  // Trade: trade, smithing, medicine, engineering
  // Athletics: riding, athletics, bow_control, shield
  
  // Equipment and inventory
  equipment;                     // Worn items (weapon, armor, accessories)
  inventory;                     // Carried items
  horse;                         // Mounted creature
  
  // Character progression
  proficiencies;                 // Weapon proficiency levels
  perks;                         // Unlocked character perks
  traits;                        // Personality traits affecting gameplay
  
  // Social status and reputation
  reputation;                    // Overall reputation
  honor;                         // Honor rating
  renown;                        // Fame level
  rightToRule;                   // Legitimacy as ruler
  
  // Relationships
  spouse;                        // Married to
  children;                      // Offspring
  companions;                    // Party companions
  enemies;                       // Personal enemies
  friends;                       // Personal allies
  
  // Current state
  location;                      // Current position/city
  party;                         // Current party/army
  faction;                       // Allegiance (can be independent)
  fiefs;                         // Controlled territories
  prisoner;                      // Is currently captured
  
  // Magic affinity and powers
  magicSchools;                  // Mastered schools of magic
  spellBook;                     // Known spells
  magicItems;                    // Enchanted equipment
  familiarBond;                  // Bonded magical creature
}

// Companion - Recruitable NPCs with personalities
struct Companion {
  id;
  name;
  background;                    // Noble, commoner, outlaw, scholar, etc.
  portrait;
  
  // Core stats (similar to player but NPC-scaled)
  level;
  attributes;                    // Primary attributes
  skills;                        // Skills and proficiencies
  
  // Personality and behavior
  personality;                   // Brave, cautious, honorable, etc.
  likes;                         // What they approve of
  dislikes;                      // What they disapprove of
  morality;                      // Good, neutral, evil alignment
  
  // Relationship with player
  relation;                      // How much they like player (-100 to 100)
  loyalty;                       // Likelihood to stay with player
  morale;                        // Current happiness/satisfaction
  
  // Capabilities and roles
  preferredRole;                 // Scout, tactician, surgeon, engineer, etc.
  partyRole;                     // Current role in party
  combatAI;                      // Combat behavior preferences
  
  // Equipment and progression
  equipment;                     // Current gear
  autoUpgrade;                   // Automatically upgrade equipment
  
  // Personal quest and story
  personalQuest;                 // Companion's personal storyline
  backstory;                     // Character history and motivations
  goals;                         // What they want to achieve
  
  // Special abilities
  uniqueSkills;                  // Companion-specific abilities
  teachableSkills;               // Skills they can teach player
  bonuses;                       // Bonuses they provide to party
  
  // Recruitment and departure
  recruitLocation;               // Where they can be found
  recruitCost;                   // Cost to recruit them
  leaveConditions;               // Conditions that make them leave
}

// Magic School - Different types of magic
struct MagicSchool {
  id;
  name;                          // Destruction, Restoration, Illusion, etc.
  description;                   // What this school focuses on
  
  // School characteristics
  primaryAttribute;              // Intelligence, Wisdom, Charisma, etc.
  difficulty;                    // Learning difficulty
  manaType;                      // Type of mana required
  castingStyle;                  // Ritual, instant, channeled
  
  // Spell categories
  offensiveSpells;               // Damage and combat spells
  defensiveSpells;               // Protection and healing spells
  utilitySpells;                 // Non-combat utility spells
  summoningSpells;               // Creature summoning spells
  
  // Learning and advancement
  teachers;                      // Who can teach this school
  books;                         // Skill books available
  practice;                      // How to practice and improve
  
  // Restrictions and requirements
  alignmentRestrictions;         // Moral restrictions on learning
  culturalBias;                  // Cultures that favor this school
  forbiddenBy;                   // Religions/cultures that forbid it
  
  // Advanced techniques
  masteryBonuses;                // Bonuses for high skill
  schoolSynergies;               // Bonuses when combined with other schools
  ultimateSpells;                // Highest level spells
}

// Spell - Individual magical abilities
struct Spell {
  id;
  name;
  school;                        // Which magic school
  level;                         // Spell level/difficulty
  
  // Casting requirements
  manaCost;                      // Mana required to cast
  castingTime;                   // Time to cast
  components;                    // Required components
  range;                         // Spell range
  
  // Effects and mechanics
  damage;                        // Damage dealt (if combat spell)
  duration;                      // How long effects last
  areaOfEffect;                  // Area affected by spell
  targets;                       // Valid targets
  
  // Learning and use
  requiredSkill;                 // Minimum skill to learn
  teacher;                       // Who can teach this spell
  spellbook;                     // Book that contains spell
  
  // Special properties
  ritual;                        // Requires ritual casting
  concentration;                 // Requires concentration to maintain
  reversible;                    // Can be cast in reverse
  
  // Upgrades and variations
  improvedVersions;              // Higher level versions
  metamagic;                     // Ways to modify the spell
  combinations;                  // Spells that work well together
}

// Magic Item - Enchanted equipment and artifacts
struct MagicItem {
  id;
  name;
  baseItem;                      // What mundane item this is based on
  rarity;                        // Common, uncommon, rare, legendary, artifact
  
  // Physical properties
  slot;                          // Where it's equipped
  appearance;                    // Visual description
  materials;                     // Magical materials used
  
  // Magical properties
  enchantments;                  // List of magical effects
  charges;                       // Limited use abilities
  activationMethod;              // How magical abilities are triggered
  
  // Requirements and restrictions
  requiredLevel;                 // Minimum level to use
  requiredSkills;                // Skills needed to use effectively
  classRestrictions;             // Who can use this item
  alignmentRestrictions;         // Moral restrictions
  
  // Creation and history
  creator;                       // Who made this item
  creation_method;               // How it was created
  age;                           // How old the item is
  previous_owners;               // Notable previous owners
  
  // Game mechanics
  durability;                    // Item condition
  repairability;                 // Can it be repaired
  upgradeable;                   // Can be enhanced further
  cursed;                        // Has negative effects
  
  // Special abilities
  setBonus;                      // Bonus when worn with other items
  scaling;                       // Grows stronger with user
  sentient;                      // Has its own personality
  questRelevance;                // Important to specific quests
}

// Familiar/Pet - Magical creature companions
struct Familiar {
  id;
  name;
  species;                       // Dragon, wolf, hawk, imp, etc.
  type;                          // Magical, mundane, construct, undead
  
  // Physical attributes
  size;                          // Tiny, small, medium, large, huge
  appearance;                    // Visual description
  lifespan;                      // Natural lifespan
  
  // Stats and abilities
  level;                         // Familiar level
  health;
  mana;
  attributes;                    // Strength, agility, etc.
  abilities;                     // Special creature abilities
  
  // Bond with master
  bondStrength;                  // Strength of magical bond
  sharedExperience;              // Gains XP when master does
  empathicLink;                  // Shares emotions/sensations
  mentalLink;                    // Can communicate telepathically
  
  // Combat and utility
  combatRole;                    // Fighter, scout, support, etc.
  combatAI;                      // Combat behavior
  skills;                        // Special skills (tracking, stealth, etc.)
  
  // Growth and development
  evolution;                     // Can evolve into different forms
  training;                      // Can be trained in new abilities
  equipment;                     // Can wear/use items
  
  // Special properties
  magical_abilities;             // Innate magical powers
  breath_weapon;                 // Special attacks
  flight;                        // Can fly
  invisibility;                  // Natural stealth abilities
  
  // Care and maintenance
  feeding;                       // Dietary requirements
  loyalty;                       // Likelihood to obey commands
  happiness;                     // Current satisfaction level
  dismissible;                   // Can be sent away temporarily
}

// Quest - Story missions and adventures
struct Quest {
  id;
  name;
  type;                          // Main, side, companion, guild, random
  category;                      // Combat, diplomatic, trade, exploration, etc.
  
  // Quest giver and context
  giver;                         // Who gave the quest
  location;                      // Where quest was received
  urgency;                       // Time pressure level
  
  // Objectives and progress
  mainObjective;                 // Primary goal
  subObjectives;                 // Secondary goals
  progress;                      // Current progress tracking
  completed;                     // Which objectives are done
  
  // Requirements and restrictions
  prerequisites;                 // Requirements to start quest
  level_requirement;             // Minimum level needed
  skill_requirements;            // Skills needed to complete
  
  // Rewards and consequences
  experience_reward;             // XP gained
  gold_reward;                   // Money earned
  item_rewards;                  // Items received
  reputation_effects;            // Reputation changes
  relationship_effects;          // How it affects relationships
  
  // Quest mechanics
  failure_conditions;            // Ways the quest can fail  
  time_limit;                    // Deadline for completion
  repeatable;                    // Can be done multiple times
  
  // Story elements
  description;                   // Quest description and lore
  dialogue;                      // Important conversations
  cutscenes;                     // Scripted story moments
  
  // Branching and choices
  choices;                       // Player decisions that matter
  branches;                      // Different quest paths
  consequences;                  // Long-term effects of choices
}

// Guild/Organization - Joinable factions with ranks and benefits
struct Guild {
  id;
  name;
  type;                          // Fighters, Mages, Thieves, Merchants, etc.
  headquarters;                  // Main base of operations
  
  // Organization structure
  ranks;                         // Hierarchy of positions
  leaders;                       // Current leadership
  members;                       // Notable members
  
  // Membership and advancement
  joinRequirements;              // How to join
  rankRequirements;              // How to advance ranks
  currentRank;                   // Player's current rank
  reputation;                    // Standing within guild
  
  // Benefits and services
  training;                      // Skills/spells taught
  equipment;                     // Special gear available
  quests;                        // Guild-specific missions
  facilities;                    // Services provided
  
  // Guild politics
  rivals;                        // Competing organizations
  allies;                        // Friendly organizations
  enemies;                       // Hostile organizations
  politics;                      // Internal power struggles
  
  // Special features
  uniqueAbilities;               // Abilities only members get
  secrets;                       // Hidden knowledge/techniques
  artifacts;                     // Powerful items controlled by guild
  
  // Questlines and stories
  mainQuestline;                 // Guild's primary story arc
  advancement_quests;            // Quests to gain ranks
  moral_choices;                 // Ethical decisions affecting standing
}

// Dungeon/Ruin - Explorable locations with treasures and dangers
struct Dungeon {
  id;
  name;
  type;                          // Cave, ruin, tomb, tower, etc.
  origin;                        // Who built it and why
  
  // Physical characteristics
  location;                      // Map coordinates
  size;                          // Small, medium, large, massive
  depth;                         // Number of levels
  layout;                        // Linear, branching, maze, etc.
  
  // Difficulty and level
  recommendedLevel;              // Suggested player level
  dangerRating;                  // Overall threat level
  
  // Contents and features
  monsters;                      // Creatures inhabiting dungeon
  traps;                         // Mechanical and magical hazards
  puzzles;                       // Logic challenges
  treasures;                     // Valuable items hidden within
  
  // Exploration mechanics
  mapping;                       // Whether player must map it
  lighting;                      // Light levels and visibility
  environmental_hazards;         // Poison gas, flooding, etc.
  
  // Story and lore
  history;                       // Background story
  mystery;                       // Central mystery to solve
  boss;                          // Final challenge/enemy
  
  // Special mechanics
  respawning;                    // Do enemies/treasures respawn
  procedural;                    // Randomly generated elements
  persistent_changes;            // Player actions that permanently change dungeon
  
  // Rewards and significance
  unique_items;                  // Items only found here
  lore_reveals;                  // Story information discovered
  quest_relevance;               // Importance to main story
}

// Monster/Creature - Enemies and neutral creatures
struct Monster {
  id;
  name;
  species;                       // Dragon, orc, skeleton, etc.
  type;                          // Beast, humanoid, undead, construct, etc.
  
  // Combat stats
  level;
  health;
  attack;
  defense;
  speed;
  
  // Special abilities
  abilities;                     // Special attacks and powers
  spells;                        // Magical abilities
  resistances;                   // Damage resistances
  vulnerabilities;               // Damage weaknesses
  
  // Behavior and AI
  aggression;                    // How hostile they are
  intelligence;                  // Problem-solving ability
  group_behavior;                // Pack tactics, lone wolf, etc.
  
  // Habitat and ecology
  preferred_terrain;             // Where they're commonly found
  diet;                          // What they eat
  social_structure;              // Solitary, pack, hive, etc.
  
  // Interaction possibilities
  tameable;                      // Can be tamed as mount/pet
  recruitable;                   // Can join player's army
  communicable;                  // Can be reasoned with
  
  // Loot and rewards
  commonDrops;                   // Items commonly dropped
  rareDrops;                     // Rare items they might drop
  experienceValue;               // XP given when defeated
  
  // Magical properties
  magical_nature;                // Natural magic abilities
  summoned;                      // Can be summoned by spells
  banishable;                    // Can be banished back to origin
}

// Prophecy/Destiny - Fate and predetermined story elements
struct Prophecy {
  id;
  name;
  source;                        // Oracle, ancient text, vision, etc.
  
  // Prophecy content
  prediction;                    // What it foretells
  conditions;                    // Conditions for fulfillment
  signs;                         // Omens that indicate progress
  
  // Game mechanics
  playerRole;                    // Player's role in prophecy
  choices;                       // How player choices affect it
  fulfillment;                   // Progress toward completion
  
  // Effects and consequences
  benefits;                      // Rewards for fulfilling prophecy
  penalties;                     // Consequences of failure/defiance
  world_changes;                 // How fulfillment changes the world
  
  // Interpretation and ambiguity
  multiple_interpretations;      // Different ways to understand it
  red_herrings;                  // False signs or misleading clues
  twist;                         // Unexpected revelation about meaning
}

// Realm/Plane - Different dimensions and magical realms
struct Realm {
  id;
  name;
  type;                          // Material, shadow, fey, elemental, etc.
  
  // Physical properties
  terrain;                       // Landscape and geography
  climate;                       // Weather and environmental conditions
  physics;                       // Different physical laws
  time_flow;                     // How time passes relative to material world
  
  // Inhabitants and rulers
  natives;                       // Creatures natural to this realm
  rulers;                        // Powerful beings who control realm
  politics;                      // Power structures and conflicts
  
  // Access and travel
  portals;                       // Ways to enter/exit realm
  requirements;                  // What's needed to survive here
  dangers;                       // Unique threats in this realm
  
  // Resources and opportunities
  unique_resources;              // Materials only found here
  magic_strength;                // How powerful magic is here
  learning_opportunities;        // Special knowledge available
  
  // Story significance
  role_in_plot;                  // Importance to main story
  quests;                        // Adventures that take place here
  artifacts;                     // Powerful items located here
}