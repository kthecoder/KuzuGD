extends Node

var myKuzuDB : KuzuGD = KuzuGD.new();

func _ready():
	print("Starting Database Test");
	
	var success_db = myKuzuDB.kuzu_init("res://my_database.kuzu");

	if success_db:
		print("Database initialized successfully!");
	else:
		print("Failed to initialize database.");

	var success_connect = myKuzuDB.kuzu_connect(1);

	if success_connect:
		print("Database Connection Established!");
	else:
		print("Failed to create a connection to the database.");

	
