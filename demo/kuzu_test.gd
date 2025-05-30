extends Node

var myKuzuDB : KuzuGD = KuzuGD.new();

func _ready():
	print("Starting Database Test");
	
	var db_path = ProjectSettings.globalize_path("res://data/database/kuzu_db");
	var success_db = myKuzuDB.kuzu_init(db_path);

	if success_db:
		print("Database initialized successfully!");
	else:
		print("Failed to initialize database.");

	var success_connect = myKuzuDB.kuzu_connect(1);

	if success_connect:
		print("Database Connection Established!");
	else:
		print("Failed to create a connection to the database.");

	myKuzuDB.execute_query("CREATE NODE TABLE person(name STRING, age INT64, PRIMARY KEY(name));");
	myKuzuDB.execute_query("CREATE (:person {name: 'Alice', age: 30});");
	myKuzuDB.execute_query("CREATE (:person {name: 'Bob', age: 40});");
	
	var queryResult : Array = myKuzuDB.execute_query("MATCH (p:person) RETURN p.*");
	print(queryResult);


	
