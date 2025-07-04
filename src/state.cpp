#include <cstdint>
#include <state.hpp>
#include <jsonpath_grammar.hpp>

namespace thalamus {

static void THALAMUS_ASSERT(bool v, const std::string& message) {
  if(!v) {
    std::cerr << message << std::endl;
    std::abort();
  }
}

[[noreturn]] static void THALAMUS_FAIL(const std::string& message) {
  std::cerr << message << std::endl;
  std::abort();
}
[[noreturn]] static void THALAMUS_ABORT(const std::string& message) {
  std::cerr << message << std::endl;
  std::abort();
}

#include <iostream>

struct Logger {
  ~Logger() {
    std::cout << std::endl;
  }
};

template<typename T>
std::ostream& operator<<(const Logger&, T t) {
  std::cout << t;
  return std::cout;
}
#define THALAMUS_LOG(c) Logger()

ObservableCollection::~ObservableCollection() {}

ObservableCollection::ValueWrapper::ValueWrapper(
    const Key &_key, std::function<Value &()> _get_value,
    std::function<bool()> _has_value, ObservableCollection *_collection)
    : key(_key), get_value(_get_value), has_value(_has_value),
      collection(_collection) {}

ObservableCollection::ValueWrapper::operator ObservableDictPtr() {
  auto value = get_value();
  if (std::holds_alternative<ObservableDictPtr>(value)) {
    return std::get<ObservableDictPtr>(value);
  } else {
    THALAMUS_FAIL("Value is not a dict");
  }
}

ObservableCollection::ValueWrapper::operator ObservableListPtr() {
  auto value = get_value();
  if (std::holds_alternative<ObservableListPtr>(value)) {
    return std::get<ObservableListPtr>(value);
  } else {
    THALAMUS_FAIL("Value is not a list");
  }
}

ObservableCollection::ValueWrapper::operator long long int() {
  auto value = get_value();
  if (std::holds_alternative<long long int>(value)) {
    return std::get<long long int>(value);
  } else if (std::holds_alternative<double>(value)) {
    return int64_t(std::get<double>(value));
  } else {
    THALAMUS_FAIL("Value is not a number");
  }
}
ObservableCollection::ValueWrapper::operator unsigned long long int() {
  auto value = get_value();
  if (std::holds_alternative<long long int>(value)) {
    return uint64_t(std::get<long long int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return uint64_t(std::get<double>(value));
  } else {
    THALAMUS_FAIL("Value is not a number");
  }
}
ObservableCollection::ValueWrapper::operator unsigned long() {
  auto value = get_value();
  if (std::holds_alternative<long long int>(value)) {
    return uint32_t(std::get<long long int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return uint32_t(std::get<double>(value));
  } else {
    THALAMUS_FAIL("Value is not a number");
  }
}
ObservableCollection::ValueWrapper::operator double() {
  auto value = get_value();
  if (std::holds_alternative<long long int>(value)) {
    return double(std::get<long long int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  } else {
    THALAMUS_FAIL("Value is not a number");
  }
}
ObservableCollection::ValueWrapper::operator bool() {
  auto value = get_value();
  if (std::holds_alternative<long long int>(value)) {
    return std::get<long long int>(value) != 0;
  } else if (std::holds_alternative<double>(value)) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
    return std::get<double>(value) != 0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  } else if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value);
  } else {
    THALAMUS_FAIL("Value is not a bool or number");
  }
}
ObservableCollection::ValueWrapper::operator std::string() {
  auto value = get_value();
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else {
    THALAMUS_FAIL("Value is not a string");
  }
}
ObservableCollection::ValueWrapper::operator ObservableCollection::Value() {
  return get_value();
}
ObservableCollection::Value ObservableCollection::ValueWrapper::get() {
  return get_value();
}
bool ObservableCollection::ValueWrapper::operator==(const Value &other) {
  return get_value() == other;
}

ObservableCollection::VectorIteratorWrapper::VectorIteratorWrapper()
    : key(0), iterator(), end(), collection(nullptr) {}
ObservableCollection::VectorIteratorWrapper::VectorIteratorWrapper(
    size_t _key, Vector::iterator _iterator, Vector::iterator _end,
    ObservableCollection *_collection)
    : key(_key), iterator(_iterator), end(_end), collection(_collection) {}

ObservableCollection::ValueWrapper&
ObservableCollection::VectorIteratorWrapper::operator*() {
  auto this_iterator = this->iterator;
  auto this_end = this->end;
  value_wrapper = ValueWrapper(
      static_cast<long long int>(key),
      [this_iterator]() -> Value & { return *this_iterator; },
      [this_iterator, this_end]() -> bool { return this_iterator != this_end; }, collection);
  return *value_wrapper;
}

ObservableCollection::VectorIteratorWrapper &
ObservableCollection::VectorIteratorWrapper::operator+(size_t count) {
  key += count;
  iterator += int64_t(count);
  return *this;
}

ObservableCollection::VectorIteratorWrapper &
ObservableCollection::VectorIteratorWrapper::operator+=(size_t count) {
  return *this + count;
}

ObservableCollection::VectorIteratorWrapper &
ObservableCollection::VectorIteratorWrapper::operator++() {
  return *this += 1;
}

ObservableCollection::VectorIteratorWrapper
ObservableCollection::VectorIteratorWrapper::operator++(int) {
  auto new_wrapper = *this;
  ++*this;
  return new_wrapper;
}

ObservableCollection::VectorIteratorWrapper &
ObservableCollection::VectorIteratorWrapper::operator-(size_t count) {
  return *this + -count;
}

ObservableCollection::VectorIteratorWrapper &
ObservableCollection::VectorIteratorWrapper::operator-=(size_t count) {
  return *this - count;
}

ObservableCollection::VectorIteratorWrapper &
ObservableCollection::VectorIteratorWrapper::operator--() {
  return *this -= 1;
}

ObservableCollection::VectorIteratorWrapper
ObservableCollection::VectorIteratorWrapper::operator--(int) {
  auto new_wrapper = *this;
  --*this;
  return new_wrapper;
}

bool ObservableCollection::VectorIteratorWrapper::operator!=(
    const VectorIteratorWrapper &other) const {
  return iterator != other.iterator;
}

ObservableCollection::MapIteratorWrapper::MapIteratorWrapper()
    : iterator(), end(), collection(nullptr) {}

ObservableCollection::MapIteratorWrapper::MapIteratorWrapper(
    Map::iterator _iterator, Map::iterator _end,
    ObservableCollection *_collection)
    : iterator(_iterator), end(_end), collection(_collection) {}

std::pair<ObservableCollection::Key, ObservableCollection::ValueWrapper>&
ObservableCollection::MapIteratorWrapper::operator*() {
  auto this_iterator = this->iterator;
  auto this_end = this->end;
  pair = std::make_pair(
      this_iterator->first,
      ValueWrapper(
          this_iterator->first,
          [this_iterator]() -> Value & { return this_iterator->second; },
          [this_iterator, this_end]() -> bool { return this_iterator != this_end; },
          collection));
  return pair.value();
}

std::pair<ObservableCollection::Key, ObservableCollection::ValueWrapper> *
ObservableCollection::MapIteratorWrapper::operator->() {
  return &(**this);
}

ObservableCollection::MapIteratorWrapper &
ObservableCollection::MapIteratorWrapper::operator++() {
  ++iterator;
  return *this;
}

ObservableCollection::MapIteratorWrapper
ObservableCollection::MapIteratorWrapper::operator++(int) {
  auto new_wrapper = *this;
  ++*this;
  return new_wrapper;
}

ObservableCollection::MapIteratorWrapper &
ObservableCollection::MapIteratorWrapper::operator--() {
  --iterator;
  return *this;
}

ObservableCollection::MapIteratorWrapper
ObservableCollection::MapIteratorWrapper::operator--(int) {
  auto new_wrapper = *this;
  --*this;
  return new_wrapper;
}

bool ObservableCollection::MapIteratorWrapper::operator!=(
    const MapIteratorWrapper &other) const {
  return iterator != other.iterator;
}

ObservableCollection::ObservableCollection(ObservableCollection *_parent)
    : parent(_parent) {}

std::string ObservableCollection::address() const {
  if (!parent) {
    return "";
  }
  auto prefix = parent->address();
  auto end_opt = parent->key_of(*this);
  THALAMUS_ASSERT(end_opt.has_value(),
                  "Failed to find self in parent collection");
  auto end = *end_opt;
  if (std::holds_alternative<long long int>(end)) {
    return absl::StrFormat("%s[%d]", prefix, std::get<long long int>(end));
  } else if (std::holds_alternative<std::string>(end)) {
    if (prefix.empty()) {
      return absl::StrFormat("['%s']", std::get<std::string>(end));
    } else {
      return absl::StrFormat("%s['%s']", prefix,
                             std::get<std::string>(end));
    }
  } else {
    THALAMUS_FAIL("Unsupported key type");
  }
}

void ObservableCollection::notify(ObservableCollection *source, Action action,
                                  const Key &key, Value &value) {
  if (source == this) {
    changed(action, key, value);
  }

  recursive_changed(source, action, key, value);

  if (parent) {
    parent->notify(source, action, key, value);
  }
}

ObservableList::ObservableList(ObservableCollection *_parent)
    : ObservableCollection(_parent), content(Vector()) {}

ObservableList::ValueWrapper ObservableList::operator[](size_t i) {
  return ValueWrapper(
      static_cast<long long int>(i),
      [this, i]() -> Value & { return content[i]; },
      [this, i]() -> bool { return i < content.size(); }, this);
}

const ObservableList::Value &ObservableList::operator[](size_t i) const {
  return content[i];
}

ObservableList::ValueWrapper ObservableList::at(size_t i) {
  return ValueWrapper(
      static_cast<long long int>(i),
      [this, i]() -> Value & { return content.at(i); },
      [this, i]() -> bool { return i < content.size(); }, this);
}

const ObservableList::Value &ObservableList::at(size_t i) const {
  return content.at(i);
}

ObservableList::VectorIteratorWrapper ObservableList::begin() {
  return VectorIteratorWrapper(0, content.begin(), content.end(), this);
}

ObservableList::Vector::const_iterator ObservableList::begin() const {
  return content.begin();
}

ObservableList::VectorIteratorWrapper ObservableList::end() {
  return VectorIteratorWrapper(content.size(), content.end(), content.end(),
                               this);
}

ObservableList::Vector::const_iterator ObservableList::end() const {
  return content.end();
}

ObservableList::Vector::const_iterator ObservableList::cend() const {
  return content.end();
}

ObservableList::VectorIteratorWrapper
ObservableList::erase(VectorIteratorWrapper i) {
  return erase(i.iterator);
}

ObservableList::VectorIteratorWrapper
ObservableList::erase(Vector::const_iterator i,
                      std::function<void(VectorIteratorWrapper)> callback,
                      bool from_remote) {
  auto key = std::distance(content.cbegin(), i);
  if (!callback) {
    callback = [](VectorIteratorWrapper) {};
  }

  if (!from_remote && this->remote_storage) {
    auto callback_wrapper = [this, key, callback] {
      auto key_after = std::min(key, ptrdiff_t(content.size()));
      callback(VectorIteratorWrapper(
          size_t(key_after), content.begin() + key_after, content.end(), this));
    };
    if (this->remote_storage(Action::Delete,
                             address() + "[" + std::to_string(key) + "]", *i,
                             callback_wrapper)) {
      return VectorIteratorWrapper();
    }
  }

  if (std::holds_alternative<ObservableListPtr>(*i)) {
    auto temp = std::get<ObservableListPtr>(*i);
    temp->set_remote_storage(decltype(this->remote_storage)());
  } else if (std::holds_alternative<ObservableDictPtr>(*i)) {
    auto temp = std::get<ObservableDictPtr>(*i);
    temp->set_remote_storage(decltype(this->remote_storage)());
  }

  auto value = *i;
  auto i2 = content.erase(i);
  notify(this, Action::Delete, key, value);

  auto distance = std::distance(content.begin(), i2);
  return VectorIteratorWrapper(size_t(distance), i2, content.end(), this);
}

ObservableList::VectorIteratorWrapper
ObservableList::erase(size_t i,
                      std::function<void(VectorIteratorWrapper)> callback,
                      bool from_remote) {
  return erase(this->content.begin() + int64_t(i), callback, from_remote);
}

void ObservableList::clear() {
  while (!content.empty()) {
    pop_back();
  }
}

void ObservableList::recap() {
  long long int i = 0;
  for (auto &v : content) {
    notify(this, Action::Set, i++, v);
  }
}

void ObservableList::recap(Observer target) {
  long long int i = 0;
  for (auto &v : content) {
    target(Action::Set, i++, v);
  }
}

size_t ObservableList::size() const { return content.size(); }

bool ObservableList::empty() const { return content.empty(); }

ObservableList &ObservableList::operator=(const boost::json::array &that) {
  ObservableList temp(that);
  return this->assign(temp);
}

std::optional<ObservableCollection::Key>
ObservableList::key_of(const ObservableCollection &v) const {
  auto i = 0;
  for (const auto &our_value : content) {
    if (std::holds_alternative<ObservableListPtr>(our_value)) {
      auto temp = std::get<ObservableListPtr>(our_value);
      auto temp2 = std::static_pointer_cast<ObservableCollection>(temp);
      if (temp2.get() == &v) {
        return i;
      }
    } else if (std::holds_alternative<ObservableDictPtr>(our_value)) {
      auto temp = std::get<ObservableDictPtr>(our_value);
      auto temp2 = std::static_pointer_cast<ObservableCollection>(temp);
      if (temp2.get() == &v) {
        return i;
      }
    }
    ++i;
  }
  return std::nullopt;
}

void ObservableList::push_back(const Value &value,
                               std::function<void()> callback,
                               bool from_remote) {
  if (!callback) {
    callback = [] {};
  }

  if (!from_remote && remote_storage) {
    if (this->remote_storage(
            Action::Set, address() + "[" + std::to_string(content.size()) + "]",
            value, callback)) {
      return;
    }
  }

  if (std::holds_alternative<ObservableDictPtr>(value)) {
    std::get<ObservableDictPtr>(value)->parent = this;
    std::get<ObservableDictPtr>(value)->set_remote_storage(remote_storage);
  } else if (std::holds_alternative<ObservableListPtr>(value)) {
    std::get<ObservableListPtr>(value)->parent = this;
    std::get<ObservableListPtr>(value)->set_remote_storage(remote_storage);
  }
  content.push_back(value);
  notify(this, Action::Set, static_cast<long long>(content.size() - 1),
         content.back());
}

void ObservableList::pop_back(std::function<void()> callback,
                              bool from_remote) {
  if (!callback) {
    callback = [] {};
  }

  auto value = content.back();
  if (!from_remote && remote_storage) {
    if (this->remote_storage(Action::Delete,
                             address() + "[" +
                                 std::to_string(content.size() - 1) + "]",
                             value, callback)) {
      return;
    }
  }

  if (std::holds_alternative<ObservableDictPtr>(value)) {
    std::get<ObservableDictPtr>(value)->parent = nullptr;
  } else if (std::holds_alternative<ObservableListPtr>(value)) {
    std::get<ObservableListPtr>(value)->parent = nullptr;
  }
  content.pop_back();
  notify(this, Action::Delete, static_cast<long long>(content.size()), value);
}

void ObservableCollection::ValueWrapper::assign(const Value &new_value,
                                                std::function<void()> callback,
                                                bool from_remote) {
  if (!callback) {
    callback = [] {};
  }

  if (has_value()) {
    auto &value = get_value();
    if (value == new_value) {
      callback();
      return;
    }
  }

  if (!from_remote && collection->remote_storage) {
    auto address = collection->address();
    if (std::holds_alternative<std::string>(key)) {
      address += "['" + to_string(key) + "']";
    } else {
      address += "[" + to_string(key) + "]";
    }

    if (collection->remote_storage(Action::Set, address, new_value, callback)) {
      return;
    }
  }

  auto &value = get_value();
  value = new_value;
  if (std::holds_alternative<ObservableDictPtr>(value)) {
    std::get<ObservableDictPtr>(value)->parent = collection;
    std::get<ObservableDictPtr>(value)->set_remote_storage(
        collection->remote_storage);
  } else if (std::holds_alternative<ObservableListPtr>(value)) {
    std::get<ObservableListPtr>(value)->parent = collection;
    std::get<ObservableListPtr>(value)->set_remote_storage(
        collection->remote_storage);
  }
  collection->notify(collection, Action::Set, key, value);
  return;
}

ObservableList &ObservableList::assign(const ObservableList &that,
                                       bool from_remote) {
  for (auto i = 0ull; i < that.content.size(); ++i) {
    auto &source = that.content[i];
    if (i >= this->size()) {
      this->content.emplace_back();
    }
    auto target = (*this)[i];
    ObservableCollection::Value target_value = target;
    if (target_value.index() == source.index()) {
      if (std::holds_alternative<ObservableDictPtr>(target_value)) {
        ObservableDictPtr target_dict = target;
        target_dict->assign(*std::get<ObservableDictPtr>(source));
        continue;
      } else if (std::holds_alternative<ObservableListPtr>(target_value)) {
        ObservableListPtr target_list = target;
        target_list->assign(*std::get<ObservableListPtr>(source));
        continue;
      }
    }

    (*this)[i].assign(that.at(i), [] {}, from_remote);
  }
  for (auto i = that.content.size(); i < content.size(); ++i) {
    this->erase(i, nullptr, from_remote);
  }
  return *this;
}

ObservableList::ObservableList(const boost::json::array &that) {
  ObservableDictPtr dict;
  ObservableListPtr list;
  for (auto &v : that) {
    switch (v.kind()) {
    case boost::json::kind::object: {
      dict = std::make_shared<ObservableDict>(v.as_object());
      dict->parent = this;
      dict->set_remote_storage(remote_storage);
      content.push_back(dict);
      break;
    }
    case boost::json::kind::array: {
      list = std::make_shared<ObservableList>(v.as_array());
      list->parent = this;
      list->set_remote_storage(remote_storage);
      content.push_back(list);
      break;
    }
    case boost::json::kind::string: {
      content.push_back(std::string(v.as_string()));
      break;
    }
    case boost::json::kind::uint64: {
      content.push_back(static_cast<long long>(v.as_uint64()));
      break;
    }
    case boost::json::kind::int64: {
      content.push_back(v.as_int64());
      break;
    }
    case boost::json::kind::double_: {
      content.push_back(v.as_double());
      break;
    }
    case boost::json::kind::bool_: {
      content.push_back(v.as_bool());
      break;
    }
    case boost::json::kind::null: {
      content.push_back(std::monostate());
      break;
    }
    }
  }
}

ObservableList::operator boost::json::array() const {
  boost::json::array result;
  for (const auto &value : content) {
    // std::shared_ptr<ObservableDict>, std::shared_ptr<ObservableList>
    if (std::holds_alternative<long long int>(value)) {
      result.emplace_back(std::get<long long int>(value));
    } else if (std::holds_alternative<double>(value)) {
      result.emplace_back(std::get<double>(value));
    } else if (std::holds_alternative<bool>(value)) {
      result.emplace_back(std::get<bool>(value));
    } else if (std::holds_alternative<std::string>(value)) {
      result.emplace_back(std::get<std::string>(value));
    } else if (std::holds_alternative<std::shared_ptr<ObservableDict>>(value)) {
      result.emplace_back(boost::json::object(
          *std::get<std::shared_ptr<ObservableDict>>(value)));
    } else if (std::holds_alternative<std::shared_ptr<ObservableList>>(value)) {
      result.emplace_back(boost::json::array(
          *std::get<std::shared_ptr<ObservableList>>(value)));
    }
  }
  return result;
}

ObservableCollection::Value
ObservableCollection::from_json(const boost::json::value &value) {
  switch (value.kind()) {
  case boost::json::kind::object: {
    return std::make_shared<ObservableDict>(value.as_object());
  }
  case boost::json::kind::array: {
    return std::make_shared<ObservableList>(value.as_array());
  }
  case boost::json::kind::string: {
    return std::string(value.as_string());
  }
  case boost::json::kind::uint64: {
    return static_cast<long long>(value.as_uint64());
  }
  case boost::json::kind::int64: {
    return value.as_int64();
  }
  case boost::json::kind::double_: {
    return value.as_double();
  }
  case boost::json::kind::bool_: {
    return value.as_bool();
  }
  case boost::json::kind::null: {
    return std::monostate();
  }
  }
}
boost::json::value
ObservableCollection::to_json(const ObservableCollection::Value &value) {
  if (std::holds_alternative<long long int>(value)) {
    return std::get<long long int>(value);
  } else if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  } else if (std::holds_alternative<bool>(value)) {
    return std::get<bool>(value);
  } else if (std::holds_alternative<std::string>(value)) {
    return boost::json::string(std::get<std::string>(value));
  } else if (std::holds_alternative<std::shared_ptr<ObservableDict>>(value)) {
    boost::json::object result =
        *std::get<std::shared_ptr<ObservableDict>>(value);
    return result;
  } else if (std::holds_alternative<std::shared_ptr<ObservableList>>(value)) {
    boost::json::array result =
        *std::get<std::shared_ptr<ObservableList>>(value);
    return result;
  }
  return boost::json::value();
}
std::string
ObservableCollection::to_string(const ObservableCollection::Value &value) {
  if (std::holds_alternative<long long int>(value)) {
    return std::to_string(std::get<long long int>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::to_string(std::get<double>(value));
  } else if (std::holds_alternative<bool>(value)) {
    return std::to_string(std::get<bool>(value));
  } else if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<std::shared_ptr<ObservableDict>>(value)) {
    boost::json::object result =
        *std::get<std::shared_ptr<ObservableDict>>(value);
    return boost::json::serialize(result);
  } else if (std::holds_alternative<std::shared_ptr<ObservableList>>(value)) {
    boost::json::array result =
        *std::get<std::shared_ptr<ObservableList>>(value);
    return boost::json::serialize(result);
  }
  THALAMUS_FAIL("Failed to convert value to string");
}
std::string
ObservableCollection::to_string(const ObservableCollection::Key &value) {
  if (std::holds_alternative<long long int>(value)) {
    return std::to_string(std::get<long long int>(value));
  } else if (std::holds_alternative<bool>(value)) {
    return std::to_string(std::get<bool>(value));
  } else if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  }
  THALAMUS_FAIL("Failed to convert key to string");
}

