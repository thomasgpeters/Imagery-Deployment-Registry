#include "ui/DeploymentListView.h"
#include "ui/MainLayout.h"

#include <Wt/WApplication.h>
#include <Wt/WMessageBox.h>
#include <Wt/WText.h>
#include <Wt/WTimer.h>

#include <chrono>

namespace dr {

namespace {

/// Minimal HTML-encode for user-supplied text embedded in XHTML literals.
std::string htmlEncode(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;        break;
        }
    }
    return out;
}

} // anonymous namespace

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

    // View-mode toggle buttons
    gridBtn_ = btnGroup->addNew<Wt::WText>(
        "<button class='btn btn-primary btn-sm'>"
        "<i class='bi bi-grid-3x3-gap'></i></button>",
        Wt::TextFormat::XHTML);
    gridBtn_->clicked().connect([this] { setViewMode(true); });

    listBtn_ = btnGroup->addNew<Wt::WText>(
        "<button class='btn btn-outline-secondary btn-sm'>"
        "<i class='bi bi-list-ul'></i></button>",
        Wt::TextFormat::XHTML);
    listBtn_->clicked().connect([this] { setViewMode(false); });

    // Refresh
    auto* refreshBtn = btnGroup->addNew<Wt::WText>(
        "<button class='btn btn-outline-primary btn-sm'>"
        "<i class='bi bi-arrow-clockwise me-1'></i>Refresh</button>",
        Wt::TextFormat::XHTML);
    refreshBtn->clicked().connect([this] { reload(); });

    // Poll toggle
    auto* pollBtn = btnGroup->addNew<Wt::WText>(
        "<button class='btn btn-outline-success btn-sm'>"
        "<i class='bi bi-play-circle me-1'></i>Auto-refresh</button>",
        Wt::TextFormat::XHTML);
    pollBtn->clicked().connect([this, pollBtn] {
        if (polling_) {
            stopPolling();
            pollBtn->setTextFormat(Wt::TextFormat::XHTML);
            pollBtn->setText(
                "<button class='btn btn-outline-success btn-sm'>"
                "<i class='bi bi-play-circle me-1'></i>Auto-refresh</button>");
        } else {
            startPolling();
            pollBtn->setTextFormat(Wt::TextFormat::XHTML);
            pollBtn->setText(
                "<button class='btn btn-success btn-sm'>"
                "<i class='bi bi-pause-circle me-1'></i>Polling (15s)</button>");
        }
    });

    // Status line
    status_ = addNew<Wt::WText>("");
    status_->setStyleClass("text-muted small mb-3");

    // ── Card grid view ───────────────────────────────────────────────────
    grid_ = addNew<Wt::WContainerWidget>();
    grid_->setStyleClass("dr-deploy-grid");

    // ── Table list view (hidden by default) ──────────────────────────────
    listWrap_ = addNew<Wt::WContainerWidget>();
    listWrap_->setStyleClass("dr-deploy-list dr-tab-card");
    listWrap_->hide();

    table_ = listWrap_->addNew<Wt::WTable>();
    table_->setStyleClass("table table-hover table-striped align-middle mb-0");
    table_->setHeaderCount(1);

    // Table header
    table_->elementAt(0, 0)->addNew<Wt::WText>("");
    table_->elementAt(0, 0)->setWidth(Wt::WLength(36));
    table_->elementAt(0, 1)->addNew<Wt::WText>("Name");
    table_->elementAt(0, 2)->addNew<Wt::WText>("Environment");
    table_->elementAt(0, 3)->addNew<Wt::WText>("Stack");
    table_->elementAt(0, 4)->addNew<Wt::WText>("Status");
    table_->elementAt(0, 5)->addNew<Wt::WText>("Target");
    table_->elementAt(0, 6)->addNew<Wt::WText>("Version");
    table_->elementAt(0, 7)->addNew<Wt::WText>("Deployed By");
    table_->elementAt(0, 8)->addNew<Wt::WText>("Deployed At");
    table_->elementAt(0, 9)->addNew<Wt::WText>("");
    table_->elementAt(0, 9)->setWidth(Wt::WLength(36));
    for (int c = 0; c < 10; ++c)
        table_->elementAt(0, c)->setStyleClass("fw-bold");
}

