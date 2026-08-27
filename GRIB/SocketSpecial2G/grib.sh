#!/bin/bash
set -u

NOMADS_URL="https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod"
S3_URL="https://noaa-gfs-bdp-pds.s3.amazonaws.com"
FILTER_URL="https://nomads.ncep.noaa.gov/cgi-bin/filter_gfs_0p25.pl"

DOWNLOAD_DIR="./gfs_download"
FILES_DIR="./Send/Files"
LOCKFILE="./gfs_download.lock"
LOGFILE="./gfs_download.log"

SOURCE="nomads"
VARS=""
LEVS=""
REGION=""
POINT=""
POINT_WINDOW=2   # градусов в каждую сторону от точки клиента (Kriging заменён на билинейную интерполяцию — нужна только ячейка сетки 0.25° вокруг точки, окно оставлено с запасом)
POINT_VARS="HGT:TMP:UGRD:VGRD:PRMSL:RH"   # соответствует h/gh, t, u, v, p/prmsl, r в Mushroom
# Стандартные изобарические уровни (мбар), покрывающие с запасом диапазон
# высот 0-8000 м по стандартной атмосфере (8000 м ~= 356 гПа), плюс
# приземные величины. Явный список вместо all_lev=on — чтобы не тянуть
# посторонние авиационные футовые эшелоны и "битые" типы уровней.
POINT_LEVS="1000_mb:975_mb:950_mb:925_mb:900_mb:850_mb:800_mb:750_mb:700_mb:650_mb:600_mb:550_mb:500_mb:450_mb:400_mb:350_mb:surface:2_m_above_ground:10_m_above_ground:mean_sea_level"

mkdir -p "$DOWNLOAD_DIR"

msg(){ echo "$@"; }
log(){ echo "$(date '+%Y-%m-%d %H:%M:%S')  $@" | tee -a "$LOGFILE"; }

usage(){
    msg "Использование:"
    msg "  $0 YYYYMMDD RUN [START END] [--source nomads|s3] [--vars V1:V2] [--levs L1:L2] [--region LEFTLON:RIGHTLON:TOPLAT:BOTTOMLAT]"
    msg
    msg "Примеры:"
    msg "  $0 20260701 00"
    msg "  $0 20260701 00 0 24 --source s3"
    msg "  $0 20260701 12 132 384 --vars HGT:TMP --levs 500_mb:sfc"
    msg "  $0 20260701 00 0 0 --vars TMP --levs surface --region 30:45:60:50"
    msg "  $0 20260701 00 0 0 --point 55.75:37.62"
    msg
    msg "Примечание: --region требует ОБЯЗАТЕЛЬНОГО указания --vars и --levs."
    msg "            Названия уровней для --region должны совпадать с именами"
    msg "            фильтра NOMADS (пример: surface, 2_m_above_ground, 500_mb),"
    msg "            а не с сокращениями из .idx (sfc и т.п.)."
    msg
    msg "            --point LAT:LONG — скачивает окно \${POINT_WINDOW}° вокруг"
    msg "            указанной точки с фиксированным набором параметров"
    msg "            (HGT/TMP/UGRD/VGRD/PRMSL/RH) на стандартных изобарических"
    msg "            уровнях 1000-350 мбар + приземные величины (surface,"
    msg "            2m, 10m, mean sea level) — покрывает высоты 0-8000 м"
    msg "            с запасом. Несовместим с --vars/--levs/--region."
    msg "            Требует --source nomads."
    exit 1
}

POSITIONAL=()
while [ $# -gt 0 ]; do
    case "$1" in
        --source) SOURCE="$2"; shift 2 ;;
        --vars)   VARS="$2"; shift 2 ;;
        --levs)   LEVS="$2"; shift 2 ;;
        --region) REGION="$2"; shift 2 ;;
        --point)  POINT="$2"; shift 2 ;;
        *) POSITIONAL+=("$1"); shift ;;
    esac
