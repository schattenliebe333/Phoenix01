#include "rael/shadow_sim.h"
#include "rael/events.h"
#include "rael/improvements.h"
#include <algorithm>
#include <sstream>

namespace rael {

// Global instances
ShadowSimulator gShadowSim;
RollbackManager gRollback;

// ═══════════════════════════════════════════════════════════════════════════
// SHADOW SIMULATOR IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

ShadowSimulator::ShadowSimulator() {}

ShadowState ShadowSimulator::create_shadow(const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);

    ShadowState shadow;
    shadow.id = next_id_++;
    shadow.description = description;
    shadow.created = std::chrono::steady_clock::now();

    // Event emittieren
    EventBus::push("SHADOW_CREATE",
        "id=" + std::to_string(shadow.id) + "|desc=" + description);

    return shadow;
}

ShadowResult ShadowSimulator::simulate(ShadowState& shadow, const ShadowAction& action) {
    ShadowResult result;

    // 1. Validierung
    std::string validation_error;
    if (action.validate && !action.validate(shadow, validation_error)) {
        result.safe_to_apply = false;
        result.risk_score = 1.0;
        result.recommendation = "ABORT: Validierung fehlgeschlagen - " + validation_error;
        result.warnings.push_back(validation_error);
        shadow.errors_count++;

        EventBus::push("SHADOW_VALIDATE_FAIL",
            "id=" + std::to_string(shadow.id) + "|error=" + validation_error);

        return result;
    }

    // 2. Simulation ausführen
    auto start_time = std::chrono::steady_clock::now();

    try {
        if (action.execute) {
            shadow.success = action.execute(shadow);
            shadow.executed = true;
            shadow.operations_count++;
        }
    } catch (const std::exception& e) {
        shadow.success = false;
        shadow.error_message = e.what();
        shadow.errors_count++;
    } catch (...) {
        shadow.success = false;
        shadow.error_message = "Unbekannter Fehler";
        shadow.errors_count++;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time).count();

    // Timeout-Check
    if (duration_ms > timeout_ms_) {
        result.warnings.push_back("Simulation hat Timeout überschritten");
        shadow.warnings_count++;
    }

    // 3. Risiko berechnen
    result.risk_score = calculate_risk(shadow, action);

    // 4. Änderungs-Preview generieren
    result.changes_preview = generate_preview(shadow);

    // 5. Rollback-Information
    result.rollback_command = generate_rollback_command(action);
    result.rollback_possible = (action.rollback != nullptr);

    // 6. Empfehlung generieren
    if (result.risk_score > risk_threshold_) {
        result.safe_to_apply = false;
        result.recommendation = "WARNUNG: Hohes Risiko (" +
            std::to_string((int)(result.risk_score * 100)) +
            "%). Manuelle Überprüfung empfohlen.";
    } else if (shadow.errors_count > 0) {
        result.safe_to_apply = false;
        result.recommendation = "FEHLER: Simulation hatte Fehler. Nicht anwenden.";
    } else if (shadow.warnings_count > 2) {
        result.safe_to_apply = false;
        result.recommendation = "VORSICHT: Mehrere Warnungen. Überprüfung empfohlen.";
    } else {
        result.safe_to_apply = true;
        result.recommendation = "OK: Simulation erfolgreich. Kann angewendet werden.";
    }

    // History speichern
    {
        std::lock_guard<std::mutex> lock(mutex_);
        history_.push_back(shadow);
        if (history_.size() > 1000) {
            prune_history(500);
        }
    }

    // Event emittieren
    std::ostringstream oss;
    oss << "id=" << shadow.id
        << "|success=" << (shadow.success ? "1" : "0")
        << "|risk=" << result.risk_score
        << "|safe=" << (result.safe_to_apply ? "1" : "0");
    EventBus::push("SHADOW_RESULT", oss.str());

    if (verbose_) {
        EventBus::push("SHADOW_DETAIL",
            "ops=" + std::to_string(shadow.operations_count) +
            "|warnings=" + std::to_string(shadow.warnings_count) +
            "|errors=" + std::to_string(shadow.errors_count));
    }

    return result;
}

ShadowResult ShadowSimulator::simulate_sequence(
    ShadowState& shadow,
    const std::vector<ShadowAction>& actions) {

    ShadowResult combined;
    combined.safe_to_apply = true;
    combined.risk_score = 0.0;

    for (const auto& action : actions) {
        ShadowResult step_result = simulate(shadow, action);

        // Kombiniere Ergebnisse
        combined.risk_score = std::max(combined.risk_score, step_result.risk_score);
        combined.safe_to_apply = combined.safe_to_apply && step_result.safe_to_apply;

        for (const auto& w : step_result.warnings) {
            combined.warnings.push_back(action.name + ": " + w);
        }
        for (const auto& c : step_result.changes_preview) {
            combined.changes_preview.push_back(action.name + ": " + c);
        }

        // Abbrechen bei kritischem Fehler
        if (!shadow.success || combined.risk_score > 0.9) {
            combined.recommendation = "ABBRUCH: Kritischer Fehler in Schritt '" + action.name + "'";
            break;
        }
    }

    // Finale Empfehlung
    if (combined.safe_to_apply) {
        combined.recommendation = "OK: Alle " + std::to_string(actions.size()) +
            " Schritte erfolgreich simuliert.";
    }

    combined.rollback_possible = true;
    combined.rollback_command = "rollback sequence";

    return combined;
}

