# MainMenu.gd - 主選單場景腳本
extends Control

# 場景路徑
const GAME_SCENE = "res://scenes/Main.tscn"
const SETTINGS_SCENE = "res://scenes/Settings.tscn"
const LOAD_GAME_SCENE = "res://scenes/LoadGame.tscn"

# UI組件引用
@onready var title_label = $VBoxContainer/TitleLabel
@onready var menu_buttons = $VBoxContainer/MenuButtons
@onready var start_button = $VBoxContainer/MenuButtons/StartButton
@onready var load_button = $VBoxContainer/MenuButtons/LoadButton
@onready var settings_button = $VBoxContainer/MenuButtons/SettingsButton
@onready var quit_button = $VBoxContainer/MenuButtons/QuitButton

# 背景和特效
@onready var background = $Background
@onready var particle_system = $ParticleSystem

func _ready():
	setup_ui()
	setup_background()
	connect_signals()
	
	# 播放進入動畫
	play_entrance_animation()

func setup_ui():
	# 設置主容器
	var main_container = VBoxContainer.new()
	main_container.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	main_container.custom_minimum_size = Vector2(400, 500)
	add_child(main_container)
	
	# 遊戲標題
	title_label = Label.new()
	title_label.text = "策略帝國"
	title_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title_label.add_theme_font_size_override("font_size", 48)
	main_container.add_child(title_label)
	
	# 添加間距
	var spacer1 = Control.new()
	spacer1.custom_minimum_size = Vector2(0, 50)
	main_container.add_child(spacer1)
	
	# 按鈕容器
	menu_buttons = VBoxContainer.new()
	menu_buttons.set_anchors_and_offsets_preset(Control.PRESET_CENTER_LEFT)
	main_container.add_child(menu_buttons)
	
	# 創建選單按鈕
	create_menu_buttons()

func create_menu_buttons():
	var button_style = create_button_style()
	var button_size = Vector2(250, 50)
	var button_spacing = 15
	
	# 開始遊戲按鈕
	start_button = create_styled_button("開始遊戲", button_size, button_style)
	menu_buttons.add_child(start_button)
	
	# 載入遊戲按鈕
	load_button = create_styled_button("載入遊戲", button_size, button_style)
	menu_buttons.add_child(load_button)
	
	# 設置按鈕
	settings_button = create_styled_button("設置", button_size, button_style)
	menu_buttons.add_child(settings_button)
	
	# 離開按鈕
	quit_button = create_styled_button("離開", button_size, button_style)
	menu_buttons.add_child(quit_button)
	
	# 設置按鈕間距
	for i in range(menu_buttons.get_child_count() - 1):
		var spacer = Control.new()
		spacer.custom_minimum_size = Vector2(0, button_spacing)
		menu_buttons.add_child(spacer)

func create_styled_button(text: String, size: Vector2, style: StyleBox) -> Button:
	var button = Button.new()
	button.text = text
	button.custom_minimum_size = size
	button.add_theme_font_size_override("font_size", 20)
	
	# 設置按鈕樣式
	button.add_theme_stylebox_override("normal", style)
	button.add_theme_stylebox_override("hover", create_hover_style())
	button.add_theme_stylebox_override("pressed", create_pressed_style())
	
	return button

func create_button_style() -> StyleBoxFlat:
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.2, 0.3, 0.5, 0.8)
	style.border_width_left = 2
	style.border_width_right = 2
	style.border_width_top = 2
	style.border_width_bottom = 2
	style.border_color = Color(0.4, 0.5, 0.7)
	style.corner_radius_top_left = 8
	style.corner_radius_top_right = 8
	style.corner_radius_bottom_left = 8
	style.corner_radius_bottom_right = 8
	return style

func create_hover_style() -> StyleBoxFlat:
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.3, 0.4, 0.6, 0.9)
	style.border_width_left = 2
	style.border_width_right = 2
	style.border_width_top = 2
	style.border_width_bottom = 2
	style.border_color = Color(0.5, 0.6, 0.8)
	style.corner_radius_top_left = 8
	style.corner_radius_top_right = 8
	style.corner_radius_bottom_left = 8
	style.corner_radius_bottom_right = 8
	return style

func create_pressed_style() -> StyleBoxFlat:
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.1, 0.2, 0.4, 0.9)
	style.border_width_left = 2
	style.border_width_right = 2
	style.border_width_top = 2
	style.border_width_bottom = 2
	style.border_color = Color(0.3, 0.4, 0.6)
	style.corner_radius_top_left = 8
	style.corner_radius_top_right = 8
	style.corner_radius_bottom_left = 8
	style.corner_radius_bottom_right = 8
	return style

func setup_background():
	# 創建背景
	background = ColorRect.new()
	background.color = Color(0.1, 0.1, 0.2)
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(background)
	move_child(background, 0)  # 移到最底層
	
	# 添加背景圖案
	create_background_pattern()

