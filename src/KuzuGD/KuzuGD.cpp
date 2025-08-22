#include "KuzuGD.h"

using namespace godot;
using namespace kuzu::main; // EXAMPLE : https://github.com/kuzudb/kuzu/blob/master/examples/cpp/main.cpp
using kuzu::common::LogicalTypeID;

KuzuGD::KuzuGD() {
	/*
		Assumes Small DB Size by
			max_num_threads, max_db_size
	*/

	config.bufferPoolSize = 1024 * 1024 * 512; // 512MB
	config.maxNumThreads = 1; // Defaults 1
	config.enableCompression = true;
	config.readOnly = false;
	config.maxDBSize = (1ULL << 30); // 1 GB default
	config.autoCheckpoint = true;
	config.checkpointThreshold = 1024 * 1024 * 100; // 100MB
}

KuzuGD::~KuzuGD() {
	myKuzuDB->~Database();
	dbConnection->~Connection();
}

/******************************************************************


	System Configuration Functions

		Define the Databases Setup Configuration


******************************************************************/

/**
 * @brief Proclaim TBD
 * @param size TBD
 * @return the result is TBD
 */
void KuzuGD::set_buffer_pool_size(uint64_t size) {
	if (size > 0) {
		config.bufferPoolSize = size;
	}
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
uint64_t KuzuGD::get_buffer_pool_size() const {
	return config.bufferPoolSize;
}

/**
 * @brief Proclaim TBD
 * @param threads TBD
 * @return the result is TBD
 */
void KuzuGD::set_max_num_threads(uint64_t threads) {
	if (threads > 0) {
		config.maxNumThreads = threads;
	}
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
uint64_t KuzuGD::get_max_num_threads() const {
	return config.maxNumThreads;
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
void KuzuGD::set_enable_compression(bool enabled) {
	config.enableCompression = enabled;
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
bool KuzuGD::get_enable_compression() const {
	return config.enableCompression;
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
void KuzuGD::set_read_only(bool readonly) {
	config.readOnly = readonly;
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
bool KuzuGD::get_read_only() const {
	return config.readOnly;
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
void KuzuGD::set_max_db_size(uint64_t size) {
	if (size != -1u) {
		config.maxDBSize = size;
	}
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
uint64_t KuzuGD::get_max_db_size() const {
	return config.maxDBSize;
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
void KuzuGD::set_auto_checkpoint(bool enabled) {
	config.autoCheckpoint = enabled;
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
bool KuzuGD::get_auto_checkpoint() const {
	return config.autoCheckpoint;
}

/**
 * @brief Proclaim TBD
 * @param threshold TBD
 * @return the result is TBD
 */
void KuzuGD::set_checkpoint_threshold(uint64_t threshold) {
	if (threshold >= 0) {
		config.checkpointThreshold = threshold;
	}
}

/**
 * @brief Proclaim TBD
 * @return the result is TBD
 */
uint64_t KuzuGD::get_checkpoint_threshold() const {
	return config.checkpointThreshold;
}

/**
 * @brief Proclaim the values of the Kuzu Configuration
 * @return the result is an Array[Buffer Pool Size, Max Num Threads, Is Compression Enabled, Is Ready Only, Max DB Size, Auto Checkpoint, Checkpoint Threshold]
 */
Array KuzuGD::get_config() const {
	Array config_array;

	config_array.append(config.bufferPoolSize);
	config_array.append(config.maxNumThreads);
	config_array.append(config.enableCompression);
	config_array.append(config.readOnly);
	config_array.append(config.maxDBSize);
	config_array.append(config.autoCheckpoint);
	config_array.append(config.checkpointThreshold);

	return config_array;
}

/******************************************************************


	Management Functions


******************************************************************/

/**
 * @brief Set the query timeout value in milliseconds for the connection
 * @return the result is success or failure
 */
bool KuzuGD::query_timeout(int timeout_millis) {
	try {
		dbConnection->setQueryTimeOut(timeout_millis);

		return true;
	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Kuzu Connection Set Query Timeout Failed: ") + e.what());

		return false;
	}
}

/**
 * @brief Interrupt the Current Query Execution
 * @return the result is success or failure
 */
bool KuzuGD::interrupt_connection() {
	try {
		dbConnection->interrupt();

		return true;
	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Kuzu Connection Interrupt Failed: ") + e.what());

		return false;
	}
}

/**
 * @brief Determines the Storage Version of Kuzu
 * @return the result is a Godot int with the Storage Version
 */
int KuzuGD::storage_version() {
	return Version::getStorageVersion();
}

/**
 * @brief Determines the Version of Kuzu
 * @return the result is a Godot String with the Version
 */
godot::String KuzuGD::get_kuzu_version() {
	return godot::String(Version::getVersion());
}

/******************************************************************


	Initialization

		Init the DB, assume Configuration is Chosen


******************************************************************/

/**
 * @brief Initializes a Kuzu Database.
 * @return the result is the success or failure.
 */
bool KuzuGD::kuzu_init(const String database_path) {
	if (database_path.is_empty()) {
		UtilityFunctions::push_error("KuzuGD ERROR | Database path is empty");
		return false;
	}

	try {
		std::string db_path_std = database_path.utf8().get_data();
		myKuzuDB = std::make_unique<Database>(db_path_std, config);

		return true;
	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Kuzu init failed: ") + e.what());

		return false;
	}
}

/**
 * @brief Initializes a Connection to the existing Database.
 * @return the result is the success or failure.
 */
bool KuzuGD::kuzu_connect(int num_threads) {
	if (!&myKuzuDB) {
		UtilityFunctions::push_error("KuzuGD ERROR | Database does not exist");
		return -1;
	}

	try {
		dbConnection = std::make_unique<Connection>(myKuzuDB);

		return true;
	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Kuzu Connection failed: ") + e.what());

		return false;
	}
}

/**
 * @brief Decides the number of max threads this connection can use.
 * @return the result is the success or failure.
 */
bool KuzuGD::connection_set_max_threads(int num_threads) {
	try {
		dbConnection->setMaxNumThreadForExec(num_threads);

		return true;
	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Kuzu Connection Set Max Threads Failed: ") + e.what());

		return false;
	}
}

/**
 * @brief Proclaims the number of max threads this connection can use.
 * @return the result is the number of maximum threads.
 */
// -1 : Failed
int KuzuGD::connection_get_max_threads() {
	try {
		return dbConnection->getMaxNumThreadForExec();

	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Kuzu Connection Get Max Threads Failed: ") + e.what());

		return -1;
	}
}

/******************************************************************


	Query

		Database Query Operations


******************************************************************/

/**
 * @brief Executes the given query and returns the result as a Godot String.
 * @param myQuery The query to execute.
 * @return the result of the query as Godot String.
 */
godot::String KuzuGD::query(const String &myQuery) {
	std::unique_ptr<kuzu::main::QueryResult> qresult;
	Array result_array;

	try {
		qresult = dbConnection->query(myQuery.utf8().get_data());

		return String(qresult->toString().c_str());

	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Kuzu Query Failed: ") + e.what());

		return godot::String(); // empty string
	}
}

/**
 * @brief Executes the given query and returns the result as a Godot Array.
 * @param myQuery The query to execute.
 * @return the result of the query as Godot Array.
 */
godot::Array KuzuGD::queryAsArray(const std::string &myQuery) {
	Array result_array;

	try {
		auto result = dbConnection->query(myQuery); // unique_ptr<QueryResult>

		// Get column names once
		std::vector<std::string> col_names = result->getColumnNames();

		// Iterate over rows
		while (result->hasNext()) {
			auto tuple = result->getNext();
			Dictionary row_dict;

			for (uint64_t col = 0; col < tuple->len(); col++) {
				auto val = tuple->getValue(col);

				if (val->isNull()) {
					row_dict[String(col_names[col].c_str())] = Variant(); // null
				} else {
					switch (val->getDataType().getLogicalTypeID()) {
						case kuzu::common::LogicalTypeID::STRING: {
							row_dict[String(col_names[col].c_str())] =
									String(val->getValue<std::string>().c_str());
							break;
						}
						case kuzu::common::LogicalTypeID::INT64: {
							row_dict[String(col_names[col].c_str())] =
									static_cast<int64_t>(val->getValue<int64_t>());
							break;
						}
						case kuzu::common::LogicalTypeID::DOUBLE: {
							row_dict[String(col_names[col].c_str())] =
									static_cast<double>(val->getValue<double>());
							break;
						}
						case kuzu::common::LogicalTypeID::BOOL: {
							row_dict[String(col_names[col].c_str())] =
									static_cast<bool>(val->getValue<bool>());
							break;
						}
						default: {
							// Fallback: convert to string
							row_dict[String(col_names[col].c_str())] =
									String(val->toString().c_str());
							break;
						}
					}
				}
			}

			result_array.push_back(row_dict);
		}

	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Query to Array failed: ") + e.what());
	}

	return result_array;
}

/**
 * @brief Prepares & Executes the given query with inputParams and returns the result.
 * @param query The query to prepare & execute.
 * @param params The parameter pack where each arg is a pair with the first element
 * being parameter name and second element being parameter value.
 * @return the result of the query.
 */
Array KuzuGD::queryWithParams(const String &query, const Dictionary &params) {
	Array result_array;

	try {
		//
		// Prepare the statement
		//
		std::unique_ptr<PreparedStatement> stmt = dbConnection->prepare(query.utf8().get_data());
		if (!stmt) {
			UtilityFunctions::push_error("KuzuGD ERROR | Query with Param's Failed to prepare statement.");
			return result_array;
		}

		//
		// Build Paramater's Map
		//
		std::unordered_map<std::string, std::unique_ptr<kuzu::common::Value>> inputParams;
		Array keys = params.keys();
		for (int i = 0; i < keys.size(); i++) {
			String key = keys[i];
			Variant val = params[key];

			if (val.get_type() == Variant::STRING) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(std::string(val.operator godot::String().utf8().get_data()));
			} else if (val.get_type() == Variant::INT) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<int64_t>(val));
			} else if (val.get_type() == Variant::FLOAT) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<double>(val));
			} else if (val.get_type() == Variant::BOOL) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<bool>(val));
			} else if (val.get_type() == Variant::NIL) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>();
			} else {
				UtilityFunctions::push_warning("Unsupported param type for key: " + key);
			}
		}

		//
		// Execute with params
		//
		std::unique_ptr<QueryResult> qresult =
				dbConnection->executeWithParams(stmt.get(), std::move(inputParams));

		//
		// Get column names
		//
		std::vector<std::string> col_names = qresult->getColumnNames();

		//
		// Iterate over rows
		//
		while (qresult->hasNext()) {
			auto tuple = qresult->getNext();
			Dictionary row;

			for (size_t col = 0; col < col_names.size(); ++col) {
				auto val = tuple->getValue(col);

				if (val->isNull()) {
					row[String(col_names[col].c_str())] = Variant();
					continue;
				}

				switch (val->getDataType().getLogicalTypeID()) {
					case LogicalTypeID::STRING:
						row[String(col_names[col].c_str())] =
								String(val->getValue<std::string>().c_str());
						break;
					case LogicalTypeID::INT64:
						row[String(col_names[col].c_str())] =
								static_cast<int64_t>(val->getValue<int64_t>());
						break;
					case LogicalTypeID::DOUBLE:
						row[String(col_names[col].c_str())] =
								static_cast<double>(val->getValue<double>());
						break;
					case LogicalTypeID::BOOL:
						row[String(col_names[col].c_str())] =
								static_cast<bool>(val->getValue<bool>());
						break;
					default:
						row[String(col_names[col].c_str())] =
								String(val->toString().c_str());
						break;
				}
			}

			result_array.push_back(row);
		}

	} catch (const std::exception &e) {
		UtilityFunctions::push_error(
				String("KuzuGD ERROR | Query with Param's execution failed: ") + e.what());
	}

	return result_array;
}

Ref<GDPreparedStatement> KuzuGD::prepare(const String &query) {
	Ref<GDPreparedStatement> wrapper;
	wrapper.instantiate();

	try {
		std::string q = query.utf8().get_data();
		auto stmt = dbConnection->prepare(q);
		wrapper->set_statement(std::move(stmt));
	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Prepare failed: ") + e.what());
	}

	return wrapper;
}

Ref<GDPreparedStatement> KuzuGD::prepareWithParams(const String &query, const Dictionary &params) {
	Ref<GDPreparedStatement> wrapper;
	wrapper.instantiate();

	try {
		// Convert Godot String to std::string
		std::string q = query.utf8().get_data();

		// Convert Godot Dictionary -> unordered_map<string, unique_ptr<Value>>
		std::unordered_map<std::string, std::unique_ptr<kuzu::common::Value>> inputParams;
		Array keys = params.keys();
		for (int i = 0; i < keys.size(); i++) {
			String key = keys[i];
			Variant val = params[key];

			if (val.get_type() == Variant::STRING) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(std::string(val.operator String().utf8().get_data()));
			} else if (val.get_type() == Variant::INT) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<int64_t>(val));
			} else if (val.get_type() == Variant::FLOAT) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<double>(val));
			} else if (val.get_type() == Variant::BOOL) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<bool>(val));
			} else if (val.get_type() == Variant::NIL) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>();
			} else {
				UtilityFunctions::push_warning(
						String("KuzuGD WARNING | Unsupported parameter type for key: ") + key);
			}
		}

		// Call Kùzu's prepareWithParams
		auto stmt = dbConnection->prepareWithParams(q, std::move(inputParams));
		wrapper->set_statement(std::move(stmt));

	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | prepareWithParams failed: ") + e.what());
	}

	return wrapper;
}

