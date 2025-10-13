# jsoncpp API Reference for ConfigManager Conversion

## Common Method Conversions

### nlohmann::json → jsoncpp equivalents:

**Type Checking:**
- `json.is_string()` → `json.isString()`
- `json.is_number()` → `json.isNumeric()`
- `json.is_number_integer()` → `json.isInt()` or `json.isInt64()`
- `json.is_number_float()` → `json.isDouble()`
- `json.is_boolean()` → `json.isBool()`
- `json.is_array()` → `json.isArray()`
- `json.is_object()` → `json.isObject()`

**Value Access:**
- `json.get<std::string>()` → `json.asString()`
- `json.get<int>()` → `json.asInt()`
- `json.get<double>()` → `json.asDouble()`
- `json.get<bool>()` → `json.asBool()`

**Object/Array Operations:**
- `json.contains(key)` → `json.isMember(key)`
- `json.push_back(value)` → `json.append(value)`
- `json.dump()` → No direct equivalent, use Json::StreamWriterBuilder
- `json::array()` → `Json::Value(Json::arrayValue)`
- `json::object()` → `Json::Value(Json::objectValue)`

**Exception Handling:**
- `json::type_error` → No direct equivalent, check types manually
- `json::out_of_range` → No direct equivalent, check bounds manually

**Iteration:**
- `for (auto& [key, value] : json.items())` → `for (auto it = json.begin(); it != json.end(); ++it)`
- `it.key()` → `it.key()` (same)
- `it.value()` → `*it`