#include "data/ArchiveWriter.h"

#include "data/databasemanager.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QVariant>
#include <QDebug>

namespace ArchiveWriter {

bool saveMeteo11Bulletin(int recordId,
                         const QJsonObject &bulletinJson,
                                     const QDateTime   &bulletinTime,
                                     const QString     &validityPeriod)
{
    if (recordId <= 0) {
        qWarning() << "saveMeteo11Bulletin: нет активного record_id, пропускаем";
        return false;
    }
    if (!DatabaseManager::instance()->connect()) {
        qWarning() << "saveMeteo11Bulletin: нет подключения к БД";
        return false;
    }

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO meteo_11_bulletin "
        "  (record_id, bulletin_data, bulletin_time, validity_period) "
        "VALUES (:record_id, :data::jsonb, :time, :period) "
        "ON CONFLICT (record_id) DO UPDATE SET "
        "  bulletin_data   = EXCLUDED.bulletin_data, "
        "  bulletin_time   = EXCLUDED.bulletin_time, "
        "  validity_period = EXCLUDED.validity_period"
        );
    query.bindValue(":record_id", recordId);
    query.bindValue(":data",      QString::fromUtf8(
                                 QJsonDocument(bulletinJson).toJson(QJsonDocument::Compact)));
    query.bindValue(":time",      bulletinTime);
    query.bindValue(":period",    validityPeriod);

    if (!query.exec()) {
        qWarning() << "saveMeteo11Bulletin: ошибка БД:"
                   << query.lastError().text();
        return false;
    }

    qInfo() << "AMSHandler: Метео-11 сохранён для record_id=" << recordId
            << "время:" << bulletinTime.toString("dd.MM.yyyy HH:mm");
    return true;
}

int createMainArchiveRecord(const QString &notes, QString &errorMsg)
{
    if (!DatabaseManager::instance()->connect()) return -1;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);

    // ИСПРАВЛЕНО: используем completion_time (название поля в реальной БД)
    query.prepare("INSERT INTO main_archive (completion_time, notes) "
                  "VALUES (NOW(), :notes) RETURNING record_id");
    query.bindValue(":notes", notes.isEmpty() ? QVariant(QVariant::String) : notes);

    if (!query.exec() || !query.next()) {
        QString error = QString("Ошибка создания записи в main_archive: %1")
                            .arg(query.lastError().text());
        qCritical() << "AMSHandler:" << error;
        errorMsg = error;
        return -1;
    }

    int recordId = query.value(0).toInt();
    qInfo() << "AMSHandler: Создана запись в main_archive с ID:" << recordId;
    return recordId;
}

bool deleteCalculatedWindProfiles(int recordId)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    // Получаем текущие profile_id для среднего и действительного
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "SELECT avg_wind_profile_id, actual_wind_profile_id "
        "FROM wind_profiles_references WHERE record_id = :rid"
        );
    refQuery.bindValue(":rid", recordId);

    if (!refQuery.exec() || !refQuery.next()) {
        // Записи нет — удалять нечего, не ошибка
        return true;
    }

    QVariant avgPidVar    = refQuery.value(0);
    QVariant actualPidVar = refQuery.value(1);

    db.transaction();

    auto deletePoints = [&](const char *table, const QVariant &pidVar) -> bool {
        if (pidVar.isNull()) return true;
        QSqlQuery q(db);
        q.prepare(QString("DELETE FROM %1 WHERE profile_id = :pid").arg(table));
        q.bindValue(":pid", pidVar.toInt());
        if (!q.exec()) {
            qWarning() << "deleteCalculatedWindProfiles:"
                       << table << "→" << q.lastError().text();
            return false;
        }
        qDebug() << "deleteCalculatedWindProfiles: удалены точки из"
                 << table << "для profile_id=" << pidVar.toInt();
        return true;
    };

    if (!deletePoints("avg_wind_profile",    avgPidVar)    ||
        !deletePoints("actual_wind_profile", actualPidVar)) {
        db.rollback();
        return false;
    }

    // Зачищаем ссылки (профильные id теперь невалидны)
    QSqlQuery updRef(db);
    updRef.prepare(
        "UPDATE wind_profiles_references "
        "SET avg_wind_profile_id = NULL, actual_wind_profile_id = NULL "
        "WHERE record_id = :rid"
        );
    updRef.bindValue(":rid", recordId);
    if (!updRef.exec()) {
        qWarning() << "deleteCalculatedWindProfiles: refs →"
                   << updRef.lastError().text();
        db.rollback();
        return false;
    }

    db.commit();
    return true;
}