// ---------------------------------------------------------------------------
// View mode toggle
// ---------------------------------------------------------------------------

void DeploymentListView::setViewMode(bool gridMode)
{
    gridMode_ = gridMode;

    if (gridMode) {
        grid_->show();
        listWrap_->hide();
        gridBtn_->setTextFormat(Wt::TextFormat::XHTML);
        gridBtn_->setText(
            "<button class='btn btn-primary btn-sm'>"
            "<i class='bi bi-grid-3x3-gap'></i></button>");
        listBtn_->setTextFormat(Wt::TextFormat::XHTML);
        listBtn_->setText(
            "<button class='btn btn-outline-secondary btn-sm'>"
            "<i class='bi bi-list-ul'></i></button>");
    } else {
        grid_->hide();
        listWrap_->show();
        gridBtn_->setTextFormat(Wt::TextFormat::XHTML);
        gridBtn_->setText(
            "<button class='btn btn-outline-secondary btn-sm'>"
            "<i class='bi bi-grid-3x3-gap'></i></button>");
        listBtn_->setTextFormat(Wt::TextFormat::XHTML);
        listBtn_->setText(
            "<button class='btn btn-primary btn-sm'>"
            "<i class='bi bi-list-ul'></i></button>");
    }

    // Repopulate the now-visible view with cached data
    if (!deployments_.empty()) {
        if (gridMode_)
            populateGrid(deployments_);
        else
            populateTable(deployments_);
    }
}

// ---------------------------------------------------------------------------
// Reload
// ---------------------------------------------------------------------------

void DeploymentListView::reload()
{
    status_->setText("Loading...");

    std::weak_ptr<bool> weak = alive_;
    client_.getAll("Deployment", [this, weak](bool ok, const nlohmann::json& items) {
        auto guard = weak.lock();
        if (!guard || !*guard) return;
        auto* app = Wt::WApplication::instance();
        if (!app) return;

        if (!ok) {
            status_->setText("Failed to fetch deployments from ALS backend ("
                             + client_.baseUrl() + "/Deployment/).");
            app->triggerUpdate();
            return;
        }

        deployments_.clear();
        for (const auto& item : items)
            deployments_.push_back(model::Deployment::fromJson(item));

        status_->setText(std::to_string(deployments_.size()) + " deployment(s)");

        if (gridMode_)
            populateGrid(deployments_);
        else
            populateTable(deployments_);

        app->triggerUpdate();
    });
}

// ---------------------------------------------------------------------------
// Card grid
// ---------------------------------------------------------------------------

void DeploymentListView::populateGrid(const std::vector<model::Deployment>& deployments)
{
    grid_->clear();

    for (const auto& d : deployments) {
        int deployId = d.id;

        auto* card = grid_->addNew<Wt::WContainerWidget>();
        card->setStyleClass("dr-deploy-card");

        // Trash icon floated top-right
        auto* trashBtn = card->addNew<Wt::WText>(
            "<i class='bi bi-trash dr-card-trash'></i>",
            Wt::TextFormat::XHTML);
        trashBtn->setStyleClass("dr-card-trash-wrap");
        trashBtn->clicked().connect([this, deployId] {
            confirmDelete(deployId);
        });

        // Main row: icon + name + status badge
        auto* topRow = card->addNew<Wt::WContainerWidget>();
        topRow->setStyleClass("d-flex align-items-center gap-3 mb-3");

        auto* iconWrap = topRow->addNew<Wt::WText>(
            targetIcon(d.target, 48), Wt::TextFormat::XHTML);
        iconWrap->setStyleClass("dr-deploy-icon");

        auto* nameBlock = topRow->addNew<Wt::WContainerWidget>();
        nameBlock->setStyleClass("flex-grow-1 overflow-hidden");

        auto* nameLink = nameBlock->addNew<Wt::WText>(
            "<span class='dr-deploy-name'>" + htmlEncode(d.displayName())
            + "</span>", Wt::TextFormat::XHTML);
        nameLink->setStyleClass("d-block");
        nameLink->clicked().connect([this, deployId] {
            layout_.showDeploymentDetail(deployId);
        });

        auto* envText = nameBlock->addNew<Wt::WText>(d.environment_name);
        envText->setStyleClass("text-muted small d-block text-truncate");

        auto* badge = topRow->addNew<Wt::WText>(d.status);
        badge->setStyleClass("badge " + statusBadgeClass(d.status) + " ms-auto flex-shrink-0");

        // Info rows
        auto* info = card->addNew<Wt::WContainerWidget>();
        info->setStyleClass("dr-deploy-info");

        auto addRow = [&](const std::string& label, const std::string& value) {
            if (value.empty()) return;
            info->addNew<Wt::WText>(
                "<div class='dr-info-row'>"
                "<span class='dr-info-label'>" + label + "</span>"
                "<span class='dr-info-value'>" + htmlEncode(value) + "</span>"
                "</div>", Wt::TextFormat::XHTML);
        };

        addRow("Stack", d.stack_name);
        addRow("Target", d.target + " / " + d.provider);
        addRow("Version", d.version_label);
        addRow("Deployed By", d.deployed_by);
        addRow("Deployed At", d.deployed_at);
    }
}

