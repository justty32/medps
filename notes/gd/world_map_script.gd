# WorldMap.gd - 世界地圖管理腳本
extends Node2D

signal tile_clicked(pos: Vector2i)
signal unit_selected(unit)
signal town_selected(town)

# 地圖尺寸
const MAP_WIDTH = 20
const MAP_HEIGHT = 15
const TILE_SIZE = 64

# 地形類型
enum TerrainType {
	GRASS,
	FOREST,
	MOUNTAIN,
	WATER,
	DESERT
}

# 地圖數據
var map_data = []
var units = []
var towns = []

# 視覺組件
var terrain_sprites = []
var unit_sprites = []
var town_sprites = []

func _ready():
	# 設置地圖點擊檢測
	set_process_input(true)

func initialize_map():
	# 生成地形
	generate_terrain()
	
	# 創建初始城鎮
	create_initial_towns()
	
	# 創建初始單位
	create_initial_units()
	
	# 渲染地圖
	render_map()

func generate_terrain():
	map_data = []
	terrain_sprites = []
	
	for y in range(MAP_HEIGHT):
		var row = []
		var sprite_row = []
		
		for x in range(MAP_WIDTH):
			# 簡單的地形生成邏輯
			var terrain_type = generate_terrain_at_position(x, y)
			row.append(terrain_type)
			
			# 創建地形精靈
			var sprite = Sprite2D.new()
			sprite.texture = get_terrain_texture(terrain_type)
			sprite.position = Vector2(x * TILE_SIZE, y * TILE_SIZE)
			add_child(sprite)
			sprite_row.append(sprite)
			
		map_data.append(row)
		terrain_sprites.append(sprite_row)

func generate_terrain_at_position(x: int, y: int) -> TerrainType:
	# 簡單的程序化地形生成
	var noise_value = (sin(x * 0.1) + cos(y * 0.1)) * 0.5
	
	if noise_value > 0.3:
		return TerrainType.MOUNTAIN
	elif noise_value > 0.1:
		return TerrainType.FOREST
	elif noise_value < -0.3:
		return TerrainType.WATER
	elif noise_value < -0.1:
		return TerrainType.DESERT
	else:
		return TerrainType.GRASS

func get_terrain_texture(terrain_type: TerrainType) -> Texture2D:
	# 這裡應該返回實際的地形材質
	# 暫時使用顏色代替
	var image = Image.create(TILE_SIZE, TILE_SIZE, false, Image.FORMAT_RGB8)
	
	match terrain_type:
		TerrainType.GRASS:
			image.fill(Color.GREEN)
		TerrainType.FOREST:
			image.fill(Color.DARK_GREEN)
		TerrainType.MOUNTAIN:
			image.fill(Color.GRAY)
		TerrainType.WATER:
			image.fill(Color.BLUE)
		TerrainType.DESERT:
			image.fill(Color.YELLOW)
	
	var texture = ImageTexture.new()
	texture.create_from_image(image)
	return texture

func create_initial_towns():
	towns = []
	town_sprites = []
	
	# 創建幾個初始城鎮
	var town_positions = [
		Vector2i(3, 3),
		Vector2i(16, 11),
		Vector2i(8, 7),
		Vector2i(12, 4)
	]
	
	for i in range(town_positions.size()):
		var town_data = {
			"id": i,
			"position": town_positions[i],
			"name": "城鎮" + str(i + 1),
			"player_id": 1 if i < 2 else 2,
			"population": 1000,
			"buildings": ["城堡"],
			"production": 10
		}
		
		towns.append(town_data)
		create_town_sprite(town_data)

func create_town_sprite(town_data):
	var sprite = Sprite2D.new()
	var image = Image.create(TILE_SIZE, TILE_SIZE, false, Image.FORMAT_RGB8)
	
	# 根據玩家設置城鎮顏色
	var color = Color.RED if town_data.player_id == 1 else Color.BLUE
	image.fill(color)
	
	var texture = ImageTexture.new()
	texture.create_from_image(image)
	sprite.texture = texture
	
	sprite.position = Vector2(town_data.position.x * TILE_SIZE, town_data.position.y * TILE_SIZE)
	add_child(sprite)
	town_sprites.append(sprite)

