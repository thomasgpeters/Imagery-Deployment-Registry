#pragma once

#include <Wt/WContainerWidget.h>
#include <Wt/WCheckBox.h>
#include <Wt/WLineEdit.h>
#include <Wt/WSpinBox.h>
#include <Wt/WText.h>

#include <Wt/WSignal.h>

#include <string>

namespace dr {

/// Settings view — configures polling timer and VCP base URL.
///
/// Emits settingsChanged() whenever any value is modified and applied.
class SettingsView : public Wt::WContainerWidget {
public:
    SettingsView();

    // ── Accessors ────────────────────────────────────────────────────────

    bool   timerEnabled()    const;
    int    timerIntervalSec() const;
    std::string vcpBaseUrl() const;

    /// Signal emitted when user clicks "Apply".
    Wt::Signal<>& settingsChanged() { return settingsChanged_; }

private:
    void buildUI();

    Wt::WCheckBox* enableCheck_   = nullptr;
    Wt::WSpinBox*  intervalSpin_  = nullptr;
    Wt::WLineEdit* vcpUrlEdit_    = nullptr;
    Wt::WText*     statusMsg_     = nullptr;

    Wt::Signal<> settingsChanged_;
};

} // namespace dr
