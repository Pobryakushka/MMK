// ─────────────────────────────────────────────────────────────────────────
// Всплывающий слой поверх окна: тост поиска, окно подтверждения и три
// шторки — датчика, «Подключить всё» и готовности.
//
// Часть реализации класса MainWindow (см. mainwindow.h). Общие для всех
// частей include и константы — в mainwindow_internal.h.
// ─────────────────────────────────────────────────────────────────────────

#include "ui/mainwindow/mainwindow_internal.h"

// Разовое уведомление об ошибке/успехе поверх главного окна — тот же
// плавающий тост, что и на страницах ТО (AngleCheckPage/InspectionPage),
// пришедший на замену модальным QMessageBox.
void MainWindow::showNotice(const QString &text, NotificationToast::Kind kind)
{
    m_notifyToast->showMessage(text, kind);
}

// =====================================================
// Методы работы с уведомлением о подключении датчиков
// =====================================================

void MainWindow::setupToastUI()
{
    m_toastWidget = new QWidget(this);
    m_toastWidget->setFixedSize(340, 105);
    m_toastWidget->setStyleSheet(
        "QWidget#toastWidget {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 12px;"
        "}"
        );
    m_toastWidget->setObjectName("toastWidget");

    // Тень для окна
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    m_toastWidget->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_toastWidget);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_toastTitle = new QLabel("Автопоиск датчиков", m_toastWidget);
    m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #1C1F22; border: none; background: transparent;");

    m_toastPercent = new QLabel("0%", m_toastWidget);
    m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #0F6B4F; border: none; background: transparent;");
    m_toastPercent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(m_toastTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_toastPercent);

    m_toastText = new QLabel("Инициализация...", m_toastWidget);
    m_toastText->setStyleSheet("font-size: 9pt; color: #666; font-family: monospace; border: none; background: transparent;");
    m_toastText->setWordWrap(true);
    m_toastText->setFixedHeight(32);

    m_toastProgress = new QProgressBar(m_toastWidget);
    m_toastProgress->setFixedHeight(6);
    m_toastProgress->setTextVisible(false);
    m_toastProgress->setStyleSheet(
        "QProgressBar {"
        "   background-color: #EFF1F1;"
        "   border: none;"
        "   border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #0F6B4F;"
        "   border-radius: 3px;"
        "}"
        );

    layout->addLayout(headerLayout);
    layout->addWidget(m_toastText);
    layout->addWidget(m_toastProgress);

    // Маленькая красная кнопка остановки поиска — в правом верхнем углу окна
    m_toastCloseBtn = new QPushButton("\u2715", m_toastWidget);
    m_toastCloseBtn->setFocusPolicy(Qt::NoFocus);
    m_toastCloseBtn->setFixedSize(12, 12);
    m_toastCloseBtn->setCursor(Qt::PointingHandCursor);
    m_toastCloseBtn->setToolTip("Остановить поиск датчиков");
    m_toastCloseBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #E53935;"
        "   color: #FFFFFF;"
        "   border: none;"
        "   border-radius: 6px;"
        "   font-size: 10pt;"
        "   font-weight: bold;"
        "   padding: 0px;"
        "}"
        "QPushButton:hover { background-color: #C62828; }"
        "QPushButton:pressed { background-color: #B71C1C; }"
        );
    // Позиционируем поверх карточки, в углу — вне общего layout
    m_toastCloseBtn->move(m_toastWidget->width() - m_toastCloseBtn->width() - 8, 8);
    m_toastCloseBtn->raise();
    m_toastCloseBtn->hide(); // видна только пока идёт активный поиск
    connect(m_toastCloseBtn, &QPushButton::clicked, this, &MainWindow::onToastCloseClicked);

    setupConfirmOverlay();

    m_toastAnimation = new QPropertyAnimation(m_toastWidget, "pos", this);
    m_toastAnimation->setDuration(400);
    m_toastAnimation->setEasingCurve(QEasingCurve::OutBack);

    m_toastHideTimer = new QTimer(this);
    m_toastHideTimer->setSingleShot(true);
    connect(m_toastHideTimer, &QTimer::timeout, this, &MainWindow::hideToast);

    // Прячем окно за пределы экрана при создании
    m_toastWidget->move(width() - m_toastWidget->width() - 20, height() + 10);
    m_toastWidget->show();
}

