// NOLINT(namespace-envoy)
#include <string>
#include <unordered_map>

#include "google/protobuf/util/json_util.h"
#include "proxy_wasm_intrinsics.h"
#include "filter.pb.h"

class ExtCallRootContext : public RootContext {
public:
  explicit ExtCallRootContext(uint32_t id, std::string_view root_id) : RootContext(id, root_id) {}
  bool onConfigure(size_t /* configuration_size */) override;

  bool onStart(size_t) override;

  std::string header_value_;
};

class ExtCallContext : public Context {
public:
  explicit ExtCallContext(uint32_t id, RootContext* root) : Context(id, root), root_(static_cast<ExtCallRootContext*>(static_cast<void*>(root))) {}

  void onCreate() override;
  FilterHeadersStatus onRequestHeaders(uint32_t headers, bool end_of_stream) override;
  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  FilterHeadersStatus onResponseHeaders(uint32_t headers, bool end_of_stream) override;
  void onDone() override;
  void onLog() override;
  void onDelete() override;
private:

  ExtCallRootContext* root_;
};
static RegisterContextFactory register_ExtCallContext(CONTEXT_FACTORY(ExtCallContext),
                                                      ROOT_FACTORY(ExtCallRootContext));

bool ExtCallRootContext::onConfigure(size_t config_buffer_length) {
  auto conf = getBufferBytes(WasmBufferType::PluginConfiguration, 0, config_buffer_length);
  LOG_INFO("onConfigure " + conf->toString());
  header_value_ = conf->toString();
  return true; 
}

bool ExtCallRootContext::onStart(size_t) { LOG_DEBUG("onStart"); return true;}

void ExtCallContext::onCreate() { LOG_DEBUG(std::string("onCreate " + std::to_string(id()))); }

FilterHeadersStatus ExtCallContext::onRequestHeaders(uint32_t, bool) {
  LOG_INFO(std::string("onRequestHeaders ") + std::to_string(id()));

  HeaderStringPairs request_headers {};
  request_headers.emplace_back(":authority", "web_service.com");
  request_headers.emplace_back(":method", "POST");
  request_headers.emplace_back(":path", "/get");
  request_headers.emplace_back("accept", "application/json");

  root_->httpCall("web_service", request_headers, {}, {}, 5000, [this](uint32_t /*headers_size*/, size_t body_size, uint32_t /*trailers*/) {

      LOG_INFO("GOT_RESPONSE");

      LOG_INFO(std::string{"CONTEXT_ID IN RESPONSE HANDLER: "} + std::to_string(id()));
      auto response_body = getBufferBytes(WasmBufferType::HttpCallResponseBody,
                                          0, body_size)->toString();
      LOG_INFO(std::string{"RESPONSE PAYLOAD IN HEADERS: "} + response_body);

      setEffectiveContext();

      auto response_result =
          sendLocalResponse(200, "OK", response_body, {}, GrpcStatus::Ok);

      LOG_INFO(std::string{"RESPONSE RESULT "} + std::to_string(static_cast<uint32_t>(response_result)));

      continueRequest();
  });

  return FilterHeadersStatus::StopIteration;
}

FilterHeadersStatus ExtCallContext::onResponseHeaders(uint32_t, bool) {
  LOG_INFO(std::string("onResponseHeaders ") + std::to_string(id()));
  addResponseHeader("newheader", root_->header_value_);
  replaceResponseHeader("location", "envoy-wasm");
  return FilterHeadersStatus::Continue;
}

FilterDataStatus ExtCallContext::onRequestBody(size_t body_buffer_length, bool end_of_stream) {
  return FilterDataStatus::Continue;
}

void ExtCallContext::onDone() { LOG_INFO(std::string("onDone " + std::to_string(id()))); }

void ExtCallContext::onLog() { LOG_INFO(std::string("onLog " + std::to_string(id()))); }

void ExtCallContext::onDelete() { LOG_INFO(std::string("onDelete " + std::to_string(id()))); }
