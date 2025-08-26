
#include "state/EventBus.h"
#include <mutex>
namespace core::bus {
namespace { std::mutex& M(){ static std::mutex m; return m;} std::vector<Command>& Q(){ static std::vector<Command> q; return q;} std::vector<Handler>& H(){ static std::vector<Handler> h; return h;} }
void push(const Command& c){ std::lock_guard<std::mutex> lk(M()); Q().push_back(c); }
void registerHandler(const Handler& h){ std::lock_guard<std::mutex> lk(M()); H().push_back(h); }
void clear(){ std::lock_guard<std::mutex> lk(M()); Q().clear(); }
std::size_t pending(){ std::lock_guard<std::mutex> lk(M()); return Q().size(); }
void process(){ std::vector<Command> b; { std::lock_guard<std::mutex> lk(M()); if(Q().empty()) return; b.swap(Q()); } auto& hs=H(); for(const auto& c:b) for(const auto& h:hs) if(h) h(c); }
} // namespace core::bus