void MainWindow::showToast()
{
    // Если toast прятался по таймеру после предыдущей отмены/завершения
    // поиска (см. onToastCloseClicked/onAutoConnectorFinished), а поиск
    // запустили заново раньше, чем этот таймер сработал — он всё равно
    // "выстрелит" через оставшееся время и спрячет уже НОВЫЙ toast. Поэтому
    // при каждом показе гарантированно останавливаем отложенное скрытие.
    m_toastHideTimer->stop();

    m_toastWidget->raise();

    int targetX = width() - m_toastWidget->width() - 20;
    int startY = height() + 10;
    int endY = height() - m_toastWidget->height() - 20;

    m_toastAnimation->stop();
    m_toastWidget->move(targetX, startY);
    m_toastAnimation->setStartValue(QPoint(targetX, startY));
    m_toastAnimation->setEndValue(QPoint(targetX, endY));
    m_toastAnimation->start();
}

void MainWindow::hideToast()
{
    int targetX = width() - m_toastWidget->width() - 20;
    int startY = m_toastWidget->y();
    int endY = height() + 10;

    m_toastAnimation->stop();
    m_toastAnimation->setStartValue(QPoint(targetX, startY));
    m_toastAnimation->setEndValue(QPoint(targetX, endY));
    m_toastAnimation->start();
}

void MainWindow::repositionToast()
{
    if (m_toastAnimation->state() == QAbstractAnimation::Running) {
        return;
    }

    int targetX = width() - m_toastWidget->width() - 20;
    int targetYVisible = height() - m_toastWidget->height() - 20;
    int targetYHidden = height() + 10;

    // Если Toast сейчас на экране, корректируем его видимую позицию
    if (m_toastWidget->y() < height()) {
        m_toastWidget->move(targetX, targetYVisible);
    } else {
        m_toastWidget->move(targetX, targetYHidden);
    }
}

// =====================================================
// Окно подтверждения остановки поиска датчиков
// =====================================================