Array KuzuGD::executeWithParams(const Ref<GDPreparedStatement> &wrapper, const Dictionary &params) {
	Array result_array;
	if (!wrapper.is_valid() || !wrapper->get_statement()) {
		UtilityFunctions::push_error("KuzuGD ERROR | Invalid prepared statement");
		return result_array;
	}

	try {
		// Convert Godot Dictionary -> unordered_map<string, unique_ptr<Value>>
		std::unordered_map<std::string, std::unique_ptr<kuzu::common::Value>> inputParams;
		Array keys = params.keys();
		for (int i = 0; i < keys.size(); i++) {
			String key = keys[i];
			Variant val = params[key];

			if (val.get_type() == Variant::STRING) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(std::string(val.operator String().utf8().get_data()));
			} else if (val.get_type() == Variant::INT) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<int64_t>(val));
			} else if (val.get_type() == Variant::FLOAT) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<double>(val));
			} else if (val.get_type() == Variant::BOOL) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>(static_cast<bool>(val));
			} else if (val.get_type() == Variant::NIL) {
				inputParams[key.utf8().get_data()] =
						std::make_unique<kuzu::common::Value>();
			}
		}

		// Execute
		std::unique_ptr<QueryResult> qresult =
				dbConnection->executeWithParams(wrapper->get_statement(), std::move(inputParams));

		// Convert QueryResult -> Array<Dictionary>
		std::vector<std::string> col_names = qresult->getColumnNames();
		while (qresult->hasNext()) {
			auto tuple = qresult->getNext();
			Dictionary row;
			for (size_t col = 0; col < col_names.size(); ++col) {
				auto val = tuple->getValue(col);
				if (val->isNull()) {
					row[String(col_names[col].c_str())] = Variant();
					continue;
				}
				switch (val->getDataType().getLogicalTypeID()) {
					case LogicalTypeID::STRING:
						row[String(col_names[col].c_str())] =
								String(val->getValue<std::string>().c_str());
						break;
					case LogicalTypeID::INT64:
						row[String(col_names[col].c_str())] =
								static_cast<int64_t>(val->getValue<int64_t>());
						break;
					case LogicalTypeID::DOUBLE:
						row[String(col_names[col].c_str())] =
								static_cast<double>(val->getValue<double>());
						break;
					case LogicalTypeID::BOOL:
						row[String(col_names[col].c_str())] =
								static_cast<bool>(val->getValue<bool>());
						break;
					default:
						row[String(col_names[col].c_str())] =
								String(val->toString().c_str());
						break;
				}
			}
			result_array.push_back(row);
		}

	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Execute failed: ") + e.what());
	}

	return result_array;
}

