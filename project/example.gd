extends Node


func _ready() -> void:
	var example := RoadManager.new()
	example.print_type(example)
