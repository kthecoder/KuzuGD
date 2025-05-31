#include "KuzuGD.h"

using namespace godot;

KuzuGD::KuzuGD() {
	/*
		Assumes Small DB Size by
			max_num_threads, max_db_size
	*/

	config.buffer_pool_size = 1024 * 1024 * 512; // 512MB
	config.max_num_threads = 1; // Defaults 1
	config.enable_compression = true;
	config.read_only = false;
	config.max_db_size = (1ULL << 30); // 1 GB default
	config.auto_checkpoint = true;
	config.checkpoint_threshold = 1024 * 1024 * 100; // 100MB
}

KuzuGD::~KuzuGD() {
	if (myKuzuDB) {
		kuzu_database_destroy(myKuzuDB);
		myKuzuDB = nullptr;
	}

	if (dbConnection) {
		kuzu_connection_destroy(dbConnection);
		dbConnection = nullptr;
	}
}

/******************************************************************


	System Configuration Functions

		Define the Databases Setup Configuration


******************************************************************/

void KuzuGD::set_buffer_pool_size(uint64_t size) {
	if (size > 0) {
		config.buffer_pool_size = size;
	}
}

uint64_t KuzuGD::get_buffer_pool_size() const {
	return config.buffer_pool_size;
}

void KuzuGD::set_max_num_threads(uint64_t threads) {
	if (threads > 0) {
		config.max_num_threads = threads;
	}
}

uint64_t KuzuGD::get_max_num_threads() const {
	return config.max_num_threads;
}

void KuzuGD::set_enable_compression(bool enabled) {
	config.enable_compression = enabled;
}
bool KuzuGD::get_enable_compression() const {
	return config.enable_compression;
}

void KuzuGD::set_read_only(bool readonly) {
	config.read_only = readonly;
}
bool KuzuGD::get_read_only() const {
	return config.read_only;
}

void KuzuGD::set_max_db_size(uint64_t size) {
	if (size != -1u) {
		config.max_db_size = size;
	}
}
uint64_t KuzuGD::get_max_db_size() const {
	return config.max_db_size;
}

void KuzuGD::set_auto_checkpoint(bool enabled) {
	config.auto_checkpoint = enabled;
}
bool KuzuGD::get_auto_checkpoint() const {
	return config.auto_checkpoint;
}

void KuzuGD::set_checkpoint_threshold(uint64_t threshold) {
	if (threshold >= 0) {
		config.checkpoint_threshold = threshold;
	}
}
uint64_t KuzuGD::get_checkpoint_threshold() const {
	return config.checkpoint_threshold;
}

Array KuzuGD::get_config() const {
	Array config_array;

	config_array.append(config.buffer_pool_size);
	config_array.append(config.max_num_threads);
	config_array.append(config.enable_compression);
	config_array.append(config.read_only);
	config_array.append(config.max_db_size);
	config_array.append(config.auto_checkpoint);
	config_array.append(config.checkpoint_threshold);

	return config_array;
}

/******************************************************************


	Management Functions


******************************************************************/

// Set the query timeout value in milliseconds for the connection
bool KuzuGD::query_timeout(int timeout_millis) {
	return kuzu_connection_set_query_timeout(dbConnection, timeout_millis);
}

//Interrupt the Current Query Execution
void KuzuGD::interrupt_connection() {
	kuzu_connection_interrupt(dbConnection);
	return;
}

int KuzuGD::storage_version() {
	return kuzu_get_storage_version();
}

String KuzuGD::get_kuzu_version() {
	return String::utf8(kuzu_get_version());
}

/******************************************************************


	Initialization

		Init the DB, assume Configuration is Chosen


******************************************************************/

bool KuzuGD::kuzu_init(const String database_path) {
	if (database_path.is_empty()) {
		UtilityFunctions::push_error("KuzuGD ERROR | Database path is empty");
		return false;
	}

	myKuzuDB = (kuzu_database *)malloc(sizeof(kuzu_database));
	if (!myKuzuDB) {
		UtilityFunctions::push_error("KuzuGD ERROR | Memory allocation failed for KuzuDB Init");
		return false;
	}
	myKuzuDB->_database = nullptr;
	memset(myKuzuDB, 0, sizeof(kuzu_database));

	kuzu_state state = kuzu_database_init(
			database_path.utf8().get_data(),
			config,
			myKuzuDB);

	return state == KuzuSuccess;
}

