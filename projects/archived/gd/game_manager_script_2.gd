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
		resources[resource_type] = resources.get(resource_type, 0) + amount[resource_type]

func get_unit_type_data(unit_type: String):
	return unit_types.get(unit_type, {})

func get_building_type_data(building_type: String):
	return building_types.get(building_type, {})

func can_recruit_unit(town_data: Dictionary, unit_type: String, player_id: int) -> bool:
	# 檢查是否有足夠的資源
	var unit_data = get_unit_type_data(unit_type)
	if unit_data.is_empty():
		return false
	
	if not can_afford(player_id, unit_data.cost):
		return false
	
	# 檢查城鎮是否有相應的建築
	var required_buildings = get_required_buildings_for_unit(unit_type)
	for building in required_buildings:
		if building not in town_data.buildings:
			return false
	
	return true

func get_required_buildings_for_unit(unit_type: String) -> Array:
	match unit_type:
		"步兵", "弓箭手":
			return ["兵營"]
		"騎兵":
			return ["馬廄"]
		_:
			return []

func recruit_unit(town_data: Dictionary, unit_type: String, player_id: int) -> bool:
	if not can_recruit_unit(town_data, unit_type, player_id):
		return false
	
	var unit_data = get_unit_type_data(unit_type)
	
	# 扣除資源
	if not spend_resources(player_id, unit_data.cost):
		return false
	
	# 創建新單位（這裡返回單位數據，實際創建由WorldMap處理）
	var new_unit = {
		"position": town_data.position,
		"player_id": player_id,
		"type": unit_type,
		"max_hp": unit_data.stats.hp,
		"current_hp": unit_data.stats.hp,
		"attack": unit_data.stats.attack,
		"max_movement": unit_data.stats.movement,
		"current_movement": unit_data.stats.movement,
		"defense": unit_data.stats.defense,
		"has_moved": false
	}
	
	print("成功招募", unit_type, "消耗資源:", unit_data.cost)
	return true

func can_build_building(town_data: Dictionary, building_type: String, player_id: int) -> bool:
	# 檢查建築是否已存在
	if building_type in town_data.buildings:
		return false
	
	# 檢查資源
	var building_data = get_building_type_data(building_type)
	if building_data.is_empty():
		return false
	
	return can_afford(player_id, building_data.cost)

func build_building(town_data: Dictionary, building_type: String, player_id: int) -> bool:
	if not can_build_building(town_data, building_type, player_id):
		return false
	
	var building_data = get_building_type_data(building_type)
	
	# 扣除資源
	if not spend_resources(player_id, building_data.cost):
		return false
	
	# 添加建築
	town_data.buildings.append(building_type)
	
	# 應用建築效果
	apply_building_effects(town_data, building_type)
	
	print("成功建造", building_type, "消耗資源:", building_data.cost)
	return true

func apply_building_effects(town_data: Dictionary, building_type: String):
	var building_data = get_building_type_data(building_type)
	var effects = building_data.get("effects", {})
	
	for effect_type in effects.keys():
		match effect_type:
			"gold_bonus":
				town_data.production += effects[effect_type]
			"food_bonus":
				# 這裡可以添加食物產量邏輯
				pass
			"defense_bonus":
				# 這裡可以添加防御加成邏輯
				pass

func process_turn_end(player_id: int, towns: Array):
	# 處理資源產出
	for town in towns:
		if town.player_id == player_id:
			# 基礎資源產出
			var gold_income = town.production
			var food_income = town.population / 100
			
			add_resources(player_id, {
				"gold": gold_income,
				"food": food_income
			})
	
	# 增加回合數（只在最後一個玩家回合結束時）
	if player_id == game_data.max_players:
		game_data.turn += 1

func get_game_state():
	return game_data.duplicate()

func check_victory_condition(towns: Array, units: Array) -> int:
	# 簡單的勝利條件：消滅所有敵方單位和城鎮
	var players_alive = {}
	
	# 檢查城鎮
	for town in towns:
		players_alive[town.player_id] = true
	
	# 檢查單位
	for unit in units:
		players_alive[unit.player_id] = true
	
	var alive_count = players_alive.keys().size()
	
	if alive_count == 1:
		return players_alive.keys()[0]  # 返回獲勝玩家ID
	
	return 0  # 遊戲繼續

func save_game(file_path: String):
	var save_data = {
		"game_data": game_data,
		"timestamp": Time.get_unix_time_from_system()
	}
	
	var file = FileAccess.open(file_path, FileAccess.WRITE)
	if file:
		file.store_string(JSON.stringify(save_data))
		file.close()
		print("遊戲已保存到:", file_path)
	else:
		print("保存失敗!")

func load_game(file_path: String) -> bool:
	var file = FileAccess.open(file_path, FileAccess.READ)
	if not file:
		print("無法打開保存文件:", file_path)
		return false
	
	var json_string = file.get_as_text()
	file.close()
	
	var json = JSON.new()
	var parse_result = json.parse(json_string)
	
	if parse_result != OK:
		print("保存文件格式錯誤")
		return false
	
	var save_data = json.data
	game_data = save_data.get("game_data", {})
	
	print("遊戲已加載")
	return true