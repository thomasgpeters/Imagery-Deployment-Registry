#include "api/AlsClient.h"

#include <memory>

using njson = nlohmann::json;

namespace dr::api {

AlsClient::AlsClient(const std::string& apiBaseUrl)
    : baseUrl_(apiBaseUrl)
{}

// ---------------------------------------------------------------------------
// asyncGet
// ---------------------------------------------------------------------------

void AlsClient::asyncGet(const std::string& url,
                          std::function<void(bool ok, const std::string& body)> handler)
{
    // Parent the Http::Client to the WApplication so it is destroyed when
    // the session ends — prevents callbacks from firing on dead widgets.
    auto* app = Wt::WApplication::instance();
    if (!app) {
        handler(false, "");
        return;
    }

    auto* client = app->addChild(std::make_unique<Wt::Http::Client>());
    client->setTimeout(std::chrono::seconds(10));
    client->setMaximumResponseSize(16 * 1024 * 1024);

    client->done().connect(
        [handler, client](Wt::AsioWrapper::error_code ec,
                          const Wt::Http::Message& msg) {
            if (!ec && msg.status() == 200) {
                handler(true, msg.body());
            } else {
                handler(false, "");
            }
            // Clean up — Wt defers the actual deletion so this is safe
            // inside the signal handler.
            if (client->parent())
                client->parent()->removeChild(client);
        });

    client->get(url);
}

// ---------------------------------------------------------------------------
// asyncRequest — POST / PATCH / DELETE
// ---------------------------------------------------------------------------

void AlsClient::asyncRequest(const std::string& method,
                              const std::string& url,
                              const std::string& body,
                              std::function<void(bool ok, int status, const std::string& responseBody)> handler)
{
    auto* app = Wt::WApplication::instance();
    if (!app) {
        handler(false, 0, "");
        return;
    }

    auto* client = app->addChild(std::make_unique<Wt::Http::Client>());
    client->setTimeout(std::chrono::seconds(10));
    client->setMaximumResponseSize(16 * 1024 * 1024);

    Wt::Http::Message msg;
    msg.addHeader("Content-Type", "application/json");
    msg.addHeader("Accept", "application/json");
    if (!body.empty())
        msg.addBodyText(body);

    client->done().connect(
        [handler, client](Wt::AsioWrapper::error_code ec,
                          const Wt::Http::Message& response) {
            if (!ec) {
                bool ok = (response.status() >= 200 && response.status() < 300);
                handler(ok, response.status(), response.body());
            } else {
                handler(false, 0, "");
            }
            if (client->parent())
                client->parent()->removeChild(client);
        });

    if (method == "POST")
        client->post(url, msg);
    else if (method == "PATCH")
        client->patch(url, msg);
    else if (method == "DELETE")
        client->deleteRequest(url, msg);
    else
        handler(false, 0, "");
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
