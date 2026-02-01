#pragma once
// ═══════════════════════════════════════════════════════════════════════════
// RAEL SHADOW SIMULATION (Verbesserung #4: Rollback/Shadow-Sim)
// ═══════════════════════════════════════════════════════════════════════════
// Isolierte Simulation für gefährliche Änderungen vor Freigabe
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <mutex>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
// SHADOW STATE - Isolierter Zustand für Simulation
// ═══════════════════════════════════════════════════════════════════════════

struct ShadowState {
    uint64_t id = 0;
    std::string description;
    std::chrono::steady_clock::time_point created;

    // Zustandskopien (Key-Value für Flexibilität)
    std::vector<std::pair<std::string, std::string>> state_snapshot;

    // Ergebnis der Simulation
    bool executed = false;
    bool success = false;
    std::string error_message;
    double confidence = 0.0;

    // Metriken während der Simulation
    int operations_count = 0;
    int warnings_count = 0;
    int errors_count = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
// SHADOW RESULT
// ═══════════════════════════════════════════════════════════════════════════

struct ShadowResult {
    bool safe_to_apply = false;      // Ist es sicher, die Änderung anzuwenden?
    double risk_score = 1.0;         // 0.0 = kein Risiko, 1.0 = hohes Risiko
    std::string recommendation;      // Empfehlung
    std::vector<std::string> warnings;
    std::vector<std::string> changes_preview;

    // Rollback-Information
    std::string rollback_command;
    bool rollback_possible = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// SHADOW ACTION - Zu simulierende Aktion
// ═══════════════════════════════════════════════════════════════════════════

struct ShadowAction {
    std::string name;
    std::string description;

    // Die eigentliche Aktion (als Funktion)
    std::function<bool(ShadowState&)> execute;

    // Validierungs-Funktion (prüft Vorbedingungen)
    std::function<bool(const ShadowState&, std::string&)> validate;

    // Rollback-Funktion
    std::function<bool(ShadowState&)> rollback;

    // Metadaten
    double estimated_risk = 0.5;
    bool requires_confirmation = true;
    std::vector<std::string> affected_components;
};

// ═══════════════════════════════════════════════════════════════════════════
// SHADOW SIMULATOR
// ═══════════════════════════════════════════════════════════════════════════

class ShadowSimulator {
public:
    ShadowSimulator();

    // ─────────────────────────────────────────────────────────────────────────
    // Simulation starten
    // ─────────────────────────────────────────────────────────────────────────

    // Erstelle einen neuen Shadow-State (Kopie des aktuellen Zustands)
    ShadowState create_shadow(const std::string& description);

    // Simuliere eine Aktion im Shadow-State
    ShadowResult simulate(ShadowState& shadow, const ShadowAction& action);

    // Simuliere mehrere Aktionen in Sequenz
    ShadowResult simulate_sequence(ShadowState& shadow,
                                   const std::vector<ShadowAction>& actions);

    // ─────────────────────────────────────────────────────────────────────────
    // Anwendung und Rollback
    // ─────────────────────────────────────────────────────────────────────────

    // Wende Shadow-Änderungen auf echten Zustand an (nach manueller Genehmigung)
    bool apply_shadow(const ShadowState& shadow, std::string& error);

    // Rollback der letzten Änderung
    bool rollback_last(std::string& error);

    // ─────────────────────────────────────────────────────────────────────────
    // History und Audit
    // ─────────────────────────────────────────────────────────────────────────

    // Hole Historie der Simulationen
    std::vector<ShadowState> get_history(size_t n = 10) const;

    // Lösche alte Simulationen
    void prune_history(size_t keep = 100);

    // ─────────────────────────────────────────────────────────────────────────
    // Konfiguration
    // ─────────────────────────────────────────────────────────────────────────

    // Maximale Simulationszeit (Timeout)
    void set_timeout_ms(uint32_t ms) { timeout_ms_ = ms; }

    // Schwellwert für automatische Ablehnung
    void set_risk_threshold(double threshold) { risk_threshold_ = threshold; }

    // Aktiviere/Deaktiviere detailliertes Logging
    void set_verbose(bool v) { verbose_ = v; }

private:
    // Berechne Risiko-Score basierend auf Simulation
    double calculate_risk(const ShadowState& shadow, const ShadowAction& action);

    // Generiere Änderungs-Preview
    std::vector<std::string> generate_preview(const ShadowState& shadow);

    // Generiere Rollback-Command
    std::string generate_rollback_command(const ShadowAction& action);

    mutable std::mutex mutex_;
    std::vector<ShadowState> history_;
    uint64_t next_id_ = 1;

    uint32_t timeout_ms_ = 5000;      // 5 Sekunden Default-Timeout
    double risk_threshold_ = 0.7;     // Über 70% Risiko → Ablehnung empfohlen
    bool verbose_ = false;
};

// ═══════════════════════════════════════════════════════════════════════════
// ROLLBACK MANAGER - Verwaltet Rollback-Punkte
// ═══════════════════════════════════════════════════════════════════════════

struct RollbackPoint {
    uint64_t id = 0;
    std::string description;
    std::chrono::steady_clock::time_point created;
    std::vector<std::pair<std::string, std::string>> state;
    std::string rollback_command;
};

class RollbackManager {
public:
    // Erstelle einen Rollback-Punkt
    uint64_t create_point(const std::string& description);

    // Rollback zu einem bestimmten Punkt
    bool rollback_to(uint64_t point_id, std::string& error);

    // Rollback zum letzten Punkt
    bool rollback_last(std::string& error);

    // Liste aller Rollback-Punkte
    std::vector<RollbackPoint> list_points(size_t n = 10) const;

    // Lösche alte Punkte
    void prune(size_t keep = 50);

    // Setze State-Provider (wie der aktuelle Zustand gelesen wird)
    using StateProvider = std::function<std::vector<std::pair<std::string, std::string>>()>;
    void set_state_provider(StateProvider provider) { state_provider_ = provider; }

    // Setze State-Restorer (wie der Zustand wiederhergestellt wird)
    using StateRestorer = std::function<bool(const std::vector<std::pair<std::string, std::string>>&)>;
    void set_state_restorer(StateRestorer restorer) { state_restorer_ = restorer; }

private:
    mutable std::mutex mutex_;
    std::vector<RollbackPoint> points_;
    uint64_t next_id_ = 1;

    StateProvider state_provider_;
    StateRestorer state_restorer_;
};

// Global instances
extern ShadowSimulator gShadowSim;
extern RollbackManager gRollback;

} // namespace rael