done
set -- "${POSITIONAL[@]}"

if [ $# -eq 2 ]; then
    CURRENT_DATE="$1"; RUN="$2"; START=0; END=384
elif [ $# -eq 4 ]; then
    CURRENT_DATE="$1"; RUN="$2"; START="$3"; END="$4"
else
    usage
fi

case "$RUN" in
    00|06|12|18) ;;
    *) msg "Ошибка: RUN должен быть 00, 06, 12 или 18."; exit 1 ;;
esac

case "$SOURCE" in
    nomads|s3) ;;
    *) msg "Ошибка: --source должен быть nomads или s3."; exit 1 ;;
esac

if [ "$START" -gt "$END" ]; then
    msg "Ошибка: START больше END."
    exit 1
fi

PARTIAL=0
if [ -n "$VARS" ] || [ -n "$LEVS" ]; then
    PARTIAL=1
    [ -z "$VARS" ] && VARS="all"
    [ -z "$LEVS" ] && LEVS="all"
fi

# --- Валидация --region ---
LEFTLON=""; RIGHTLON=""; TOPLAT=""; BOTTOMLAT=""
if [ -n "$REGION" ]; then
    if [ "$SOURCE" != "nomads" ]; then
        msg "Ошибка: --region поддерживается только с --source nomads (фильтр-сервис NOMADS)."
        exit 1
    fi
    if [ -z "$VARS" ] || [ "$VARS" = "all" ] || [ -z "$LEVS" ] || [ "$LEVS" = "all" ]; then
        msg "Ошибка: --region требует явного указания --vars и --levs (нельзя 'all')."
        exit 1
    fi
    IFS=':' read -r LEFTLON RIGHTLON TOPLAT BOTTOMLAT <<< "$REGION"
    if [ -z "$LEFTLON" ] || [ -z "$RIGHTLON" ] || [ -z "$TOPLAT" ] || [ -z "$BOTTOMLAT" ]; then
        msg "Ошибка: --region должен быть в формате LEFTLON:RIGHTLON:TOPLAT:BOTTOMLAT."
        exit 1
    fi
fi

# --- Валидация --point ---
POINT_LAT=""; POINT_LONG=""
if [ -n "$POINT" ]; then
    if [ -n "$REGION" ] || [ -n "$VARS" ] || [ -n "$LEVS" ]; then
        msg "Ошибка: --point несовместим с --region/--vars/--levs."
        exit 1
    fi
    if [ "$SOURCE" != "nomads" ]; then
        msg "Ошибка: --point поддерживается только с --source nomads (фильтр-сервис NOMADS)."
        exit 1
    fi
    IFS=':' read -r POINT_LAT POINT_LONG <<< "$POINT"
    if [ -z "$POINT_LAT" ] || [ -z "$POINT_LONG" ]; then
        msg "Ошибка: --point должен быть в формате LAT:LONG."
        exit 1
    fi

    # Вычисляем окно вокруг точки; широту зажимаем в [-90, 90],
    # долготу передаём как есть (фильтр-сервис принимает и отрицательные значения).
    LEFTLON=$(awk -v lo="$POINT_LONG" -v w="$POINT_WINDOW" 'BEGIN{printf "%.4f", lo-w}')
    RIGHTLON=$(awk -v lo="$POINT_LONG" -v w="$POINT_WINDOW" 'BEGIN{printf "%.4f", lo+w}')
    TOPLAT=$(awk -v la="$POINT_LAT" -v w="$POINT_WINDOW" 'BEGIN{v=la+w; if (v>90) v=90; printf "%.4f", v}')
    BOTTOMLAT=$(awk -v la="$POINT_LAT" -v w="$POINT_WINDOW" 'BEGIN{v=la-w; if (v<-90) v=-90; printf "%.4f", v}')

    # Имя выгружаемого файла (gfs.tRUNz.pgrb2.0p25.fFH) не зависит от даты
    # и точки, поэтому общий FILES_DIR не годится: при повторном запросе на
    # тот же цикл (00/06/12/18), но на другую дату или другую станцию,
    # скрипт увидел бы уже лежащий там чужой файл и тихо отдал бы данные не
    # для той даты/точки. Поэтому под каждый (дата, цикл, точка) — свой
    # подкаталог. Ключ подкаталога должен точно совпадать с тем, что
    # вычисляет GribMeteo11Pipeline::pointDataDir() на стороне C++
    # (Meteo11Grib/GribMeteo11Pipeline.cpp) — там указывается тот же
    # каталог для запуска Mushroom.
    FILES_DIR="${FILES_DIR}/${CURRENT_DATE}_${RUN}_pt${POINT_LAT}_${POINT_LONG}"
