#include <thalamus/tracing.hpp>
#include <state_manager.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <grpcpp/grpcpp.h>
#include <thalamus.pb.h>
#include <thalamus.grpc.pb.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

using namespace thalamus;
using namespace std::chrono_literals;

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

class StateReactor : public grpc::ClientBidiReactor<thalamus_grpc::ObservableTransaction, thalamus_grpc::ObservableTransaction> {
public:
  std::mutex mutex;
  std::condition_variable condition;
  thalamus_grpc::ObservableTransaction in;
  std::list<thalamus_grpc::ObservableTransaction> out;
  grpc::ClientContext context;
  std::map<unsigned long long, std::function<void()>> pending_changes;
  bool done = false;
  bool writing = false;
  boost::asio::io_context& io_context;
  ObservableCollection::Value state;
  StateReactor(boost::asio::io_context& _io_context, ObservableCollection::Value _state) : io_context(_io_context), state(_state) {}
  ~StateReactor() override {
    //grpc::ClientBidiReactor<task_controller_grpc::TaskResult, task_controller_grpc::TaskConfig>::StartWritesDone();
    context.TryCancel();
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&] { return done; });
  }
  void start() {
    grpc::ClientBidiReactor<thalamus_grpc::ObservableTransaction, thalamus_grpc::ObservableTransaction>::StartRead(&in);
    grpc::ClientBidiReactor<thalamus_grpc::ObservableTransaction, thalamus_grpc::ObservableTransaction>::StartCall();
  }
  void OnReadDone(bool ok) override {
    if(!ok) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
      }
      condition.notify_all();
      return;
    }
    TRACE_EVENT("thalamus", "observable_bridge");
    if (in.acknowledged()) {
      TRACE_EVENT(
          "thalamus", "StateManager::acknowledged",
          perfetto::TerminatingFlow::ProcessScoped(in.acknowledged()));
      std::cout << "Acknowledged " << in.acknowledged();
      std::function<void()> callback;
      {
        std::unique_lock<std::mutex> lock(mutex);
        callback = pending_changes.at(in.acknowledged());
        pending_changes.erase(in.acknowledged());
      }
      boost::asio::post(io_context, callback);
      grpc::ClientBidiReactor<thalamus_grpc::ObservableTransaction, thalamus_grpc::ObservableTransaction>::StartRead(&in);
      return;
    }

    // std::cout << change.address() << " " << change.value() << "ACK: " <<
    // change.acknowledged() << std::endl;
    std::vector<std::promise<void>> promises;
    std::vector<std::future<void>> futures;
    for (auto &change : in.changes()) {
      THALAMUS_LOG(trace)
          << change.address() << " " << change.value() << std::endl;

      boost::json::value parsed = boost::json::parse(change.value());
      auto value = ObservableCollection::from_json(parsed);

      promises.emplace_back();
      futures.push_back(promises.back().get_future());
      boost::asio::post(
          io_context, [&promise = promises.back(), c_state = state,
                       action = change.action(), address = change.address(),
                       new_value = std::move(value)] {
            TRACE_EVENT("thalamus", "observable_bridge(post)");
            if (action == thalamus_grpc::ObservableChange_Action_Set) {
              set_jsonpath(c_state, address, new_value, true);
            } else {
              delete_jsonpath(c_state, address, true);
            }
            promise.set_value();
          });
    }
    for (auto &future : futures) {
      while (future.wait_for(1s) == std::future_status::timeout && !done) {
      }
      if (done) {
        return;
      }
    }
    grpc::ClientBidiReactor<thalamus_grpc::ObservableTransaction, thalamus_grpc::ObservableTransaction>::StartRead(&in);
  }
  void OnWriteDone(bool ok) override {
    std::cout << "wrote " << ok << std::endl;
    if(!ok) {
      {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
      }
      condition.notify_all();
      return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    out.pop_front();
    if(!out.empty()) {
      grpc::ClientBidiReactor<thalamus_grpc::ObservableTransaction, thalamus_grpc::ObservableTransaction>::StartWrite(&out.front());
    } else {
      writing = false;
    }
  }
  void send(thalamus_grpc::ObservableTransaction&& message) {
    std::lock_guard<std::mutex> lock(mutex);
    out.push_back(std::move(message));
    if(!writing) {
      grpc::ClientBidiReactor<thalamus_grpc::ObservableTransaction, thalamus_grpc::ObservableTransaction>::StartWrite(&out.front());
      writing = true;
    }
  }
  void OnDone(const grpc::Status&) override {
    std::cout << "OnDone" << std::endl;
    {
      std::lock_guard<std::mutex> lock(mutex);
      done = true;
    }
    condition.notify_all();
  }
};