func create_background_pattern():
	# 創建背景裝飾圖案
	for i in range(20):
		var decoration = ColorRect.new()
		decoration.color = Color(0.15, 0.15, 0.3, 0.3)
		decoration.size = Vector2(randf_range(50, 100), randf_range(50, 100))
		decoration.position = Vector2(
			randf_range(0, get_viewport().size.x),
			randf_range(0, get_viewport().size.y)
		)
		background.add_child(decoration)

func connect_signals():
	start_button.pressed.connect(_on_start_button_pressed)
	load_button.pressed.connect(_on_load_button_pressed)
	settings_button.pressed.connect(_on_settings_button_pressed)
	quit_button.pressed.connect(_on_quit_button_pressed)
	
	# 添加懸停效果
	start_button.mouse_entered.connect(_on_button_hover.bind(start_button))
	load_button.mouse_entered.connect(_on_button_hover.bind(load_button))
	settings_button.mouse_entered.connect(_on_button_hover.bind(settings_button))
	quit_button.mouse_entered.connect(_on_button_hover.bind(quit_button))

func play_entrance_animation():
	# 標題進入動畫
	title_label.modulate.a = 0.0
	title_label.scale = Vector2(0.5, 0.5)
	
	var title_tween = create_tween()
	title_tween.parallel().tween_property(title_label, "modulate:a", 1.0, 1.0)
	title_tween.parallel().tween_property(title_label, "scale", Vector2(1.0, 1.0), 1.0)
	title_tween.set_trans(Tween.TRANS_BACK)
	title_tween.set_ease(Tween.EASE_OUT)
	
	# 按鈕逐個出現動畫
	var buttons = [start_button, load_button, settings_button, quit_button]
	for i in range(buttons.size()):
		var button = buttons[i]
		button.modulate.a = 0.0
		button.position.x += 100
		
		var button_tween = create_tween()
		button_tween.tween_delay(0.5 + i * 0.2)
		button_tween.parallel().tween_property(button, "modulate:a", 1.0, 0.5)
		button_tween.parallel().tween_property(button, "position:x", button.position.x - 100, 0.5)
		button_tween.set_trans(Tween.TRANS_CUBIC)
		button_tween.set_ease(Tween.EASE_OUT)

func _on_button_hover(button: Button):
	# 懸停動畫效果
	var hover_tween = create_tween()
	hover_tween.tween_property(button, "scale", Vector2(1.05, 1.05), 0.1)
	hover_tween.tween_property(button, "scale", Vector2(1.0, 1.0), 0.1)

func _on_start_button_pressed():
	print("開始新遊戲")
	transition_to_scene(GAME_SCENE)

func _on_load_button_pressed():
	print("載入遊戲")
	transition_to_scene(LOAD_GAME_SCENE)

func _on_settings_button_pressed():
	print("開啟設置")
	transition_to_scene(SETTINGS_SCENE)

func _on_quit_button_pressed():
	print("離開遊戲")
	show_quit_confirmation()

func show_quit_confirmation():
	# 創建確認對話框
	var dialog = AcceptDialog.new()
	dialog.title = "確認離開"
	dialog.dialog_text = "確定要離開遊戲嗎？"
	dialog.ok_button_text = "確定"
	dialog.add_cancel_button("取消")
	
	add_child(dialog)
	dialog.popup_centered()
	
	# 連接信號
	dialog.confirmed.connect(_on_quit_confirmed)
	dialog.canceled.connect(_on_quit_canceled.bind(dialog))

func _on_quit_confirmed():
	# 淡出動畫然後退出
	var fade_tween = create_tween()
	fade_tween.tween_property(self, "modulate:a", 0.0, 0.5)
	fade_tween.tween_callback(get_tree().quit)

func _on_quit_canceled(dialog):
	dialog.queue_free()

func transition_to_scene(scene_path: String):
	# 場景切換動畫
	var transition_tween = create_tween()
	transition_tween.tween_property(self, "modulate:a", 0.0, 0.3)
	transition_tween.tween_callback(_change_scene.bind(scene_path))

func _change_scene(scene_path: String):
	# 檢查場景文件是否存在
	if ResourceLoader.exists(scene_path):
		get_tree().change_scene_to_file(scene_path)
	else:
		print("場景文件不存在: ", scene_path)
		# 顯示錯誤訊息
		show_error_message("場景文件不存在: " + scene_path)

func show_error_message(message: String):
	var error_dialog = AcceptDialog.new()
	error_dialog.title = "錯誤"
	error_dialog.dialog_text = message
	add_child(error_dialog)
	error_dialog.popup_centered()
	error_dialog.confirmed.connect(error_dialog.queue_free)

func _input(event):
	# 快捷鍵支持
	if event.is_action_pressed("ui_accept"):
		_on_start_button_pressed()
	elif event.is_action_pressed("ui_cancel"):
		_on_quit_button_pressed()