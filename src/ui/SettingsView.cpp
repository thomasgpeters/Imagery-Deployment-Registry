#include "ui/SettingsView.h"

#include <Wt/WBreak.h>
#include <Wt/WLabel.h>
#include <Wt/WText.h>

namespace dr {

SettingsView::SettingsView()
{
    buildUI();
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------

void SettingsView::buildUI()
{
    setStyleClass("container-fluid");

    // Title
    addNew<Wt::WText>("<h4>Settings</h4>", Wt::TextFormat::XHTML);
    addNew<Wt::WText>(
        "<p class='text-muted mb-4'>Configure polling and pull settings "
        "for the Deployment Registry.</p>",
        Wt::TextFormat::XHTML);

    // ── Card: Polling Timer ──────────────────────────────────────────────
    auto* timerCard = addNew<Wt::WContainerWidget>();
    timerCard->setStyleClass("dr-tab-card mb-4");

    timerCard->addNew<Wt::WText>(
        "<h6 class='mb-3'><i class='bi bi-clock me-2'></i>"
        "Polling Timer</h6>",
        Wt::TextFormat::XHTML);

    // Enable toggle row
    auto* enableRow = timerCard->addNew<Wt::WContainerWidget>();
    enableRow->setStyleClass("d-flex align-items-center gap-3 mb-3");

    enableCheck_ = enableRow->addNew<Wt::WCheckBox>("Enable auto-refresh polling");
    enableCheck_->setStyleClass("form-check-input me-1");
    enableCheck_->setChecked(false);

    // Interval row
    auto* intervalRow = timerCard->addNew<Wt::WContainerWidget>();
    intervalRow->setStyleClass("row align-items-center mb-3");

    auto* labelCol = intervalRow->addNew<Wt::WContainerWidget>();
    labelCol->setStyleClass("col-auto");
    auto* intervalLabel = labelCol->addNew<Wt::WLabel>("Interval (seconds):");
    intervalLabel->setStyleClass("form-label mb-0");

    auto* spinCol = intervalRow->addNew<Wt::WContainerWidget>();
    spinCol->setStyleClass("col-auto");
    intervalSpin_ = spinCol->addNew<Wt::WSpinBox>();
    intervalSpin_->setStyleClass("form-control");
    intervalSpin_->setRange(5, 300);
    intervalSpin_->setValue(15);
    intervalSpin_->setSingleStep(5);
    intervalSpin_->setWidth(Wt::WLength(100));

    auto* hintCol = intervalRow->addNew<Wt::WContainerWidget>();
    hintCol->setStyleClass("col-auto");
    hintCol->addNew<Wt::WText>(
        "<span class='text-muted small'>5–300 seconds</span>",
        Wt::TextFormat::XHTML);

    intervalLabel->setBuddy(intervalSpin_);

    // ── Separator ────────────────────────────────────────────────────────
    addNew<Wt::WText>("<hr class='my-2'/>", Wt::TextFormat::XHTML);

    // ── Card: VCP Configuration ──────────────────────────────────────────
    auto* vcpCard = addNew<Wt::WContainerWidget>();
    vcpCard->setStyleClass("dr-tab-card mb-4");

    vcpCard->addNew<Wt::WText>(
        "<h6 class='mb-3'><i class='bi bi-cloud-arrow-down me-2'></i>"
        "VCP Pull Configuration</h6>",
        Wt::TextFormat::XHTML);

    auto* urlRow = vcpCard->addNew<Wt::WContainerWidget>();
    urlRow->setStyleClass("mb-3");

    auto* urlLabel = urlRow->addNew<Wt::WLabel>("VCP Base URL:");
    urlLabel->setStyleClass("form-label");

    vcpUrlEdit_ = urlRow->addNew<Wt::WLineEdit>();
    vcpUrlEdit_->setStyleClass("form-control");
    vcpUrlEdit_->setPlaceholderText("http://127.0.0.1:5670/api");
    vcpUrlEdit_->setTextSize(60);

    auto* urlHint = urlRow->addNew<Wt::WText>(
        "<div class='form-text text-muted'>"
        "Base URL for the VCP / ALS backend that the registry polls for "
        "deployment data. Leave empty to use the default from environment "
        "variables (ALS_URL, ALS_HOST, ALS_PORT).</div>",
        Wt::TextFormat::XHTML);

    urlLabel->setBuddy(vcpUrlEdit_);

    // ── Apply button ─────────────────────────────────────────────────────
    addNew<Wt::WText>("<hr class='my-2'/>", Wt::TextFormat::XHTML);

    auto* actionRow = addNew<Wt::WContainerWidget>();
    actionRow->setStyleClass("d-flex align-items-center gap-3");

    auto* applyBtn = actionRow->addNew<Wt::WText>(
        "<button class='btn btn-primary'>"
        "<i class='bi bi-check-lg me-1'></i>Apply</button>",
        Wt::TextFormat::XHTML);

    statusMsg_ = actionRow->addNew<Wt::WText>("");
    statusMsg_->setStyleClass("text-success small");

    applyBtn->clicked().connect([this] {
        statusMsg_->setText("Settings applied.");
        settingsChanged_.emit();
    });
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

bool SettingsView::timerEnabled() const
{
    return enableCheck_ ? enableCheck_->isChecked() : false;
}

int SettingsView::timerIntervalSec() const
{
    return intervalSpin_ ? intervalSpin_->value() : 15;
}

std::string SettingsView::vcpBaseUrl() const
{
    return vcpUrlEdit_ ? vcpUrlEdit_->text().toUTF8() : "";
}

} // namespace dr