bool KuzuGD::kuzu_connect(int num_threads) {
	dbConnection = (kuzu_connection *)malloc(sizeof(kuzu_connection));
	if (!dbConnection) {
		UtilityFunctions::push_error("KuzuGD ERROR | Memory allocation failed for KuzuDB Connect");
		return false;
	}
	memset(dbConnection, 0, sizeof(kuzu_connection));

	kuzu_state state = kuzu_connection_init(
			myKuzuDB,
			dbConnection);

	if (state == KuzuSuccess) {
		// Set the Thread Count for Queries
		kuzu_connection_set_max_num_thread_for_exec(
				dbConnection,
				num_threads);
		return state == KuzuSuccess;

	} else {
		UtilityFunctions::push_error("KuzuGD ERROR | Kuzu Connection FAILED");
		return state == KuzuError;
	}
}

bool KuzuGD::connection_set_max_threads(int num_threads) {
	kuzu_state state = kuzu_connection_set_max_num_thread_for_exec(
			dbConnection,
			num_threads);

	if (state == KuzuError) {
		UtilityFunctions::push_error("KuzuGD ERROR | Kuzu Set Max Threads FAILED");
	}

	return state == KuzuSuccess;
}

int KuzuGD::connection_get_max_threads() {
	uint64_t result = 0;
	kuzu_state state = kuzu_connection_get_max_num_thread_for_exec(
			dbConnection,
			&result);

	if (state == KuzuSuccess) {
		return static_cast<int>(result);
	} else {
		UtilityFunctions::push_error("KuzuGD ERROR | FAILED to Obtain Max Num Threads for Connection");

		return -1;
	}
}

/******************************************************************


	Query

		Database Query Operations


******************************************************************/

Array KuzuGD::execute_query(const String &query) {
	kuzu_query_result result;
	kuzu_state state = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	Array result_array;

	if (state != KuzuSuccess) {
		char *error_msg = kuzu_query_result_get_error_message(&result);

		UtilityFunctions::push_error("KuzuGD ERROR | Query - " + String(error_msg));
		result_array.append("ERROR: " + String(error_msg));

		//CleanUp
		kuzu_query_result_destroy(&result);
		kuzu_destroy_string(error_msg);

		return result_array;
	}

	while (true) {
		kuzu_flat_tuple row;
		if (kuzu_query_result_get_next(&result, &row) != KuzuSuccess) {
			break;
		}

		Dictionary row_dict;

		for (uint32_t i = 0; i < kuzu_query_result_get_num_columns(&result); i++) {
			char *column_name;
			if (kuzu_query_result_get_column_name(&result, i, &column_name) == KuzuSuccess) {
				kuzu_value value;
				if (kuzu_flat_tuple_get_value(&row, i, &value) == KuzuSuccess) {
					row_dict[String(column_name)] = kuzu_value_to_string(&value);
				}
			}
			kuzu_destroy_string(column_name);
		}

		result_array.append(row_dict);
	}

	//Clean Up
	kuzu_query_result_destroy(&result);

	return result_array;
}