// ---------------------------------------------------------------------------
// Table list
// ---------------------------------------------------------------------------

void DeploymentListView::populateTable(const std::vector<model::Deployment>& deployments)
{
    // Clear data rows (keep header)
    while (table_->rowCount() > 1)
        table_->removeRow(1);

    int row = 1;
    for (const auto& d : deployments) {
        int deployId = d.id;

        table_->elementAt(row, 0)->addNew<Wt::WText>(
            targetIcon(d.target, 24), Wt::TextFormat::XHTML);
        table_->elementAt(row, 0)->setStyleClass("text-center");

        auto* nameText = table_->elementAt(row, 1)->addNew<Wt::WText>(
            "<span class='dr-deploy-name'>" + htmlEncode(d.displayName())
            + "</span>", Wt::TextFormat::XHTML);
        nameText->clicked().connect([this, deployId] {
            layout_.showDeploymentDetail(deployId);
        });

        table_->elementAt(row, 2)->addNew<Wt::WText>(d.environment_name);
        table_->elementAt(row, 3)->addNew<Wt::WText>(d.stack_name);

        auto* badge = table_->elementAt(row, 4)->addNew<Wt::WText>(d.status);
        badge->setStyleClass("badge " + statusBadgeClass(d.status));

        table_->elementAt(row, 5)->addNew<Wt::WText>(d.target + " / " + d.provider);
        table_->elementAt(row, 6)->addNew<Wt::WText>(d.version_label);
        table_->elementAt(row, 7)->addNew<Wt::WText>(d.deployed_by);
        table_->elementAt(row, 8)->addNew<Wt::WText>(d.deployed_at);

        auto* trashCell = table_->elementAt(row, 9);
        trashCell->setStyleClass("dr-trash-cell");
        auto* rowTrash = trashCell->addNew<Wt::WText>(
            "<i class='bi bi-trash'></i>", Wt::TextFormat::XHTML);
        rowTrash->clicked().connect([this, deployId] {
            confirmDelete(deployId);
        });

        ++row;
    }
}

// ---------------------------------------------------------------------------
// Polling
// ---------------------------------------------------------------------------

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
    reload();
}