bool saveAvgWindProfile(int recordId, const QVector<WindProfileData> &data,
                        QString &errorMsg)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    QDateTime measurementTime = QDateTime::currentDateTime();

    db.transaction();

    // ШАГ 1: Генерируем ОДИН profile_id для всего профиля
    QSqlQuery idQuery(db);
    if (!idQuery.exec("SELECT nextval('avg_wind_profile_profile_id_seq')")) {
        db.rollback();
        QString error = QString("Ошибка генерации profile_id: %1")
                            .arg(idQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        errorMsg = error;
        return false;
    }

    if (!idQuery.next()) {
        db.rollback();
        qCritical() << "AMSHandler: Не удалось получить profile_id";
        return false;
    }

    int profileId = idQuery.value(0).toInt();
    qDebug() << "AMSHandler: Сгенерирован profile_id =" << profileId << "для avg_wind_profile";

    // ШАГ 2: Вставляем ВСЕ точки с ОДНИМ И ТЕМ ЖЕ profile_id
    QSqlQuery query(db);
    query.prepare("INSERT INTO avg_wind_profile "
                  "(profile_id, height, wind_speed, wind_direction, measurement_time) "
                  "VALUES (:profile_id, :height, :speed, :direction, :time)");

    for (int i = 0; i < data.size(); i++) {
        const WindProfileData &point = data[i];

        qDebug() << "AMSHandler: avg_wind точка" << i
                 << "height=" << point.height
                 << "speed=" << point.windSpeed
                 << "direction=" << point.windDirection;

        query.bindValue(":profile_id", profileId);
        query.bindValue(":height", point.height);
        query.bindValue(":speed", point.windSpeed);
        query.bindValue(":direction", point.windDirection);
        query.bindValue(":time", measurementTime);

        if (!query.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки среднего ветра: %1")
                                .arg(query.lastError().text());
            qCritical() << "AMSHandler:" << error;
            errorMsg = error;
            return false;
        }
    }

    db.commit();

    qInfo() << "AMSHandler: Сохранён профиль среднего ветра:"
            << data.size() << "точек с profile_id =" << profileId;

    // ШАГ 3: Обновляем ссылку в wind_profiles_references (upsert по record_id)
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "INSERT INTO wind_profiles_references (record_id, avg_wind_profile_id) "
        "VALUES (:record_id, :profile_id) "
        "ON CONFLICT (record_id) DO UPDATE SET avg_wind_profile_id = EXCLUDED.avg_wind_profile_id"
        );
    refQuery.bindValue(":record_id", recordId);
    refQuery.bindValue(":profile_id", profileId);
    if (!refQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка upsert avg_wind_profile_id:" << refQuery.lastError().text();
    }

    return true;
}

bool saveActualWindProfile(int recordId, const QVector<WindProfileData> &data,
                           QString &errorMsg)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    QDateTime measurementTime = QDateTime::currentDateTime();

    db.transaction();

    // ШАГ 1: Генерируем ОДИН profile_id для всего профиля
    QSqlQuery idQuery(db);
    if (!idQuery.exec("SELECT nextval('actual_wind_profile_profile_id_seq')")) {
        db.rollback();
        QString error = QString("Ошибка генерации profile_id: %1")
                            .arg(idQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        errorMsg = error;
        return false;
    }

    if (!idQuery.next()) {
        db.rollback();
        qCritical() << "AMSHandler: Не удалось получить profile_id";
        return false;
    }

    int profileId = idQuery.value(0).toInt();
    qDebug() << "AMSHandler: Сгенерирован profile_id =" << profileId << "для actual_wind_profile";

    // ШАГ 2: Вставляем ВСЕ точки с ОДНИМ И ТЕМ ЖЕ profile_id
    QSqlQuery query(db);
    query.prepare("INSERT INTO actual_wind_profile "
                  "(profile_id, height, wind_speed, wind_direction, measurement_time) "
                  "VALUES (:profile_id, :height, :speed, :direction, :time)");

    for (int i = 0; i < data.size(); i++) {
        const WindProfileData &point = data[i];

        qDebug() << "AMSHandler: actual_wind точка" << i
                 << "height=" << point.height
                 << "speed=" << point.windSpeed
                 << "direction=" << point.windDirection;

        query.bindValue(":profile_id", profileId);
        query.bindValue(":height", point.height);
        query.bindValue(":speed", point.windSpeed);
        query.bindValue(":direction", point.windDirection);
        query.bindValue(":time", measurementTime);

        if (!query.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки действительного ветра: %1")
                                .arg(query.lastError().text());
            qCritical() << "AMSHandler:" << error;
            errorMsg = error;
            return false;
        }
    }

    db.commit();

    qInfo() << "AMSHandler: Сохранён профиль действительного ветра:"
            << data.size() << "точек с profile_id =" << profileId;

    // ШАГ 3: Обновляем ссылку в wind_profiles_references (upsert по record_id)
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "INSERT INTO wind_profiles_references (record_id, actual_wind_profile_id) "
        "VALUES (:record_id, :profile_id) "
        "ON CONFLICT (record_id) DO UPDATE SET actual_wind_profile_id = EXCLUDED.actual_wind_profile_id"
        );
    refQuery.bindValue(":record_id", recordId);
    refQuery.bindValue(":profile_id", profileId);
    if (!refQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка upsert actual_wind_profile_id:" << refQuery.lastError().text();
    }

    return true;
}

