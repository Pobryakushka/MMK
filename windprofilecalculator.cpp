#include "windprofilecalculator.h"

#include <QDebug>
#include <QScopedArrayPointer>
#include <cmath>
#include <cstring>
#include <chrono>
#include <ctime>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

// Внешние библиотеки расчёта профиля (plow + ClimatData).
// Путь к Profile/profile.h — относительно корня plow. INCLUDEPATH в MMK.pro
// должен указывать на корень plow и на ClimatData (см. ClimatData.pri в plow).
#include "climatdata.h"
#include "Profile/profile.h"
#include "InData/Constants.h"      // PlowAlgoritm::Constants::numStL_out, LOW_NUM_DATA, UNDEF_*
#include "mhn/structures.h"        // PlowAlgoritm::ProfilePoint

namespace {

// Размер массива входного профиля для ProfileReal::setProfRpv.
// Соответствует размеру m_meaProfile из примера calculateProfile.txt (320 точек).
constexpr int kProfileBufferSize = 320;

// Размер выходных массивов на каждый вид ветра.
// Внутри буфера: [0..n-1] — TETA, [n..2n-1] — V (см. profile.cpp финальный цикл).
constexpr int kOutSizePerKind = PlowAlgoritm::Constants::numStL_out;

// Стандартные высоты профиля ветра библиотеки plow (Constants::numStL_out шт.).
// Источник — getprofile__6p.cpp::GetProfileLayer():
//   уровни 0..5  — фиксированные 25,50,75,100,150,200;
//   уровни 6..32 — Constants::StandartLayer[1..27].
// И средний, и действительный ветер раскладываются по этим же высотам.
constexpr float kPlowStandardHeights[] = {
    25.f,   50.f,   75.f,   100.f,  150.f,  200.f,
    300.f,  400.f,  500.f,  600.f,  700.f,  800.f,  900.f,  1000.f,
    1100.f, 1200.f, 1400.f, 1600.f, 1800.f, 2000.f, 2200.f, 2400.f,
    2600.f, 2700.f, 3000.f, 3500.f, 4000.f, 4500.f, 5000.f, 5500.f,
    6000.f, 7000.f, 8000.f
};
constexpr int kPlowStandardHeightCount =
    static_cast<int>(sizeof(kPlowStandardHeights) / sizeof(kPlowStandardHeights[0]));

static_assert(kPlowStandardHeightCount == PlowAlgoritm::Constants::numStL_out,
              "kPlowStandardheights: число высот не совпадает с numStL_out");
} // namespace


WindProfileCalculator::WindProfileCalculator(const QString &climatDataPath)
{
    // climatDataPath может быть относительным ("climatData/climat/").
    // Относительный путь резолвим от папки ИСПОЛНЯЕМОГО ФАЙЛА, а не от
    // рабочей директории процесса. Рабочая директория зависит от способа
    // запуска: из Qt Creator это build-папка, из терминала — текущая папка.
    // applicationDirPath() стабилен в обоих случаях.
    QFileInfo fi(climatDataPath);
    if (fi.isAbsolute()) {
        m_climatDataPath = climatDataPath;
    } else {
        const QString appDir = QCoreApplication::applicationDirPath();
        m_climatDataPath = QDir(appDir).absoluteFilePath(climatDataPath);
    }

    // ClimatData склеивает путь как (pathDataFile + nameDataFile) простой
    // конкатенацией строк, поэтому путь ОБЯЗАН заканчиваться разделителем,
    // иначе получится ".../climatwarm0405.out" вместо ".../climat/warm0405.out".
    if (!m_climatDataPath.endsWith('/') && !m_climatDataPath.endsWith('\\'))
        m_climatDataPath += '/';

    qInfo() << "[WindProfileCalculator] путь к климатической базе:"
            << m_climatDataPath;
}

WindProfileCalculator::~WindProfileCalculator() = default;


QString WindProfileCalculator::resultString(Result r)
{
    switch (r) {
    case OK:                     return QStringLiteral("OK");
    case ERR_NO_MEASURED_DATA:   return QStringLiteral("Нет валидных точек измеренного ветра");
    case ERR_PROFILE_NOT_CONFIG: return QStringLiteral("ProfileReal: профиль не сконфигурирован");
    case ERR_CLIMAT_NOT_READY:   return QStringLiteral("ClimatData: климатическая база не готова для координат");
    case ERR_HEIGHT_NOT_CONFIG:  return QStringLiteral("ProfileReal: высота станции не задана");
    case ERR_EXCEPTION:          return QStringLiteral("Исключение при расчёте");
    }
    return QStringLiteral("Неизвестная ошибка");
}