ObservableCollection::Value get_jsonpath(ObservableCollection::Value store,
                                         const std::list<std::string> &tokens) {
  ObservableCollection::Value current = store;
  for (auto &token : tokens) {
    if (std::holds_alternative<ObservableDictPtr>(current)) {
      const auto &held = std::get<ObservableDictPtr>(current);
      current = held->at(token);
    } else if (std::holds_alternative<ObservableListPtr>(current)) {
      const auto &held = std::get<ObservableListPtr>(current);
      size_t index;
      auto success = absl::SimpleAtoi(token, &index);
      THALAMUS_ASSERT(success, "Failed to convert index into number");
      current = held->at(index);
    } else {
      THALAMUS_ASSERT(false,
                       "Attempted to index something that isn't a collection");
    }
  }
  return current;
}

struct ToBoolVisitor {
  bool operator()(std::monostate) {
    return false;
  }
  bool operator()(long long int val) {
    return val == 0;
  }
  bool operator()(double val) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
    return val == 0;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  }
  bool operator()(bool val) {
    return val;
  }
  bool operator()(std::string val) {
    return !val.empty();
  }
  bool operator()(const ObservableDictPtr& val) {
    return val != nullptr;
  }
  bool operator()(const ObservableListPtr& val) {
    return val != nullptr;
  }
};

