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

	#
	# Validate Simple queries
	# 		Queries that are a single text string
	#

	myKuzuDB.execute_query("CREATE NODE TABLE IF NOT EXISTS person(name STRING, age INT64, PRIMARY KEY(name));");
	var q1Result : Array = myKuzuDB.execute_query("MATCH (p:Person {name: 'Alice'}) RETURN p;");
	if(q1Result.is_empty()):
		myKuzuDB.execute_query("CREATE (:person {name: 'Alice', age: 30});");
	
	var q2Result : Array = myKuzuDB.execute_query("MATCH (p:Person {name: 'Bob'}) RETURN p;");
	if(q2Result.is_empty()):
		myKuzuDB.execute_query("CREATE (:person {name: 'Bob', age: 40});");
	
	var queryResult : Array = myKuzuDB.execute_query("MATCH (p:person) RETURN p.*");
	print("Test Query Result");
	print(queryResult);

	#
	#
	# Validate Prepared Queries
	#
	#

	var q3Result : Array = myKuzuDB.execute_query("MATCH (p:Person {name: 'Jake'}) RETURN p;");
	if(q3Result.is_empty()):
		myKuzuDB.execute_query("CREATE (:person {name: 'Jake', age: 16});");
		
	var q4Result : Array = myKuzuDB.execute_query("MATCH (p:Person {name: 'Jessica'}) RETURN p;");
	if(q4Result.is_empty()):
		myKuzuDB.execute_query("CREATE (:person {name: 'Jessica', age: 25});");
	
	var q5Result : Array = myKuzuDB.execute_query("MATCH (p:Person {name: 'Joanna'}) RETURN p;");
	if(q5Result.is_empty()):
		myKuzuDB.execute_query("CREATE (:person {name: 'Joanna', age: 31});");

	var min_age : int = 18
	var max_age : int = 30
	
	var preparedResult : Array = myKuzuDB.execute_prepared_query("MATCH (p:Person) WHERE p.age > $min_age and p.age < $max_age RETURN p.name", {"min_age": min_age, "max_age": max_age});
	print("Test Prepared Query Result");
	print(preparedResult);

	# Correct Console Output (On Run 2):
	#
	# 	Starting Database Test
	#	Database initialized successfully!
	#	Database Connection Established!
	#	Test Query Result
	#	[{ "p.name": "Alice", "p.age": "30" }, { "p.name": "Bob", "p.age": "40" }, { "p.name": "Jake", "p.age": "16" }, { "p.name": "Jessica", "p.age": "25" }, { "p.name": "Joanna", "p.age": "31" }]
	#	Breaking
	#	Test Prepared Query Result
	#	[{ "p.name": "Jessica" }]