void MainWindow::setupConfirmOverlay()
{
    // Полупрозрачный оверлей на всё окно — затемняет фон, перехватывает клики "мимо"
    m_stopConfirmOverlay = new QWidget(this);
    m_stopConfirmOverlay->setObjectName("stopConfirmOverlay");
    m_stopConfirmOverlay->setStyleSheet(
        "QWidget#stopConfirmOverlay { background-color: rgba(28, 31, 34, 140); }"
        );
    m_stopConfirmOverlay->setAttribute(Qt::WA_StyledBackground, true);
    m_stopConfirmOverlay->setGeometry(rect());
    m_stopConfirmOverlay->hide();

    // Плавное появление/исчезновение затемнения. ВАЖНО: этот эффект висит
    // ТОЛЬКО на затемняющем фоне, а не на карточке с кнопками (см. ниже) —
    // иначе Qt рендерит весь поддерево через закэшированный офскрин-буфер,
    // который не обновляется на :hover у кнопок, и они визуально "пропадают"
    // при наведении курсора. Это была причина бага.
    m_stopConfirmOpacity = new QGraphicsOpacityEffect(m_stopConfirmOverlay);
    m_stopConfirmOverlay->setGraphicsEffect(m_stopConfirmOpacity);
    m_stopConfirmOpacity->setOpacity(0.0);

    m_stopConfirmAnimation = new QPropertyAnimation(m_stopConfirmOpacity, "opacity", this);
    m_stopConfirmAnimation->setDuration(200);
    connect(m_stopConfirmAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_stopConfirmOpacity->opacity() < 0.01)
            m_stopConfirmOverlay->hide();
    });

    // Карточка подтверждения — ОБЩАЯ для "остановить поиск" и "отключить
    // датчик": текст/кнопки/колбэк выставляются в showConfirmOverlay().
    // Дочерний виджет MainWindow (НЕ m_stopConfirmOverlay!) — намеренно, по
    // причине, описанной выше. Позиционируется/поднимается поверх оверлея
    // вручную в showConfirmOverlay()/resizeEvent().
    m_stopConfirmCard = new QWidget(this);
    m_stopConfirmCard->setObjectName("stopConfirmCard");
    m_stopConfirmCard->setFixedSize(360, 210);
    m_stopConfirmCard->setStyleSheet(
        "QWidget#stopConfirmCard {"
        "   background-color: #FFFFFF;"
        "   border-radius: 20px;"
        "}"
        );
    m_stopConfirmCard->hide();

    QGraphicsDropShadowEffect *cardShadow = new QGraphicsDropShadowEffect(m_stopConfirmCard);
    cardShadow->setBlurRadius(30);
    cardShadow->setColor(QColor(0, 0, 0, 90));
    cardShadow->setOffset(0, 8);
    m_stopConfirmCard->setGraphicsEffect(cardShadow);

    QVBoxLayout *cardLayout = new QVBoxLayout(m_stopConfirmCard);
    cardLayout->setContentsMargins(24, 22, 24, 20);
    cardLayout->setSpacing(10);

    QLabel *icon = new QLabel("\u26A0", m_stopConfirmCard);
    icon->setFixedSize(48, 48);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(
        "font-size: 20pt; color: #B71C1C; background-color: #FFEBEE; border-radius: 24px;"
        );
    QHBoxLayout *iconRow = new QHBoxLayout();
    iconRow->addStretch();
    iconRow->addWidget(icon);
    iconRow->addStretch();

    m_confirmTitleLabel = new QLabel(m_stopConfirmCard);
    m_confirmTitleLabel->setAlignment(Qt::AlignCenter);
    m_confirmTitleLabel->setWordWrap(true);
    m_confirmTitleLabel->setStyleSheet(
        "font-weight: bold; font-size: 12pt; color: #1C1F22; border: none; background: transparent;"
        );

    m_confirmSubtitleLabel = new QLabel(m_stopConfirmCard);
    m_confirmSubtitleLabel->setAlignment(Qt::AlignCenter);
    m_confirmSubtitleLabel->setWordWrap(true);
    m_confirmSubtitleLabel->setStyleSheet(
        "font-size: 9pt; color: #666666; border: none; background: transparent;"
        );

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);

    m_confirmYesBtn = new QPushButton(m_stopConfirmCard);
    m_confirmYesBtn->setFocusPolicy(Qt::NoFocus);
    m_confirmYesBtn->setCursor(Qt::PointingHandCursor);
    m_confirmYesBtn->setFixedHeight(44);
    m_confirmYesBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #FFFFFF;"
        "   color: #B71C1C;"
        "   border: 1px solid #FFCDD2;"
        "   border-radius: 14px;"
        "   font-size: 10pt;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #FFEBEE; }"
        "QPushButton:pressed { background-color: #FFCDD2; }"
        );

    m_confirmNoBtn = new QPushButton(m_stopConfirmCard);
    m_confirmNoBtn->setFocusPolicy(Qt::NoFocus);
    m_confirmNoBtn->setCursor(Qt::PointingHandCursor);
    m_confirmNoBtn->setFixedHeight(44);
    m_confirmNoBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #0F6B4F;"
        "   color: #FFFFFF;"
        "   border: none;"
        "   border-radius: 14px;"
        "   font-size: 10pt;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: #0B5A42; }"
        "QPushButton:pressed { background-color: #094B37; }"
        );

    btnRow->addWidget(m_confirmYesBtn);
    btnRow->addWidget(m_confirmNoBtn);

    cardLayout->addLayout(iconRow);
    cardLayout->addWidget(m_confirmTitleLabel);
    cardLayout->addWidget(m_confirmSubtitleLabel);
    cardLayout->addStretch();
    cardLayout->addLayout(btnRow);

    connect(m_confirmYesBtn, &QPushButton::clicked, this, &MainWindow::onConfirmOverlayAccepted);
    connect(m_confirmNoBtn, &QPushButton::clicked, this, &MainWindow::onConfirmOverlayCancelled);
}

void MainWindow::showConfirmOverlay(const QString &title, const QString &subtitle,
                                    std::function<void()> onConfirm,
                                    const QString &yesLabel, const QString &noLabel)
{
    if (!m_stopConfirmOverlay || !m_stopConfirmCard) return;

    m_confirmTitleLabel->setText(title);
    m_confirmSubtitleLabel->setText(subtitle);
    m_confirmSubtitleLabel->setVisible(!subtitle.isEmpty());
    m_confirmYesBtn->setText(yesLabel);
    m_confirmNoBtn->setText(noLabel);
    m_confirmCallback = std::move(onConfirm);

    m_stopConfirmOverlay->setGeometry(rect());
    m_stopConfirmOverlay->show();
    m_stopConfirmOverlay->raise();

    // Карточка больше не в layout'е оверлея (она вообще не его потомок) —
    // центрируем вручную и поднимаем НАД оверлеем.
    m_stopConfirmCard->move((width() - m_stopConfirmCard->width()) / 2,
                            (height() - m_stopConfirmCard->height()) / 2);
    m_stopConfirmCard->show();
    m_stopConfirmCard->raise();

    m_stopConfirmAnimation->stop();
    m_stopConfirmAnimation->setStartValue(m_stopConfirmOpacity->opacity());
    m_stopConfirmAnimation->setEndValue(1.0);
    m_stopConfirmAnimation->start();
}

