// highlighter.h — coloration syntaxique minimale du C# dans le panneau de code.
#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

class CSharpHighlighter : public QSyntaxHighlighter {
public:
    explicit CSharpHighlighter(QTextDocument* doc) : QSyntaxHighlighter(doc) {
        auto add = [this](const QString& pattern, const char* color, bool bold = false) {
            QTextCharFormat f;
            f.setForeground(QColor(color));
            if (bold) f.setFontWeight(QFont::Bold);
            rules_.append({QRegularExpression(pattern), f});
        };
        add(R"(\b(using|class|static|void|string|double|int|bool|new|if|else|for|while|return|true|false|null|var)\b)",
            "#c586c0", true);
        add(R"(\b(Console|Convert|Math|Random|Thread|Program)\b)", "#4ec9b0");
        add(R"(\b\d+(\.\d+)?([eE][+-]?\d+)?\b)", "#b5cea8");
        add(R"("([^"\\]|\\.)*")", "#ce9178");
        add(R"(//[^\n]*)", "#6a9955");
    }

protected:
    void highlightBlock(const QString& text) override {
        for (const Rule& r : std::as_const(rules_)) {
            auto it = r.re.globalMatch(text);
            while (it.hasNext()) {
                const auto m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), r.fmt);
            }
        }
    }

private:
    struct Rule { QRegularExpression re; QTextCharFormat fmt; };
    QVector<Rule> rules_;
};
