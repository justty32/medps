# UIManager.gd - UI管理腳本
extends CanvasLayer

signal end_turn_pressed
signal town_button_pressed

# UI組件引用
@onready var info_panel = $InfoPanel
@onready var unit_info = $InfoPanel/UnitInfo
@onready var town_info = $InfoPanel/TownInfo
@onready var game_ui = $GameUI
@onready var current_player_label = $GameUI/PlayerInfo/CurrentPlayer
@onready var end_turn_button = $GameUI/Controls/EndTurnButton
@onready var town_management = $TownManagement

var current_selected_unit = null
var current_selected_town = null

func _ready():
	setup_ui()
	connect_signals()

func setup_ui():
	# 設置基本UI佈局
	create_info_panel()
	create_game_ui()
	create_town_management()
	
	# 初始化顯示
	hide_selection_info()
	update_current_player(1)

func create_info_panel():
	# 創建信息面板
	info_panel = Panel.new()
	info_panel.size = Vector2(300, 400)
	info_panel.position = Vector2(10, 10)
	info_panel.visible = false
	add_child(info_panel)
	
	# 單位信息容器
	unit_info = VBoxContainer.new()
	unit_info.position = Vector2(10, 10)
	info_panel.add_child(unit_info)
	
	# 城鎮信息容器
	town_info = VBoxContainer.new()
	town_info.position = Vector2(10, 10)
	town_info.visible = false
	info_panel.add_child(town_info)

func create_game_ui():
	# 創建遊戲UI
	game_ui = Control.new()
	add_child(game_ui)
	
	# 玩家信息面板
	var player_info = Panel.new()
	player_info.size = Vector2(200, 60)
	player_info.position = Vector2(get_viewport().size.x - 220, 10)
	game_ui.add_child(player_info)
	
	# 當前玩家標籤
	current_player_label = Label.new()
	current_player_label.text = "當前玩家: 1"
	current_player_label.position = Vector2(10, 10)
	player_info.add_child(current_player_label)
	
	# 控制面板
	var controls = Panel.new()
	controls.size = Vector2(200, 100)
	controls.position = Vector2(get_viewport().size.x - 220, 80)
	game_ui.add_child(controls)
	
	# 結束回合按鈕
	end_turn_button = Button.new()
	end_turn_button.text = "結束回合"
	end_turn_button.size = Vector2(180, 40)
	end_turn_button.position = Vector2(10, 10)
	controls.add_child(end_turn_button)
	
	# 城鎮管理按鈕
	var town_button = Button.new()
	town_button.text = "管理城鎮"
	town_button.size = Vector2(180, 40)
	town_button.position = Vector2(10, 55)
	town_button.pressed.connect(_on_town_button_pressed)
	controls.add_child(town_button)

func create_town_management():
	# 創建城鎮管理界面
	town_management = Panel.new()
	town_management.size = Vector2(600, 400)
	town_management.position = Vector2(200, 100)
	town_management.visible = false
	add_child(town_management)
	
	# 標題
	var title = Label.new()
	title.text = "城鎮管理"
	title.position = Vector2(10, 10)
	town_management.add_child(title)
	
	# 關閉按鈕
	var close_button = Button.new()
	close_button.text = "關閉"
	close_button.size = Vector2(80, 30)
	close_button.position = Vector2(510, 10)
	close_button.pressed.connect(_on_close_town_management)
	town_management.add_child(close_button)

func connect_signals():
	end_turn_button.pressed.connect(_on_end_turn_pressed)

func show_unit_info(unit):
	current_selected_unit = unit
	current_selected_town = null
	
	# 清空之前的信息
	for child in unit_info.get_children():
		child.queue_free()
	
	# 添加單位信息
	var name_label = Label.new()
	name_label.text = "類型: " + unit.type
	unit_info.add_child(name_label)
	
	var hp_label = Label.new()
	hp_label.text = "生命值: %d/%d" % [unit.current_hp, unit.max_hp]
	unit_info.add_child(hp_label)
	
	var attack_label = Label.new()
	attack_label.text = "攻擊力: " + str(unit.attack)
	unit_info.add_child(attack_label)
	
	var movement_label = Label.new()
	movement_label.text = "移動力: %d/%d" % [unit.current_movement, unit.max_movement]
	unit_info.add_child(movement_label)
	
	var position_label = Label.new()
	position_label.text = "位置: (%d, %d)" % [unit.position.x, unit.position.y]
	unit_info.add_child(position_label)
	
	# 顯示面板
	unit_info.visible = true
	town_info.visible = false
	info_panel.visible = true