static bool to_bool(ObservableCollection::Value v) {
  return std::visit(ToBoolVisitor{}, v);
}

struct JsonpathVisitor {
  ObservableCollection::Value abs;
  ObservableCollection::Value rel;

  ObservableCollection::Value operator()(const Literal &logic) {
    if(std::holds_alternative<int>(logic)) {
      return std::get<int>(logic);
    } else {
      return std::get<std::string>(logic);
    }
  }

  ObservableCollection::Value operator()(const SingularQuery &query) {
    ObservableCollection::Value current = rel;
    for(const auto& segment : query) {
      if(std::holds_alternative<std::monostate>(current)) {
        return current;
      }
      current = std::visit(JsonpathVisitor{abs, current}, segment);
    }
    return current;
  }

  ObservableCollection::Value operator()(const Compare &logic) {
    auto lhs = std::visit(*this, logic.lhs);
    auto rhs = std::visit(*this, logic.rhs);
    if(logic.op == "==") {
      return lhs == rhs;
    } else if (logic.op == "!=") {
      return lhs != rhs;
    } else if (logic.op == "<=") {
      return lhs <= rhs;
    } else if (logic.op == ">=") {
      return lhs >= rhs;
    } else {
      return std::monostate();
    }
  }

  ObservableCollection::Value operator()(const LogicalAnd &logic) {
    std::vector<ObservableCollection::Value> values;
    auto handler = [&] (ObservableCollection::Value value) {
      auto result = true;
      for(auto expr : logic.expressions) {
        result = result && to_bool((*this)(expr));
        if(!result) {
          break;
        }
      }
      if(result) {
        values.push_back(value);
      }
    };
    handler(rel);
    return values.empty() ? std::monostate() : values.front();
  }

