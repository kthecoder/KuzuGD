extends Camera3D

func _ready():
	
	size = 10
	position = Vector3(0, 20, 0)  # Adjust position to view the GridMap
	look_at(Vector3(50, 1, 50))