Array KuzuGD::execute_prepared_query(const String &query, const Dictionary &params) {
	kuzu_prepared_statement stmt;
	if (kuzu_connection_prepare(dbConnection, query.utf8().get_data(), &stmt) != KuzuSuccess) {
		UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to prepare query");

		//CleanUp
		kuzu_prepared_statement_destroy(&stmt);

		return Array();
	}

	// Bind parameters
	for (int i = 0; i < params.size(); i++) {
		String key = params.keys()[i];
		Variant value = params.values()[i];

		//! Godot does not support the same types as Kuzu
		switch (value.get_type()) {
			case Variant::INT: {
				kuzu_state stateSuccess = kuzu_prepared_statement_bind_int64(&stmt, key.utf8().get_data(), value.operator int64_t());

				if (stateSuccess != KuzuSuccess) {
					UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind INT to Query Parameter");
				}

				break;
			}

			case Variant::STRING: {
				string string_value = value.operator String().utf8().get_data();

				if (is_date(string_value)) {
					kuzu_date_t *kuzuDate = nullptr;

					kuzu_state stateSuccess = kuzu_date_from_string(string_value.c_str(), kuzuDate);
					if (stateSuccess != KuzuSuccess) {
						UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to convert STRING into Date");
					}

					kuzu_state stateSuccess2 = kuzu_prepared_statement_bind_date(
							&stmt, key.utf8().get_data(),
							*kuzuDate);

					if (stateSuccess2 != KuzuSuccess) {
						UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind Date to Query Parameter");
					}
				}

				else if (is_timestamp_tz(string_value)) {
					ParsedTime tmp_time;
					string_to_tm(string_value, tmp_time);

					kuzu_timestamp_tz_t kuzu_ts;
					kuzu_state stateSuccess = kuzu_timestamp_tz_from_tm(tmp_time.tm_value, &kuzu_ts);
					kuzu_ts.value += tmp_time.tz_offset_microseconds;

					if (stateSuccess == KuzuSuccess) {
						kuzu_state stateSuccess2 = kuzu_prepared_statement_bind_timestamp_tz(
								&stmt, key.utf8().get_data(),
								kuzu_ts);

						if (stateSuccess2 != KuzuSuccess) {
							UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind TimeZone TimeStamp to Query Parameter");
						}
					} else {
						UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED Timestamp Format is not a Time Zone");
					}
				}

				else if (is_timestamp(string_value)) {
					ParsedTime tmp_time;
					string_to_tm(string_value, tmp_time);

					kuzu_timestamp_t kuzu_ts;
					kuzu_state stateSuccess = kuzu_timestamp_from_tm(tmp_time.tm_value, &kuzu_ts);

					if (stateSuccess == KuzuSuccess) {
						kuzu_state stateSuccess2 = kuzu_prepared_statement_bind_timestamp(
								&stmt, key.utf8().get_data(),
								kuzu_ts);
						if (stateSuccess2 != KuzuSuccess) {
							UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind TIMESTAMP to Query Parameter");
						}
					} else {
						UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED Time is not a Timestamp");
					}
				}

				else if (is_interval(string_value)) {
					kuzu_state stateSuccess = kuzu_prepared_statement_bind_interval(
							&stmt, key.utf8().get_data(),
							string_to_kuzu_interval(string_value));

					if (stateSuccess != KuzuSuccess) {
						UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind INTERVAL to Query Parameter");
					}
				}

				else {
					kuzu_state stateSuccess = kuzu_prepared_statement_bind_string(&stmt, key.utf8().get_data(), string_value.c_str());

					if (stateSuccess != KuzuSuccess) {
						UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind STRING to Query Parameter");
					}
				}

				break;
			}

			case Variant::BOOL: {
				kuzu_state stateSuccess = kuzu_prepared_statement_bind_bool(&stmt, key.utf8().get_data(), value.operator bool());

				if (stateSuccess != KuzuSuccess) {
					UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind BOOL to Query Parameter");
				}

				break;
			}

			case Variant::FLOAT: {
				kuzu_state stateSuccess = kuzu_prepared_statement_bind_float(&stmt, key.utf8().get_data(), value.operator float());

				if (stateSuccess != KuzuSuccess) {
					UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED to bind FLOAT to Query Parameter");
				}

				break;
			}

			default:
				UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - FAILED Unsupported data type for binding: " + key);
		}
	}

	Array result_array;
	// Execute the query
	kuzu_query_result result;
	if (kuzu_connection_execute(dbConnection, &stmt, &result) != KuzuSuccess) {
		char *error_msg = kuzu_prepared_statement_get_error_message(&stmt);
		UtilityFunctions::push_error("KuzuGD ERROR | Prepared Query - " + String(error_msg));

		result_array.append("ERROR: " + String(error_msg));

		//Cleanup
		kuzu_query_result_destroy(&result);
		kuzu_destroy_string(error_msg);

		return Array(); // Return empty array if execution fails
	}

	// Process result
	while (true) {
		kuzu_flat_tuple row;
		if (kuzu_query_result_get_next(&result, &row) != KuzuSuccess) {
			break;
		}

		Dictionary row_dict;
		for (uint32_t i = 0; i < kuzu_query_result_get_num_columns(&result); i++) {
			char *column_name;
			if (kuzu_query_result_get_column_name(&result, i, &column_name) == KuzuSuccess) {
				kuzu_value value;
				if (kuzu_flat_tuple_get_value(&row, i, &value) == KuzuSuccess) {
					row_dict[String(column_name)] = kuzu_value_to_string(&value);
				}
			}
			kuzu_destroy_string(column_name);
		}
		result_array.append(row_dict);
	}

	//Clean Up
	kuzu_query_result_destroy(&result);
	kuzu_prepared_statement_destroy(&stmt);

	return result_array;
}

