#include "ui/MainLayout.h"
#include "ui/DeploymentDetailView.h"
#include "ui/DeploymentListView.h"

#include <Wt/WApplication.h>
#include <Wt/WImage.h>
#include <Wt/WText.h>
#include <cstdlib>

namespace dr {

MainLayout::MainLayout()
{
    // Pick up ALS URL from environment if set
    if (const char* url = std::getenv("ALS_URL"))
        alsClient_.setBaseUrl(url);

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
        "<i class='bi bi-box-seam me-2'></i>Deployment Registry", Wt::TextFormat::XHTML);
    brandTitle_->setStyleClass("h6 text-white");

    // Separator
    sidebar_->addNew<Wt::WText>("<hr class='border-secondary'>", Wt::TextFormat::XHTML);

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

    // Version
    sidebar_->addNew<Wt::WText>("<hr class='border-secondary mt-auto'>", Wt::TextFormat::XHTML);
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
    content_->setStyleClass("flex-grow-1 overflow-auto p-4");
}

// ---------------------------------------------------------------------------
// Routing
// ---------------------------------------------------------------------------

void MainLayout::handleInternalPath(const std::string& path)
{
    if (path.find("/deployment/") == 0) {
        // Extract deployment ID from path: /deployment/{id}
        std::string idStr = path.substr(std::string("/deployment/").size());
        try {
            int id = std::stoi(idStr);
            showDeploymentDetail(id);
        } catch (...) {
            menu_->select(0);
        }
    } else if (path == "/deployments" || path == "/") {
        menu_->select(0);
        listView_->refresh();
    }
}

void MainLayout::showDeploymentDetail(int deploymentId)
{
    menu_->select(1);
    detailView_->loadDeployment(deploymentId);
    Wt::WApplication::instance()->setInternalPath(
        "/deployment/" + std::to_string(deploymentId), false);
}

} // namespace dr
