# Main.gd - 主遊戲場景腳本
extends Node2D

# 遊戲狀態枚舉
enum GameState {
	MAP_VIEW,
	TOWN_VIEW,
	UNIT_SELECTION,
	BATTLE
}

# 遊戲組件引用
@onready var world_map = $WorldMap
@onready var ui_manager = $UIManager
@onready var game_manager = $GameManager

var current_state = GameState.MAP_VIEW
var selected_unit = null
var selected_town = null
var current_player = 1
var max_players = 2

func _ready():
	# 初始化遊戲
	initialize_game()
	
func initialize_game():
	# 初始化世界地圖
	world_map.initialize_map()
	
	# 初始化UI
	ui_manager.setup_ui()
	
	# 連接信號
	connect_signals()
	
func connect_signals():
	# 連接地圖點擊事件
	world_map.tile_clicked.connect(_on_tile_clicked)
	world_map.unit_selected.connect(_on_unit_selected)
	world_map.town_selected.connect(_on_town_selected)
	
	# 連接UI事件
	ui_manager.end_turn_pressed.connect(_on_end_turn)
	ui_manager.town_button_pressed.connect(_on_town_button_pressed)

func _on_tile_clicked(pos: Vector2i):
	match current_state:
		GameState.MAP_VIEW:
			if selected_unit:
				# 移動選中的單位
				move_unit_to_position(selected_unit, pos)
			else:
				# 選擇該位置的單位或城鎮
				select_object_at_position(pos)
		
		GameState.UNIT_SELECTION:
			# 取消選擇
			deselect_all()

func _on_unit_selected(unit):
	selected_unit = unit
	current_state = GameState.UNIT_SELECTION
	ui_manager.show_unit_info(unit)

func _on_town_selected(town):
	selected_town = town
	ui_manager.show_town_info(town)

func _on_town_button_pressed():
	if selected_town:
		current_state = GameState.TOWN_VIEW
		ui_manager.show_town_management(selected_town)

func move_unit_to_position(unit, target_pos: Vector2i):
	if world_map.is_valid_move(unit, target_pos):
		world_map.move_unit(unit, target_pos)
		deselect_all()

func select_object_at_position(pos: Vector2i):
	var unit = world_map.get_unit_at_position(pos)
	var town = world_map.get_town_at_position(pos)
	
	if unit and unit.player_id == current_player:
		_on_unit_selected(unit)
	elif town:
		_on_town_selected(town)

func deselect_all():
	selected_unit = null
	selected_town = null
	current_state = GameState.MAP_VIEW
	ui_manager.hide_selection_info()

func _on_end_turn():
	# 結束回合邏輯
	end_current_turn()

func end_current_turn():
	# 重置單位移動次數
	world_map.reset_units_for_player(current_player)
	
	# 切換玩家
	current_player = current_player % max_players + 1
	
	# 更新UI
	ui_manager.update_current_player(current_player)
	
	# 清除選擇
	deselect_all()

func _input(event):
	if event.is_action_pressed("ui_cancel"):
		deselect_all()
	elif event.is_action_pressed("end_turn"):
		end_current_turn()