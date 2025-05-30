#include "./KuzuGD/KuzuGD.h"

#include <iostream>

int main() {
	// Define database path
	const char *db_path = "./kuzu-db-dir";

	// Initialize KuzuDB
	KuzuGD *myKuzu;
	if (!myKuzu) {
		std::cerr << "Failed to initialize KuzuDB!" << std::endl;
		return 1;
	}

	// Create the Database
	myKuzu->kuzu_init(db_path);

	// Create a connection
	bool conn = myKuzu->kuzu_connect(1);
	if (!conn) {
		std::cerr << "Failed to create connection!" << std::endl;
		delete myKuzu;
		return 1;
	}

	// Execute a simple query
	const char *query = "CREATE (a:Person {name: 'Alice'});";
	myKuzu->execute_query(query);

	delete myKuzu;

	return 0;
}