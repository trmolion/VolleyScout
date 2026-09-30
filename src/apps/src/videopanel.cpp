#include "videopanel.h"
#include "timelinewidget.h"
#include "theme/icons.h"
#include "theme/themetokens.h"

#include "volleyscout/entrymodel.h"
#include "volleyscout/textformat.h"

#include <QAudioOutput>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QPushButton>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QVideoWidget>

namespace {
constexpr qint64 kSkipMs = 5000;
constexpr int kSeekThrottleMs = 80;
}

VideoPanel::VideoPanel(vs::EntryModel *entries, QWidget *parent)
    : QWidget(parent)
    , m_entries(entries)
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
    , m_videoWidget(new QVideoWidget(this))
    , m_fileLabel(new QLabel(tr("Видео не загружено"), this))
    , m_videoInfo(new QLabel(this))
    , m_playButton(new QPushButton(this))
    , m_timeLabel(new QLabel(this))
    , m_speed(new QComboBox(this))
    , m_timeline(new TimelineWidget(this))
    , m_trimBar(new TrimBar(this))
    , m_trimStartButton(new QPushButton(Icons::get(QStringLiteral("trim-start")), tr("Начало здесь"), this))
    , m_trimEndButton(new QPushButton(Icons::get(QStringLiteral("trim-end")), tr("Конец здесь"), this))
    , m_trimResetButton(new QPushButton(Icons::get(QStringLiteral("trim-reset"), Icons::Role::Muted), QString(), this))
    , m_seekThrottle(new QTimer(this))
{
    m_seekThrottle->setSingleShot(true);
    m_seekThrottle->setInterval(kSeekThrottleMs);

    m_player->setAudioOutput(m_audio);
    m_player->setVideoOutput(m_videoWidget);
    m_videoWidget->setMinimumHeight(240);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoWidget->setProperty("surface", "video");

    // --- Заголовок видео (на фоне видеообласти, оверлей-чипы) ---
    auto *openButton = new QPushButton(Icons::get(QStringLiteral("folder-open")), tr("Открыть видео…"), this);
    openButton->setProperty("variant", "overlay");
    m_fileLabel->setProperty("role", "overlayChip");
    m_videoInfo->setProperty("role", "overlayChip");
    m_videoInfo->hide(); // до загрузки видео сведений нет

    auto *fileBar = new QWidget(this);
    fileBar->setProperty("surface", "video");
    auto *fileRow = new QHBoxLayout(fileBar);
    fileRow->setContentsMargins(14, tokens::spacing::l, tokens::spacing::l, tokens::spacing::m);
    fileRow->addWidget(m_fileLabel);
    fileRow->addWidget(m_videoInfo);
    fileRow->addStretch();
    fileRow->addWidget(openButton);

    // --- Транспорт ---
    auto *backButton = new QPushButton(Icons::get(QStringLiteral("seek-back")), QString(), this);
    auto *forwardButton = new QPushButton(Icons::get(QStringLiteral("seek-forward")), QString(), this);
    backButton->setProperty("variant", "ghost");
    forwardButton->setProperty("variant", "ghost");
    backButton->setToolTip(tr("Назад на 5 секунд"));
    forwardButton->setToolTip(tr("Вперёд на 5 секунд"));
    m_playButton->setToolTip(tr("Воспроизведение / пауза (пробел)"));

    // Иконки «плей» и «пауза» одного размера, ширина кнопки фиксирована стилем (wide) —
    // при переключении кнопки транспорта не сдвигаются.
    m_playButton->setIcon(Icons::get(QStringLiteral("play")));
    m_playButton->setProperty("variant", "primary");
    m_playButton->setProperty("wide", true);
    m_timeLabel->setProperty("role", "mono");

    // Пробел — горячая клавиша плеера, поэтому кнопки не забирают фокус (иначе пробел нажмёт их).
    for (QWidget *w : std::initializer_list<QWidget *>{backButton, m_playButton, forwardButton, m_speed})
        w->setFocusPolicy(Qt::NoFocus);

    for (double rate : {0.25, 0.5, 1.0, 1.5, 2.0})
        m_speed->addItem(QStringLiteral("%1×").arg(rate), rate);
    m_speed->setCurrentIndex(m_speed->findData(1.0));

    auto *speedLabel = new QLabel(tr("Скорость"), this);
    speedLabel->setProperty("role", "muted");

    auto *transportBar = new QWidget(this);
    transportBar->setProperty("surface", "panel");
    transportBar->setProperty("borders", "bottom");
    transportBar->setFixedHeight(tokens::metric::transportHeight);
    auto *transport = new QHBoxLayout(transportBar);
    transport->setContentsMargins(tokens::spacing::l, 0, tokens::spacing::l, 0);
    transport->setSpacing(tokens::spacing::s);
    transport->addWidget(backButton);
    transport->addWidget(m_playButton);
    transport->addWidget(forwardButton);
    transport->addSpacing(tokens::spacing::m);
    transport->addWidget(m_timeLabel);
    transport->addStretch();
    transport->addWidget(speedLabel);
    transport->addWidget(m_speed);

    // --- Обрезка ---
    m_trimStartButton->setToolTip(tr("Отрезать всё до текущей позиции"));
    m_trimEndButton->setToolTip(tr("Отрезать всё после текущей позиции"));
    m_trimResetButton->setToolTip(tr("Сбросить обрезку и восстановить исходные таймкоды"));

    for (QPushButton *button : {m_trimStartButton, m_trimEndButton, m_trimResetButton})
        button->setProperty("size", "small");
    m_trimResetButton->setProperty("variant", "ghost");
    m_trimResetButton->setProperty("iconRole", "muted");
    auto *sourceLabel = new QLabel(tr("Исходное видео"), this);
    sourceLabel->setProperty("role", "muted");

    auto *trimRow = new QHBoxLayout;
    trimRow->setSpacing(10);
    trimRow->addWidget(sourceLabel);
    trimRow->addWidget(m_trimBar, 1);
    trimRow->addWidget(m_trimStartButton);
    trimRow->addWidget(m_trimEndButton);
    trimRow->addWidget(m_trimResetButton);

    auto *timelineBlock = new QWidget(this);
    timelineBlock->setProperty("surface", "panel2");
    timelineBlock->setProperty("borders", "bottom");
    auto *timelineLayout = new QVBoxLayout(timelineBlock);
    timelineLayout->setContentsMargins(tokens::spacing::xl, 6, tokens::spacing::xl, tokens::spacing::m);
    timelineLayout->setSpacing(tokens::spacing::xs);
    timelineLayout->addWidget(m_timeline);
    timelineLayout->addLayout(trimRow);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(fileBar);
    layout->addWidget(m_videoWidget, 1);
    layout->addWidget(transportBar);
    layout->addWidget(timelineBlock);

    connect(openButton, &QPushButton::clicked, this, &VideoPanel::openVideo);
    connect(m_playButton, &QPushButton::clicked, this, &VideoPanel::togglePlay);
    connect(backButton, &QPushButton::clicked, this,
            [this] { seekTrimmed(trimmedPosition() - kSkipMs); });
    connect(forwardButton, &QPushButton::clicked, this,
            [this] { seekTrimmed(trimmedPosition() + kSkipMs); });
    connect(m_speed, &QComboBox::currentIndexChanged, this,
            [this] { m_player->setPlaybackRate(m_speed->currentData().toDouble()); });
    connect(m_timeline, &TimelineWidget::seekRequested, this, &VideoPanel::seekTrimmed);

    connect(m_trimStartButton, &QPushButton::clicked, this, &VideoPanel::trimStartHere);
    connect(m_trimEndButton, &QPushButton::clicked, this, &VideoPanel::trimEndHere);
    connect(m_trimResetButton, &QPushButton::clicked, this, &VideoPanel::resetTrim);

    connect(m_player, &QMediaPlayer::positionChanged, this, &VideoPanel::onSourcePositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &VideoPanel::onSourceDurationChanged);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, &VideoPanel::updateVideoInfo);
    connect(m_seekThrottle, &QTimer::timeout, this, &VideoPanel::flushSeek);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        m_playButton->setIcon(Icons::get(state == QMediaPlayer::PlayingState ? QStringLiteral("pause")
                                                                             : QStringLiteral("play")));
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &text) { emit message(tr("Ошибка видео: %1").arg(text)); });

    connect(m_entries, &vs::EntryModel::trimChanged, this, [this] {
        clampToTrim();
        updateTimeline();
        updateMarkers();
    });
    connect(m_entries, &QAbstractItemModel::rowsInserted, this, &VideoPanel::updateMarkers);
    connect(m_entries, &QAbstractItemModel::rowsRemoved, this, &VideoPanel::updateMarkers);
    connect(m_entries, &QAbstractItemModel::modelReset, this, &VideoPanel::updateMarkers);

    updateTimeline();
}