struct StateManager::Impl {
  thalamus_grpc::Thalamus::Stub *stub;
  StateReactor reactor;
  ObservableCollection::Value state;
  ObservableCollection::Value root;
  boost::asio::io_context &io_context;
  std::atomic_bool running;
  std::atomic<
      ::grpc::ClientReaderWriter<::thalamus_grpc::ObservableTransaction,
                                 ::thalamus_grpc::ObservableTransaction> *>
      stream;
  const size_t CONNECT = 0;
  const size_t READ = 1;
  const size_t WRITE = 2;
  std::vector<thalamus_grpc::ObservableTransaction> outbox;
  std::mutex mutex;
  ::grpc::ClientContext context;
  std::thread grpc_thread;
  boost::signals2::signal<void(ObservableCollection::Action action,
                               const std::string &address,
                               ObservableCollection::Value value)>
      signal;
  std::vector<boost::signals2::scoped_connection> state_connections;
  std::unique_ptr<thalamus_grpc::Thalamus::Stub> redirected_stub = nullptr;

  Impl(thalamus_grpc::Thalamus::Stub *_stub, ObservableCollection::Value _state,
       boost::asio::io_context &_io_context)
      : stub(_stub), reactor(_io_context, _state), state(_state), root(state), io_context(_io_context),
        running(true) {

    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    auto redirect_context = std::make_shared<grpc::ClientContext>();
    auto request = std::make_shared<thalamus_grpc::Empty>();
    auto response = std::make_shared<thalamus_grpc::Redirect>();
    stub->async()->get_redirect(redirect_context.get(), request.get(), response.get(),
    [&,moved_context=redirect_context,moved_request=request,moved_response=response](grpc::Status s) {
      auto redirect = moved_response->redirect();
      THALAMUS_LOG(info) << absl::StrFormat("redirect status, redirect: %s, status: %s", redirect, s.error_message());
      if(!s.ok()) {
        THALAMUS_LOG(warning) << "Aborting";
        std::abort();
      }
      if(!redirect.empty()) {
        auto credentials = grpc::InsecureChannelCredentials();
        auto thalamus_channel = grpc::CreateChannel(redirect, credentials);
        redirected_stub = thalamus_grpc::Thalamus::NewStub(thalamus_channel);
        stub = redirected_stub.get();
      }
      promise.set_value();
    });
    future.get();

    stub->async()->observable_bridge_v2(&context, &reactor);
    reactor.start();
    if (std::holds_alternative<ObservableListPtr>(state)) {
      auto temp = std::get<ObservableListPtr>(state);
      temp->set_remote_storage(
          std::bind(&Impl::send_change, this, _1, _2, _3, _4));
    } else if (std::holds_alternative<ObservableDictPtr>(state)) {
      auto temp = std::get<ObservableDictPtr>(state);
      temp->set_remote_storage(
          std::bind(&Impl::send_change, this, _1, _2, _3, _4));
    }
  }

  ~Impl() {
    running = false;
    context.TryCancel();
  }

  bool send_change(ObservableCollection::Action action,
                   const std::string &address,
                   ObservableCollection::Value value,
                   std::function<void()> callback) {
    auto id = get_unique_id();
    TRACE_EVENT("thalamus", "StateManager::send_change",
                perfetto::Flow::ProcessScoped(id));
    if (io_context.stopped()) {
      return true;
    }

    auto json_value = ObservableCollection::to_json(value);
    auto string_value = boost::json::serialize(json_value);
    thalamus_grpc::ObservableTransaction transaction;
    auto change = transaction.add_changes();
    change->set_address(address);
    change->set_value(string_value);
    if (action == ObservableCollection::Action::Set) {
      change->set_action(thalamus_grpc::ObservableChange_Action_Set);
    } else {
      change->set_action(thalamus_grpc::ObservableChange_Action_Delete);
    }
    transaction.set_id(id);

    THALAMUS_LOG(trace) << "Change " << transaction.id();

    {
      std::unique_lock<std::mutex> lock(reactor.mutex);
      reactor.pending_changes[transaction.id()] = callback;
    }

    reactor.send(std::move(transaction));
    return true;
  }
};

StateManager::StateManager(thalamus_grpc::Thalamus::Stub *stub,
                           ObservableCollection::Value state,
                           boost::asio::io_context &io_context)
    : impl(new Impl(stub, state, io_context)) {}

StateManager::~StateManager() {}