  ObservableCollection::Value operator()(const LogicalOr &logic) {
    std::vector<ObservableCollection::Value> values;
    auto handler = [&] (ObservableCollection::Value value) {
      auto result = false;
      for(auto expr : logic.expressions) {
        result = result || to_bool(JsonpathVisitor{abs, value}(expr));
        if(result) {
          break;
        }
      }
      if(result) {
        values.push_back(value);
      }
    };
    if (std::holds_alternative<ObservableDictPtr>(rel)) {
      auto held = std::get<ObservableDictPtr>(rel);
      for(auto i = held->begin();i != held->end();++i) {
        handler(i->second);
      }
    } else if (std::holds_alternative<ObservableListPtr>(rel)) {
      auto held = std::get<ObservableListPtr>(rel);
      for(auto i = held->begin();i != held->end();++i) {
        handler(*i);
      }
    }
    return values.empty() ? std::monostate() : values.front();
  }

  ObservableCollection::Value operator()(const Selector &selector) {
    return std::visit(*this, selector.value);
  }

  ObservableCollection::Value operator()(int index) {
    if (std::holds_alternative<ObservableListPtr>(rel)) {
      const auto &held = std::get<ObservableListPtr>(rel);
      return held->size() > size_t(index) ? ObservableCollection::Value(held->at(size_t(index))) : std::monostate();
    } else if (std::holds_alternative<ObservableDictPtr>(rel)) {
      const auto &held = std::get<ObservableDictPtr>(rel);
      return held->contains(index) ? ObservableCollection::Value(held->at(index)) : std::monostate();
    } else {
      return std::monostate();
    }
  }