fi

mkdir -p "$FILES_DIR"

exec 200>"$LOCKFILE"
flock -n 200 || { msg "Скрипт уже запущен."; exit 1; }

CURRENT_TMPFILE=""
cleanup(){
    [ -n "$CURRENT_TMPFILE" ] && rm -f "$CURRENT_TMPFILE"
    rm -f "$LOCKFILE"
    rm -rf "$DOWNLOAD_DIR"
}
on_int(){ cleanup; trap - EXIT; exit 130; }
trap cleanup EXIT
trap on_int INT TERM

log "Дата       : $CURRENT_DATE"
log "Цикл       : $RUN"
log "Источник   : $SOURCE"
if [ -n "$POINT" ]; then
    MODE_LOG="точка (vars=$POINT_VARS levs=$POINT_LEVS)"
    REGION_LOG="окно ±${POINT_WINDOW}° вокруг точки, см. строку «Точка» ниже"
elif [ $PARTIAL -eq 1 ]; then
    MODE_LOG="частичный (vars=$VARS levs=$LEVS)"
    REGION_LOG="$([ -n "$REGION" ] && echo "$REGION (leftlon:rightlon:toplat:bottomlat)" || echo "весь земной шар")"
else
    MODE_LOG="полный файл"
    REGION_LOG="весь земной шар"
fi
log "Режим      : $MODE_LOG"
log "Регион     : $REGION_LOG"
log "Точка      : $([ -n "$POINT" ] && echo "$POINT, окно ±${POINT_WINDOW}° (leftlon=$LEFTLON rightlon=$RIGHTLON toplat=$TOPLAT bottomlat=$BOTTOMLAT), vars=$POINT_VARS, levs=$POINT_LEVS" || echo "не задана")"
log "Диапазон   : f$(printf "%03d" "$START") - f$(printf "%03d" "$END")"

build_regex(){
    local list="$1"
    list="${list//:/|}"
    list="${list//_/ }"
    if echo "$list" | grep -qi '^all$'; then
        echo "."
    else
        echo ":($list):"
    fi
}

# Собирает query-параметры вида var_TMP=on&var_RH=on из списка "TMP:RH"
build_filter_params(){
    local prefix="$1"
    local list="$2"
    local out=""
    local IFS=':'
    for item in $list; do
        out="${out}&${prefix}_${item}=on"
    done
    echo "$out"
}

VARS_RE=""
LEVS_RE=""
if [ $PARTIAL -eq 1 ] && [ -z "$REGION" ]; then
    VARS_RE=$(build_regex "$VARS")
    LEVS_RE=$(build_regex "$LEVS")
fi

# Счётчик реально полученных файлов (скачанных сейчас или уже лежавших
# на диске с прошлого запуска). Скрипт исторически не выставлял ненулевой
# exit code при неудачах curl/пустых файлах внутри цикла — он просто
# логировал ошибку и переходил к следующему часу, поэтому вызывающая
# сторона (GfsDownloadRunner в MMK, который проверяет только exit code)
# видела "успех", даже если ни один файл не был скачан. Ниже по коду
# считаем успехи и в конце скрипта завершаемся с ошибкой, если не
# получили вообще ничего.
OK_COUNT=0

