# LoadGame.gd - 載入遊戲場景腳本
extends Control

# 場景路徑
const MAIN_MENU_SCENE = "res://scenes/MainMenu.tscn"
const GAME_SCENE = "res://scenes/Main.tscn"

# 存檔文件路徑
const SAVE_DIR = "user://saves/"
const SAVE_EXTENSION = ".save"

# UI組件引用
@onready var save_list = $VBoxContainer/SaveList
@onready var load_button = $VBoxContainer/ButtonContainer/LoadButton
@onready var delete_button = $VBoxContainer/ButtonContainer/DeleteButton
@onready var back_button = $VBoxContainer/ButtonContainer/BackButton

# 存檔數據
var save_files = []
var selected_save_index = -1

# 存檔項目場景
var save_item_scene = preload("res://scenes/SaveItem.tscn") if ResourceLoader.exists("res://scenes/SaveItem.tscn") else null

func _ready():
	setup_ui()
	scan_save_files()
	connect_signals()
	apply_entrance_animation()

func setup_ui():
	# 設置背景
	var background = ColorRect.new()
	background.color = Color(0.1, 0.1, 0.2)
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(background)
	move_child(background, 0)
	
	# 主容器
	var main_container = VBoxContainer.new()
	main_container.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	main_container.custom_minimum_size = Vector2(700, 600)
	add_child(main_container)
	
	# 標題
	var title = Label.new()
	title.text = "載入遊戲"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 32)
	main_container.add_child(title)
	
	# 添加間距
	var spacer1 = Control.new()
	spacer1.custom_minimum_size = Vector2(0, 20)
	main_container.add_child(spacer1)
	
	# 存檔列表容器
	var list_container = Panel.new()
	list_container.custom_minimum_size = Vector2(680, 400)
	main_container.add_child(list_container)
	
	# 滾動容器
	var scroll_container = ScrollContainer.new()
	scroll_container.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	scroll_container.add_theme_constant_override("margin_left", 10)
	scroll_container.add_theme_constant_override("margin_right", 10)
	scroll_container.add_theme_constant_override("margin_top", 10)
	scroll_container.add_theme_constant_override("margin_bottom", 10)
	list_container.add_child(scroll_container)
	
	# 存檔列表
	save_list = VBoxContainer.new()
	save_list.custom_minimum_size = Vector2(640, 0)
	scroll_container.add_child(save_list)
	
	# 添加間距
	var spacer2 = Control.new()
	spacer2.custom_minimum_size = Vector2(0, 20)
	main_container.add_child(spacer2)
	
	# 按鈕容器
	var button_container = HBoxContainer.new()
	button_container.alignment = BoxContainer.ALIGNMENT_CENTER
	main_container.add_child(button_container)
	
	# 創建按鈕
	load_button = create_button("載入遊戲")
	delete_button = create_button("刪除存檔")
	back_button = create_button("返回")
	
	load_button.disabled = true
	delete_button.disabled = true
	
	button_container.add_child(load_button)
	button_container.add_child(delete_button)
	button_container.add_child(back_button)

func create_button(text: String) -> Button:
	var button = Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(120, 40)
	
	# 設置按鈕樣式
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.2, 0.3, 0.5, 0.8)
	style.corner_radius_top_left = 5
	style.corner_radius_top_right = 5
	style.corner_radius_bottom_left = 5
	style.corner_radius_bottom_right = 5
	button.add_theme_stylebox_override("normal", style)
	
	return button

func scan_save_files():
	save_files.clear()
	
	# 創建存檔目錄（如果不存在）
	if not DirAccess.dir_exists_absolute(SAVE_DIR):
		DirAccess.open("user://").make_dir_recursive("saves")
	
	# 掃描存檔文件
	var dir = DirAccess.open(SAVE_DIR)
	if dir:
		dir.list_dir_begin()
		var file_name = dir.get_next()
		
		while file_name != "":
			if file_name.ends_with(SAVE_EXTENSION):
				var save_data = load_save_file(SAVE_DIR + file_name)
				if save_data:
					save_data["file_path"] = SAVE_DIR + file_name
					save_data["file_name"] = file_name
					save_files.append(save_data)
			
			file_name = dir.get_next()
	
	# 按時間排序（最新的在上面）
	save_files.sort_custom(func(a, b): return a["timestamp"] > b["timestamp"])
	
	# 更新UI列表
	update_save_list()

