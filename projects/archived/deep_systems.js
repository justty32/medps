// Deep Systems: Nomads, Organizations, History & Noble Families

// Nomad Clan - Mobile tribal societies with complex social structures
struct NomadClan {
  id;
  name;
  tribalName;                    // Traditional tribal designation
  totem;                         // Sacred animal or symbol
  
  // Clan identity and culture
  ethnicity;                     // Ethnic/cultural background
  language;                      // Spoken language and dialects
  traditions;                    // Cultural practices and taboos
  oralHistory;                   // Stories passed down through generations
  ancestralSpirits;              // Revered ancestors and their guidance
  
  // Leadership and hierarchy
  khan;                          // Current clan leader
  chieftains;                    // Sub-clan leaders
  elders;                        // Wise advisors and tradition keepers
  shamans;                       // Spiritual leaders and healers
  warLeaders;                    // Military commanders
  
  // Social structure
  families;                      // Family units within clan
  bloodlines;                    // Important family lineages
  marriageAlliances;             // Inter-clan marriage connections
  adoptedMembers;                // Non-blood members accepted into clan
  slaves;                        // Captured or indentured individuals
  
  // Population and demographics
  totalPopulation;               // Total clan members
  warriors;                      // Fighting-age males and females
  herders;                       // Livestock specialists
  craftsmen;                     // Specialized workers
  children;                      // Next generation
  elders;                        // Elderly members
  
  // Mobility and territory
  currentLocation;               // Present position
  seasonalRoute;                 // Traditional migration path
  territorialClaims;             // Areas considered clan territory
  sacredSites;                   // Holy places along migration routes
  camps;                         // Temporary settlement locations
  
  // Livestock and resources
  herds;                         // Horses, cattle, sheep, goats, camels
  herdComposition;               // Numbers and types of animals
  pastureLands;                  // Grazing areas used
  waterRights;                   // Access to water sources
  tradableGoods;                 // Items for trade with settled peoples
  
  // Military organization
  tribalWarriors;                // Clan fighting force
  horseArchers;                  // Mounted bowmen
  cavalry;                       // Heavy mounted warriors
  scouts;                        // Reconnaissance specialists
  warBands;                      // Raiding parties
  
  // Economic activities
  animalHusbandry;               // Livestock breeding and care
  hunting;                       // Wild game hunting
  gathering;                     // Plant and resource collection
  crafting;                      // Weapon, tool, and textile production
  trading;                       // Commerce with other groups
  raiding;                       // Pillaging settled areas
  
  // Relationships and diplomacy
  alliedClans;                   // Friendly neighboring clans
  rivalClans;                    // Competing or hostile clans
  tributeRelations;              // Clans paying or receiving tribute
  settledRelations;              // Relations with agricultural societies
  tradePartners;                 // Regular trading relationships
  
  // Spiritual and cultural practices
  religion;                      // Dominant religious beliefs
  rituals;                       // Ceremonial practices
  festivals;                     // Seasonal celebrations
  comingOfAge;                   // Initiation ceremonies
  funeralRites;                  // Death and burial customs
  
  // Adaptation and survival
  weatherPrediction;             // Knowledge of climate patterns
  animalBehavior;                // Understanding of herd animals
  navigation;                    // Wayfinding and route knowledge
  resourceManagement;            // Sustainable use of resources
  conflictResolution;            // Internal dispute mechanisms
  
  // Historical memory
  greatMigrations;               // Major historical movements
  famousKhans;                   // Legendary leaders
  tribalWars;                    // Significant conflicts
  culturalHeroes;                // Mythical and historical figures
  prophecies;                    // Tribal predictions and omens
  
  // Change and development
  sedentarization;               // Trend toward settling down
  urbanization;                  // Interaction with cities
  technological_adoption;        // Uptake of new technologies
  cultural_change;               // Evolution of traditions
  political_evolution;           // Changes in governance
}

// Organization - Formal groups, institutions, and power structures
struct Organization {
  id;
  name;                          // Official name
  commonName;                    // How it's commonly known
  motto;                         // Official motto or creed
  symbol;                        // Heraldic symbol or emblem
  
