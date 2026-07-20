# Settings.gd - 設置場景腳本
extends Control

# 返回主選單的場景路徑
const MAIN_MENU_SCENE = "res://scenes/MainMenu.tscn"

# 設置數據
var settings_data = {
	"master_volume": 100,
	"music_volume": 80,
	"sfx_volume": 100,
	"screen_mode": 0,  # 0=視窗, 1=全螢幕
	"resolution": 0,   # 0=1280x720, 1=1920x1080, 2=2560x1440
	"vsync": true,
	"auto_save": true,
	"game_speed": 1,   # 0=慢, 1=正常, 2=快
	"show_grid": true,
	"show_tooltips": true
}

# UI組件引用
@onready var settings_container = $ScrollContainer/SettingsContainer
@onready var back_button = $BackButton
@onready var apply_button = $ApplyButton
@onready var reset_button = $ResetButton

# 控制項引用
var master_volume_slider: HSlider
var music_volume_slider: HSlider
var sfx_volume_slider: HSlider
var screen_mode_option: OptionButton
var resolution_option: OptionButton
var vsync_check: CheckBox
var auto_save_check: CheckBox
var game_speed_option: OptionButton
var show_grid_check: CheckBox
var show_tooltips_check: CheckBox

func _ready():
	setup_ui()
	load_settings()
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
	main_container.custom_minimum_size = Vector2(600, 500)
	add_child(main_container)
	
	# 標題
	var title = Label.new()
	title.text = "遊戲設置"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 32)
	main_container.add_child(title)
	
	# 添加間距
	var spacer = Control.new()
	spacer.custom_minimum_size = Vector2(0, 20)
	main_container.add_child(spacer)
	
	# 設置滾動容器
	var scroll_container = ScrollContainer.new()
	scroll_container.custom_minimum_size = Vector2(580, 350)
	main_container.add_child(scroll_container)
	
	settings_container = VBoxContainer.new()
	scroll_container.add_child(settings_container)
	
	# 創建設置選項
	create_audio_settings()
	create_display_settings()
	create_gameplay_settings()
	
	# 按鈕容器
	var button_container = HBoxContainer.new()
	button_container.alignment = BoxContainer.ALIGNMENT_CENTER
	main_container.add_child(button_container)
	
	# 創建按鈕
	back_button = create_button("返回")
	apply_button = create_button("應用")
	reset_button = create_button("重置")
	
	button_container.add_child(back_button)
	button_container.add_child(apply_button)
	button_container.add_child(reset_button)

func create_audio_settings():
	# 音訊設置區塊
	var audio_section = create_section("音訊設置")
	settings_container.add_child(audio_section)
	
	# 主音量
	var master_volume_container = create_setting_container("主音量")
	master_volume_slider = create_volume_slider(settings_data.master_volume)
	master_volume_container.add_child(master_volume_slider)
	audio_section.add_child(master_volume_container)
	
	# 音樂音量
	var music_volume_container = create_setting_container("音樂音量")
	music_volume_slider = create_volume_slider(settings_data.music_volume)
	music_volume_container.add_child(music_volume_slider)
	audio_section.add_child(music_volume_container)
	
	# 音效音量
	var sfx_volume_container = create_setting_container("音效音量")
	sfx_volume_slider = create_volume_slider(settings_data.sfx_volume)
	sfx_volume_container.add_child(sfx_volume_slider)
	audio_section.add_child(sfx_volume_container)

func create_display_settings():
	# 顯示設置區塊
	var display_section = create_section("顯示設置")
	settings_container.add_child(display_section)
	
	# 螢幕模式
	var screen_mode_container = create_setting_container("螢幕模式")
	screen_mode_option = OptionButton.new()
	screen_mode_option.add_item("視窗模式")
	screen_mode_option.add_item("全螢幕")
	screen_mode_option.selected = settings_data.screen_mode
	screen_mode_container.add_child(screen_mode_option)
	display_section.add_child(screen_mode_container)
	
	# 解析度
	var resolution_container = create_setting_container("解析度")
	resolution_option = OptionButton.new()
	resolution_option.add_item("1280x720")
	resolution_option.add_item("1920x1080")
	resolution_option.add_item("2560x1440")
	resolution_option.selected = settings_data.resolution
	resolution_container.add_child(resolution_option)
	display_section.add_child(resolution_container)
	
	# 垂直同步
	var vsync_container = create_setting_container("垂直同步")
	vsync_check = CheckBox.new()
	vsync_check.button_pressed = settings_data.vsync
	vsync_container.add_child(vsync_check)
	display_section.add_child(vsync_container)