for ((HOUR=START; HOUR<=END; HOUR++))
do
    FH=$(printf "%03d" "$HOUR")
    FILE="gfs.t${RUN}z.pgrb2.0p25.f${FH}"

    if [ "$SOURCE" = "nomads" ]; then
        URL="${NOMADS_URL}/gfs.${CURRENT_DATE}/${RUN}/atmos/${FILE}"
    else
        URL="${S3_URL}/gfs.${CURRENT_DATE}/${RUN}/atmos/${FILE}"
    fi

    TMPFILE="${DOWNLOAD_DIR}/${FILE}.tmp"
    DESTFILE="${FILES_DIR}/${FILE}"

    if [ -f "$DESTFILE" ]; then
        log "f${FH} уже существует."
        OK_COUNT=$((OK_COUNT + 1))
        continue
    fi

    CURRENT_TMPFILE="$TMPFILE"
    log "Скачивание $FILE"

    if [ -n "$REGION" ]; then
        # --- Режим региональной обрезки через фильтр-сервис NOMADS ---
        VAR_PARAMS=$(build_filter_params "var" "$VARS")
        LEV_PARAMS=$(build_filter_params "lev" "$LEVS")
        DIR_PARAM="%2Fgfs.${CURRENT_DATE}%2F${RUN}%2Fatmos"

        FILTER_QUERY="file=${FILE}${VAR_PARAMS}${LEV_PARAMS}&subregion=&leftlon=${LEFTLON}&rightlon=${RIGHTLON}&toplat=${TOPLAT}&bottomlat=${BOTTOMLAT}&dir=${DIR_PARAM}"
        FULL_URL="${FILTER_URL}?${FILTER_QUERY}"

        log "URL (region filter): $FULL_URL"

        if curl --fail --silent --show-error --location \
             --retry 3 --connect-timeout 30 --max-time 1800 \
             --output "$TMPFILE" "$FULL_URL"
        then
            SIZE=$(stat -c%s "$TMPFILE" 2>/dev/null || echo 0)
            log "Размер: ${SIZE} байт"
            if [ "$SIZE" -gt 0 ]; then
                mv "$TMPFILE" "$DESTFILE"
                log "f${FH} успешно скачан (регион)."
                OK_COUNT=$((OK_COUNT + 1))
            else
                log "Получен пустой файл (нет совпадений vars/levs/region?)."
                rm -f "$TMPFILE"
            fi
        else
            RC=$?
            log "f${FH} ошибка скачивания региона (curl код $RC)."
            rm -f "$TMPFILE"
        fi

        CURRENT_TMPFILE=""
        continue
    fi

    if [ -n "$POINT" ]; then
        # --- Режим "точка клиента": окно ±POINT_WINDOW°, фиксированный набор
        #     параметров Mushroom (HGT/TMP/UGRD/VGRD/PRMSL/RH), стандартные
        #     изобарические уровни 1000-350 мбар + приземные величины ---
        VAR_PARAMS=$(build_filter_params "var" "$POINT_VARS")
        LEV_PARAMS=$(build_filter_params "lev" "$POINT_LEVS")
        DIR_PARAM="%2Fgfs.${CURRENT_DATE}%2F${RUN}%2Fatmos"

        FILTER_QUERY="file=${FILE}${VAR_PARAMS}${LEV_PARAMS}&subregion=&leftlon=${LEFTLON}&rightlon=${RIGHTLON}&toplat=${TOPLAT}&bottomlat=${BOTTOMLAT}&dir=${DIR_PARAM}"
        FULL_URL="${FILTER_URL}?${FILTER_QUERY}"

        log "URL (point filter): $FULL_URL"

        if curl --fail --silent --show-error --location \
             --retry 3 --connect-timeout 30 --max-time 1800 \
             --output "$TMPFILE" "$FULL_URL"
        then
            SIZE=$(stat -c%s "$TMPFILE" 2>/dev/null || echo 0)
            log "Размер: ${SIZE} байт"
            if [ "$SIZE" -gt 0 ]; then
                mv "$TMPFILE" "$DESTFILE"
                log "f${FH} успешно скачан (точка $POINT)."
                OK_COUNT=$((OK_COUNT + 1))
            else
                log "Получен пустой файл (нет данных для этой точки/даты?)."
                rm -f "$TMPFILE"
            fi
        else
            RC=$?
            log "f${FH} ошибка скачивания точки (curl код $RC)."
            rm -f "$TMPFILE"
        fi

        CURRENT_TMPFILE=""
        continue
    fi

    log "URL: $URL"

    if [ $PARTIAL -eq 1 ]; then
        IDX_TMP="${DOWNLOAD_DIR}/${FILE}.idx.tmp"
        if ! curl --fail --silent --show-error --location \
             --connect-timeout 30 --output "$IDX_TMP" "${URL}.idx"; then
            log "f${FH} .idx недоступен."
            rm -f "$IDX_TMP"
            CURRENT_TMPFILE=""
            continue
        fi

        RANGE=$(awk -F: -v vre="$VARS_RE" -v lre="$LEVS_RE" '
            {
                start[NR]=$2
                line[NR]=$0
            }
            END{
                for(i=1;i<=NR;i++){
                    to = (i<NR)?start[i+1]-1:""
                    if (line[i] ~ lre && line[i] ~ vre) {
                        from=start[i]
                        if (to=="") to="9999999999"
                        if (lastto+1==from) { lastto=to }
                        else {
                            if (lastfrom!="") printf "%s%s-%s", (range!=""?",":""), lastfrom, lastto
                            if (lastfrom!="") range=range","
                            lastfrom=from; lastto=to
                        }
                    }
                }
                if (lastfrom!="") printf "%s%s-%s", (range!=""?",":""), lastfrom, lastto
            }
        ' "$IDX_TMP" 2>/dev/null)
        rm -f "$IDX_TMP"

        if [ -z "$RANGE" ]; then
            log "f${FH} нет совпадений vars/levs, пропуск."
            CURRENT_TMPFILE=""
            continue
        fi

        if curl --fail --silent --show-error --location \
             --retry 3 --connect-timeout 30 --max-time 1800 \
             -r "$RANGE" --output "$TMPFILE" "$URL"
        then
            mv "$TMPFILE" "$DESTFILE"
            log "f${FH} успешно скачан (частично)."
            OK_COUNT=$((OK_COUNT + 1))
        else
            RC=$?
            log "f${FH} ошибка частичного скачивания."
            rm -f "$TMPFILE"
            if [ "$RC" -eq 23 ]; then
                log "curl 23 (write fail), стоп."
                exit 1
            fi
        fi
    else
        if curl --fail --silent --show-error --location \
             --retry 3 --connect-timeout 30 --max-time 1800 \
             --output "$TMPFILE" "$URL"
        then
            SIZE=$(stat -c%s "$TMPFILE" 2>/dev/null || echo 0)
            log "Размер: ${SIZE} байт"
            if [ "$SIZE" -gt 1000000 ]; then
                mv "$TMPFILE" "$DESTFILE"
                log "f${FH} успешно скачан."
                OK_COUNT=$((OK_COUNT + 1))
            else
                log "Получен слишком маленький файл."
                rm -f "$TMPFILE"
            fi
        else
            RC=$?
            log "f${FH} отсутствует."
            rm -f "$TMPFILE"
            if [ "$RC" -eq 23 ]; then
                log "curl 23 (write fail), стоп."
                exit 1
            fi
        fi
    fi

    CURRENT_TMPFILE=""
done

if [ "$OK_COUNT" -eq 0 ]; then
    log "Ни один файл не получен (0 из $((END - START + 1))) — завершаем с ошибкой."
    exit 1
fi

log "Готово: получено файлов $OK_COUNT из $((END - START + 1))."
exit 0
