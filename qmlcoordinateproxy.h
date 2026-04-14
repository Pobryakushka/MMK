#ifndef QMLCOORDINATEPROXY_H
#define QMLCOORDINATEPROXY_H

#include <QObject>
#include <QtPositioning/QGeoCoordinate>
#include <QStringList>

class QmlCoordinateProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QGeoCoordinate coordinateFrom READ coordinateFrom WRITE setCoordinateFrom NOTIFY coordinateFromChanged)
    Q_PROPERTY(QGeoCoordinate coordinateTo READ coordinateTo WRITE setCoordinateTo NOTIFY coordinateToChanged)
    Q_PROPERTY(bool coldZonesVisible READ coldZonesVisible WRITE setColdZonesVisible NOTIFY coldZonesVisibleChanged)
    Q_PROPERTY(bool warmZonesVisible READ warmZonesVisible WRITE setWarmZonesVisible NOTIFY warmZonesVisibleChanged)
    Q_PROPERTY(bool fitView READ fitView WRITE setFitView NOTIFY fitViewChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QStringList searchResult READ searchResult WRITE setSearchResult NOTIFY searchResultChanged)
    Q_PROPERTY(QStringList mapTypes READ mapTypes WRITE setMapTypes NOTIFY mapTypesChanged)
    Q_PROPERTY(uint currentMapType READ currentMapType WRITE setCurrentMapType NOTIFY currentMapTypeChanged)

    // Видимая область карты — обновляется из QML при изменении visibleRegion
    Q_PROPERTY(double visibleNorth READ visibleNorth WRITE setVisibleNorth NOTIFY visibleBoundsChanged)
    Q_PROPERTY(double visibleSouth READ visibleSouth WRITE setVisibleSouth NOTIFY visibleBoundsChanged)
    Q_PROPERTY(double visibleWest  READ visibleWest  WRITE setVisibleWest  NOTIFY visibleBoundsChanged)
    Q_PROPERTY(double visibleEast  READ visibleEast  WRITE setVisibleEast  NOTIFY visibleBoundsChanged)

    QGeoCoordinate m_coordinateFrom;
    QGeoCoordinate m_coordinateTo;

    bool m_coldZonesVisible;
    bool m_warmZonesVisible;
    bool m_fitView;

    QString m_searchText;
    QStringList m_searchResult;
    QStringList m_mapTypes;
    uint m_currentMapType;

    double m_visibleNorth = 0.0;
    double m_visibleSouth = 0.0;
    double m_visibleWest  = 0.0;
    double m_visibleEast  = 0.0;

public:
    QmlCoordinateProxy() : QObject(Q_NULLPTR),
        m_coordinateFrom(QGeoCoordinate(54.192, 37.5921)), m_coordinateTo(m_coordinateFrom),
        m_coldZonesVisible(false), m_warmZonesVisible(false), m_fitView(false), m_currentMapType(0)
    {

    }

    bool coldZonesVisible() const
    {
        return m_coldZonesVisible;
    }

    bool warmZonesVisible() const
    {
        return m_warmZonesVisible;
    }

    bool fitView() const
    {
        return m_fitView;
    }

    QString searchText() const
    {
        return m_searchText;
    }

    QStringList searchResult() const
    {
        return m_searchResult;
    }

    QGeoCoordinate coordinateFrom() const
    {
        return m_coordinateFrom;
    }

    QGeoCoordinate coordinateTo() const
    {
        return m_coordinateTo;
    }

    QStringList mapTypes() const
    {
        return m_mapTypes;
    }

    uint currentMapType() const
    {
        return m_currentMapType;
    }

    double visibleNorth() const { return m_visibleNorth; }
    double visibleSouth() const { return m_visibleSouth; }
    double visibleWest()  const { return m_visibleWest;  }
    double visibleEast()  const { return m_visibleEast;  }

public slots:
    void setColdZonesVisible(bool coldZonesVisible)
    {
        if (m_coldZonesVisible == coldZonesVisible)
            return;

        m_coldZonesVisible = coldZonesVisible;
        emit coldZonesVisibleChanged(m_coldZonesVisible);
    }

    void setWarmZonesVisible(bool warmZonesVisible)
    {
        if (m_warmZonesVisible == warmZonesVisible)
            return;

        m_warmZonesVisible = warmZonesVisible;
        emit warmZonesVisibleChanged(m_warmZonesVisible);
    }

    void setFitView(bool fitView)
    {
        if (m_fitView == fitView)
            return;

        m_fitView = fitView;
        emit fitViewChanged(m_fitView);
    }

    void setSearchText(const QString &searchText)
    {
        if (m_searchText == searchText)
            return;

        m_searchText = searchText;
        emit searchTextChanged(m_searchText);
    }

    void setSearchResult(const QStringList &searchResult)
    {
        if (m_searchResult == searchResult)
            return;

        m_searchResult = searchResult;
        emit searchResultChanged(m_searchResult);
    }

    void setCoordinateFrom(const QGeoCoordinate &coordinateFrom)
    {
        if (m_coordinateFrom == coordinateFrom)
            return;

        m_coordinateFrom = coordinateFrom;
        emit coordinateFromChanged(m_coordinateFrom);
    }

    void setCoordinateTo(const QGeoCoordinate &coordinateTo)
    {
        if (m_coordinateTo == coordinateTo)
            return;

        m_coordinateTo = coordinateTo;
        emit coordinateToChanged(m_coordinateTo);
    }

    void setMapTypes(const QStringList &types)
    {
        if (m_mapTypes != types){
            m_mapTypes = types;
            emit mapTypesChanged(m_mapTypes);
        }
    }

    void setCurrentMapType(uint index)
    {
        if (m_currentMapType != index) {
            m_currentMapType = index;
            emit currentMapTypeChanged(index);
        }
    }

    void setVisibleNorth(double v) { m_visibleNorth = v; }
    void setVisibleSouth(double v) { m_visibleSouth = v; }
    void setVisibleWest (double v) { m_visibleWest  = v; }
    void setVisibleEast (double v) { m_visibleEast  = v; emit visibleBoundsChanged(); }

signals:
    void coldZonesVisibleChanged(bool coldZonesVisible);
    void warmZonesVisibleChanged(bool warmZonesVisible);
    void fitViewChanged(bool fitView);
    void searchTextChanged(const QString &searchText);
    void searchResultChanged(const QStringList &searchResult);
    void coordinateFromChanged(const QGeoCoordinate &coordinateFrom);
    void coordinateToChanged(const QGeoCoordinate &coordinateTo);
    void mapTypesChanged(const QStringList &types);
    void currentMapTypeChanged(uint index);
    void visibleBoundsChanged();
};

#endif // QMLCOORDINATEPROXY_H