  // Organizational type and purpose
  type;                          // Military, religious, trade, academic, criminal, etc.
  purpose;                       // Primary mission and goals
  scope;                         // Local, regional, national, international
  legalStatus;                   // Legal, illegal, semi-legal, secret
  
  // Structure and hierarchy
  ranks;                         // Hierarchical positions
  leadership;                    // Current leaders at each level
  membership;                    // Current members by rank
  totalMembers;                  // Total membership count
  recruitmentCriteria;           // Requirements for joining
  
  // Governance and decision-making
  governanceStructure;           // How decisions are made
  councils;                      // Governing bodies and committees
  elections;                     // Leadership selection process
  constitution;                  // Founding documents and rules
  byLaws;                        // Operational procedures
  
  // Resources and assets
  treasury;                      // Financial resources
  properties;                    // Owned buildings and lands
  equipment;                     // Tools, weapons, supplies
  knowledge;                     // Specialized information and secrets
  connections;                   // Network of contacts and allies
  
  // Operations and activities
  primaryActivities;             // Main organizational functions
  secretActivities;              // Hidden or covert operations
  publicFace;                    // How organization presents itself
  recruitment;                   // Methods of gaining new members
  training;                      // Education and skill development
  
  // Geographic presence
  headquarters;                  // Main base of operations
  chapters;                      // Local branches or divisions
  territories;                   // Areas of influence or control
  safeHouses;                    // Secure locations for operations
  networks;                      // Communication and supply lines
  
  // Relationships and politics
  allies;                        // Friendly organizations
  enemies;                       // Hostile organizations
  rivals;                        // Competing organizations
  patrons;                       // Sponsors and benefactors
  clients;                       // Those who receive services
  
  // Internal dynamics
  factions;                      // Internal competing groups
  loyalties;                     // Member loyalty levels
  morale;                        // Overall organizational morale
  discipline;                    // Adherence to rules and hierarchy
  corruption;                    // Level of internal corruption
  
  // History and tradition
  founded;                       // When organization was established
  founder;                       // Who established it
  traditions;                    // Long-standing customs
  ceremonies;                    // Ritual observances
  martyrs;                       // Members who died for the cause
  
  // Influence and power
  politicalInfluence;            // Sway over government
  economicPower;                 // Control over resources/trade
  militaryStrength;              // Fighting capability
  informationNetwork;            // Intelligence gathering ability
  culturalImpact;                // Influence on society
  
  // Adaptation and evolution
  reforms;                       // Major organizational changes
  schisms;                       // Splits or breakaway factions
  mergers;                       // Combinations with other groups
  succession;                    // Leadership transition planning
  modernization;                 // Adaptation to changing times
}

// Historical Event - Significant occurrences that shape the world
struct HistoricalEvent {
  id;
  name;                          // Event name
  type;                          // War, plague, discovery, catastrophe, etc.
  
  // Temporal aspects
  startDate;                     // When event began
  endDate;                       // When event concluded
  duration;                      // How long it lasted
  era;                           // Historical period
  
  // Geographic scope
  location;                      // Where event occurred
  scope;                         // Local, regional, continental, global
  affected_regions;              // Areas impacted by event
  
  // Participants and actors
  primaryActors;                 // Main individuals involved
  factions;                      // Groups and nations involved
  heroes;                        // Celebrated figures from event
  villains;                      // Notorious figures from event
  victims;                       // Those who suffered
  
  // Causes and context
  immediate_causes;              // Direct triggers
  underlying_causes;             // Long-term factors
  catalyst;                      // Final trigger that started event
  precursor_events;              // Events that led to this one
  
  // Course of events
  phases;                        // Major stages of the event
  turning_points;                // Critical moments that changed outcome
  climax;                        // Peak or most intense moment
  key_battles;                   // Important military engagements (if applicable)
  negotiations;                  // Diplomatic efforts
  
