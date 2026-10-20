// history.h — historique d'annulation par « instantanés » du projet entier.
#pragma once

#include <QByteArray>
#include <QVector>

class History {
public:
    void reset(const QByteArray& state) {
        states_.clear();
        states_.push_back(state);
        idx_ = 0;
    }
    // Enregistre un nouvel état (sans effet s'il est identique à l'état courant)
    bool push(const QByteArray& state) {
        if (states_.isEmpty()) { reset(state); return true; }
        if (state == states_[idx_]) return false;
        states_.resize(idx_ + 1);              // on oublie l'ancien « futur »
        states_.push_back(state);
        if (states_.size() > kMax) states_.removeFirst();
        idx_ = states_.size() - 1;
        return true;
    }
    bool canUndo() const { return idx_ > 0; }
    bool canRedo() const { return idx_ + 1 < states_.size(); }
    QByteArray undo() { return states_[--idx_]; }
    QByteArray redo() { return states_[++idx_]; }

private:
    static constexpr int kMax = 200;
    QVector<QByteArray> states_;
    int idx_ = -1;
};
