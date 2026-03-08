#include "api/AlsClient.h"

#include <memory>

using njson = nlohmann::json;

namespace dr::api {

AlsClient::AlsClient(const std::string& apiBaseUrl)
    : baseUrl_(apiBaseUrl)
{}

AlsClient::~AlsClient()
{
    *alive_ = false;
    // Abort every in-flight request so the done() signal won't fire after
    // this object (or the widgets that own it) is destroyed.
    for (auto& c : activeClients_)
        c->abort();
    activeClients_.clear();
    retiredClients_.clear();
}

// ---------------------------------------------------------------------------
// asyncGet
// ---------------------------------------------------------------------------

void AlsClient::asyncGet(const std::string& url,
                          std::function<void(bool ok, const std::string& body)> handler)
{
    cleanupRetired();
    auto client = std::make_shared<Wt::Http::Client>();
    client->setTimeout(std::chrono::seconds(10));
    client->setMaximumResponseSize(16 * 1024 * 1024);

    auto* raw = client.get();
    activeClients_.push_back(std::move(client));

    std::weak_ptr<bool> weak = alive_;
    raw->done().connect(
        [this, handler, raw, weak](Wt::AsioWrapper::error_code ec,
                                   const Wt::Http::Message& msg) {
            auto guard = weak.lock();
            if (!guard || !*guard) return;
            // Copy response data before retiring the client, because the
            // Http::Message reference is owned by the Http::Client.
            bool ok = (!ec && msg.status() == 200);
            std::string body = ok ? msg.body() : std::string();
            // retireClient moves the shared_ptr to a local so the
            // Http::Client outlives this signal emission.
            retireClient(raw);
            handler(ok, body);
        });

    raw->get(url);
}

// ---------------------------------------------------------------------------
// asyncRequest — POST / PATCH / DELETE
// ---------------------------------------------------------------------------

void AlsClient::asyncRequest(const std::string& method,
                              const std::string& url,
                              const std::string& body,
                              std::function<void(bool ok, int status, const std::string& responseBody)> handler)
{
    cleanupRetired();
    auto client = std::make_shared<Wt::Http::Client>();
    client->setTimeout(std::chrono::seconds(10));
    client->setMaximumResponseSize(16 * 1024 * 1024);

    Wt::Http::Message msg;
    msg.addHeader("Content-Type", "application/json");
    msg.addHeader("Accept", "application/json");
    if (!body.empty())
        msg.addBodyText(body);

    auto* raw = client.get();
    activeClients_.push_back(std::move(client));

    std::weak_ptr<bool> weak = alive_;
    raw->done().connect(
        [this, handler, raw, weak](Wt::AsioWrapper::error_code ec,
                                   const Wt::Http::Message& response) {
            auto guard = weak.lock();
            if (!guard || !*guard) return;
            // Copy response data before retiring the client.
            bool ok = (!ec && response.status() >= 200 && response.status() < 300);
            int status = !ec ? response.status() : 0;
            std::string body = !ec ? response.body() : std::string();
            retireClient(raw);
            handler(ok, status, body);
        });

    if (method == "POST")
        raw->post(url, msg);
    else if (method == "PATCH")
        raw->patch(url, msg);
    else if (method == "DELETE")
        raw->deleteRequest(url, msg);
    else
        handler(false, 0, "");
}

void AlsClient::retireClient(Wt::Http::Client* raw)
{
    // Move the client to the retired list instead of destroying it.
    // We cannot destroy an Http::Client while its done() signal is being
    // emitted (the signal owns the lambda that called us).  Retired clients
    // are cleaned up at the start of the next request.
    for (auto it = activeClients_.begin(); it != activeClients_.end(); ++it) {
        if (it->get() == raw) {
            retiredClients_.push_back(std::move(*it));
            activeClients_.erase(it);
            break;
        }
    }
}

void AlsClient::cleanupRetired()
{
    retiredClients_.clear();
}

// ---------------------------------------------------------------------------
// getAll
// ---------------------------------------------------------------------------

void AlsClient::getAll(const std::string& resource, ListCallback cb)
{
    if (!cb) return;
    std::string url = baseUrl_ + "/" + resource + "/?page%5Boffset%5D=0&page%5Blimit%5D=1000";

    asyncGet(url, [cb](bool ok, const std::string& body) {
        if (!ok) {
            cb(false, njson::array());
            return;
        }
        try {
            auto j = njson::parse(body);
            if (j.contains("data") && j["data"].is_array()) {
                cb(true, j["data"]);
            } else if (j.is_array()) {
                cb(true, j);
            } else {
                cb(false, njson::array());
            }
        } catch (...) {
            cb(false, njson::array());
        }
    });
}

void AlsClient::getAll(const std::string& resource,
                        const std::string& filterKey, int filterValue,
                        ListCallback cb)
{
    if (!cb) return;
    std::string url = baseUrl_ + "/" + resource
        + "/?filter%5B" + filterKey + "%5D=" + std::to_string(filterValue)
        + "&page%5Boffset%5D=0&page%5Blimit%5D=1000";

    asyncGet(url, [cb](bool ok, const std::string& body) {
        if (!ok) {
            cb(false, njson::array());
            return;
        }
        try {
            auto j = njson::parse(body);
            if (j.contains("data") && j["data"].is_array()) {
                cb(true, j["data"]);
            } else if (j.is_array()) {
                cb(true, j);
            } else {
                cb(false, njson::array());
            }
        } catch (...) {
            cb(false, njson::array());
        }
    });
}

// ---------------------------------------------------------------------------
// getOne
// ---------------------------------------------------------------------------

void AlsClient::getOne(const std::string& resource, int id, ItemCallback cb)
{
    if (!cb) return;
    std::string url = baseUrl_ + "/" + resource + "/" + std::to_string(id);

    asyncGet(url, [cb](bool ok, const std::string& body) {
        if (!ok) {
            cb(false, njson::object());
            return;
        }
        try {
            auto j = njson::parse(body);
            if (j.contains("data") && j["data"].is_object()) {
                cb(true, j["data"]);
            } else {
                cb(false, njson::object());
            }
        } catch (...) {
            cb(false, njson::object());
        }
    });
}

// ---------------------------------------------------------------------------
// create
// ---------------------------------------------------------------------------

void AlsClient::create(const std::string& resource,
                        const njson& attributes,
                        ItemCallback cb)
{
    if (!cb) return;
    std::string url = baseUrl_ + "/" + resource + "/";

    njson payload = {
        {"data", {
            {"attributes", attributes},
            {"type", resource}
        }}
    };

    asyncRequest("POST", url, payload.dump(),
        [cb](bool ok, int /*status*/, const std::string& body) {
            if (!ok) {
                cb(false, njson::object());
                return;
            }
            try {
                auto j = njson::parse(body);
                if (j.contains("data") && j["data"].is_object()) {
                    cb(true, j["data"]);
                } else {
                    cb(true, j);
                }
            } catch (...) {
                cb(false, njson::object());
            }
        });
}

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

void AlsClient::update(const std::string& resource, int id,
                        const njson& attributes,
                        ItemCallback cb)
{
    if (!cb) return;
    std::string url = baseUrl_ + "/" + resource + "/" + std::to_string(id);

    njson payload = {
        {"data", {
            {"attributes", attributes},
            {"id", std::to_string(id)},
            {"type", resource}
        }}
    };

    asyncRequest("PATCH", url, payload.dump(),
        [cb](bool ok, int /*status*/, const std::string& body) {
            if (!ok) {
                cb(false, njson::object());
                return;
            }
            try {
                auto j = njson::parse(body);
                if (j.contains("data") && j["data"].is_object()) {
                    cb(true, j["data"]);
                } else {
                    cb(true, j);
                }
            } catch (...) {
                cb(false, njson::object());
            }
        });
}

// ---------------------------------------------------------------------------
// remove
// ---------------------------------------------------------------------------

void AlsClient::remove(const std::string& resource, int id, StatusCallback cb)
{
    if (!cb) return;
    std::string url = baseUrl_ + "/" + resource + "/" + std::to_string(id) + "/";

    asyncRequest("DELETE", url, "",
        [cb](bool ok, int /*status*/, const std::string& /*body*/) {
            cb(ok);
        });
}

} // namespace dr::api