int KuzuGD::query_columns_count(const String &query) {
	kuzu_query_result result;
	kuzu_state state = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	if (state != KuzuSuccess) {
		char *error_msg = kuzu_query_result_get_error_message(&result);

		UtilityFunctions::push_error("KuzuGD ERROR | Query Columns Count - " + String(error_msg));

		kuzu_query_result_destroy(&result);
		kuzu_destroy_string(error_msg);

		return -1;
	}

	int columnCount = kuzu_query_result_get_num_columns(&result);

	//Clean Up
	kuzu_query_result_destroy(&result);

	return columnCount;
}

String KuzuGD::query_column_name(const String &query, int colIndex) {
	kuzu_query_result result;
	kuzu_state stateSuccess = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	if (stateSuccess != KuzuSuccess) {
		char *error_msg = kuzu_query_result_get_error_message(&result);

		UtilityFunctions::push_error("KuzuGD ERROR | Query Columns Count - " + String(error_msg));

		kuzu_query_result_destroy(&result);
		kuzu_destroy_string(error_msg);

		return "UNDEFINED";
	}

	char **column_name;
	kuzu_state stateSuccess2 = kuzu_query_result_get_column_name(&result, colIndex, column_name);

	if (stateSuccess2 != KuzuSuccess) {
		UtilityFunctions::push_error("KuzuGD ERROR | Query Columns Count - Column Name NOT FOUND");

		kuzu_query_result_destroy(&result);

		return "UNDEFINED";
	}

	String gd_column_name = String::utf8(*column_name);

	//Clean Up
	kuzu_query_result_destroy(&result);
	kuzu_destroy_string(*column_name);

	return gd_column_name;
}

String KuzuGD::query_column_data_type(const String &query, int colIndex) {
	kuzu_query_result result;
	kuzu_state stateSuccess = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	if (stateSuccess != KuzuSuccess) {
		char *error_msg = kuzu_query_result_get_error_message(&result);

		UtilityFunctions::push_error("KuzuGD ERROR | Query Column Data Type - " + String(error_msg));

		kuzu_query_result_destroy(&result);
		kuzu_destroy_string(error_msg);

		return "UNDEFINED";
	}

	kuzu_logical_type column_data_type;
	kuzu_state stateSuccess2 = kuzu_query_result_get_column_data_type(&result, colIndex, &column_data_type);

	if (stateSuccess2 != KuzuSuccess) {
		UtilityFunctions::push_error("KuzuGD ERROR | Query Column Data Type - Column Name NOT FOUND");

		kuzu_query_result_destroy(&result);

		return "UNDEFINED";
	}

	String column_type;

	switch (kuzu_data_type_get_id(&column_data_type)) {
		case KUZU_ANY:
			column_type = "ANY";
		case KUZU_NODE:
			column_type = "NODE";
		case KUZU_REL:
			column_type = "REL";
		case KUZU_RECURSIVE_REL:
			column_type = "RECURSIVE_REL";
		case KUZU_SERIAL:
			column_type = "SERIAL";
		case KUZU_BOOL:
			column_type = "BOOL";
		case KUZU_INT64:
			column_type = "INT64";
		case KUZU_INT32:
			column_type = "INT32";
		case KUZU_INT16:
			column_type = "INT16";
		case KUZU_INT8:
			column_type = "INT8";
		case KUZU_UINT64:
			column_type = "UINT64";
		case KUZU_UINT32:
			column_type = "UINT32";
		case KUZU_UINT16:
			column_type = "UINT16";
		case KUZU_UINT8:
			column_type = "UINT8";
		case KUZU_INT128:
			column_type = "INT128";
		case KUZU_DOUBLE:
			column_type = "DOUBLE";
		case KUZU_FLOAT:
			column_type = "FLOAT";
		case KUZU_DATE:
			column_type = "DATE";
		case KUZU_TIMESTAMP:
			column_type = "TIMESTAMP";
		case KUZU_TIMESTAMP_SEC:
			column_type = "TIMESTAMP_SEC";
		case KUZU_TIMESTAMP_MS:
			column_type = "TIMESTAMP_MS";
		case KUZU_TIMESTAMP_NS:
			column_type = "TIMESTAMP_NS";
		case KUZU_TIMESTAMP_TZ:
			column_type = "TIMESTAMP_TZ";
		case KUZU_INTERVAL:
			column_type = "INTERVAL";
		case KUZU_DECIMAL:
			column_type = "DECIMAL";
		case KUZU_INTERNAL_ID:
			column_type = "INTERNAL_ID";
		case KUZU_STRING:
			column_type = "STRING";
		case KUZU_BLOB:
			column_type = "BLOB";
		case KUZU_LIST:
			column_type = "LIST";
		case KUZU_ARRAY:
			column_type = "ARRAY";
		case KUZU_STRUCT:
			column_type = "STRUCT";
		case KUZU_MAP:
			column_type = "MAP";
		case KUZU_UNION:
			column_type = "UNION";
		case KUZU_POINTER:
			column_type = "POINTER";
		case KUZU_UUID:
			column_type = "UUID";
		default:
			column_type = "UNDEFINED";
	}

	//Clean Up
	kuzu_query_result_destroy(&result);

	return column_type;
}