  ObservableCollection::Value operator()(const std::string &query) {
    if (std::holds_alternative<ObservableDictPtr>(rel)) {
      const auto &held = std::get<ObservableDictPtr>(rel);
      return held->contains(query) ? ObservableCollection::Value(held->at(query)) : std::monostate();
    } else {
      return std::monostate();
    }
  }

  ObservableCollection::Value operator()(const JsonpathQuery &query) {
    ObservableCollection::Value current = abs;
    for(const auto& segment : query.segments) {
      current = std::visit(JsonpathVisitor{abs, current}, segment.segment);
    }
    return current;
  }
};

static ObservableCollection::Value get_jsonpath(ObservableCollection::Value store,
                                         const JsonpathQuery &query) {
  return JsonpathVisitor{store, store}(query);
}

ObservableCollection::Value get_jsonpath(ObservableCollection::Value store,
                                         const std::string &query) {
  auto begin = query.begin();
  auto end = query.end();
  Parser<std::string::const_iterator> p;
  JsonpathQuery parsed;
  auto r = phrase_parse(begin, end, p, ascii::space_type(), parsed);
  if(r && begin == end) {
    return get_jsonpath(store, parsed);
  }
  return std::monostate();
}

void set_jsonpath(ObservableCollection::Value store, const std::string &query,
                  ObservableCollection::Value value, bool from_remote) {
  auto begin = query.begin();
  auto text_end = query.end();
  Parser<std::string::const_iterator> p;
  JsonpathQuery parsed;
  auto r = phrase_parse(begin, text_end, p, ascii::space_type(), parsed);
  if(!r || begin != text_end) {
    THALAMUS_LOG(error) << "Failed to parse JSON Path expression at " << std::string(begin, text_end);
    return;
  }

  auto& tokens = parsed.segments;
  ObservableCollection::Value current = store;
  ObservableCollection::Key end;
  if (!tokens.empty()) {
    auto end_segment = tokens.back();
    if(std::holds_alternative<Selector>(end_segment.segment)) {
      auto selector = std::get<Selector>(end_segment.segment);
      if(std::holds_alternative<int>(selector.value)) {
        end = std::get<int>(selector.value);
      } else if(std::holds_alternative<std::string>(selector.value)) {
        end = std::get<std::string>(selector.value);
      }
    } else if(std::holds_alternative<std::string>(end_segment.segment)) {
      end = std::get<std::string>(end_segment.segment);
    }
    tokens.pop_back();
    current = get_jsonpath(store, parsed);
  }

  if (std::holds_alternative<ObservableDictPtr>(current)) {
    auto held = std::get<ObservableDictPtr>(current);
    if (std::holds_alternative<std::monostate>(end)) {
      BOOST_ASSERT(std::holds_alternative<ObservableDictPtr>(value));
      auto unwrapped_value = std::get<ObservableDictPtr>(value);
      held->assign(*unwrapped_value, from_remote);
    } else {
      (*held)[end].assign(value, nullptr, from_remote);
    }
  } else if (std::holds_alternative<ObservableListPtr>(current)) {
    auto held = std::get<ObservableListPtr>(current);
    if (std::holds_alternative<std::monostate>(end)) {
      BOOST_ASSERT(std::holds_alternative<ObservableListPtr>(value));
      auto unwrapped_value = std::get<ObservableListPtr>(value);
      held->assign(*unwrapped_value, from_remote);
    } else {
      size_t index = size_t(std::get<long long int>(end));
      while (held->size() < index) {
        held->push_back(ObservableCollection::Value(), nullptr, from_remote);
      }
      if (held->size() == index) {
        held->push_back(value, nullptr, from_remote);
      } else {
        held->at(index).assign(value, nullptr, from_remote);
      }
    }
  } else {
    THALAMUS_ASSERT(false,
                     "Attempted to index something that isn't a collection");
  }
}