void DeploymentListView::stopPolling()
{
    polling_ = false;
    if (pollTimer_)
        pollTimer_->stop();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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

std::string DeploymentListView::targetIcon(const std::string& target, int size)
{
    std::string sz = std::to_string(size);

    // Docker — stacked containers icon (Bootstrap bi-stack style)
    std::string dockerSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='" + sz + "' height='" + sz + "' "
        "viewBox='0 0 16 16' fill='#2496ED'>"
        "<path d='M14.763.075A.5.5 0 0 1 15 .5v15a.5.5 0 0 1-.5.5h-3a.5.5 "
        "0 0 1-.5-.5V14h-1v1.5a.5.5 0 0 1-.5.5h-9a.5.5 0 0 1-.5-.5V10a.5.5 "
        "0 0 1 .342-.474L6 7.64V4.5a.5.5 0 0 1 .276-.447l8-4a.5.5 0 0 1 "
        ".487.022zM6 8.694 1 10.36V15h5V8.694zM7 15h2v-1.5a.5.5 0 0 1 "
        ".5-.5h2a.5.5 0 0 1 .5.5V15h2V1.309l-7 3.5V15z'/>"
        "<path d='M2 11h1v1H2v-1zm2 0h1v1H4v-1zm-2 2h1v1H2v-1zm2 "
        "0h1v1H4v-1zm4-4h1v1H8V9zm2 0h1v1h-1V9zm-2 2h1v1H8v-1zm2 "
        "0h1v1h-1v-1zm2-2h1v1h-1V9zm0 2h1v1h-1v-1zM8 7h1v1H8V7zm2 "
        "0h1v1h-1V7zm2 0h1v1h-1V7zM8 5h1v1H8V5zm2 0h1v1h-1V5zm2 "
        "0h1v1h-1V5zm0-2h1v1h-1V3z'/>"
        "</svg>";

    // Kubernetes (helm wheel)
    std::string k8sSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='" + sz + "' height='" + sz + "' "
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
    std::string genericSvg =
        "<svg xmlns='http://www.w3.org/2000/svg' width='" + sz + "' height='" + sz + "' "
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

// ---------------------------------------------------------------------------
// Delete
// ---------------------------------------------------------------------------

void DeploymentListView::confirmDelete(int deploymentId)
{
    auto* msgBox = addChild(std::make_unique<Wt::WMessageBox>(
        "Delete Deployment",
        "Are you sure you want to delete this deployment and all its "
        "related data? This action cannot be undone.",
        Wt::Icon::Warning,
        Wt::StandardButton::Yes | Wt::StandardButton::No));
    msgBox->setModal(true);
    msgBox->buttonClicked().connect([this, msgBox, deploymentId] {
        if (msgBox->buttonResult() == Wt::StandardButton::Yes) {
            executeDelete(deploymentId);
        }
        removeChild(msgBox);
    });
    msgBox->show();
}

void DeploymentListView::executeDelete(int deploymentId)
{
    status_->setText("Deleting deployment #" + std::to_string(deploymentId) + "...");

    // Number of child resource types to cascade-delete before the parent.
    auto pending = std::make_shared<int>(6);
    std::weak_ptr<bool> weak = alive_;

    auto onChildrenDone = [this, deploymentId, pending, weak] {
        if (--(*pending) > 0) return;

        auto guard = weak.lock();
        if (!guard || !*guard) return;

        client_.remove("Deployment", deploymentId,
            [this, weak](bool ok) {
                auto g = weak.lock();
                if (!g || !*g) return;
                auto* a = Wt::WApplication::instance();
                if (!a) return;

                if (ok) {
                    reload();
                } else {
                    status_->setText("Failed to delete deployment.");
                }
                a->triggerUpdate();
            });
    };

    static const char* childTypes[] = {
        "DeploymentImage", "DeploymentPort", "DeploymentEnvVar",
        "DeploymentHealth", "DeploymentTarget", "DeploymentLog"
    };

    for (const char* type : childTypes) {
        client_.getAll(std::string(type), "deployment_id", deploymentId,
            [this, type = std::string(type), onChildrenDone, weak]
            (bool ok, const nlohmann::json& items) {
                auto guard = weak.lock();
                if (!guard || !*guard) return;

                if (!ok || items.empty()) {
                    onChildrenDone();
                    auto* a = Wt::WApplication::instance();
                    if (a) a->triggerUpdate();
                    return;
                }

                auto childPending = std::make_shared<int>(static_cast<int>(items.size()));
                for (const auto& item : items) {
                    int childId = model::jint(item, "id");
                    client_.remove(type, childId,
                        [childPending, onChildrenDone, weak](bool /*ok*/) {
                            auto g = weak.lock();
                            if (!g || !*g) return;
                            if (--(*childPending) == 0) {
                                onChildrenDone();
                            }
                            auto* a = Wt::WApplication::instance();
                            if (a) a->triggerUpdate();
                        });
                }
            });
    }
}

} // namespace dr