  // Consequences and aftermath
  immediate_effects;             // Direct results
  long_term_effects;             // Lasting consequences
  cultural_impact;               // Changes to society and culture
  political_changes;             // Governmental and power shifts
  economic_impact;               // Financial and trade effects
  technological_changes;         // Technological developments
  
  // Human cost
  casualties;                    // Deaths and injuries
  displacement;                  // Population movements
  destruction;                   // Property and infrastructure damage
  suffering;                     // Human misery caused
  
  // Documentation and memory
  chronicles;                    // Written records of event
  witnesses;                     // Eyewitness accounts
  artifacts;                     // Physical evidence remaining
  monuments;                     // Memorials and commemorations
  legends;                       // Mythologized versions of event
  
  // Historical interpretation
  multiple_perspectives;         // Different viewpoints on event
  controversies;                 // Disputed aspects
  historical_significance;       // Why event is important
  lessons;                       // What can be learned
  parallels;                     // Similar events in history
  
  // Ongoing relevance
  modern_echoes;                 // How event still affects present
  commemorations;                // How event is remembered
  political_use;                 // How event is used in current politics
  educational_value;             // Teaching importance
}

// Noble House - Aristocratic families with complex genealogies
struct NobleHouse {
  id;
  name;                          // House name
  fullTitle;                     // Complete formal title
  motto;                         // House motto
  colors;                        // Heraldic colors
  sigil;                         // Coat of arms/symbol
  
  // House status and rank
  rank;                          // Duke, Count, Baron, etc.
  precedence;                    // Order of precedence at court
  peerage;                       // Type of nobility (hereditary, life, etc.)
  nobility_level;                // High, middle, minor nobility
  
  // Geographic holdings
  primary_seat;                  // Main castle/estate
  secondary_holdings;            // Other properties owned
  ancestral_lands;               // Traditional family territories
  recent_acquisitions;           // Newly gained properties
  lost_territories;              // Former holdings no longer owned
  
  // Family structure and genealogy
  head_of_house;                 // Current family leader
  heir;                          // Designated successor
  family_tree;                   // Complete genealogical record
  living_members;                // All current family members
  cadet_branches;                // Junior branches of family
  
  // Wealth and resources
  treasury;                      // Family wealth
  income_sources;                // How money is made
  debts;                         // Money owed
  valuable_assets;               // Precious possessions
  trade_interests;               // Commercial investments
  
  // Political influence
  court_positions;               // Roles at royal court
  government_posts;              // Administrative positions held
  military_commands;             // Army leadership roles
  diplomatic_missions;           // Foreign relations involvement
  voting_power;                  // Influence in noble assemblies
  
  // Alliances and relationships
  allied_houses;                 // Friendly noble families
  rival_houses;                  // Competing families
  vassal_houses;                 // Lesser nobles who serve this house
  liege_lord;                    // Higher noble this house serves
  marriage_connections;          // Inter-house marriage links
  
  // Military capabilities
  household_guard;               // Personal military force
  levy_troops;                   // Soldiers from house lands
  knights;                       // Elite heavy cavalry
  fortifications;                // Defensive structures
  military_traditions;           // Martial customs and honors
  
  // Cultural and intellectual pursuits
  patronage;                     // Artists, scholars supported
  library;                       // Books and knowledge collection
  artworks;                      // Valuable art collection
  scholarly_interests;           // Academic pursuits
  cultural_contributions;        // Impact on arts and learning
  
  // Reputation and honor
  honor;                         // Family honor rating
  reputation;                    // Public perception
  scandals;                      // Past disgraceful incidents
  achievements;                  // Notable accomplishments
  legends;                       // Mythical stories about family
  
  // Succession and inheritance
  succession_law;                // How leadership is inherited
  inheritance_disputes;          // Conflicts over succession
  legitimacy_issues;             // Questions about rightful heir
  adoption_records;              // Non-blood family members
  
  // Historical significance
  founding;                      // When and how house was established
  founder;                       // First head of house
  great_deeds;                   // Famous historical actions
  dark_periods;                  // Times of disgrace or decline
  restoration;                   // Recovery from difficult times
  