func load_save_file(file_path: String) -> Dictionary:
	var file = FileAccess.open(file_path, FileAccess.READ)
	if not file:
		return {}
	
	var json_string = file.get_as_text()
	file.close()
	
	var json = JSON.new()
	var parse_result = json.parse(json_string)
	
	if parse_result != OK:
		print("存檔文件格式錯誤: ", file_path)
		return {}
	
	return json.data

func update_save_list():
	# 清空現有列表
	for child in save_list.get_children():
		child.queue_free()
	
	if save_files.is_empty():
		# 顯示無存檔訊息
		var no_saves_label = Label.new()
		no_saves_label.text = "沒有找到存檔文件"
		no_saves_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		no_saves_label.add_theme_font_size_override("font_size", 18)
		no_saves_label.add_theme_color_override("font_color", Color.GRAY)
		save_list.add_child(no_saves_label)
		return
	
	# 創建存檔項目
	for i in range(save_files.size()):
		var save_data = save_files[i]
		var save_item = create_save_item(save_data, i)
		save_list.add_child(save_item)

func create_save_item(save_data: Dictionary, index: int) -> Control:
	var item_container = Panel.new()
	item_container.custom_minimum_size = Vector2(620, 80)
	item_container.name = "SaveItem_" + str(index)
	
	# 設置選中效果
	var normal_style = StyleBoxFlat.new()
	normal_style.bg_color = Color(0.15, 0.15, 0.25, 0.8)
	normal_style.border_width_left = 2
	normal_style.border_width_right = 2
	normal_style.border_width_top = 2
	normal_style.border_width_bottom = 2
	normal_style.border_color = Color(0.3, 0.3, 0.4)
	item_container.add_theme_stylebox_override("panel", normal_style)
	
	# 內容容器
	var content = HBoxContainer.new()
	content.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	content.add_theme_constant_override("margin_left", 15)
	content.add_theme_constant_override("margin_right", 15)
	content.add_theme_constant_override("margin_top", 10)
	content.add_theme_constant_override("margin_bottom", 10)
	item_container.add_child(content)
	
	# 存檔信息
	var info_container = VBoxContainer.new()
	content.add_child(info_container)
	
	# 存檔名稱
	var save_name = Label.new()
	save_name.text = save_data.get("save_name", "未命名存檔")
	save_name.add_theme_font_size_override("font_size", 16)
	info_container.add_child(save_name)
	
	# 遊戲信息
	var game_info = Label.new()
	var turn_text = "回合: " + str(save_data.get("turn", 1))
	var player_text = "玩家: " + str(save_data.get("current_player", 1))
	game_info.text = turn_text + " | " + player_text
	game_info.add_theme_font_size_override("font_size", 12)
	game_info.add_theme_color_override("font_color", Color.LIGHT_GRAY)
	info_container.add_child(game_info)
	
	# 時間信息
	var time_info = Label.new()
	var timestamp = save_data.get("timestamp", 0)
	var datetime = Time.get_datetime_dict_from_unix_time(timestamp)
	time_info.text = "%04d/%02d/%02d %02d:%02d" % [
		datetime.year, datetime.month, datetime.day,
		datetime.hour, datetime.minute
	]
	time_info.add_theme_font_size_override("font_size", 12)
	time_info.add_theme_color_override("font_color", Color.YELLOW)
	info_container.add_child(time_info)
	
	# 添加間距
	var spacer = Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	content.add_child(spacer)
	
	# 存檔預覽圖（佔位符）
	var preview = ColorRect.new()
	preview.color = Color(0.2, 0.2, 0.3)
	preview.custom_minimum_size = Vector2(60, 60)
	content.add_child(preview)
	
	# 添加點擊事件
	var click_detector = Button.new()
	click_detector.flat = true
	click_detector.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	click_detector.pressed.connect(_on_save_item_selected.bind(index))
	item_container.add_child(click_detector)
	
	return item_container

