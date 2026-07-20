# GameManager.gd - 遊戲管理器腳本
extends Node

# 遊戲數據
var game_data = {
	"turn": 1,
	"current_player": 1,
	"max_players": 2,
	"resources": {
		1: {"gold": 1000, "food": 500, "wood": 300},
		2: {"gold": 1000, "food": 500, "wood": 300}
	},
	"research": {
		1: [],
		2: []
	}
}

# 單位類型數據
var unit_types = {
	"步兵": {
		"cost": {"gold": 100, "food": 50},
		"stats": {"hp": 100, "attack": 20, "movement": 2, "defense": 15}
	},
	"弓箭手": {
		"cost": {"gold": 120, "food": 30, "wood": 20},
		"stats": {"hp": 80, "attack": 25, "movement": 2, "defense": 10}
	},
	"騎兵": {
		"cost": {"gold": 200, "food": 100},
		"stats": {"hp": 150, "attack": 30, "movement": 3, "defense": 20}
	}
}

# 建築類型數據
var building_types = {
	"城堡": {
		"cost": {"gold": 0, "wood": 0},
		"effects": {"defense_bonus": 5}
	},
	"兵營": {
		"cost": {"gold": 200, "wood": 100},
		"effects": {"can_recruit": ["步兵", "弓箭手"]}
	},
	"市場": {
		"cost": {"gold": 150, "wood": 80},
		"effects": {"gold_bonus": 5}
	},
	"馬廄": {
		"cost": {"gold": 300, "wood": 150},
		"effects": {"can_recruit": ["騎兵"]}
	},
	"農場": {
		"cost": {"gold": 100, "wood": 50},
		"effects": {"food_bonus": 10}
	}
}

func _ready():
	# 初始化遊戲數據
	initialize_game_data()

func initialize_game_data():
	# 重置遊戲數據到初始狀態
	game_data.turn = 1
	game_data.current_player = 1

func get_player_resources(player_id: int):
	return game_data.resources.get(player_id, {})

func can_afford(player_id: int, cost: Dictionary) -> bool:
	var resources = get_player_resources(player_id)
	
	for resource_type in cost.keys():
		if resources.get(resource_type, 0) < cost[resource_type]:
			return false
	
	return true

func spend_resources(player_id: int, cost: Dictionary) -> bool:
	if not can_afford(player_id, cost):
		return false
	
	var resources = get_player_resources(player_id)
	
	for resource_type in cost.keys():
		resources[resource_type] -= cost[resource_type]
	
	return true

func add_resources(player_id: int, amount: Dictionary):
	var resources = get_player_resources(player_id)
	
	for resource_type in amount.keys():
		resources[resource_type] =