func create_gameplay_settings():
	# 遊戲玩法設置區塊
	var gameplay_section = create_section("遊戲設置")
	settings_container.add_child(gameplay_section)
	
	# 自動保存
	var auto_save_container = create_setting_container("自動保存")
	auto_save_check = CheckBox.new()
	auto_save_check.button_pressed = settings_data.auto_save
	auto_save_container.add_child(auto_save_check)
	gameplay_section.add_child(auto_save_container)
	
	# 遊戲速度
	var game_speed_container = create_setting_container("遊戲速度")
	game_speed_option = OptionButton.new()
	game_speed_option.add_item("慢速")
	game_speed_option.add_item("正常")
	game_speed_option.add_item("快速")
	game_speed_option.selected = settings_data.game_speed
	game_speed_container.add_child(game_speed_option)
	gameplay_section.add_child(game_speed_container)
	
	# 顯示網格
	var show_grid_container = create_setting_container("顯示網格")
	show_grid_check = CheckBox.new()
	show_grid_check.button_pressed = settings_data.show_grid
	show_grid_container.add_child(show_grid_check)
	gameplay_section.add_child(show_grid_container)
	
	# 顯示提示
	var show_tooltips_container = create_setting_container("顯示提示")
	show_tooltips_check = CheckBox.new()
	show_tooltips_check.button_pressed = settings_data.show_tooltips
	show_tooltips_container.add_child(show_tooltips_check)
	gameplay_section.add_child(show_tooltips_container)

func create_section(title: String) -> VBoxContainer:
	var section = VBoxContainer.new()
	
	# 區塊標題
	var section_title = Label.new()
	section_title.text = title
	section_title.add_theme_font_size_override("font_size", 20)
	section_title.add_theme_color_override("font_color", Color.YELLOW)
	section.add_child(section_title)
	
	# 分隔線
	var separator = HSeparator.new()
	section.add_child(separator)
	
	# 添加間距
	var spacer = Control.new()
	spacer.custom_minimum_size = Vector2(0, 10)
	section.add_child(spacer)
	
	return section

func create_setting_container(label_text: String) -> HBoxContainer:
	var container = HBoxContainer.new()
	container.custom_minimum_size = Vector2(0, 40)
	
	# 設置標籤
	var label = Label.new()
	label.text = label_text
	label.custom_minimum_size = Vector2(150, 0)
	container.add_child(label)
	
	return container

func create_volume_slider(initial_value: float) -> HSlider:
	var slider = HSlider.new()
	slider.min_value = 0
	slider.max_value = 100
	slider.step = 1
	slider.value = initial_value
	slider.custom_minimum_size = Vector2(200, 0)
	
	# 添加數值顯示
	var value_label = Label.new()
	value_label.text = str(int(initial_value))
	value_label.custom_minimum_size = Vector2(50, 0)
	
	slider.value_changed.connect(_on_volume_changed.bind(value_label))
	
	var slider_container = HBoxContainer.new()
	slider_container.add_child(slider)
	slider_container.add_child(value_label)
	
	return slider

func create_button(text: String) -> Button:
	var button = Button.new()
	button.text = text
	button.custom_minimum_size = Vector2(100, 40)
	
	# 設置按鈕樣式
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.2, 0.3, 0.5, 0.8)
	style.corner_radius_top_left = 5
	style.corner_radius_top_right = 5
	style.corner_radius_bottom_left = 5
	style.corner_radius_bottom_right = 5
	button.add_theme_stylebox_override("normal", style)
	
	return button

func _on_volume_changed(value: float, label: Label):
	label.text = str(int(value))

func connect_signals():
	back_button.pressed.connect(_on_back_pressed)
	apply_button.pressed.connect(_on_apply_pressed)
	reset_button.pressed.connect(_on_reset_pressed)
	
	# 即時預覽音量變化
	master_volume_slider.value_changed.connect(_on_master_volume_changed)
	music_volume_slider.value_changed.connect(_on_music_volume_changed)
	sfx_volume_slider.value_changed.connect(_on_sfx_volume_changed)

func apply_entrance_animation():
	# 淡入動畫
	modulate.a = 0.0
	var tween = create_tween()
	tween.tween_property(self, "modulate:a", 1.0, 0.5)

func load_settings():
	# 從配置文件加載設置
	var config = ConfigFile.new()
	var err = config.load("user://settings.cfg")
	
	if err == OK:
		settings_data.master_volume = config.get_value("audio", "master_volume", 100)
		settings_data.music_volume = config.get_value("audio", "music_volume", 80)
		settings_data.sfx_volume = config.get_value("audio", "sfx_volume", 100)
		settings_data.screen_mode = config.get_value("display", "screen_mode", 0)
		settings_data.resolution = config.get_value("display", "resolution", 0)
		settings_data.vsync = config.get_value("display", "vsync", true)
		settings_data.auto_save = config.get_value("gameplay", "auto_save", true)
		settings_data.game_speed = config.get_value("gameplay", "game_speed", 1)
		settings_data.show_grid = config.get_value("gameplay", "show_grid", true)
		settings_data.show_tooltips = config.get_value("gameplay", "show_tooltips", true)