func connect_signals():
	load_button.pressed.connect(_on_load_pressed)
	delete_button.pressed.connect(_on_delete_pressed)
	back_button.pressed.connect(_on_back_pressed)

func apply_entrance_animation():
	modulate.a = 0.0
	var tween = create_tween()
	tween.tween_property(self, "modulate:a", 1.0, 0.5)

func _on_save_item_selected(index: int):
	# 更新選中狀態
	update_selection(index)
	
	# 啟用按鈕
	load_button.disabled = false
	delete_button.disabled = false

func update_selection(index: int):
	selected_save_index = index
	
	# 更新視覺選中效果
	for i in range(save_list.get_child_count()):
		var item = save_list.get_child(i)
		if item.has_method("add_theme_stylebox_override"):
			var style = StyleBoxFlat.new()
			if i == index:
				# 選中樣式
				style.bg_color = Color(0.3, 0.4, 0.6, 0.9)
				style.border_color = Color(0.5, 0.6, 0.8)
			else:
				# 普通樣式
				style.bg_color = Color(0.15, 0.15, 0.25, 0.8)
				style.border_color = Color(0.3, 0.3, 0.4)
			
			style.border_width_left = 2
			style.border_width_right = 2
			style.border_width_top = 2
			style.border_width_bottom = 2
			item.add_theme_stylebox_override("panel", style)

func _on_load_pressed():
	if selected_save_index >= 0 and selected_save_index < save_files.size():
		var save_data = save_files[selected_save_index]
		print("載入存檔: ", save_data.get("save_name", "未命名"))
		
		# 設置全域存檔數據供遊戲場景使用
		GameGlobals.loaded_save_data = save_data
		
		# 切換到遊戲場景
		transition_to_scene(GAME_SCENE)

func _on_delete_pressed():
	if selected_save_index >= 0 and selected_save_index < save_files.size():
		var save_data = save_files[selected_save_index]
		show_delete_confirmation(save_data)

func show_delete_confirmation(save_data: Dictionary):
	var dialog = ConfirmationDialog.new()
	dialog.title = "刪除存檔"
	dialog.dialog_text = "確定要刪除存檔 \"" + save_data.get("save_name", "未命名") + "\" 嗎？\n此操作無法復原。"
	add_child(dialog)
	dialog.popup_centered()
	dialog.confirmed.connect(_on_delete_confirmed.bind(dialog, save_data))
	dialog.canceled.connect(dialog.queue_free)

func _on_delete_confirmed(dialog, save_data: Dictionary):
	var file_path = save_data.get("file_path", "")
	if file_path != "":
		var file = FileAccess.open(file_path, FileAccess.READ)
		if file:
			file.close()
			DirAccess.remove_absolute(file_path)
			print("已刪除存檔: ", file_path)
			
			# 重新掃描存檔
			scan_save_files()
			
			# 重置選擇
			selected_save_index = -1
			load_button.disabled = true
			delete_button.disabled = true
	
	dialog.queue_free()

func _on_back_pressed():
	transition_to_scene(MAIN_MENU_SCENE)

func transition_to_scene(scene_path: String):
	var tween = create_tween()
	tween.tween_property(self, "modulate:a", 0.0, 0.3)
	tween.tween_callback(_change_scene.bind(scene_path))

func _change_scene(scene_path: String):
	if ResourceLoader.exists(scene_path):
		get_tree().change_scene_to_file(scene_path)
	else:
		print("場景文件不存在: ", scene_path)

func _input(event):
	if event.is_action_pressed("ui_cancel"):
		_on_back_pressed()
	elif event.is_action_pressed("ui_accept") and not load_button.disabled:
		_on_load_pressed()

# 全域存檔數據管理器（單例）
class_name GameGlobals
extends RefCounted

static var loaded_save_data: Dictionary = {}