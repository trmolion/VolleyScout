#pragma once

#include <QString>
#include <QStringView>

#include <array>
#include <optional>

namespace vs {

// Действие игрока. Порядок совпадает с порядком групп в таблице статистики.
enum class Action { Serve, Reception, Attack, Block, Dig };

// Оценка действия — от лучшей к худшей: # + ! - =
enum class Grade { Perfect, Positive, Average, Negative, Error };

inline constexpr std::array<Action, 5> kAllActions{
    Action::Serve, Action::Reception, Action::Attack, Action::Block, Action::Dig};
inline constexpr std::array<Grade, 5> kAllGrades{
    Grade::Perfect, Grade::Positive, Grade::Average, Grade::Negative, Grade::Error};

inline constexpr int kMaxSets = 5;

QChar actionCode(Action action);
QString actionName(Action action);
std::optional<Action> actionFromCode(QChar code);

QChar gradeCode(Grade grade);
QString gradeName(Grade grade);
std::optional<Grade> gradeFromCode(QChar code);

// Оценка «!» (средне) существует только у приёма и защиты.
bool isGradeAllowed(Action action, Grade grade);

// Краткая запись действия игрока, например «13s+».
struct Command
{
    int player = 0;
    Action action = Action::Serve;
    Grade grade = Grade::Perfect;

    QString toString() const;
    // «№13 · подача · хорошо»
    QString describe() const;
    // «подача · хорошо»
    QString describeAction() const;

    friend bool operator==(const Command &a, const Command &b)
    {
        return a.player == b.player && a.action == b.action && a.grade == b.grade;
    }
};

struct ParseResult
{
    std::optional<Command> command;
    QString error; // пусто, если команда разобрана

    bool ok() const { return command.has_value(); }
};

// Формат: номер (1–2 цифры) + действие (s r a b d) + оценка (# + ! - =).
ParseResult parseCommand(QStringView text);

} // namespace vs