WindProfileCalculator::Result
WindProfileCalculator::calculate(const Input &in, Output &out)
{
    out.actualWind.clear();
    out.avgWind.clear();
    out.debugSummary.clear();

    Q_ASSERT(kPlowStandardHeightCount == PlowAlgoritm::Constants::numStL_out);

    // ── 1. Подготовка входного профиля (по примеру calculateProfile.txt) ─────
    // Создаём буфер на 320 точек; копируем только валидные точки (reliability == 2
    // и speed не NaN). Счётчик h — фактическое число валидных уровней.
    QScopedArrayPointer<PlowAlgoritm::ProfilePoint> profRPV(
        new PlowAlgoritm::ProfilePoint[kProfileBufferSize]);

    int h = 0;
    const int srcN = qMin(in.measuredWind.size(), kProfileBufferSize);
    for (int i = 0; i < srcN; ++i) {
        const MeasuredWindData &m = in.measuredWind.at(i);
        // Полярность достоверности по протоколу АМС: 1 — достоверная точка,
        // 0 — недостоверная. В расчёт берём только достоверные.
        const bool curr = (m.reliability == 1);
        if (!curr) continue;
        if (std::isnan(m.windSpeed) || std::isnan(m.height)) continue;

        profRPV[h].height = m.height;
        profRPV[h].speed  = m.windSpeed;
        profRPV[h].azimut = m.windDirection;
        profRPV[h].curr   = 1;
        ++h;
    }

    qDebug() << "[WindProfileCalculator] валидных точек измерения:" << h
             << "из" << in.measuredWind.size();

    if (h <= 0) {
        // Полное отсутствие измерений — расчёт по приземному ветру теоретически
        // возможен (ProfileReal::bottomLayer), но текущая реализация требует
        // как минимум одного валидного уровня для корректной экстраполяции.
        out.debugSummary = QStringLiteral("Нет валидных точек измеренного ветра");
        return ERR_NO_MEASURED_DATA;
    }

    // ── 2. Инициализация ClimatData ──────────────────────────────────────────
    ClimatData cd(m_climatDataPath.toStdString());

    // Месяц берём из времени зондирования, если оно валидно; иначе — текущий.
    int month;
    if (in.sondingTime.isValid()) {
        month = in.sondingTime.date().month();
    } else {
        const auto now = std::chrono::system_clock::now();
        const std::time_t tt = std::chrono::system_clock::to_time_t(now);
        month = std::localtime(&tt)->tm_mon + 1;
    }

    cd.setClZone(static_cast<float>(in.latitudeDeg),
                 static_cast<float>(in.longitudeDeg),
                 month);

    if (!cd.isDataReady()) {
        out.debugSummary = QStringLiteral("Климатическая база не готова для широты=%1, долготы=%2, месяца=%3")
                            .arg(in.latitudeDeg).arg(in.longitudeDeg).arg(month);
        qWarning() << "[WindProfileCalculator]" << out.debugSummary;
        return ERR_CLIMAT_NOT_READY;
    }

    // ── 3. Конфигурируем ProfileReal ────────────────────────────────────────
    ProfileReal profileReal;
    profileReal.setProfRpv(profRPV.data(), h);
    profileReal.setClimatData(&cd);

    const QDateTime dtCur = in.sondingTime.isValid()
                              ? in.sondingTime
                              : QDateTime::currentDateTime();
    profileReal.setDateTime(dtCur.date().year(),
                            dtCur.date().month(),
                            dtCur.date().day(),
                            dtCur.time().hour() + dtCur.time().minute() / 60.0);

    profileReal.setHeight(in.stationAltitudeM);
    profileReal.setGroundWind(in.surfaceWindSpeedMs,
                              in.surfaceWindDirDeg,
                              in.groundWindHeightM);
    profileReal.setz0(in.z0);

    // setMeteoMid — не вызываем: входящий бюллетень не используется
    // (поведение совпадает с веткой, когда m_oldBulletinReady == false).

    // ── 4. Запуск расчёта ────────────────────────────────────────────────────
    float realWindBuf[kOutSizePerKind * 2];
    float avgWindBuf [kOutSizePerKind * 2];
    std::memset(realWindBuf, 0, sizeof(realWindBuf));
    std::memset(avgWindBuf,  0, sizeof(avgWindBuf));

    ProfileReal::MessErrors ret;
    try {
        ret = profileReal.GetProfile(realWindBuf, avgWindBuf);
    } catch (...) {
        out.debugSummary = QStringLiteral("Исключение в ProfileReal::GetProfile");
        qCritical() << "[WindProfileCalculator]" << out.debugSummary;
        return ERR_EXCEPTION;
    }

    switch (ret) {
    case ProfileReal::RESULT_OK:
        break;
    case ProfileReal::PROFILE_NOT_CONFIG_PROF_RPV:
        out.debugSummary = QStringLiteral("ProfileReal::PROFILE_NOT_CONFIG_PROF_RPV");
        qWarning() << "[WindProfileCalculator]" << out.debugSummary;
        return ERR_PROFILE_NOT_CONFIG;
    case ProfileReal::PROFILE_CLIMAT_NOT_READY:
        out.debugSummary = QStringLiteral("ProfileReal::PROFILE_CLIMAT_NOT_READY");
        return ERR_CLIMAT_NOT_READY;
    case ProfileReal::PROFILE_NOT_CONFIG_HEIGHT:
        out.debugSummary = QStringLiteral("ProfileReal::PROFILE_NOT_CONFIG_HEIGHT");
        return ERR_HEIGHT_NOT_CONFIG;
    }

    // ── 5. Конвертация выходных массивов в QVector<WindProfileData> ──────────
    // Формат буферов (как в profile.cpp):
    //   [0 .. LOW_NUM_DATA-1]                — TETA  (направление, град)
    //   [LOW_NUM_DATA .. 2*LOW_NUM_DATA-1]   — V     (скорость, м/с)
    // Высоты совпадают со стандартными высотами действительного/среднего ветра
    // АМС — берём их из AMSProtocol, чтобы профиль был совместим с уже работающим
    // кодом (отображение, экспорт, БД).
    const int n = kPlowStandardHeightCount;

    //Средний и действительный ветер раскладываются по ОДНИМ И ТЕМ ЖЕ ВЫСОТАМ -
    // профиль plow единый
    QVector<float> realHeights;
    QVector<float> avgHeights;
    realHeights.reserve(n);
    avgHeights.reserve(n);
    for (int i = 0; i < n; i++) {
        realHeights.append(kPlowStandardHeights[i]);
        avgHeights.append(kPlowStandardHeights[i]);
        }

    out.actualWind.reserve(n);
    out.avgWind.reserve(n);

    int validReal = 0, validAvg = 0;

    for (int i = 0; i < n; ++i) {
        // Действительный ветер
        const float realTeta = realWindBuf[i];
        const float realV    = realWindBuf[i + kOutSizePerKind];

        WindProfileData rp;
        rp.height = (i < realHeights.size()) ? realHeights[i] : 0.0f;
        // ProfileReal помечает «нет данных» через TETA == -9999, V == 0
        // (см. финальный цикл в profile.cpp).
        if (realTeta <= -9000.f || std::isnan(realTeta) || std::isnan(realV)) {
            rp.windSpeed     = 0.0f;
            rp.windDirection = 0.0f;
            rp.isValid       = false;
        } else {
            rp.windSpeed     = realV;
            rp.windDirection = realTeta;
            rp.isValid       = true;
            ++validReal;
        }
        out.actualWind.append(rp);

        // Средний ветер
        const float avgTeta = avgWindBuf[i];
        const float avgV    = avgWindBuf[i + kOutSizePerKind];

        WindProfileData ap;
        ap.height = (i < avgHeights.size()) ? avgHeights[i] : 0.0f;
        if (avgTeta <= -9000.f || std::isnan(avgTeta) || std::isnan(avgV)) {
            ap.windSpeed     = 0.0f;
            ap.windDirection = 0.0f;
            ap.isValid       = false;
        } else {
            ap.windSpeed     = avgV;
            ap.windDirection = avgTeta;
            ap.isValid       = true;
            ++validAvg;
        }
        out.avgWind.append(ap);
    }

    out.debugSummary = QStringLiteral("OK: actual=%1/%2, avg=%3/%2, источник=измеренный+IWS")
                         .arg(validReal).arg(n).arg(validAvg);
    qInfo() << "[WindProfileCalculator]" << out.debugSummary;

    return OK;
}