void delete_jsonpath(ObservableCollection::Value store,
                     const std::string &query, bool from_remote) {
  std::list<std::string> tokens =
      absl::StrSplit(query, absl::ByAnyChar("[].'\""), absl::SkipEmpty());
  THALAMUS_ASSERT(!tokens.empty(), "Cant delete root");
  ObservableCollection::Value current = store;
  std::string end = tokens.back();
  tokens.pop_back();
  current = get_jsonpath(store, tokens);

  if (std::holds_alternative<ObservableDictPtr>(current)) {
    auto held = std::get<ObservableDictPtr>(current);
    held->erase(end, [](auto) {}, from_remote);
  } else if (std::holds_alternative<ObservableListPtr>(current)) {
    auto held = std::get<ObservableListPtr>(current);
    size_t index;
    auto success = absl::SimpleAtoi(end, &index);
    THALAMUS_ASSERT(success, "Failed to convert index into number");
    held->erase(index, [](auto) {}, from_remote);
  } else {
    THALAMUS_ASSERT(false,
                     "Attempted to index something that isn't a collection");
  }
}

void ObservableList::set_remote_storage(
    std::function<bool(Action, const std::string &, ObservableCollection::Value,
                       std::function<void()>)>
        _remote_storage) {
  this->remote_storage = _remote_storage;
  for (auto &c : content) {
    if (std::holds_alternative<ObservableListPtr>(c)) {
      auto temp = std::get<ObservableListPtr>(c);
      temp->set_remote_storage(_remote_storage);
    } else if (std::holds_alternative<ObservableDictPtr>(c)) {
      auto temp = std::get<ObservableDictPtr>(c);
      temp->set_remote_storage(_remote_storage);
    }
  }
}

