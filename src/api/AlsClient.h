#pragma once

#include <Wt/Http/Client.h>
#include <Wt/Http/Message.h>
#include <Wt/WApplication.h>

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

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
    ~AlsClient();

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

    /// Remove a completed client from the active list, breaking the
    /// shared_ptr cycle so it can be freed.
    void retireClient(Wt::Http::Client* raw);

    std::string baseUrl_;
    std::vector<std::shared_ptr<Wt::Http::Client>> activeClients_;

    /// Shared alive flag — destroyed (and set to false) when AlsClient dies.
    /// Lambdas capture a weak_ptr copy so they can detect destruction.
    std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
};

} // namespace dr::api