func show_town_info(town):
	current_selected_town = town
	current_selected_unit = null
	
	# 清空之前的信息
	for child in town_info.get_children():
		child.queue_free()
	
	# 添加城鎮信息
	var name_label = Label.new()
	name_label.text = "城鎮: " + town.name
	town_info.add_child(name_label)
	
	var owner_label = Label.new()
	owner_label.text = "所有者: 玩家 " + str(town.player_id)
	town_info.add_child(owner_label)
	
	var population_label = Label.new()
	population_label.text = "人口: " + str(town.population)
	town_info.add_child(population_label)
	
	var production_label = Label.new()
	production_label.text = "產量: " + str(town.production)
	town_info.add_child(production_label)
	
	var position_label = Label.new()
	position_label.text = "位置: (%d, %d)" % [town.position.x, town.position.y]
	town_info.add_child(position_label)
	
	# 建築列表
	var buildings_label = Label.new()
	buildings_label.text = "建築: " + ", ".join(town.buildings)
	town_info.add_child(buildings_label)
	
	# 顯示面板
	unit_info.visible = false
	town_info.visible = true
	info_panel.visible = true

func hide_selection_info():
	info_panel.visible = false
	current_selected_unit = null
	current_selected_town = null

func show_town_management(town):
	# 顯示城鎮管理界面
	town_management.visible = true
	
	# 更新城鎮管理內容
	update_town_management_content(town)

func update_town_management_content(town):
	# 清除舊內容（除了標題和關閉按鈕）
	for child in town_management.get_children():
		if child.name not in ["Title", "CloseButton"]:
			child.queue_free()
	
	# 城鎮詳細信息
	var info_container = VBoxContainer.new()
	info_container.position = Vector2(20, 50)
	town_management.add_child(info_container)
	
	var info_text = [
		"城鎮名稱: " + town.name,
		"人口: " + str(town.population),
		"產量: " + str(town.production),
		"建築: " + ", ".join(town.buildings)
	]
	
	for text in info_text:
		var label = Label.new()
		label.text = text
		info_container.add_child(label)
	
	# 建築按鈕
	var building_container = HBoxContainer.new()
	building_container.position = Vector2(20, 200)
	town_management.add_child(building_container)
	
	var build_barracks = Button.new()
	build_barracks.text = "建造兵營"
	build_barracks.size = Vector2(120, 40)
	build_barracks.pressed.connect(_on_build_barracks.bind(town))
	building_container.add_child(build_barracks)
	
	var build_market = Button.new()
	build_market.text = "建造市場"
	build_market.size = Vector2(120, 40)
	build_market.pressed.connect(_on_build_market.bind(town))
	building_container.add_child(build_market)
	
	# 招募單位按鈕
	var recruit_container = HBoxContainer.new()
	recruit_container.position = Vector2(20, 260)
	town_management.add_child(recruit_container)
	
	var recruit_infantry = Button.new()
	recruit_infantry.text = "招募步兵"
	recruit_infantry.size = Vector2(120, 40)
	recruit_infantry.pressed.connect(_on_recruit_infantry.bind(town))
	recruit_container.add_child(recruit_infantry)

func update_current_player(player_id: int):
	current_player_label.text = "當前玩家: " + str(player_id)

func _on_end_turn_pressed():
	end_turn_pressed.emit()

func _on_town_button_pressed():
	if current_selected_town:
		town_button_pressed.emit()

func _on_close_town_management():
	town_management.visible = false

func _on_build_barracks(town):
	if "兵營" not in town.buildings:
		town.buildings.append("兵營")
		print("在", town.name, "建造了兵營")
		update_town_management_content(town)

func _on_build_market(town):
	if "市場" not in town.buildings:
		town.buildings.append("市場")
		town.production += 5
		print("在", town.name, "建造了市場，產量增加")
		update_town_management_content(town)

func _on_recruit_infantry(town):
	if "兵營" in town.buildings:
		print("在", town.name, "招募了步兵")
		# 這裡應該調用主遊戲的招募邏輯
	else:
		print("需要先建造兵營才能招募步兵")