bool saveMeasuredWindProfile(int recordId, const QVector<MeasuredWindData> &data,
                             QString &errorMsg)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();

    QDateTime measurementTime = QDateTime::currentDateTime();

    db.transaction();

    // ШАГ 1: Генерируем ОДИН profile_id для всего профиля
    QSqlQuery idQuery(db);
    if (!idQuery.exec("SELECT nextval('measured_wind_profile_profile_id_seq')")) {
        db.rollback();
        QString error = QString("Ошибка генерации profile_id: %1")
                            .arg(idQuery.lastError().text());
        qCritical() << "AMSHandler:" << error;
        errorMsg = error;
        return false;
    }

    if (!idQuery.next()) {
        db.rollback();
        qCritical() << "AMSHandler: Не удалось получить profile_id";
        return false;
    }

    int profileId = idQuery.value(0).toInt();
    qDebug() << "AMSHandler: Сгенерирован profile_id =" << profileId << "для measured_wind_profile";

    // ШАГ 2: Вставляем ВСЕ точки с ОДНИМ И ТЕМ ЖЕ profile_id.
    // reliability — признак достоверности точки от АМС (1 - достоверная, 0 - нет).
    QSqlQuery query(db);
    query.prepare("INSERT INTO measured_wind_profile "
                  "(profile_id, height, wind_speed, wind_direction, measurement_time, reliability) "
                  "VALUES (:profile_id, :height, :speed, :direction, :time, :reliability)");

    for (const MeasuredWindData &point : data) {
        query.bindValue(":profile_id", profileId);  // ОДИН И ТОТ ЖЕ!
        query.bindValue(":height", point.height);
        query.bindValue(":speed", point.windSpeed);
        query.bindValue(":direction", point.windDirection);
        query.bindValue(":time", measurementTime);
        query.bindValue(":reliability", point.reliability);

        if (!query.exec()) {
            db.rollback();
            QString error = QString("Ошибка записи точки измеренного ветра: %1")
                                .arg(query.lastError().text());
            qCritical() << "AMSHandler:" << error;
            errorMsg = error;
            return false;
        }
    }

    db.commit();

    qInfo() << "AMSHandler: Сохранён профиль измеренного ветра:"
            << data.size() << "точек с profile_id =" << profileId;

    // ШАГ 3: Обновляем ссылку в wind_profiles_references (upsert по record_id)
    QSqlQuery refQuery(db);
    refQuery.prepare(
        "INSERT INTO wind_profiles_references (record_id, measured_wind_profile_id) "
        "VALUES (:record_id, :profile_id) "
        "ON CONFLICT (record_id) DO UPDATE SET measured_wind_profile_id = EXCLUDED.measured_wind_profile_id"
        );
    refQuery.bindValue(":record_id", recordId);
    refQuery.bindValue(":profile_id", profileId);
    if (!refQuery.exec()) {
        qCritical() << "AMSHandler: Ошибка upsert measured_wind_profile_id:" << refQuery.lastError().text();
    }

    return true;
}

bool saveStationCoordinates(int recordId, const StationCoordinates &coords,
                            QString &errorMsg)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare("INSERT INTO station_coordinates (record_id, latitude, longitude, altitude) "
                  "VALUES (:record_id, :lat, :lon, :alt)");

    double latDeg = coords.latitude / 3600.0;
    double lonDeg = coords.longitude / 3600.0;

    query.bindValue(":record_id", recordId);
    query.bindValue(":lat", latDeg);
    query.bindValue(":lon", lonDeg);
    query.bindValue(":alt", coords.altitude);

    if (!query.exec()) {
        QString error = QString("Ошибка записи координат: %1").arg(query.lastError().text());
        qCritical() << "AMSHandler:" << error;
        errorMsg = error;
        return false;
    }

    qInfo() << "AMSHandler: Сохранены координаты станции";
    return true;
}

bool saveCriticalMessage(int recordId, const QString &message, const QString &severity,
                         QString &errorMsg)
{
    if (!DatabaseManager::instance()->connect()) return false;

    QSqlDatabase db = DatabaseManager::instance()->database();
    QSqlQuery query(db);
    query.prepare("INSERT INTO critical_messages (record_id, message_text, message_time, severity_level) "
                  "VALUES (:record_id, :message, NOW(), :severity)");

    query.bindValue(":record_id", recordId);
    query.bindValue(":message", message);
    query.bindValue(":severity", severity);

    if (!query.exec()) {
        QString error = QString("Ошибка записи критического сообщения: %1").arg(query.lastError().text());
        qCritical() << "AMSHandler:" << error;
        errorMsg = error;
        return false;
    }

    qInfo() << "AMSHandler: Сохранено критическое сообщение:" << message;
    return true;
}
} // namespace ArchiveWriter