qint64 VideoPanel::sourcePosition() const
{
    return m_player->position();
}

qint64 VideoPanel::trimmedPosition() const
{
    return m_entries->toTrimmed(m_player->position());
}

void VideoPanel::openVideo()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Открыть видео"), QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        tr("Видео (*.mp4 *.mkv *.avi *.mov *.webm *.wmv *.m4v *.mpg *.mpeg *.ts);;Все файлы (*)"));
    if (path.isEmpty())
        return;

    m_awaitingDuration = true;
    m_pendingSeek = -1;
    m_player->setSource(QUrl::fromLocalFile(path));
    m_fileLabel->setText(QFileInfo(path).fileName());
    m_videoInfo->clear();
    m_player->pause(); // показать первый кадр
}

void VideoPanel::seekTrimmed(qint64 ms)
{
    ms = qBound<qint64>(0, ms, m_entries->trimmedDuration());
    m_pendingSeek = m_entries->toSource(ms);
    m_timeline->setPosition(ms); // маркер двигается сразу, не дожидаясь декодера
    m_trimBar->setPosition(m_pendingSeek);

    // Первый запрос выполняется сразу, последующие в пределах окна — только последний.
    if (!m_seekThrottle->isActive())
        flushSeek();
}

void VideoPanel::flushSeek()
{
    if (m_pendingSeek < 0)
        return;
    m_player->setPosition(m_pendingSeek);
    m_pendingSeek = -1;
    m_seekThrottle->start();
}