  // Traditions and customs
  family_traditions;             // Unique customs
  ceremonies;                    // Ritual observances
  heirlooms;                     // Treasured family possessions
  burial_grounds;                // Family cemetery
  ghost_stories;                 // Supernatural family legends
}

// Character - Individual people with detailed backgrounds
struct Character {
  id;
  firstName;
  lastName;
  titles;                        // Noble titles, military ranks, etc.
  nicknames;                     // Common names and epithets
  
  // Basic information
  gender;
  age;
  birthDate;
  birthplace;
  deathDate;                     // If deceased
  causeOfDeath;                  // How they died
  
  // Physical characteristics
  height;
  build;                         // Muscular, thin, stocky, etc.
  hairColor;
  eyeColor;
  distinguishingMarks;           // Scars, tattoos, etc.
  health;                        // Overall physical condition
  
  // Social background
  socialClass;                   // Noble, merchant, peasant, etc.
  house;                         // Noble house (if applicable)
  profession;                    // Primary occupation
  education;                     // Level and type of education
  
  // Family relationships
  parents;                       // Mother and father
  siblings;                      // Brothers and sisters
  spouse;                        // Current marriage partner
  formerSpouses;                 // Previous marriages
  children;                      // Offspring
  
  // Personality traits
  personality;                   // Core personality characteristics
  virtues;                       // Positive qualities
  vices;                         // Negative traits
  fears;                         // What they're afraid of
  ambitions;                     // What they want to achieve
  
  // Skills and abilities
  combat_skills;                 // Fighting abilities
  intellectual_skills;           // Mental capabilities
  social_skills;                 // Interpersonal abilities
  specialized_knowledge;         // Expert areas
  
  // Political and social connections
  allies;                        // Political friends and supporters
  enemies;                       // Political opponents
  patrons;                       // Those who support them
  clients;                       // Those who depend on them
  reputation;                    // How others view them
  
  // Wealth and possessions
  personal_wealth;               // Money and valuables
  properties;                    // Land and buildings owned
  prized_possessions;            // Special items they value
  debts;                         // Money owed
  
  // Life events and experiences
  major_events;                  // Significant life experiences
  achievements;                  // Notable accomplishments
  failures;                      // Major setbacks
  secrets;                       // Hidden aspects of their life
  
  // Current status
  location;                      // Where they currently are
  position;                      // Current role or job
  health_status;                 // Current physical condition
  mental_state;                  // Current psychological condition
  loyalty;                       // Degree of faithfulness to causes
  
  // Relationships and influence
  influence_network;             // Web of social connections
  blackmail_material;            // Secrets they know about others
  favors_owed;                   // People who owe them
  obligations;                   // What they owe others
}

// Genealogy - Family tree relationships and bloodlines
struct Genealogy {
  id;
  house;                         // Which noble house this tracks
  
  // Family tree structure
  generations;                   // Number of generations recorded
  founding_ancestor;             // First recorded family member
  family_branches;               // Different family lines
  
  // Relationship tracking
  marriages;                     // All recorded marriages
  births;                        // All recorded births
  deaths;                        // All recorded deaths
  adoptions;                     // Non-blood family additions
  
  // Bloodline purity and claims
  legitimacy_records;            // Legal marriage and birth records
  bastard_lines;                 // Illegitimate children and descendants
  claims_to_inheritance;         // Who has rights to what
  disputed_parentage;            // Questionable family connections
  
  // Genetic traits and characteristics
  inherited_traits;              // Physical characteristics passed down
  family_diseases;               // Hereditary health issues
  bloodline_powers;              // Magical or special abilities (if fantasy)
  
  // Documentation and records
  family_chronicles;             // Written family histories
  genealogical_charts;           // Visual family trees
  DNA_evidence;                  // Genetic proof of relationships (if modern)
  witness_testimony;             // People who can verify relationships
  
  // Political implications
  succession_rights;             // Who can inherit titles
  alliance_connections;          // How marriages create political bonds
  blood_feuds;                   // Hereditary conflicts between families
  royal_connections;             // Links to ruling families
}