func create_initial_units():
	units = []
	unit_sprites = []
	
	# 為每個玩家創建初始單位
	var unit_positions_p1 = [Vector2i(2, 3), Vector2i(4, 3)]
	var unit_positions_p2 = [Vector2i(15, 11), Vector2i(17, 11)]
	
	# 玩家1的單位
	for pos in unit_positions_p1:
		create_unit(pos, 1, "步兵", 100, 20, 2)
	
	# 玩家2的單位
	for pos in unit_positions_p2:
		create_unit(pos, 2, "步兵", 100, 20, 2)

func create_unit(pos: Vector2i, player_id: int, unit_type: String, hp: int, attack: int, movement: int):
	var unit_data = {
		"id": units.size(),
		"position": pos,
		"player_id": player_id,
		"type": unit_type,
		"max_hp": hp,
		"current_hp": hp,
		"attack": attack,
		"max_movement": movement,
		"current_movement": movement,
		"has_moved": false
	}
	
	units.append(unit_data)
	create_unit_sprite(unit_data)

func create_unit_sprite(unit_data):
	var sprite = Sprite2D.new()
	var image = Image.create(TILE_SIZE - 10, TILE_SIZE - 10, false, Image.FORMAT_RGB8)
	
	# 根據玩家設置單位顏色
	var color = Color.DARK_RED if unit_data.player_id == 1 else Color.DARK_BLUE
	image.fill(color)
	
	var texture = ImageTexture.new()
	texture.create_from_image(image)
	sprite.texture = texture
	
	sprite.position = Vector2(unit_data.position.x * TILE_SIZE + 5, unit_data.position.y * TILE_SIZE + 5)
	add_child(sprite)
	unit_sprites.append(sprite)

func render_map():
	# 地圖已在generate_terrain()中渲染
	pass

func _input(event):
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var click_pos = event.position
			var tile_pos = Vector2i(int(click_pos.x / TILE_SIZE), int(click_pos.y / TILE_SIZE))
			
			if is_valid_position(tile_pos):
				tile_clicked.emit(tile_pos)

func is_valid_position(pos: Vector2i) -> bool:
	return pos.x >= 0 and pos.x < MAP_WIDTH and pos.y >= 0 and pos.y < MAP_HEIGHT

func get_unit_at_position(pos: Vector2i):
	for unit in units:
		if unit.position == pos:
			return unit
	return null

func get_town_at_position(pos: Vector2i):
	for town in towns:
		if town.position == pos:
			return town
	return null

func is_valid_move(unit, target_pos: Vector2i) -> bool:
	if not is_valid_position(target_pos):
		return false
	
	if unit.has_moved or unit.current_movement <= 0:
		return false
	
	# 檢查目標位置是否被占用
	if get_unit_at_position(target_pos) != null:
		return false
	
	# 檢查移動距離
	var distance = abs(target_pos.x - unit.position.x) + abs(target_pos.y - unit.position.y)
	return distance <= unit.current_movement

func move_unit(unit, target_pos: Vector2i):
	if not is_valid_move(unit, target_pos):
		return false
	
	# 計算移動消耗
	var distance = abs(target_pos.x - unit.position.x) + abs(target_pos.y - unit.position.y)
	
	# 更新單位位置
	unit.position = target_pos
	unit.current_movement -= distance
	unit.has_moved = true
	
	# 更新視覺位置
	var unit_index = units.find(unit)
	if unit_index >= 0:
		unit_sprites[unit_index].position = Vector2(target_pos.x * TILE_SIZE + 5, target_pos.y * TILE_SIZE + 5)
	
	return true

func reset_units_for_player(player_id: int):
	for unit in units:
		if unit.player_id == player_id:
			unit.current_movement = unit.max_movement
			unit.has_moved = false

func get_terrain_at_position(pos: Vector2i) -> TerrainType:
	if is_valid_position(pos):
		return map_data[pos.y][pos.x]
	return TerrainType.GRASS