void MainWindow::hideConfirmOverlay()
{
    if (!m_stopConfirmOverlay) return;

    if (m_stopConfirmCard)
        m_stopConfirmCard->hide(); // сразу — карточка не участвует в фейде фона

    m_stopConfirmAnimation->stop();
    m_stopConfirmAnimation->setStartValue(m_stopConfirmOpacity->opacity());
    m_stopConfirmAnimation->setEndValue(0.0);
    m_stopConfirmAnimation->start();
}

void MainWindow::onConfirmOverlayAccepted()
{
    hideConfirmOverlay();
    if (m_confirmCallback) {
        auto callback = std::move(m_confirmCallback);
        m_confirmCallback = nullptr;
        callback();
    }
}

void MainWindow::onConfirmOverlayCancelled()
{
    hideConfirmOverlay();
    m_confirmCallback = nullptr;
}

void MainWindow::onToastCloseClicked()
{
    // Крестик активен только пока идёт реальный поиск
    if (!m_autoConnector->isDetecting()) return;

    showConfirmOverlay(
        "Остановить поиск датчиков?",
        "Уже найденные датчики останутся подключены,\nостальные придётся искать заново.",
        [this]() {
            m_autoConnector->stopDetection();

            ui->btnConnectSensors->setEnabled(true);
            statusBar()->clearMessage();
            m_toastCloseBtn->hide();

            // Поиск прерван пользователем — пересчитываем видимость
            // "Подключить всё": появится, только если к этому моменту не
            // подключился ни один датчик.
            updateConnectAllButtonVisibility();

            m_toastTitle->setText("Поиск остановлен");
            m_toastTitle->setStyleSheet("font-weight: bold; font-size: 10pt; color: #B71C1C; border: none; background: transparent;");
            m_toastPercent->setStyleSheet("font-size: 10pt; font-weight: bold; color: #B71C1C; border: none; background: transparent;");
            m_toastProgress->setStyleSheet(
                "QProgressBar { background-color: #FFEBEE; border: none; border-radius: 3px; }"
                "QProgressBar::chunk { background-color: #C62828; border-radius: 3px; }"
                );
            m_toastText->setText("Поиск остановлен пользователем");

            m_toastHideTimer->start(2000);
        },
        "Да, остановить", "Нет, продолжить");
}

// ─────────────────────────────────────────────────────────────────────────
// Всплывающая карточка у индикатора состояния приземных данных
// ─────────────────────────────────────────────────────────────────────────

void MainWindow::setupReadinessPopup()
{
    m_readinessPopup = new QWidget(this);
    m_readinessPopup->setObjectName("readinessPopup");
    m_readinessPopup->setFixedWidth(300);
    m_readinessPopup->setStyleSheet(
        "QWidget#readinessPopup {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 14px;"
        "}"
        );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    m_readinessPopup->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_readinessPopup);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(8);

    m_readinessPopupTitle = new QLabel(m_readinessPopup);
    m_readinessPopupTitle->setWordWrap(true);
    m_readinessPopupTitle->setStyleSheet(
        "font-weight: bold; font-size: 10pt; color: #1C1F22; background: transparent; border: none;");

    m_readinessPopupSubtitle = new QLabel(m_readinessPopup);
    m_readinessPopupSubtitle->setWordWrap(true);
    m_readinessPopupSubtitle->setStyleSheet(
        "font-size: 9pt; color: #6B7278; background: transparent; border: none;");

    QHBoxLayout *btnRow = new QHBoxLayout();
    m_readinessPopupNo = new QPushButton("Нет", m_readinessPopup);
    m_readinessPopupNo->setFocusPolicy(Qt::NoFocus);
    m_readinessPopupNo->setStyleSheet(
        "QPushButton { background:#FFFFFF; color:#1C1F22; border:1px solid #DDE1E3;"
        " border-radius:8px; padding:6px 18px; font-weight:600; }"
        "QPushButton:pressed { background:#F0F1F2; }");
    m_readinessPopupYes = new QPushButton("Да", m_readinessPopup);
    m_readinessPopupYes->setFocusPolicy(Qt::NoFocus);
    m_readinessPopupYes->setStyleSheet(
        "QPushButton { background:#0F6B4F; color:#FFFFFF; border:none;"
        " border-radius:8px; padding:6px 18px; font-weight:700; }"
        "QPushButton:pressed { background:#0B5A41; }");
    btnRow->addStretch();
    btnRow->addWidget(m_readinessPopupNo);
    btnRow->addWidget(m_readinessPopupYes);

    layout->addWidget(m_readinessPopupTitle);
    layout->addWidget(m_readinessPopupSubtitle);
    layout->addLayout(btnRow);

    m_readinessPopup->hide();

    m_readinessPopupAnimation = new QPropertyAnimation(m_readinessPopup, "pos", this);
    m_readinessPopupAnimation->setDuration(300);
    m_readinessPopupAnimation->setEasingCurve(QEasingCurve::OutBack);

    connect(m_readinessPopupYes, &QPushButton::clicked, this, [this]() {
        hideReadinessPopup();
        if (GroundMeteoParams *gmp = GroundMeteoParams::instance())
            ui->stackedWidget->setCurrentWidget(gmp);
    });
    connect(m_readinessPopupNo, &QPushButton::clicked, this, [this]() {
        hideReadinessPopup();
    });

    // Закрытие по клику вне уведомления и вне самого индикатора — фильтр
    // ставится один раз на приложение, работает только пока попап виден.
    qApp->installEventFilter(this);
}