void VideoPanel::togglePlay()
{
    if (m_player->source().isEmpty())
        return;
    if (m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        if (m_player->position() >= m_entries->trimEnd())
            m_player->setPosition(m_entries->trimStart());
        m_player->play();
    }
}

void VideoPanel::onSourcePositionChanged(qint64 sourceMs)
{
    if (sourceMs > m_entries->trimEnd() || sourceMs < m_entries->trimStart()) {
        if (sourceMs > m_entries->trimEnd())
            m_player->pause();
        clampToTrim();
        return;
    }
    // Пока идёт перетаскивание, не возвращаем маркер к устаревшей позиции декодера.
    if (m_pendingSeek < 0)
        updateTimeline();
    emit positionChanged(m_entries->toTrimmed(sourceMs));
}

void VideoPanel::onSourceDurationChanged(qint64 ms)
{
    // durationChanged может приходить повторно — обрезку сбрасываем только для нового файла.
    if (!m_awaitingDuration || ms <= 0)
        return;
    m_awaitingDuration = false;
    m_entries->setSourceDuration(ms);
}

void VideoPanel::trimStartHere()
{
    const qint64 cut = trimmedPosition();
    if (cut <= 0)
        return;
    m_entries->setTrim(m_entries->trimStart() + cut, m_entries->trimEnd());
    emit message(tr("Начало обрезано на %1. Таймкоды всех записей сдвинуты на −%1.")
                     .arg(vs::formatTime(cut)));
}

void VideoPanel::trimEndHere()
{
    const qint64 position = trimmedPosition();
    if (position <= 0)
        return;
    m_entries->setTrim(m_entries->trimStart(), m_entries->trimStart() + position);
    emit message(tr("Конец обрезан: длительность теперь %1.").arg(vs::formatTime(position)));
}

void VideoPanel::resetTrim()
{
    m_entries->resetTrim();
    emit message(tr("Обрезка сброшена, исходные таймкоды восстановлены."));
}

void VideoPanel::clampToTrim()
{
    const qint64 position = m_player->position();
    const qint64 clamped = qBound(m_entries->trimStart(), position, m_entries->trimEnd());
    if (clamped != position)
        m_player->setPosition(clamped);
    else
        emit positionChanged(m_entries->toTrimmed(position));
}

void VideoPanel::updateTimeline()
{
    const bool hasVideo = m_entries->sourceDuration() > 0;
    const qint64 duration = hasVideo ? m_entries->trimmedDuration() : 0;
    const qint64 position = hasVideo ? trimmedPosition() : 0;

    m_timeLabel->setText(QStringLiteral("%1 / %2").arg(vs::formatTime(position), vs::formatTime(duration)));
    m_timeline->setDuration(duration);
    m_timeline->setPosition(position);
    m_trimBar->setRange(m_entries->sourceDuration(), m_entries->trimStart(), m_entries->trimEnd());
    m_trimBar->setPosition(m_player->position());

    m_trimStartButton->setEnabled(hasVideo);
    m_trimEndButton->setEnabled(hasVideo);
    m_trimResetButton->setEnabled(hasVideo && m_entries->isTrimmed());
}

void VideoPanel::updateVideoInfo()
{
    const QMediaMetaData meta = m_player->metaData();
    QStringList parts;
    const QSize resolution = meta.value(QMediaMetaData::Resolution).toSize();
    if (resolution.isValid())
        parts << QStringLiteral("%1×%2").arg(resolution.width()).arg(resolution.height());
    const double fps = meta.value(QMediaMetaData::VideoFrameRate).toDouble();
    if (fps > 0)
        parts << QStringLiteral("%1 fps").arg(qRound(fps));
    m_videoInfo->setText(parts.join(QStringLiteral(" · ")));
    m_videoInfo->setVisible(!parts.isEmpty());
}

void VideoPanel::updateMarkers()
{
    QList<TimelineWidget::Marker> markers;
    for (const vs::ScoutEntry &entry : m_entries->entriesInRange())
        markers.append({m_entries->toTrimmed(entry.sourceMs), entry.command.grade});
    m_timeline->setMarkers(markers);
}
