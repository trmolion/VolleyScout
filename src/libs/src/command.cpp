#include "volleyscout/command.h"

namespace vs {

QChar actionCode(Action action)
{
    switch (action) {
    case Action::Serve:     return u's';
    case Action::Reception: return u'r';
    case Action::Attack:    return u'a';
    case Action::Block:     return u'b';
    case Action::Dig:       return u'd';
    }
    return {};
}

QString actionName(Action action)
{
    switch (action) {
    case Action::Serve:     return QStringLiteral("подача");
    case Action::Reception: return QStringLiteral("приём");
    case Action::Attack:    return QStringLiteral("атака");
    case Action::Block:     return QStringLiteral("блок");
    case Action::Dig:       return QStringLiteral("защита");
    }
    return {};
}

std::optional<Action> actionFromCode(QChar code)
{
    for (Action action : kAllActions) {
        if (actionCode(action) == code)
            return action;
    }
    return std::nullopt;
}

QChar gradeCode(Grade grade)
{
    switch (grade) {
    case Grade::Perfect:  return u'#';
    case Grade::Positive: return u'+';
    case Grade::Average:  return u'!';
    case Grade::Negative: return u'-';
    case Grade::Error:    return u'=';
    }
    return {};
}

QString gradeName(Grade grade)
{
    switch (grade) {
    case Grade::Perfect:  return QStringLiteral("отлично");
    case Grade::Positive: return QStringLiteral("хорошо");
    case Grade::Average:  return QStringLiteral("средне");
    case Grade::Negative: return QStringLiteral("плохо");
    case Grade::Error:    return QStringLiteral("ошибка");
    }
    return {};
}

std::optional<Grade> gradeFromCode(QChar code)
{
    for (Grade grade : kAllGrades) {
        if (gradeCode(grade) == code)
            return grade;
    }
    return std::nullopt;
}

bool isGradeAllowed(Action action, Grade grade)
{
    if (grade != Grade::Average)
        return true;
    return action == Action::Reception || action == Action::Dig;
}

QString Command::toString() const
{
    return QString::number(player) + actionCode(action) + gradeCode(grade);
}

QString Command::describe() const
{
    return QStringLiteral("№%1 · %2").arg(player).arg(describeAction());
}

QString Command::describeAction() const
{
    return actionName(action) + QStringLiteral(" · ") + gradeName(grade);
}

ParseResult parseCommand(QStringView text)
{
    static const QString formatError = QStringLiteral("Формат: номер + s r a b d + # + ! - =");

    text = text.trimmed();

    qsizetype digits = 0;
    while (digits < text.size() && text[digits].isDigit())
        ++digits;

    if (digits < 1 || digits > 2 || text.size() != digits + 2)
        return {std::nullopt, formatError};

    const auto action = actionFromCode(text[digits]);
    const auto grade = gradeFromCode(text[digits + 1]);
    if (!action || !grade)
        return {std::nullopt, formatError};

    if (!isGradeAllowed(*action, *grade))
        return {std::nullopt, QStringLiteral("Оценка «!» есть только у приёма (r) и защиты (d)")};

    return {Command{text.first(digits).toInt(), *action, *grade}, {}};
}

} // namespace vs