// Попап общего индикатора — ИСКЛЮЧИТЕЛЬНО информационный (см. header):
// заголовок + список причин, без единой кнопки перехода к вводу данных.
// Переход к ручному вводу конкретных данных делается из шторки конкретного
// датчика — см. m_sensorPopupManualBtn/onSensorPopupManualClicked.
void MainWindow::populateReadinessPopupContent()
{
    const bool measurementRunning =
        (m_amsHandler && m_amsHandler->getMeasurementStatus() == STATUS_RUNNING);

    m_readinessPopupYes->setVisible(false);
    m_readinessPopupNo->setVisible(false);

    if (measurementRunning) {
        m_readinessPopupTitle->setText("Идёт измерение АМС");
        m_readinessPopupSubtitle->clear();
    } else {
        const QStringList issues = collectReadinessIssues();
        if (!issues.isEmpty()) {
            m_readinessPopupTitle->setText("Отказ");
            m_readinessPopupSubtitle->setText(issues.join("\n"));
        } else if (m_lastKnownSurfaceState == GroundMeteoParams::Stale) {
            m_readinessPopupTitle->setText("Данные устарели");
            m_readinessPopupSubtitle->setText(
                "Приземные данные старше 30 минут. Рекомендуется обновить.");
        } else {
            m_readinessPopupTitle->setText("Система готова");
            m_readinessPopupSubtitle->clear();
        }
    }

    m_readinessPopup->adjustSize();
    m_readinessPopup->setFixedWidth(300); // adjustSize мог сжать по ширине — держим фиксированную
}

void MainWindow::onReadinessIndicatorClicked()
{
    if (m_readinessPopup->isVisible()) {
        hideReadinessPopup();
        return;
    }

    populateReadinessPopupContent();
    showReadinessPopup();
}

void MainWindow::showReadinessPopup()
{
    const QPoint frameBottomLeft = ui->readinessIndicatorFrame->mapTo(
        this, QPoint(0, ui->readinessIndicatorFrame->height()));
    const int targetX = frameBottomLeft.x();
    const int targetY = frameBottomLeft.y() + 8;
    const int startY  = frameBottomLeft.y() - 20; // "выезжает" из-под индикатора

    m_readinessPopup->raise();
    m_readinessPopupAnimation->stop();
    m_readinessPopup->move(targetX, startY);
    m_readinessPopup->show();
    m_readinessPopupAnimation->setStartValue(QPoint(targetX, startY));
    m_readinessPopupAnimation->setEndValue(QPoint(targetX, targetY));
    m_readinessPopupAnimation->start();
}

void MainWindow::hideReadinessPopup()
{
    if (!m_readinessPopup->isVisible())
        return;
    m_readinessPopupAnimation->stop();
    m_readinessPopup->hide();
}

