#include "ui/DeploymentListView.h"
#include "ui/MainLayout.h"

#include <Wt/WApplication.h>
#include <Wt/WText.h>
#include <Wt/WTimer.h>

#include <chrono>

namespace dr {

DeploymentListView::DeploymentListView(api::AlsClient& client, MainLayout& layout)
    : client_(client), layout_(layout)
{
    buildUI();
}

DeploymentListView::~DeploymentListView()
{
    stopPolling();
    *alive_ = false;
}

void DeploymentListView::buildUI()
{
    setStyleClass("container-fluid");

    // Header row
    auto* header = addNew<Wt::WContainerWidget>();
    header->setStyleClass("d-flex justify-content-between align-items-center mb-4");

    title_ = header->addNew<Wt::WText>("<h4>Deployments</h4>", Wt::TextFormat::XHTML);

    auto* btnGroup = header->addNew<Wt::WContainerWidget>();
    btnGroup->setStyleClass("d-flex gap-2");

    auto* refreshBtn = btnGroup->addNew<Wt::WText>(
        "<button class='btn btn-outline-primary btn-sm'>"
        "<i class='bi bi-arrow-clockwise me-1'></i>Refresh</button>",
        Wt::TextFormat::XHTML);
    refreshBtn->clicked().connect([this] { reload(); });

    auto* pollBtn = btnGroup->addNew<Wt::WText>(
        "<button class='btn btn-outline-success btn-sm'>"
        "<i class='bi bi-play-circle me-1'></i>Auto-refresh</button>",
        Wt::TextFormat::XHTML);
    pollBtn->clicked().connect([this, pollBtn] {
        if (polling_) {
            stopPolling();
            pollBtn->setText(
                "<button class='btn btn-outline-success btn-sm'>"
                "<i class='bi bi-play-circle me-1'></i>Auto-refresh</button>");
        } else {
            startPolling();
            pollBtn->setText(
                "<button class='btn btn-success btn-sm'>"
                "<i class='bi bi-pause-circle me-1'></i>Polling (15s)</button>");
        }
    });

    // Status line
    status_ = addNew<Wt::WText>("");
    status_->setStyleClass("text-muted small mb-3");

    // Table — wrapped in a card panel
    auto* card = addNew<Wt::WContainerWidget>();
    card->setStyleClass("dr-tab-card");
    table_ = card->addNew<Wt::WTable>();
    table_->setStyleClass("table table-hover table-striped align-middle");
    table_->setHeaderCount(1);

    // Header cells — column 0 is the target icon
    table_->elementAt(0, 0)->addNew<Wt::WText>("");           // icon column
    table_->elementAt(0, 0)->setStyleClass("fw-bold text-center");
    table_->elementAt(0, 0)->setWidth(Wt::WLength(40));
    table_->elementAt(0, 1)->addNew<Wt::WText>("Name");
    table_->elementAt(0, 2)->addNew<Wt::WText>("Environment");
    table_->elementAt(0, 3)->addNew<Wt::WText>("Stack");
    table_->elementAt(0, 4)->addNew<Wt::WText>("Status");
    table_->elementAt(0, 5)->addNew<Wt::WText>("Target");
    table_->elementAt(0, 6)->addNew<Wt::WText>("Version");
    table_->elementAt(0, 7)->addNew<Wt::WText>("Deployed By");
    table_->elementAt(0, 8)->addNew<Wt::WText>("Deployed At");

    for (int c = 1; c < 9; ++c)
        table_->elementAt(0, c)->setStyleClass("fw-bold");
}

void DeploymentListView::reload()
{
    status_->setText("Loading...");

    std::weak_ptr<bool> weak = alive_;
    client_.getAll("Deployment", [this, weak](bool ok, const nlohmann::json& items) {
        auto guard = weak.lock();
        if (!guard || !*guard) return;
        auto* app = Wt::WApplication::instance();
        if (!app) return;
        // Note: no UpdateLock — Http::Client done() fires inside the
        // application's event loop which already holds the session lock.

        if (!ok) {
            status_->setText("Failed to fetch deployments from ALS backend ("
                             + client_.baseUrl() + "/Deployment/).");
            app->triggerUpdate();
            return;
        }

        std::vector<model::Deployment> deployments;
        for (const auto& item : items) {
            deployments.push_back(model::Deployment::fromJson(item));
        }

        status_->setText(std::to_string(deployments.size()) + " deployment(s)");
        populateTable(deployments);

        app->triggerUpdate();
    });
}

void DeploymentListView::populateTable(const std::vector<model::Deployment>& deployments)
{
    // Clear data rows (keep header)
    while (table_->rowCount() > 1)
        table_->removeRow(1);

    int row = 1;
    for (const auto& d : deployments) {
        // Column 0: target icon (Docker / Kubernetes)
        table_->elementAt(row, 0)->addNew<Wt::WText>(
            targetIcon(d.target), Wt::TextFormat::XHTML);
        table_->elementAt(row, 0)->setStyleClass("text-center");

        table_->elementAt(row, 1)->addNew<Wt::WText>(d.name);
        table_->elementAt(row, 1)->setStyleClass("fw-semibold");
        table_->elementAt(row, 2)->addNew<Wt::WText>(d.environment_name);
        table_->elementAt(row, 3)->addNew<Wt::WText>(d.stack_name);

        auto* badge = table_->elementAt(row, 4)->addNew<Wt::WText>(d.status);
        badge->setStyleClass("badge " + statusBadgeClass(d.status));

        table_->elementAt(row, 5)->addNew<Wt::WText>(d.target + " / " + d.provider);
        table_->elementAt(row, 6)->addNew<Wt::WText>(d.version_label);
        table_->elementAt(row, 7)->addNew<Wt::WText>(d.deployed_by);
        table_->elementAt(row, 8)->addNew<Wt::WText>(d.deployed_at);

        // Click row → navigate to detail
        int deployId = d.id;
        table_->rowAt(row)->setStyleClass("cursor-pointer");
        for (int c = 0; c < 9; ++c) {
            table_->elementAt(row, c)->clicked().connect([this, deployId] {
                layout_.showDeploymentDetail(deployId);
            });
        }

        ++row;
    }
}

void DeploymentListView::startPolling(int intervalSeconds)
{
    if (polling_) return;
    polling_ = true;

    if (!pollTimer_) {
        pollTimer_ = std::make_unique<Wt::WTimer>();
        pollTimer_->timeout().connect([this] { reload(); });
    }
    pollTimer_->setInterval(std::chrono::seconds(intervalSeconds));
    pollTimer_->start();
    reload();  // immediate first fetch
}

void DeploymentListView::stopPolling()
{
    polling_ = false;
    if (pollTimer_)
        pollTimer_->stop();
}

std::string DeploymentListView::statusBadgeClass(const std::string& status)
{
    if (status == "Running")    return "bg-success";
    if (status == "Deploying")  return "bg-warning text-dark";
    if (status == "Failed")     return "bg-danger";
    if (status == "Stopped")    return "bg-secondary";
    if (status == "RolledBack") return "bg-info text-dark";
    if (status == "Pending")    return "bg-light text-dark";
    return "bg-secondary";
}

std::string DeploymentListView::targetIcon(const std::string& target)
{
    // Docker (whale) — simplified Moby logo, 24×24
    static const char* dockerSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' "
        "viewBox='0 0 24 24' fill='#2496ED'>"
        "<path d='M13.98 11.08h2.12a.09.09 0 0 0 .09-.09V9.16a.09.09 0 0 "
        "0-.09-.09h-2.12a.09.09 0 0 0-.09.09v1.83c0 .05.04.09.09.09m-2.3 "
        "0h2.12a.09.09 0 0 0 .09-.09V9.16a.09.09 0 0 0-.09-.09H11.68a.09.09 "
        "0 0 0-.09.09v1.83c0 .05.04.09.09.09m-2.3 0h2.12a.09.09 0 0 0 "
        ".09-.09V9.16a.09.09 0 0 0-.09-.09H9.38a.09.09 0 0 0-.09.09v1.83c0 "
        ".05.04.09.09.09m-2.3 0h2.12a.09.09 0 0 0 .09-.09V9.16a.09.09 0 0 "
        "0-.09-.09H7.08a.09.09 0 0 0-.09.09v1.83c0 .05.04.09.09.09m-2.3 "
        "0h2.12a.09.09 0 0 0 .09-.09V9.16a.09.09 0 0 0-.09-.09H4.78a.09.09 "
        "0 0 0-.09.09v1.83c0 .05.04.09.09.09m2.3-2.3h2.12a.09.09 0 0 0 "
        ".09-.09V6.86a.09.09 0 0 0-.09-.09H7.08a.09.09 0 0 0-.09.09v1.83c0 "
        ".05.04.09.09.09m2.3 0h2.12a.09.09 0 0 0 .09-.09V6.86a.09.09 0 0 "
        "0-.09-.09H9.38a.09.09 0 0 0-.09.09v1.83c0 .05.04.09.09.09m2.3 "
        "0h2.12a.09.09 0 0 0 .09-.09V6.86a.09.09 0 0 0-.09-.09H11.68a.09.09 "
        "0 0 0-.09.09v1.83c0 .05.04.09.09.09m0-2.3h2.12a.09.09 0 0 0 "
        ".09-.09V4.56a.09.09 0 0 0-.09-.09H11.68a.09.09 0 0 0-.09.09v1.83c0 "
        ".05.04.09.09.09m10.42 3.89c-.55-.32-1.82-.44-2.79-.28-.13-.95-.64-1.78-1.25-2.46l-.25-.3-.3.25c-.63.52-1.19 "
        "1.4-1.04 2.8-1.52-.53-3.13-.41-4.38.13-.16.07-.12.2.06.19 1.7-.15 "
        "5.72-.15 8.42 2.08.36.3.61-.14.43-.38-.55-.74-1.48-1.42-2.55-1.74-.17-.05-.21.19-.05.25 "
        "1.2.45 2.22 1.3 2.82 2.56.06.12.21.12.28.01.49-.78-.06-2.03-.4-3.11z'/>"
        "</svg>";

    // Kubernetes (helm wheel) — simplified logo, 24×24
    static const char* k8sSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' "
        "viewBox='0 0 32 32' fill='#326CE5'>"
        "<path d='M15.9.5a2.4 2.4 0 0 0-1.1.3L4.4 6.5a2.4 2.4 0 0 0-1.1 "
        "1.4l-2.7 11a2.4 2.4 0 0 0 .3 1.8l7.1 8.7a2.4 2.4 0 0 0 1.6.8h11.4a2.4 "
        "2.4 0 0 0 1.6-.8l7.1-8.7a2.4 2.4 0 0 0 .3-1.8l-2.7-11a2.4 2.4 0 0 "
        "0-1.1-1.4L15.9.8 15.9.5z'/>"
        "<path fill='#fff' d='M16 5.7l-.2.7-.3 2.4v.3l-.1.2a1.3 1.3 0 0 "
        "1-.8.9l-.2.1-.2.1-2.3.7-.6.2.5.4 1.9 1.5.2.1.1.2c.2.3.2.7.1 "
        "1l-.1.2v.2l-.8 2.3-.2.7.6-.4 2-1.3.2-.2h.2c.4-.1.7-.1 1 "
        "0h.3l.2.1 2 1.3.6.4-.2-.7-.8-2.3v-.2l-.1-.2a1.3 1.3 0 0 1 "
        ".1-1l.1-.2.2-.1 1.9-1.5.5-.4-.6-.2-2.3-.7-.2-.1-.2-.1a1.3 1.3 0 0 "
        "1-.8-.9l-.1-.2V8.8l-.3-2.4-.2-.7z'/>"
        "</svg>";

    // Generic fallback — box icon
    static const char* genericSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' "
        "viewBox='0 0 16 16' fill='#6c757d'>"
        "<path d='M8.186 1.113a.5.5 0 0 0-.372 0L1.846 "
        "3.5 8 5.961 14.154 3.5 8.186 1.113zM15 4.239l-6.5 "
        "2.6v7.922l6.5-2.6V4.24zM7.5 14.762V6.838L1 "
        "4.239v7.923l6.5 2.6z'/>"
        "</svg>";

    if (target == "DockerCompose" || target == "Docker")
        return dockerSvg;
    if (target == "Kubernetes")
        return k8sSvg;
    return genericSvg;
}

} // namespace dr
