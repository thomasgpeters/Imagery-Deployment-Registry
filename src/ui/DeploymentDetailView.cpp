#include "ui/DeploymentDetailView.h"

#include <Wt/WApplication.h>
#include <Wt/WTable.h>

namespace dr {

DeploymentDetailView::DeploymentDetailView(api::AlsClient& client)
    : client_(client)
{
    buildUI();
}

void DeploymentDetailView::buildUI()
{
    setStyleClass("container-fluid");

    title_ = addNew<Wt::WText>("<h4>Deployment Detail</h4>", Wt::TextFormat::XHTML);
    title_->setStyleClass("mb-3");

    tabs_ = addNew<Wt::WTabWidget>();
    tabs_->setStyleClass("mt-2");

    overviewTab_ = new Wt::WContainerWidget();
    tabs_->addTab(std::unique_ptr<Wt::WContainerWidget>(overviewTab_),
                  "<i class='bi bi-info-circle me-1'></i>Overview",
                  Wt::ContentLoading::Eager)->setTextFormat(Wt::TextFormat::XHTML);

    imagesTab_ = new Wt::WContainerWidget();
    tabs_->addTab(std::unique_ptr<Wt::WContainerWidget>(imagesTab_),
                  "<i class='bi bi-layers me-1'></i>Images",
                  Wt::ContentLoading::Eager)->setTextFormat(Wt::TextFormat::XHTML);

    portsTab_ = new Wt::WContainerWidget();
    tabs_->addTab(std::unique_ptr<Wt::WContainerWidget>(portsTab_),
                  "<i class='bi bi-plug me-1'></i>Ports",
                  Wt::ContentLoading::Eager)->setTextFormat(Wt::TextFormat::XHTML);

    composeTab_ = new Wt::WContainerWidget();
    tabs_->addTab(std::unique_ptr<Wt::WContainerWidget>(composeTab_),
                  "<i class='bi bi-file-earmark-code me-1'></i>Compose",
                  Wt::ContentLoading::Eager)->setTextFormat(Wt::TextFormat::XHTML);

    envVarsTab_ = new Wt::WContainerWidget();
    tabs_->addTab(std::unique_ptr<Wt::WContainerWidget>(envVarsTab_),
                  "<i class='bi bi-gear me-1'></i>Env Vars",
                  Wt::ContentLoading::Eager)->setTextFormat(Wt::TextFormat::XHTML);

    healthTab_ = new Wt::WContainerWidget();
    tabs_->addTab(std::unique_ptr<Wt::WContainerWidget>(healthTab_),
                  "<i class='bi bi-heart-pulse me-1'></i>Health",
                  Wt::ContentLoading::Eager)->setTextFormat(Wt::TextFormat::XHTML);

    auditLogTab_ = new Wt::WContainerWidget();
    tabs_->addTab(std::unique_ptr<Wt::WContainerWidget>(auditLogTab_),
                  "<i class='bi bi-journal-text me-1'></i>Audit Log",
                  Wt::ContentLoading::Eager)->setTextFormat(Wt::TextFormat::XHTML);
}

// ---------------------------------------------------------------------------
// loadDeployment
// ---------------------------------------------------------------------------

void DeploymentDetailView::loadDeployment(int deploymentId)
{
    currentDeploymentId_ = deploymentId;
    title_->setTextFormat(Wt::TextFormat::XHTML);
    title_->setText("<h4>Deployment #" + std::to_string(deploymentId) + "</h4>");

    // Fetch core deployment record
    client_.getOne("Deployment", deploymentId,
        [this, deploymentId](bool ok, const nlohmann::json& item) {
            if (!ok) {
                title_->setText("<h4>Deployment not found</h4>");  // format already set to XHTML
                return;
            }
            auto d = model::Deployment::fromJson(item);
            populateOverview(d);
            populateCompose(d.compose_content);

            Wt::WApplication::instance()->triggerUpdate();
        });

    // Fetch child resources in parallel
    fetchChildResources(deploymentId);
}

void DeploymentDetailView::fetchChildResources(int deploymentId)
{
    // Images
    client_.getAll("DeploymentImage",
        [this, deploymentId](bool ok, const nlohmann::json& items) {
            std::vector<model::DeploymentImage> images;
            if (ok) {
                for (const auto& it : items) {
                    auto img = model::DeploymentImage::fromJson(it);
                    if (img.deployment_id == deploymentId)
                        images.push_back(img);
                }
            }
            populateImages(images);
            Wt::WApplication::instance()->triggerUpdate();
        });

    // Ports
    client_.getAll("DeploymentPort",
        [this, deploymentId](bool ok, const nlohmann::json& items) {
            std::vector<model::DeploymentPort> ports;
            if (ok) {
                for (const auto& it : items) {
                    auto p = model::DeploymentPort::fromJson(it);
                    if (p.deployment_id == deploymentId)
                        ports.push_back(p);
                }
            }
            populatePorts(ports);
            Wt::WApplication::instance()->triggerUpdate();
        });

    // Env Vars
    client_.getAll("DeploymentEnvVar",
        [this, deploymentId](bool ok, const nlohmann::json& items) {
            std::vector<model::DeploymentEnvVar> vars;
            if (ok) {
                for (const auto& it : items) {
                    auto v = model::DeploymentEnvVar::fromJson(it);
                    if (v.deployment_id == deploymentId)
                        vars.push_back(v);
                }
            }
            populateEnvVars(vars);
            Wt::WApplication::instance()->triggerUpdate();
        });

    // Health
    client_.getAll("DeploymentHealth",
        [this, deploymentId](bool ok, const nlohmann::json& items) {
            std::vector<model::DeploymentHealth> checks;
            if (ok) {
                for (const auto& it : items) {
                    auto h = model::DeploymentHealth::fromJson(it);
                    if (h.deployment_id == deploymentId)
                        checks.push_back(h);
                }
            }
            populateHealth(checks);
            Wt::WApplication::instance()->triggerUpdate();
        });

    // Audit Log
    client_.getAll("DeploymentLog",
        [this, deploymentId](bool ok, const nlohmann::json& items) {
            std::vector<model::DeploymentLog> logs;
            if (ok) {
                for (const auto& it : items) {
                    auto l = model::DeploymentLog::fromJson(it);
                    if (l.deployment_id == deploymentId)
                        logs.push_back(l);
                }
            }
            populateAuditLog(logs);
            Wt::WApplication::instance()->triggerUpdate();
        });
}

// ---------------------------------------------------------------------------
// Tab populators
// ---------------------------------------------------------------------------

void DeploymentDetailView::populateOverview(const model::Deployment& d)
{
    overviewTab_->clear();

    auto* tbl = overviewTab_->addNew<Wt::WTable>();
    tbl->setStyleClass("table table-bordered w-auto");

    auto addRow = [tbl](const std::string& label, const std::string& value) {
        int row = tbl->rowCount();
        tbl->elementAt(row, 0)->addNew<Wt::WText>(label);
        tbl->elementAt(row, 0)->setStyleClass("fw-bold text-muted pe-4");
        tbl->elementAt(row, 1)->addNew<Wt::WText>(value);
    };

    addRow("Environment",   d.environment_name);
    addRow("Stack",         d.stack_name);
    addRow("Pipeline",      d.pipeline_name);
    addRow("Target",        d.target + " / " + d.provider);
    addRow("Status",        d.status);
    addRow("Version",       d.version_label);
    addRow("Deployed By",   d.deployed_by);
    addRow("Deployed At",   d.deployed_at);
    addRow("Finished At",   d.finished_at);
    addRow("Project Name",  d.compose_project_name);

    if (!d.notes.empty()) {
        auto* notesBox = overviewTab_->addNew<Wt::WContainerWidget>();
        notesBox->setStyleClass("mt-3 p-3 bg-light rounded");
        notesBox->addNew<Wt::WText>("<strong>Notes:</strong><br/>" + d.notes, Wt::TextFormat::XHTML);
    }
}

void DeploymentDetailView::populateImages(const std::vector<model::DeploymentImage>& images)
{
    imagesTab_->clear();

    if (images.empty()) {
        imagesTab_->addNew<Wt::WText>("<em class='text-muted'>No images recorded.</em>", Wt::TextFormat::XHTML);
        return;
    }

    auto* tbl = imagesTab_->addNew<Wt::WTable>();
    tbl->setStyleClass("table table-hover table-striped align-middle");
    tbl->setHeaderCount(1);

    tbl->elementAt(0, 0)->addNew<Wt::WText>("Tier");
    tbl->elementAt(0, 1)->addNew<Wt::WText>("Service");
    tbl->elementAt(0, 2)->addNew<Wt::WText>("Image");
    tbl->elementAt(0, 3)->addNew<Wt::WText>("Tag");
    tbl->elementAt(0, 4)->addNew<Wt::WText>("Registry");
    tbl->elementAt(0, 5)->addNew<Wt::WText>("Digest");
    for (int c = 0; c < 6; ++c)
        tbl->elementAt(0, c)->setStyleClass("fw-bold");

    int row = 1;
    for (const auto& img : images) {
        tbl->elementAt(row, 0)->addNew<Wt::WText>(img.tier);
        tbl->elementAt(row, 1)->addNew<Wt::WText>(img.service_name);
        tbl->elementAt(row, 2)->addNew<Wt::WText>(img.image_name);
        tbl->elementAt(row, 3)->addNew<Wt::WText>(img.tag);
        tbl->elementAt(row, 4)->addNew<Wt::WText>(
            img.registry_host.empty() ? "(local)" : img.registry_host);
        tbl->elementAt(row, 5)->addNew<Wt::WText>(
            img.digest.empty() ? "-" : img.digest.substr(0, 16) + "...");
        ++row;
    }
}

void DeploymentDetailView::populatePorts(const std::vector<model::DeploymentPort>& ports)
{
    portsTab_->clear();

    if (ports.empty()) {
        portsTab_->addNew<Wt::WText>("<em class='text-muted'>No port mappings recorded.</em>", Wt::TextFormat::XHTML);
        return;
    }

    auto* tbl = portsTab_->addNew<Wt::WTable>();
    tbl->setStyleClass("table table-hover table-striped align-middle");
    tbl->setHeaderCount(1);

    tbl->elementAt(0, 0)->addNew<Wt::WText>("Service");
    tbl->elementAt(0, 1)->addNew<Wt::WText>("Host Port");
    tbl->elementAt(0, 2)->addNew<Wt::WText>("Container Port");
    tbl->elementAt(0, 3)->addNew<Wt::WText>("Protocol");
    tbl->elementAt(0, 4)->addNew<Wt::WText>("Mapping");
    for (int c = 0; c < 5; ++c)
        tbl->elementAt(0, c)->setStyleClass("fw-bold");

    int row = 1;
    for (const auto& p : ports) {
        tbl->elementAt(row, 0)->addNew<Wt::WText>(p.service_name);
        tbl->elementAt(row, 1)->addNew<Wt::WText>(std::to_string(p.host_port));
        tbl->elementAt(row, 2)->addNew<Wt::WText>(std::to_string(p.container_port));
        tbl->elementAt(row, 3)->addNew<Wt::WText>(p.protocol);
        tbl->elementAt(row, 4)->addNew<Wt::WText>(p.mappingString());
        ++row;
    }
}

void DeploymentDetailView::populateCompose(const std::string& composeContent)
{
    composeTab_->clear();

    if (composeContent.empty()) {
        composeTab_->addNew<Wt::WText>(
            "<em class='text-muted'>No compose content captured for this deployment.</em>",
            Wt::TextFormat::XHTML);
        return;
    }

    composeTab_->addNew<Wt::WText>(
        "<pre class='bg-dark text-light p-3 rounded' "
        "style='max-height:600px;overflow:auto;white-space:pre-wrap;'>" +
        composeContent + "</pre>",
        Wt::TextFormat::XHTML);
}

void DeploymentDetailView::populateEnvVars(const std::vector<model::DeploymentEnvVar>& vars)
{
    envVarsTab_->clear();

    if (vars.empty()) {
        envVarsTab_->addNew<Wt::WText>(
            "<em class='text-muted'>No environment variables recorded.</em>",
            Wt::TextFormat::XHTML);
        return;
    }

    auto* tbl = envVarsTab_->addNew<Wt::WTable>();
    tbl->setStyleClass("table table-hover table-striped align-middle");
    tbl->setHeaderCount(1);

    tbl->elementAt(0, 0)->addNew<Wt::WText>("Service");
    tbl->elementAt(0, 1)->addNew<Wt::WText>("Variable");
    tbl->elementAt(0, 2)->addNew<Wt::WText>("Value");
    tbl->elementAt(0, 3)->addNew<Wt::WText>("Secret");
    for (int c = 0; c < 4; ++c)
        tbl->elementAt(0, c)->setStyleClass("fw-bold");

    int row = 1;
    for (const auto& v : vars) {
        tbl->elementAt(row, 0)->addNew<Wt::WText>(v.service_name);
        tbl->elementAt(row, 1)->addNew<Wt::WText>(v.var_name);
        tbl->elementAt(row, 2)->addNew<Wt::WText>(v.displayValue());
        tbl->elementAt(row, 3)->addNew<Wt::WText>(v.is_secret ? "Yes" : "No");
        if (v.is_secret)
            tbl->elementAt(row, 2)->setStyleClass("text-muted fst-italic");
        ++row;
    }
}

void DeploymentDetailView::populateHealth(const std::vector<model::DeploymentHealth>& checks)
{
    healthTab_->clear();

    if (checks.empty()) {
        healthTab_->addNew<Wt::WText>(
            "<em class='text-muted'>No health check data available.</em>",
            Wt::TextFormat::XHTML);
        return;
    }

    auto* tbl = healthTab_->addNew<Wt::WTable>();
    tbl->setStyleClass("table table-hover table-striped align-middle");
    tbl->setHeaderCount(1);

    tbl->elementAt(0, 0)->addNew<Wt::WText>("Service");
    tbl->elementAt(0, 1)->addNew<Wt::WText>("Status");
    tbl->elementAt(0, 2)->addNew<Wt::WText>("Checked At");
    tbl->elementAt(0, 3)->addNew<Wt::WText>("Response (ms)");
    tbl->elementAt(0, 4)->addNew<Wt::WText>("Message");
    for (int c = 0; c < 5; ++c)
        tbl->elementAt(0, c)->setStyleClass("fw-bold");

    int row = 1;
    for (const auto& h : checks) {
        tbl->elementAt(row, 0)->addNew<Wt::WText>(h.service_name);
        auto* statusBadge = tbl->elementAt(row, 1)->addNew<Wt::WText>(h.status);
        if (h.status == "Healthy")
            statusBadge->setStyleClass("badge bg-success");
        else if (h.status == "Unhealthy")
            statusBadge->setStyleClass("badge bg-danger");
        else if (h.status == "Starting")
            statusBadge->setStyleClass("badge bg-warning text-dark");
        else
            statusBadge->setStyleClass("badge bg-secondary");

        tbl->elementAt(row, 2)->addNew<Wt::WText>(h.checked_at);
        tbl->elementAt(row, 3)->addNew<Wt::WText>(std::to_string(h.response_time_ms));
        tbl->elementAt(row, 4)->addNew<Wt::WText>(h.message);
        ++row;
    }
}

void DeploymentDetailView::populateAuditLog(const std::vector<model::DeploymentLog>& logs)
{
    auditLogTab_->clear();

    if (logs.empty()) {
        auditLogTab_->addNew<Wt::WText>(
            "<em class='text-muted'>No audit log entries.</em>",
            Wt::TextFormat::XHTML);
        return;
    }

    auto* tbl = auditLogTab_->addNew<Wt::WTable>();
    tbl->setStyleClass("table table-hover table-striped align-middle");
    tbl->setHeaderCount(1);

    tbl->elementAt(0, 0)->addNew<Wt::WText>("Time");
    tbl->elementAt(0, 1)->addNew<Wt::WText>("Action");
    tbl->elementAt(0, 2)->addNew<Wt::WText>("Status");
    tbl->elementAt(0, 3)->addNew<Wt::WText>("By");
    tbl->elementAt(0, 4)->addNew<Wt::WText>("Message");
    for (int c = 0; c < 5; ++c)
        tbl->elementAt(0, c)->setStyleClass("fw-bold");

    int row = 1;
    for (const auto& l : logs) {
        tbl->elementAt(row, 0)->addNew<Wt::WText>(l.created_at);
        tbl->elementAt(row, 1)->addNew<Wt::WText>(l.action);

        auto* sBadge = tbl->elementAt(row, 2)->addNew<Wt::WText>(l.status);
        if (l.status == "Succeeded")
            sBadge->setStyleClass("badge bg-success");
        else if (l.status == "Failed")
            sBadge->setStyleClass("badge bg-danger");
        else
            sBadge->setStyleClass("badge bg-info text-dark");

        tbl->elementAt(row, 3)->addNew<Wt::WText>(l.created_by);
        tbl->elementAt(row, 4)->addNew<Wt::WText>(l.message);
        ++row;
    }
}

} // namespace dr