boost::json::value ObservableList::to_json() {
  boost::json::array result = *this;
  return result;
}

ObservableDict::ObservableDict(ObservableCollection *_parent)
    : ObservableCollection(_parent), content(Map()) {}

ObservableDict::ValueWrapper ObservableDict::operator[](const Key &i) {
  return ValueWrapper(
      i, [this, i]() -> Value & { return content[i]; },
      [this, i]() -> bool { return content.contains(i); }, this);
}

ObservableDict::ValueWrapper ObservableDict::at(const Key &i) {
  return ValueWrapper(
      i, [this, i]() -> Value & { return content.at(i); },
      [this, i]() -> bool { return content.contains(i); }, this);
}

const ObservableDict::Value &
ObservableDict::at(const ObservableDict::Key &i) const {
  return content.at(i);
}

bool ObservableDict::contains(const ObservableDict::Key &i) const {
  return content.contains(i);
}

ObservableDict::MapIteratorWrapper ObservableDict::begin() {
  return MapIteratorWrapper(content.begin(), content.end(), this);
}

ObservableDict::Map::const_iterator ObservableDict::begin() const {
  return content.begin();
}

ObservableDict::MapIteratorWrapper ObservableDict::end() {
  return MapIteratorWrapper(content.end(), content.end(), this);
}

ObservableDict::Map::const_iterator ObservableDict::end() const {
  return content.end();
}

ObservableDict::Map::const_iterator ObservableDict::cend() const {
  return content.end();
}

ObservableDict::MapIteratorWrapper ObservableDict::erase(MapIteratorWrapper i) {
  return erase(i.iterator);
}

ObservableDict::MapIteratorWrapper
ObservableDict::erase(Map::const_iterator i,
                      std::function<void(MapIteratorWrapper)> callback,
                      bool from_remote) {
  auto pair = *i;
  auto next_i = i;
  ++next_i;
  if (!callback) {
    callback = [](MapIteratorWrapper) {};
  }

  if (!from_remote && this->remote_storage) {
    std::function<void()> callback_wrapper;
    if (next_i == content.end()) {
      callback_wrapper = [callback] { callback(MapIteratorWrapper()); };
    } else {
      callback_wrapper = [this, next_pair = *next_i, callback] {
        callback(this->find(next_pair.first));
      };
    }
    auto address = this->address() + "['" + to_string(pair.first) + "']";
    if (this->remote_storage(Action::Delete, address, pair.second,
                             callback_wrapper)) {
      return MapIteratorWrapper();
    }
  }

  if (std::holds_alternative<ObservableListPtr>(i->second)) {
    auto temp = std::get<ObservableListPtr>(i->second);
    temp->set_remote_storage(decltype(this->remote_storage)());
  } else if (std::holds_alternative<ObservableDictPtr>(i->second)) {
    auto temp = std::get<ObservableDictPtr>(i->second);
    temp->set_remote_storage(decltype(this->remote_storage)());
  }

  auto i2 = content.erase(i);
  notify(this, Action::Delete, pair.first, pair.second);

  return MapIteratorWrapper(i2, content.end(), this);
}

ObservableDict::MapIteratorWrapper
ObservableDict::erase(const ObservableCollection::Key &i,
                      std::function<void(MapIteratorWrapper)> callback,
                      bool from_remote) {
  return erase(content.find(i), callback, from_remote);
}

ObservableDict::MapIteratorWrapper ObservableDict::find(const Key &i) {
  return MapIteratorWrapper(content.find(i), content.end(), this);
}

ObservableDict::Map::const_iterator ObservableDict::find(const Key &i) const {
  return content.find(i);
}

void ObservableDict::clear() {
  while (!content.empty()) {
    auto the_end = end();
    --the_end;
    erase(the_end);
  }
}

void ObservableDict::recap() {
  for (auto &i : content) {
    notify(this, Action::Set, i.first, i.second);
  }
}

void ObservableDict::recap(Observer target) {
  for (auto &i : content) {
    target(Action::Set, i.first, i.second);
  }
}

size_t ObservableDict::size() const { return content.size(); }

bool ObservableDict::empty() const { return content.empty(); }