int KuzuGD::query_num_tuples(const String &query) {
	kuzu_query_result result;
	kuzu_state stateSuccess = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	if (stateSuccess != KuzuSuccess) {
		char *error_msg = kuzu_query_result_get_error_message(&result);

		UtilityFunctions::push_error("KuzuGD ERROR | Query Tuple Count - " + String(error_msg));

		//CleanUp
		kuzu_query_result_destroy(&result);
		kuzu_destroy_string(error_msg);

		return 0;
	}

	int tupleCount = kuzu_query_result_get_num_tuples(&result);

	//Clean Up
	kuzu_query_result_destroy(&result);

	return tupleCount;
}

float KuzuGD::query_summary_compile_time(const String &query) {
	kuzu_query_result result;
	kuzu_query_summary out_query_summary;

	kuzu_state stateSuccess = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	if (stateSuccess != KuzuSuccess) {
		char *error_msg = kuzu_query_result_get_error_message(&result);

		UtilityFunctions::push_error("KuzuGD ERROR | Query Summary Compile Time - " + String(error_msg));

		//CleanUp
		kuzu_destroy_string(error_msg);
		kuzu_query_result_destroy(&result);
		kuzu_query_summary_destroy(&out_query_summary);

		return 0;
	}

	kuzu_state stateSuccess2 = kuzu_query_result_get_query_summary(&result, &out_query_summary);

	if (stateSuccess2 != KuzuSuccess) {
		UtilityFunctions::push_error("KuzuGD ERROR | Query Summary Compile Time - Get Query Summary FAILED");

		//CleanUp
		kuzu_query_result_destroy(&result);
		kuzu_query_summary_destroy(&out_query_summary);

		return 0;
	}

	//Clean Up
	kuzu_query_result_destroy(&result);
	kuzu_query_summary_destroy(&out_query_summary);

	return kuzu_query_summary_get_compiling_time(&out_query_summary);
}

float KuzuGD::query_summary_execution_time(const String &query) {
	kuzu_query_result result;
	kuzu_query_summary out_query_summary;

	kuzu_state stateSuccess = kuzu_connection_query(dbConnection, query.utf8().get_data(), &result);

	if (stateSuccess != KuzuSuccess) {
		char *error_msg = kuzu_query_result_get_error_message(&result);

		UtilityFunctions::push_error("KuzuGD ERROR | Query Summary Execution Time - " + String(error_msg));

		//CleanUp
		kuzu_destroy_string(error_msg);
		kuzu_query_result_destroy(&result);
		kuzu_query_summary_destroy(&out_query_summary);

		return 0;
	}

	kuzu_state stateSuccess2 = kuzu_query_result_get_query_summary(&result, &out_query_summary);

	if (stateSuccess2 != KuzuSuccess) {
		UtilityFunctions::push_error("KuzuGD ERROR | Query Summary Execution Time - Get Query Summary FAILED");

		//CleanUp
		kuzu_query_result_destroy(&result);
		kuzu_query_summary_destroy(&out_query_summary);

		return 0;
	}

	//Clean Up
	kuzu_query_result_destroy(&result);
	kuzu_query_summary_destroy(&out_query_summary);

	return kuzu_query_summary_get_execution_time(&out_query_summary);
}

/******************************************************************


	Helper Functions


******************************************************************/

bool KuzuGD::is_date(const string &value) {
	regex date_regex(R"(^\d{4}-\d{2}-\d{2}$)");
	return regex_match(value, date_regex);
}

bool KuzuGD::is_timestamp(const string &value) {
	regex timestamp_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
	return regex_match(value, timestamp_regex);
}

bool KuzuGD::is_timestamp_tz(const string &value) {
	regex timestamp_tz_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} [+-]\d{2}:\d{2}$)");
	return regex_match(value, timestamp_tz_regex);
}

bool KuzuGD::is_timestamp_ns(const string &value) {
	regex timestamp_ns_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{9}$)");
	return regex_match(value, timestamp_ns_regex);
}