void MainWindow::setupSensorPopup()
{
    m_sensorPopup = new QWidget(this);
    m_sensorPopup->setObjectName("sensorPopup");
    m_sensorPopup->setFixedWidth(280);
    m_sensorPopup->setStyleSheet(
        "QWidget#sensorPopup {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 14px;"
        "}"
        );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    m_sensorPopup->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_sensorPopup);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    m_sensorPopupTitle = new QLabel(m_sensorPopup);
    m_sensorPopupTitle->setStyleSheet(
        "font-weight: bold; font-size: 11pt; color: #1C1F22; background: transparent; border: none;");

    m_sensorPopupStatus = new QLabel(m_sensorPopup);
    m_sensorPopupStatus->setStyleSheet(
        "font-size: 9pt; font-weight: 600; background: transparent; border: none;");

    m_sensorPopupInfo = new QLabel(m_sensorPopup);
    m_sensorPopupInfo->setWordWrap(true);
    m_sensorPopupInfo->setStyleSheet(
        "font-size: 8.5pt; color: #6B7278; background: transparent; border: none;");

    m_sensorPopupReason = new QLabel(m_sensorPopup);
    m_sensorPopupReason->setWordWrap(true);
    m_sensorPopupReason->setStyleSheet(
        "font-size: 8.5pt; color: #B71C1C; background: transparent; border: none;");

    m_sensorPopupActionBtn = new QPushButton(m_sensorPopup);
    m_sensorPopupActionBtn->setFocusPolicy(Qt::NoFocus);
    m_sensorPopupActionBtn->setCursor(Qt::PointingHandCursor);
    m_sensorPopupActionBtn->setFixedHeight(36);

    // Вторая кнопка — переход к ручному вводу данных этого датчика (ИВС →
    // приземные данные, ГНСС/БИНС → положение/ориентация). Показывается
    // только когда датчик не подключён И соответствующих данных ещё нет —
    // см. populateSensorPopupContent()/sensorHasRequiredData().
    m_sensorPopupManualBtn = new QPushButton(m_sensorPopup);
    m_sensorPopupManualBtn->setFocusPolicy(Qt::NoFocus);
    m_sensorPopupManualBtn->setCursor(Qt::PointingHandCursor);
    m_sensorPopupManualBtn->setFixedHeight(36);
    m_sensorPopupManualBtn->hide();

    layout->addWidget(m_sensorPopupTitle);
    layout->addWidget(m_sensorPopupStatus);
    layout->addWidget(m_sensorPopupInfo);
    layout->addWidget(m_sensorPopupReason);
    layout->addSpacing(4);
    layout->addWidget(m_sensorPopupActionBtn);
    layout->addWidget(m_sensorPopupManualBtn);

    m_sensorPopup->hide();

    m_sensorPopupAnimation = new QPropertyAnimation(m_sensorPopup, "pos", this);
    m_sensorPopupAnimation->setDuration(300);
    m_sensorPopupAnimation->setEasingCurve(QEasingCurve::OutBack);

    connect(m_sensorPopupActionBtn, &QPushButton::clicked, this, &MainWindow::onSensorPopupActionClicked);
    connect(m_sensorPopupManualBtn, &QPushButton::clicked, this, &MainWindow::onSensorPopupManualClicked);
}

void MainWindow::populateSensorPopupContent()
{
    const AutoConnector::DeviceType type = m_currentPopupSensor;
    m_sensorPopupTitle->setText(sensorDisplayName(type));

    const bool connected = isSensorConnected(type);
    const bool searchRunning = m_autoConnector->isDetecting();
    const bool searchingThis = searchRunning && m_autoConnector->singleSearchTarget() == type;

    if (connected) {
        m_sensorPopupStatus->setText("Подключено");
        m_sensorPopupStatus->setStyleSheet(
            "font-size: 9pt; font-weight: 600; color: #0F6B4F; background: transparent; border: none;");

        QString info = sensorExtraInfo(type);
        const QString lastSeen = sensorLastSeenText(type);
        if (!lastSeen.isEmpty())
            info = info.isEmpty() ? lastSeen : (info + "\n" + lastSeen);
        m_sensorPopupInfo->setText(info);
        m_sensorPopupInfo->setVisible(!info.isEmpty());

        // Отключать вручную из шторки не нужно (см. задачу) — отключение
        // отслеживается автоматически через health-check.
        m_sensorPopupReason->setVisible(false);
        m_sensorPopupActionBtn->setVisible(false);
        m_sensorPopupManualBtn->setVisible(false);
    } else {
        m_sensorPopupStatus->setText("Не подключено");
        m_sensorPopupStatus->setStyleSheet(
            "font-size: 9pt; font-weight: 600; color: #C62828; background: transparent; border: none;");
        m_sensorPopupInfo->setVisible(false);

        m_sensorPopupReason->setText(sensorProblemReason(type));
        m_sensorPopupReason->setVisible(true);

        m_sensorPopupActionBtn->setVisible(true);
        const bool queued = m_sensorSearchQueue.contains(type);
        if (searchingThis) {
            m_sensorPopupActionBtn->setText("Идёт поиск...");
            m_sensorPopupActionBtn->setEnabled(false);
        } else if (queued) {
            m_sensorPopupActionBtn->setText("В очереди поиска...");
            m_sensorPopupActionBtn->setEnabled(false);
        } else if (searchRunning) {
            // Другой датчик уже ищется — не блокируем, а ставим в очередь
            // по клику (см. startSingleSensorSearch).
            m_sensorPopupActionBtn->setText("Подключить (в очередь)");
            m_sensorPopupActionBtn->setEnabled(true);
        } else {
            m_sensorPopupActionBtn->setText("Подключить");
            m_sensorPopupActionBtn->setEnabled(true);
        }
        m_sensorPopupActionBtn->setStyleSheet(
            "QPushButton { background:#0F6B4F; color:#FFFFFF; border:none;"
            " border-radius:8px; font-weight:700; }"
            "QPushButton:pressed { background:#0B5A41; }"
            "QPushButton:disabled { background:#B9BFC2; color:#FFFFFF; }");

        // Кнопка перехода к ручному вводу — только для ИВС/ГНСС/БИНС и
        // только пока соответствующих данных ДЕЙСТВИТЕЛЬНО нет (ни с
        // датчика, ни введены вручную ранее). Для АМС не показывается —
        // для него нет ручной замены.
        const QString manualText = sensorManualEntryButtonText(type);
        const bool showManual = !manualText.isEmpty() && !sensorHasRequiredData(type);
        m_sensorPopupManualBtn->setVisible(showManual);
        if (showManual) {
            m_sensorPopupManualBtn->setText(manualText);
            m_sensorPopupManualBtn->setStyleSheet(
                "QPushButton { background:#FFF8E1; color:#8a6100; border:1px solid #FFE082;"
                " border-radius:8px; font-weight:700; font-size:8pt; padding:4px 6px; }"
                "QPushButton:pressed { background:#FFECB3; }");
        }
    }

    m_sensorPopup->adjustSize();
    m_sensorPopup->setFixedWidth(280);
}