ObservableDict &ObservableDict::assign(const ObservableDict &that,
                                       bool from_remote) {
  std::set<ObservableCollection::Key> missing;
  for (auto &i : content) {
    missing.insert(i.first);
  }
  for (auto &i : that.content) {
    missing.erase(i.first);
    auto target = (*this)[i.first];
    ObservableCollection::Value target_value = target;
    if (target_value.index() == i.second.index()) {
      if (std::holds_alternative<ObservableDictPtr>(target_value)) {
        ObservableDictPtr target_dict = target;
        target_dict->assign(*std::get<ObservableDictPtr>(i.second),
                            from_remote);
        continue;
      } else if (std::holds_alternative<ObservableListPtr>(target_value)) {
        ObservableListPtr target_list = target;
        target_list->assign(*std::get<ObservableListPtr>(i.second),
                            from_remote);
        continue;
      }
    }
    target.assign(i.second, [] {}, from_remote);
  }
  for (auto &i : missing) {
    this->erase(i, nullptr, from_remote);
  }
  return *this;
}

ObservableDict &ObservableDict::operator=(const boost::json::object &that) {
  ObservableDict temp(that);
  return this->assign(temp);
}

ObservableDict::ObservableDict(const boost::json::object &that) {
  ObservableDictPtr dict;
  ObservableListPtr list;
  for (auto &v : that) {
    switch (v.value().kind()) {
    case boost::json::kind::object: {
      dict = std::make_shared<ObservableDict>(v.value().as_object());
      dict->parent = this;
      dict->set_remote_storage(remote_storage);
      content[v.key()] = dict;
      break;
    }
    case boost::json::kind::array: {
      list = std::make_shared<ObservableList>(v.value().as_array());
      list->parent = this;
      list->set_remote_storage(remote_storage);
      content[v.key()] = list;
      break;
    }
    case boost::json::kind::string: {
      content[v.key()] = std::string(v.value().as_string());
      break;
    }
    case boost::json::kind::uint64: {
      content[v.key()] = static_cast<long long>(v.value().as_uint64());
      break;
    }
    case boost::json::kind::int64: {
      content[v.key()] = v.value().as_int64();
      break;
    }
    case boost::json::kind::double_: {
      content[v.key()] = v.value().as_double();
      break;
    }
    case boost::json::kind::bool_: {
      content[v.key()] = v.value().as_bool();
      break;
    }
    case boost::json::kind::null: {
      content[v.key()] = std::monostate();
      break;
    }
    }
  }
}

ObservableDict::operator boost::json::object() const {
  boost::json::object result;
  for (const auto &pair : content) {
    // std::shared_ptr<ObservableDict>, std::shared_ptr<ObservableList>

    std::string key;
    if (std::holds_alternative<long long int>(pair.first)) {
      key = std::to_string(std::get<long long int>(pair.first));
    } else if (std::holds_alternative<std::string>(pair.first)) {
      key = std::get<std::string>(pair.first);
    } else {
      THALAMUS_ABORT("Unexpect key type");
    }

    if (std::holds_alternative<long long int>(pair.second)) {
      result[key] = std::get<long long int>(pair.second);
    } else if (std::holds_alternative<double>(pair.second)) {
      result[key] = std::get<double>(pair.second);
    } else if (std::holds_alternative<bool>(pair.second)) {
      result[key] = std::get<bool>(pair.second);
    } else if (std::holds_alternative<std::string>(pair.second)) {
      result[key] = std::get<std::string>(pair.second);
    } else if (std::holds_alternative<std::shared_ptr<ObservableDict>>(
                   pair.second)) {
      result[key] = boost::json::object(
          *std::get<std::shared_ptr<ObservableDict>>(pair.second));
    } else if (std::holds_alternative<std::shared_ptr<ObservableList>>(
                   pair.second)) {
      result[key] = boost::json::array(
          *std::get<std::shared_ptr<ObservableList>>(pair.second));
    }
  }
  return result;
}

std::optional<ObservableCollection::Key>
ObservableDict::key_of(const ObservableCollection &v) const {
  for (const auto &our_pair : content) {
    const auto &our_value = our_pair.second;
    if (std::holds_alternative<ObservableListPtr>(our_value)) {
      auto temp = std::get<ObservableListPtr>(our_value);
      auto temp2 = std::static_pointer_cast<ObservableCollection>(temp);
      if (temp2.get() == &v) {
        return our_pair.first;
      }
    } else if (std::holds_alternative<ObservableDictPtr>(our_value)) {
      auto temp = std::get<ObservableDictPtr>(our_value);
      auto temp2 = std::static_pointer_cast<ObservableCollection>(temp);
      if (temp2.get() == &v) {
        return our_pair.first;
      }
    }
  }
  return std::nullopt;
}

void ObservableDict::set_remote_storage(
    std::function<bool(Action, const std::string &, ObservableCollection::Value,
                       std::function<void()>)>
        _remote_storage) {
  this->remote_storage = _remote_storage;
  for (auto &c : content) {
    if (std::holds_alternative<ObservableListPtr>(c.second)) {
      auto temp = std::get<ObservableListPtr>(c.second);
      temp->set_remote_storage(_remote_storage);
    } else if (std::holds_alternative<ObservableDictPtr>(c.second)) {
      auto temp = std::get<ObservableDictPtr>(c.second);
      temp->set_remote_storage(_remote_storage);
    }
  }
}

boost::json::value ObservableDict::to_json() {
  boost::json::object result = *this;
  return result;
}

} // namespace thalamus