/******************************************************************


	Query Summary Methods

		QuerySummary stores the execution time, plan, compiling time and query options of a query.


******************************************************************/

Ref<GDQuerySummary> KuzuGD::query_with_summary(const String &query) {
	Ref<GDQuerySummary> summary_ref;
	summary_ref.instantiate();

	try {
		auto result = dbConnection->query(std::string(query.utf8()));
		if (result) {
			auto *qs = result->getQuerySummary();
			if (qs) {
				summary_ref->set_summary(*qs);
			}
		}
	} catch (const std::exception &e) {
		UtilityFunctions::push_error(String("KuzuGD ERROR | Query With Summary Failed: ") + e.what());
	}

	return summary_ref;
}

/******************************************************************


	GDExtension Bind Methods

		Bind the Middleware Wrapper Functions for GDSCript usage


******************************************************************/

void KuzuGD::_bind_methods() {
	/********************************************

		System Configuration Functions

			Define the Databases Setup Configuration

	********************************************/
	ClassDB::bind_method(D_METHOD("set_buffer_pool_size", "size"), &KuzuGD::set_buffer_pool_size);
	ClassDB::bind_method(D_METHOD("get_buffer_pool_size"), &KuzuGD::get_buffer_pool_size);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "buffer_pool_size"), "set_buffer_pool_size", "get_buffer_pool_size");

	ClassDB::bind_method(D_METHOD("set_max_num_threads", "threads"), &KuzuGD::set_max_num_threads);
	ClassDB::bind_method(D_METHOD("get_max_num_threads"), &KuzuGD::get_max_num_threads);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "max_num_threads"), "set_max_num_threads", "get_max_num_threads");

	ClassDB::bind_method(D_METHOD("set_enable_compression", "enabled"), &KuzuGD::set_enable_compression);
	ClassDB::bind_method(D_METHOD("get_enable_compression"), &KuzuGD::get_enable_compression);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "enable_compression"), "set_enable_compression", "get_enable_compression");

	ClassDB::bind_method(D_METHOD("set_read_only", "readonly"), &KuzuGD::set_read_only);
	ClassDB::bind_method(D_METHOD("get_read_only"), &KuzuGD::get_read_only);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "read_only"), "set_read_only", "get_read_only");

	ClassDB::bind_method(D_METHOD("set_max_db_size", "size"), &KuzuGD::set_max_db_size);
	ClassDB::bind_method(D_METHOD("get_max_db_size"), &KuzuGD::get_max_db_size);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "max_db_size"), "set_max_db_size", "get_max_db_size");

	ClassDB::bind_method(D_METHOD("set_auto_checkpoint", "enabled"), &KuzuGD::set_auto_checkpoint);
	ClassDB::bind_method(D_METHOD("get_auto_checkpoint"), &KuzuGD::get_auto_checkpoint);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "auto_checkpoint"), "set_auto_checkpoint", "get_auto_checkpoint");

	ClassDB::bind_method(D_METHOD("set_checkpoint_threshold", "threshold"), &KuzuGD::set_checkpoint_threshold);
	ClassDB::bind_method(D_METHOD("get_checkpoint_threshold"), &KuzuGD::get_checkpoint_threshold);
	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "checkpoint_threshold"), "set_checkpoint_threshold", "get_checkpoint_threshold");

	ClassDB::bind_method(D_METHOD("get_config"), &KuzuGD::get_config);

	/********************************************

		Management Functions

	********************************************/

	ClassDB::bind_method(D_METHOD("query_timeout", "timeout_millis"), &KuzuGD::query_timeout);

	ClassDB::bind_method(D_METHOD("interrupt_connection"), &KuzuGD::interrupt_connection);

	ClassDB::bind_method(D_METHOD("get_kuzu_version"), &KuzuGD::get_kuzu_version);

	ClassDB::bind_method(D_METHOD("storage_version"), &KuzuGD::storage_version);

	/********************************************

		Initialization

	********************************************/

	ClassDB::bind_method(D_METHOD("kuzu_init", "database_path"), &KuzuGD::kuzu_init);

	ClassDB::bind_method(D_METHOD("kuzu_connect", "num_threads"), &KuzuGD::kuzu_connect);

	ClassDB::bind_method(D_METHOD("connection_set_max_threads", "num_threads"), &KuzuGD::connection_set_max_threads);

	ClassDB::bind_method(D_METHOD("connection_get_max_threads"), &KuzuGD::connection_get_max_threads);

	ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "num_threads"), "connection_set_max_threads", "connection_get_max_threads");

	/********************************************

		Query

	********************************************/

	ClassDB::bind_method(D_METHOD("queryAsArray", "query"), &KuzuGD::queryAsArray);

	ClassDB::bind_method(D_METHOD("query", "query"), &KuzuGD::query);

	ClassDB::bind_method(D_METHOD("queryWithParams", "query", "params"), &KuzuGD::queryWithParams);

	ClassDB::bind_method(D_METHOD("prepare", "query"), &KuzuGD::prepare);

	ClassDB::bind_method(D_METHOD("prepareWithParams", "query", "params"), &KuzuGD::prepareWithParams);

	ClassDB::bind_method(D_METHOD("executeWithParams", "statement", "params"), &KuzuGD::executeWithParams);

	/********************************************

		Query Summary

	********************************************/

	ClassDB::bind_method(D_METHOD("query_with_summary", "query"), &KuzuGD::query_with_summary);
}