func save_settings():
	# 保存設置到配置文件
	var config = ConfigFile.new()
	
	config.set_value("audio", "master_volume", settings_data.master_volume)
	config.set_value("audio", "music_volume", settings_data.music_volume)
	config.set_value("audio", "sfx_volume", settings_data.sfx_volume)
	config.set_value("display", "screen_mode", settings_data.screen_mode)
	config.set_value("display", "resolution", settings_data.resolution)
	config.set_value("display", "vsync", settings_data.vsync)
	config.set_value("gameplay", "auto_save", settings_data.auto_save)
	config.set_value("gameplay", "game_speed", settings_data.game_speed)
	config.set_value("gameplay", "show_grid", settings_data.show_grid)
	config.set_value("gameplay", "show_tooltips", settings_data.show_tooltips)
	
	config.save("user://settings.cfg")

func apply_settings():
	# 應用當前設置
	update_settings_from_ui()
	
	# 應用音訊設置
	AudioServer.set_bus_volume_db(0, linear_to_db(settings_data.master_volume / 100.0))
	
	# 應用顯示設置
	match settings_data.screen_mode:
		0:  # 視窗模式
			DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
		1:  # 全螢幕
			DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
	
	# 應用解析度
	var resolutions = [
		Vector2i(1280, 720),
		Vector2i(1920, 1080),
		Vector2i(2560, 1440)
	]
	if settings_data.resolution < resolutions.size():
		DisplayServer.window_set_size(resolutions[settings_data.resolution])
	
	# 應用垂直同步
	DisplayServer.window_set_vsync_mode(
		DisplayServer.VSYNC_ENABLED if settings_data.vsync else DisplayServer.VSYNC_DISABLED
	)
	
	save_settings()
	print("設置已應用並保存")

func update_settings_from_ui():
	# 從UI控制項更新設置數據
	settings_data.master_volume = master_volume_slider.value
	settings_data.music_volume = music_volume_slider.value
	settings_data.sfx_volume = sfx_volume_slider.value
	settings_data.screen_mode = screen_mode_option.selected
	settings_data.resolution = resolution_option.selected
	settings_data.vsync = vsync_check.button_pressed
	settings_data.auto_save = auto_save_check.button_pressed
	settings_data.game_speed = game_speed_option.selected
	settings_data.show_grid = show_grid_check.button_pressed
	settings_data.show_tooltips = show_tooltips_check.button_pressed

func reset_to_defaults():
	# 重置到預設值
	settings_data = {
		"master_volume": 100,
		"music_volume": 80,
		"sfx_volume": 100,
		"screen_mode": 0,
		"resolution": 0,
		"vsync": true,
		"auto_save": true,
		"game_speed": 1,
		"show_grid": true,
		"show_tooltips": true
	}
	
	# 更新UI控制項
	master_volume_slider.value = settings_data.master_volume
	music_volume_slider.value = settings_data.music_volume
	sfx_volume_slider.value = settings_data.sfx_volume
	screen_mode_option.selected = settings_data.screen_mode
	resolution_option.selected = settings_data.resolution
	vsync_check.button_pressed = settings_data.vsync
	auto_save_check.button_pressed = settings_data.auto_save
	game_speed_option.selected = settings_data.game_speed
	show_grid_check.button_pressed = settings_data.show_grid
	show_tooltips_check.button_pressed = settings_data.show_tooltips

func _on_back_pressed():
	# 返回主選單
	transition_to_scene(MAIN_MENU_SCENE)

func _on_apply_pressed():
	apply_settings()
	show_message("設置已應用")

func _on_reset_pressed():
	# 顯示確認對話框
	var dialog = ConfirmationDialog.new()
	dialog.title = "重置設置"
	dialog.dialog_text = "確定要重置所有設置到預設值嗎？"
	add_child(dialog)
	dialog.popup_centered()
	dialog.confirmed.connect(_on_reset_confirmed.bind(dialog))
	dialog.canceled.connect(dialog.queue_free)

func _on_reset_confirmed(dialog):
	reset_to_defaults()
	show_message("設置已重置")
	dialog.queue_free()

func _on_master_volume_changed(value: float):
	# 即時預覽主音量
	AudioServer.set_bus_volume_db(0, linear_to_db(value / 100.0))

func _on_music_volume_changed(value: float):
	# 即時預覽音樂音量
	# 這裡可以調整音樂總線的音量
	pass

func _on_sfx_volume_changed(value: float):
	# 即時預覽音效音量
	# 這裡可以調整音效總線的音量
	pass

func show_message(text: String):
	var message_dialog = AcceptDialog.new()
	message_dialog.title = "提示"
	message_dialog.dialog_text = text
	add_child(message_dialog)
	message_dialog.popup_centered()
	message_dialog.confirmed.connect(message_dialog.queue_free)

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