bool KuzuGD::is_interval(const string &value) {
	regex interval_regex(R"(^\d+ (YEAR|MONTH|DAY|HOUR|MINUTE|SECOND|MICROSECOND)(, .*)?$)");
	return regex_match(value, interval_regex);
}

void KuzuGD::string_to_tm(const string &time_str, ParsedTime &timeStruct) {
	regex date_regex(R"(^\d{4}-\d{2}-\d{2}$)");
	regex timestamp_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$)");
	regex timestamp_ns_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.(\d{1,9})$)");
	regex timestamp_tz_regex(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} ([+-]\d{2}:\d{2})$)");

	tm out_tm = {};
	int64_t tz_offset_microseconds = 0, nanoseconds = 0;
	istringstream ss(time_str);
	string tz_offset_str, nano_str;
	char dash1, dash2, space;

	if (regex_match(time_str, date_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday;
		out_tm.tm_hour = 0;
		out_tm.tm_min = 0;
		out_tm.tm_sec = 0;
	} else if (regex_match(time_str, timestamp_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday >> space >> out_tm.tm_hour >> dash1 >> out_tm.tm_min >> dash2 >> out_tm.tm_sec;
	} else if (regex_match(time_str, timestamp_ns_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday >> space >> out_tm.tm_hour >> dash1 >> out_tm.tm_min >> dash2 >> out_tm.tm_sec >> space >> nano_str;

		// Convert nanoseconds
		while (nano_str.length() < 9)
			nano_str += "0"; // Normalize to 9 digits
		nanoseconds = stoll(nano_str);
	} else if (regex_match(time_str, timestamp_tz_regex)) {
		ss >> out_tm.tm_year >> dash1 >> out_tm.tm_mon >> dash2 >> out_tm.tm_mday >> space >> out_tm.tm_hour >> dash1 >> out_tm.tm_min >> dash2 >> out_tm.tm_sec >> space >> tz_offset_str;
		char sign = tz_offset_str[0];
		int tz_hour = stoi(tz_offset_str.substr(1, 2));
		int tz_minute = stoi(tz_offset_str.substr(4, 2));

		tz_offset_microseconds = (tz_hour * 3600 + tz_minute * 60) * 1000000;
		if (sign == '-') {
			tz_offset_microseconds = -tz_offset_microseconds;
		}
	} else {
		UtilityFunctions::push_error("string_to_tm -> Invalid Time Format!");
	}

	// Adjust for `tm` structure
	out_tm.tm_year -= 1900;
	out_tm.tm_mon -= 1;

	timeStruct = { out_tm, tz_offset_microseconds, nanoseconds };
}

kuzu_interval_t KuzuGD::string_to_kuzu_interval(const string &interval_str) {
	int32_t months = 0, days = 0;
	int64_t micros = 0;
	istringstream ss(interval_str);
	string token;

	while (getline(ss, token, ',')) {
		istringstream part(token);
		int value;
		string unit;

		part >> value >> unit;

		if (unit.find("year") != string::npos) {
			months += value * 12; // Convert years to months
		} else if (unit.find("month") != string::npos) {
			months += value;
		} else if (unit.find("day") != string::npos) {
			days += value;
		} else if (unit.find("microsecond") != string::npos) {
			micros += value;
		} else if (unit.find("millisecond") != string::npos) {
			micros += value * 1000;
		} else if (unit.find("second") != string::npos) {
			micros += value * 1000000;
		}
	}

	return kuzu_interval_t{ months, days, micros };
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

	ClassDB::bind_method(D_METHOD("execute_query", "query"), &KuzuGD::execute_query);

	ClassDB::bind_method(D_METHOD("execute_prepared_query", "query", "params"), &KuzuGD::execute_prepared_query);

	ClassDB::bind_method(D_METHOD("query_columns_count", "query"), &KuzuGD::query_columns_count);

	ClassDB::bind_method(D_METHOD("query_column_name", "query", "colIndex"), &KuzuGD::query_column_name);

	ClassDB::bind_method(D_METHOD("query_column_data_type", "query", "colIndex"), &KuzuGD::query_column_data_type);

	ClassDB::bind_method(D_METHOD("query_num_tuples", "query"), &KuzuGD::query_num_tuples);

	ClassDB::bind_method(D_METHOD("query_summary_compile_time", "query"), &KuzuGD::query_summary_compile_time);

	ClassDB::bind_method(D_METHOD("query_summary_execution_time", "query"), &KuzuGD::query_summary_execution_time);
}