bool ShadowSimulator::apply_shadow(const ShadowState& shadow, std::string& error) {
    if (!shadow.success) {
        error = "Shadow-Simulation war nicht erfolgreich";
        return false;
    }

    // Erstelle Rollback-Punkt vor der Anwendung
    uint64_t rollback_id = gRollback.create_point("Before applying shadow #" +
        std::to_string(shadow.id));

    EventBus::push("SHADOW_APPLY",
        "id=" + std::to_string(shadow.id) +
        "|rollback_point=" + std::to_string(rollback_id));

    // Hier würde die tatsächliche Anwendung erfolgen
    // (Abhängig von der konkreten Implementierung)

    return true;
}

bool ShadowSimulator::rollback_last(std::string& error) {
    return gRollback.rollback_last(error);
}

std::vector<ShadowState> ShadowSimulator::get_history(size_t n) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ShadowState> result;
    size_t start = history_.size() > n ? history_.size() - n : 0;

    for (size_t i = start; i < history_.size(); ++i) {
        result.push_back(history_[i]);
    }

    std::reverse(result.begin(), result.end());
    return result;
}

void ShadowSimulator::prune_history(size_t keep) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (history_.size() > keep) {
        history_.erase(history_.begin(), history_.end() - keep);
    }
}

double ShadowSimulator::calculate_risk(const ShadowState& shadow, const ShadowAction& action) {
    double risk = action.estimated_risk;

    // Erhöhe Risiko bei Fehlern
    risk += shadow.errors_count * 0.2;

    // Erhöhe Risiko bei Warnungen
    risk += shadow.warnings_count * 0.05;

    // Reduziere Risiko bei hoher Confidence
    risk -= shadow.confidence * 0.1;

    // Erhöhe Risiko für viele betroffene Komponenten
    risk += action.affected_components.size() * 0.05;

    return std::max(0.0, std::min(1.0, risk));
}

std::vector<std::string> ShadowSimulator::generate_preview(const ShadowState& shadow) {
    std::vector<std::string> preview;

    for (const auto& kv : shadow.state_snapshot) {
        preview.push_back(kv.first + " = " + kv.second);
    }

    if (preview.empty()) {
        preview.push_back("(keine Zustandsänderungen)");
    }

    return preview;
}

std::string ShadowSimulator::generate_rollback_command(const ShadowAction& action) {
    return "rollback " + action.name;
}

// ═══════════════════════════════════════════════════════════════════════════
// ROLLBACK MANAGER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

uint64_t RollbackManager::create_point(const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);

    RollbackPoint point;
    point.id = next_id_++;
    point.description = description;
    point.created = std::chrono::steady_clock::now();

    // Aktuellen Zustand speichern
    if (state_provider_) {
        point.state = state_provider_();
    }

    points_.push_back(point);

    // Prune wenn nötig
    if (points_.size() > 100) {
        prune(50);
    }

    EventBus::push("ROLLBACK_POINT",
        "id=" + std::to_string(point.id) + "|desc=" + description);

    return point.id;
}

bool RollbackManager::rollback_to(uint64_t point_id, std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Finde den Punkt
    auto it = std::find_if(points_.begin(), points_.end(),
        [point_id](const RollbackPoint& p) { return p.id == point_id; });

    if (it == points_.end()) {
        error = "Rollback-Punkt nicht gefunden: " + std::to_string(point_id);
        return false;
    }

    // Zustand wiederherstellen
    if (state_restorer_) {
        if (!state_restorer_(it->state)) {
            error = "Zustand konnte nicht wiederhergestellt werden";
            return false;
        }
    }

    // Alle neueren Punkte entfernen
    points_.erase(it + 1, points_.end());

    EventBus::push("ROLLBACK_DONE", "to_point=" + std::to_string(point_id));

    // Improvement emittieren
    ImprovementBus::emit(ImprovementSource::SELF_OPT,
        "Rollback durchgeführt",
        "Zurückgerollt zu Punkt #" + std::to_string(point_id),
        3, 1);

    return true;
}

bool RollbackManager::rollback_last(std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (points_.size() < 2) {
        error = "Kein vorheriger Rollback-Punkt verfügbar";
        return false;
    }

    // Letzten Punkt entfernen und zum vorletzten zurückkehren
    uint64_t target_id = points_[points_.size() - 2].id;

    // Unlock für rekursiven Aufruf
    mutex_.unlock();
    bool result = rollback_to(target_id, error);
    mutex_.lock();

    return result;
}

std::vector<RollbackPoint> RollbackManager::list_points(size_t n) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<RollbackPoint> result;
    size_t start = points_.size() > n ? points_.size() - n : 0;

    for (size_t i = start; i < points_.size(); ++i) {
        result.push_back(points_[i]);
    }

    std::reverse(result.begin(), result.end());
    return result;
}

void RollbackManager::prune(size_t keep) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (points_.size() > keep) {
        points_.erase(points_.begin(), points_.end() - keep);
    }
}

} // namespace rael