// Переход к ручному вводу данных, которые обычно даёт этот датчик.
// ИВС → страница GroundMeteoParams (свой класс). ГНСС/БИНС → поля
// положения/ориентации живут прямо на странице "Исходные данные"
// (mainwindow.ui) — переключаемся туда и, если ручной режим ещё не
// включён, включаем его (onManualInputClicked), чтобы поля сразу стали
// редактируемыми.
void MainWindow::onSensorPopupManualClicked()
{
    const AutoConnector::DeviceType type = m_currentPopupSensor;
    hideSensorPopup();

    switch (type) {
    case AutoConnector::DEVICE_IWS:
        if (GroundMeteoParams *gmp = GroundMeteoParams::instance())
            ui->stackedWidget->setCurrentWidget(gmp);
        break;
    case AutoConnector::DEVICE_GNSS:
    case AutoConnector::DEVICE_BINS:
        // Поля положения/ориентации живут на ui->page_position (см. .ui),
        // а НЕ в sourceDataInstance (тот — для приземных данных/Метео-11).
        ui->stackedWidget->setCurrentWidget(ui->page_position);
        if (!m_manualInputEnabled)
            onManualInputClicked();
        break;
    default:
        break;
    }
}

void MainWindow::showSensorPopup(AutoConnector::DeviceType type)
{
    // Повторный клик по той же плашке — закрыть (как readiness popup)
    if (m_sensorPopup->isVisible() && m_currentPopupSensor == type) {
        hideSensorPopup();
        return;
    }

    QWidget *indicator = sensorIndicatorWidget(type);
    if (!indicator) return;

    m_currentPopupSensor = type;
    populateSensorPopupContent();

    const QPoint bottomLeft = indicator->mapTo(this, QPoint(0, indicator->height()));
    const int targetX = bottomLeft.x();
    const int targetY = bottomLeft.y() + 8;
    const int startY  = bottomLeft.y() - 20;

    m_sensorPopup->raise();
    m_sensorPopupAnimation->stop();
    m_sensorPopup->move(targetX, startY);
    m_sensorPopup->show();
    m_sensorPopupAnimation->setStartValue(QPoint(targetX, startY));
    m_sensorPopupAnimation->setEndValue(QPoint(targetX, targetY));
    m_sensorPopupAnimation->start();
}

void MainWindow::hideSensorPopup()
{
    if (!m_sensorPopup || !m_sensorPopup->isVisible())
        return;
    m_sensorPopupAnimation->stop();
    m_sensorPopup->hide();
}

void MainWindow::onSensorPopupActionClicked()
{
    startSingleSensorSearch(m_currentPopupSensor);
}

// ─────────────────────────────────────────────────────────────────────────
// Шторка кнопки "Подключить всё" — по стилю и механике повторяет шторку
// датчика (m_sensorPopup), только без per-датчикового состояния: она и так
// видна лишь когда ни один датчик не подключён.
// Кнопка сейчас полностью скрыта (см. kConnectAllButtonEnabled в
// updateConnectAllButtonVisibility()), поэтому иконки у неё нет — код ниже
// оставлен нетронутым на случай, если понадобится вернуть.
// ─────────────────────────────────────────────────────────────────────────

