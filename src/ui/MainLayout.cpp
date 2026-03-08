#include "ui/MainLayout.h"
#include "ui/DeploymentDetailView.h"
#include "ui/DeploymentListView.h"
#include "ui/SettingsView.h"

#include <Wt/WApplication.h>
#include <Wt/WImage.h>
#include <Wt/WLogger.h>
#include <Wt/WText.h>
#include <cstdlib>

namespace dr {

MainLayout::MainLayout()
{
    // Pick up ALS connection from environment (matches scripts/env.sh)
    if (const char* url = std::getenv("ALS_URL")) {
        alsClient_.setBaseUrl(url);
    } else {
        const char* host = std::getenv("ALS_HOST");
        const char* port = std::getenv("ALS_PORT");
        if (host || port) {
            alsClient_.setBaseUrl(
                "http://" + std::string(host ? host : "127.0.0.1")
                + ":" + std::string(port ? port : "5670") + "/api");
        }
    }

    Wt::log("info") << "MainLayout: ALS base URL = " << alsClient_.baseUrl();

    setStyleClass("d-flex flex-grow-1 overflow-hidden");

    buildSidebar();
    buildContent();
}

// ---------------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------------

void MainLayout::buildSidebar()
{
    sidebar_ = addNew<Wt::WContainerWidget>();
    sidebar_->setStyleClass("dr-sidebar d-flex flex-column p-3");
    sidebar_->setWidth(Wt::WLength(240));

    // Brand
    auto* brandBox = sidebar_->addNew<Wt::WContainerWidget>();
    brandBox->setStyleClass("text-center mb-4");
    brandTitle_ = brandBox->addNew<Wt::WText>(
        "<span class='bi bi-box-seam me-2'></span>Deployment Registry", Wt::TextFormat::XHTML);
    brandTitle_->setStyleClass("h6 text-white");

    // Separator
    sidebar_->addNew<Wt::WText>("<hr class='border-secondary'/>", Wt::TextFormat::XHTML);

    // Navigation menu
    content_ = new Wt::WStackedWidget();
    menu_ = sidebar_->addNew<Wt::WMenu>(content_);
    menu_->setStyleClass("nav flex-column");
    menu_->setInternalPathEnabled("/");

    // List view
    listView_ = new DeploymentListView(alsClient_, *this);
    auto* listItem = menu_->addItem(
        "Deployments", std::unique_ptr<DeploymentListView>(listView_));
    listItem->setPathComponent("deployments");
    listItem->setStyleClass("nav-item");

    // Detail view
    detailView_ = new DeploymentDetailView(alsClient_);
    auto* detailItem = menu_->addItem(
        "Detail", std::unique_ptr<DeploymentDetailView>(detailView_));
    detailItem->setPathComponent("deployment");
    detailItem->setStyleClass("nav-item");

    // ── Separator before settings ────────────────────────────────────────
    sidebar_->addNew<Wt::WText>(
        "<hr class='border-secondary my-2'/>", Wt::TextFormat::XHTML);

    auto* settingsLabel = sidebar_->addNew<Wt::WText>(
        "<span class='text-muted small text-uppercase px-1'>Configuration</span>",
        Wt::TextFormat::XHTML);

    // Settings view
    settingsView_ = new SettingsView();
    auto* settingsItem = menu_->addItem(
        "<i class='bi bi-gear me-2'></i>Settings",
        std::unique_ptr<SettingsView>(settingsView_));
    settingsItem->setPathComponent("settings");
    settingsItem->setStyleClass("nav-item");

    // Wire settings → apply
    settingsView_->settingsChanged().connect(this, &MainLayout::applySettings);

    // Version
    sidebar_->addNew<Wt::WText>("<hr class='border-secondary mt-auto'/>", Wt::TextFormat::XHTML);
    auto* ver = sidebar_->addNew<Wt::WText>("v0.1.0");
    ver->setStyleClass("text-muted small text-center");
}

// ---------------------------------------------------------------------------
// Content area
// ---------------------------------------------------------------------------

void MainLayout::buildContent()
{
    auto* rightCol = addNew<Wt::WContainerWidget>();
    rightCol->setStyleClass("d-flex flex-column flex-grow-1 overflow-hidden");

    // Content
    rightCol->addWidget(std::unique_ptr<Wt::WStackedWidget>(content_));
    content_->setStyleClass("flex-grow-1 overflow-auto p-4 dr-content-area");
}

// ---------------------------------------------------------------------------
// Routing
// ---------------------------------------------------------------------------

void MainLayout::handleInternalPath(const std::string& path)
{
    if (handlingPath_)
        return;
    handlingPath_ = true;

    if (path.find("/deployment/") == 0) {
        std::string idStr = path.substr(std::string("/deployment/").size());
        try {
            int id = std::stoi(idStr);
            showDeploymentDetail(id);
        } catch (...) {
            menu_->select(0);
        }
    } else if (path == "/settings") {
        menu_->select(2);
    } else if (path == "/deployments" || path == "/") {
        menu_->select(0);
        listView_->reload();
    }

    handlingPath_ = false;
}

void MainLayout::showDeploymentDetail(int deploymentId)
{
    menu_->select(1);
    detailView_->loadDeployment(deploymentId);
    Wt::WApplication::instance()->setInternalPath(
        "/deployment/" + std::to_string(deploymentId), false);
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

void MainLayout::applySettings()
{
    if (!settingsView_) return;

    // Update VCP / ALS base URL if provided
    std::string url = settingsView_->vcpBaseUrl();
    if (!url.empty()) {
        alsClient_.setBaseUrl(url);
        Wt::log("info") << "MainLayout: ALS base URL changed to " << url;
    }

    // Update polling timer
    if (settingsView_->timerEnabled()) {
        int interval = settingsView_->timerIntervalSec();
        listView_->stopPolling();
        listView_->startPolling(interval);
        Wt::log("info") << "MainLayout: Polling enabled, interval="
                         << interval << "s";
    } else {
        listView_->stopPolling();
        Wt::log("info") << "MainLayout: Polling disabled";
    }
}

} // namespace dr
