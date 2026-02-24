#pragma once

#include <Wt/Http/Client.h>
#include <Wt/Http/Message.h>
#include <Wt/WApplication.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <string>

namespace dr::api {

/// Async REST client for the Deployment Registry ALS backend (JSON:API).
///
/// All methods are asynchronous — callbacks fire inside the Wt event loop
/// so widget access is safe.
class AlsClient {
public:
    using ListCallback   = std::function<void(bool ok, const nlohmann::json& items)>;
    using ItemCallback   = std::function<void(bool ok, const nlohmann::json& item)>;
    using StatusCallback = std::function<void(bool ok)>;

    explicit AlsClient(const std::string& apiBaseUrl);

    const std::string& baseUrl() const { return baseUrl_; }
    void setBaseUrl(const std::string& url) { baseUrl_ = url; }

    /// GET /api/{resource} — fetch all items.
    void getAll(const std::string& resource, ListCallback cb);

    /// GET /api/{resource}/{id} — fetch one item.
    void getOne(const std::string& resource, int id, ItemCallback cb);

    /// POST /api/{resource}/ — create a new item.
    void create(const std::string& resource,
                const nlohmann::json& attributes,
                ItemCallback cb);

    /// PATCH /api/{resource}/{id} — update an existing item.
    void update(const std::string& resource, int id,
                const nlohmann::json& attributes,
                ItemCallback cb);

    /// DELETE /api/{resource}/{id}/ — remove an item.
    void remove(const std::string& resource, int id, StatusCallback cb);

private:
    void asyncGet(const std::string& url,
                  std::function<void(bool ok, const std::string& body)> handler);

    void asyncRequest(const std::string& method,
                      const std::string& url,
                      const std::string& body,
                      std::function<void(bool ok, int status, const std::string& responseBody)> handler);

    std::string baseUrl_;
};

} // namespace dr::api