void MainWindow::setupConnectAllPopup()
{
    m_connectAllPopup = new QWidget(this);
    m_connectAllPopup->setObjectName("connectAllPopup");
    m_connectAllPopup->setFixedWidth(260);
    m_connectAllPopup->setStyleSheet(
        "QWidget#connectAllPopup {"
        "   background-color: #FFFFFF;"
        "   border: 1px solid #DDE1E3;"
        "   border-radius: 14px;"
        "}"
        );

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setColor(QColor(0, 0, 0, 50));
    shadow->setOffset(0, 6);
    m_connectAllPopup->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(m_connectAllPopup);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);

    m_connectAllPopupTitle = new QLabel(m_connectAllPopup);
    m_connectAllPopupTitle->setText("Ни один датчик не подключён");
    m_connectAllPopupTitle->setStyleSheet(
        "font-weight: bold; font-size: 11pt; color: #1C1F22; background: transparent; border: none;");
    m_connectAllPopupTitle->setWordWrap(true);

    m_connectAllPopupSubtitle = new QLabel(m_connectAllPopup);
    m_connectAllPopupSubtitle->setText("Запустите поиск, чтобы найти и подключить GNSS, АМС, БИНС и ИВС");
    m_connectAllPopupSubtitle->setWordWrap(true);
    m_connectAllPopupSubtitle->setStyleSheet(
        "font-size: 8.5pt; color: #6B7278; background: transparent; border: none;");

    m_connectAllPopupActionBtn = new QPushButton(m_connectAllPopup);
    m_connectAllPopupActionBtn->setText("Подключить все датчики");
    m_connectAllPopupActionBtn->setFocusPolicy(Qt::NoFocus);
    m_connectAllPopupActionBtn->setCursor(Qt::PointingHandCursor);
    m_connectAllPopupActionBtn->setFixedHeight(36);
    m_connectAllPopupActionBtn->setStyleSheet(
        "QPushButton { background:#0F6B4F; color:#FFFFFF; border:none;"
        " border-radius:8px; font-weight:700; }"
        "QPushButton:pressed { background:#0B5A41; }");

    layout->addWidget(m_connectAllPopupTitle);
    layout->addWidget(m_connectAllPopupSubtitle);
    layout->addSpacing(4);
    layout->addWidget(m_connectAllPopupActionBtn);

    m_connectAllPopup->hide();

    m_connectAllPopupAnimation = new QPropertyAnimation(m_connectAllPopup, "pos", this);
    m_connectAllPopupAnimation->setDuration(300);
    m_connectAllPopupAnimation->setEasingCurve(QEasingCurve::OutBack);

    connect(m_connectAllPopupActionBtn, &QPushButton::clicked, this, &MainWindow::onConnectAllPopupActionClicked);
}

void MainWindow::populateConnectAllPopupContent()
{
    // Состояние сейчас всегда одно и то же (шторка доступна только при
    // 0 из 4 подключённых), но проверка на случай поиска в процессе не
    // помешает — на будущее, если правило показа кнопки изменится.
    const bool searching = m_autoConnector && m_autoConnector->isDetecting();
    m_connectAllPopupActionBtn->setEnabled(!searching);
    m_connectAllPopupActionBtn->setText(searching ? "Идёт поиск..." : "Подключить все датчики");
}

void MainWindow::showConnectAllPopup()
{
    // Повторный клик по иконке — закрыть (как у остальных шторок).
    if (m_connectAllPopup->isVisible()) {
        hideConnectAllPopup();
        return;
    }

    populateConnectAllPopupContent();

    const QPoint bottomLeft = ui->btnConnectAll->mapTo(this, QPoint(0, ui->btnConnectAll->height()));
    const int targetX = bottomLeft.x();
    const int targetY = bottomLeft.y() + 8;
    const int startY  = bottomLeft.y() - 20;

    m_connectAllPopup->raise();
    m_connectAllPopupAnimation->stop();
    m_connectAllPopup->move(targetX, startY);
    m_connectAllPopup->show();
    m_connectAllPopupAnimation->setStartValue(QPoint(targetX, startY));
    m_connectAllPopupAnimation->setEndValue(QPoint(targetX, targetY));
    m_connectAllPopupAnimation->start();
}

void MainWindow::hideConnectAllPopup()
{
    if (!m_connectAllPopup || !m_connectAllPopup->isVisible())
        return;
    m_connectAllPopupAnimation->stop();
    m_connectAllPopup->hide();
}
