#ifndef TRANSFERTYPES_H
#define TRANSFERTYPES_H

////////////////////////////////////////////////////////////////////////////////
#include "ccomand.h"
const unsigned char COMMAND_1V16_LONGITUDE = 1;
const int LONGITUDE_R = 0;
const int LONGITUDE_A = 7;

const unsigned char COMMAND_1V16_LATITUDE = 2;
const int LATITUDE_R = 0;
const int LATITUDE_A = 7;

const unsigned char COMMAND_1V16_POSITION = 3;
const int POSITION_R = 0;
const int POSITION_A = 8;

const unsigned char COMMAND_1V16_SPEED = 4;
const int SPEED_R = 0;
const int SPEED_A = 120;

const unsigned char COMMAND_1V16_DIRECT = 5;
const int DIRECT_R = 0;
const int DIRECT_A = 120;

const unsigned char COMMAND_1V16_SPEED_AVERAGE = 6;
const int SPEED_AVERAGE_R = 0;
const int SPEED_AVERAGE_A = 64;

const unsigned char COMMAND_1V16_DIRECT_AVERAGE = 7;
const int DIRECT_AVERAGE_R = 0;
const int DIRECT_AVERAGE_A = 64;

const unsigned char COMMAND_1V16_METEO = 8;
const int METEO_R = 0;
const int METEO_A = 251;

const unsigned char COMMAND_1V16_METEO_ACCURATE  = 9;
const int METEO_ACCURATE_R = 0;
const int METEO_ACCURATE_A = 251;

const unsigned char COMMAND_1V16_METEO_FORMALIZE  = 10;
const int METEO_FORMALIZE_R = 0;
const int METEO_FORMALIZE_A = 90;

const unsigned char COMMAND_1V16_WEATHER_PARAMETERS_1B65 = 11;
const int PARAMETERS_1B65_R = 0;
const int PARAMETERS_1B65_A = 20;

const unsigned char COMMAND_1V16_ACTUAL_WIND = 12;
const int ACTUAL_WIND_R = 0;
const int ACTUAL_WIND_A_AMS = 1600;
const int ACTUAL_WIND_A_AMS_T = 5120;

const unsigned char COMMAND_1V16_STATION_STATE = 13;
const int STATION_STATE_R = 0;
const int STATION_STATE_A = 22;

const unsigned char COMMAND_1V16_TIME = 14;
const int TIME_R = 0;
//const int TIME_A = 5;
const int TIME_A = 16;

const unsigned char COMMAND_1V16_WEATHER_FROM_1V16 = 15;
const int WEATHER_R = 0;
const int WEATHER_A = 20;

const unsigned char COMMAND_1V16_SONDING_START = 0x11;
const int SONDING_START_R = 1;
const int SONDING_START_A = 1;

const unsigned char COMMAND_1V16_REALWIND_HEIGHT = 0x12;
const int REALWIND_HEIGHT_R = 0;
const int REALWIND_HEIGHT_A = 480;

const unsigned char COMMAND_1V16_AVERAGEWIND_HEIGHT = 0x13;
const int AVERAGEWIND_HEIGHT_R = 0;
const int AVERAGEWIND_HEIGHT_A = 480;

const unsigned char COMMAND_1V16_METEO11_UPDATED = 0x14;
const int METEO11_UPDATED_R = 0;
const int METEO11_UPDATED_A = 321;

const unsigned char COMMAND_1V16_FORM_METEO11_UPDATED = 0x15;
const int FORM_METEO11_UPDATED_R = 0;
const int FORM_METEO11_UPDATED_A = 160;

////////////////////////////////////////////////////////////////////////////////

//const unsigned char COMMAND_BUOI_TEST = 'T';
const unsigned char COMMAND_BUOI_TEST = 0xA0;
const int TEST_R = 16;
const int TEST_A = 16;

//const unsigned char COMMAND_BUOI_MODE  = 'M';
const unsigned char COMMAND_BUOI_MODE  = 0xA1;
const int MODE_R = 2;
const int MODE_A = 0;

//const unsigned char COMMAND_BUOI_COORDINATES = 'C';
const unsigned char COMMAND_BUOI_COORDINATES = 0xA2;
const int COORDINATES_R = 16;
const int COORDINATES_A = 0;

//const unsigned char COMMAND_BUOI_START = 'S';
const unsigned char COMMAND_BUOI_START = 0xA3;
const int START_R = 0;
const int START_A = 1;

//const unsigned char COMMAND_BUOI_DATA = 'D';
const unsigned char COMMAND_BUOI_DATA = 0xA4;
const int DATA_R = 1;
const int DATA_A = 8;

const unsigned char COMMAND_BUOI_CLISTRON_TRAINING = 0xA5;
const int CLISTRON_TRAINING_R = 1;
const int CLISTRON_TRAINING_A = 2;

// ???????? ?????? ??? ????????? ??__????__?__??
// ?????_??????_???????-?????-??-???????_??????
const unsigned char COMMAND_BUOI_AVERAGE_WIND_1 = 0xA6; //I
const int AVERAGE_WIND_1_R = 224;
const int AVERAGE_WIND_1_A = 0;

//const unsigned char COMMAND_BUOI_FUNC_CONTROL = 'F';
const unsigned char COMMAND_BUOI_FUNC_CONTROL = 0xA7;
const int FUNC_CONTROL_R = 0;
const int FUNC_CONTROL_A = 12;

//const unsigned char COMMAND_BUOI_BOTTOM_WIND = 'V';
const unsigned char COMMAND_BUOI_BOTTOM_WIND = 0xA8;
const int BOTTOM_WIND_R = 8;
const int BOTTOM_WIND_A = 0;

// ?????? ? ??????? ????? ??__??__?__????
// ?????? - ???????????_????????....???????????_???????? = 16?2?4 (float)
const unsigned char COMMAND_BUOI_AVERAGE_WIND_2 = 0xA9; //R
const int AVERAGE_WIND_2_R = 0;
const int AVERAGE_WIND_2_A = 128;

//const unsigned char COMMAND_BUOI_REAL_WIND = 'O';
// ?????? ? ???????? ????? ??__??__?__????
// ?????? - ???????????_????????....???????????_???????? = 30?2?4 (float)
const unsigned char COMMAND_BUOI_REAL_WIND = 0xAA;
const int REAL_WIND_R = 0;
const int REAL_WIND_A = 240;

//const unsigned char COMMAND_TEST_MESSAGE = 19;
//const int TEST_MESSAGE_R = 0;
//const int TEST_MESSAGE_A = 500;

// ???????? ???????????? ????????? ?????
const unsigned char COMMAND_BUOI_CHECK_ANGLE = 0xAB;
const int CHECK_ANGLE_R = 1;
const int CHECK_ANGLE_A = 4;

const unsigned char COMMAND_BUOI_ACTUAL_WIND = 0xAC;
//const int ACTUAL_WIND_R = 0;
//const int ACTUAL_WIND_A = 1600;

const unsigned char COMMAND_BUOI_ANTENNA = 0xAD;
const int ANTENNA_R = 1;
const int ANTENNA_A = 1;

const unsigned char COMMAND_BUOI_TIME = 0xAE; //I
const int BUOI_TIME_R = 12;
const int BUOI_TIME_A = 0;

struct CommandsForBUOIFrom1V16 {
    CommandAttr longitude;
    CommandAttr latitude;
    CommandAttr position;
    CommandAttr speed;
    CommandAttr direction;
    CommandAttr speedAverage;
    CommandAttr directionAverage;
    CommandAttr meteo;
    CommandAttr meteoAccurate;
    CommandAttr meteoFormalize;
    CommandAttr parameters1b65;
    CommandAttr actualWind;
    CommandAttr dateTime;
    CommandAttr weatherFrom1v16;
    CommandAttr stationState;
    CommandAttr sondingStart;
    CommandAttr averageHeight;
    CommandAttr realHeight;
    CommandAttr meteo11Upd;
    CommandAttr formMeteoUpd;


    CommandsForBUOIFrom1V16(bool isBUOI) {
        if (isBUOI) {
            CommandAttr::init(&longitude, COMMAND_1V16_LONGITUDE, Message::ANSWER, LONGITUDE_R, LONGITUDE_A);
            CommandAttr::init(&latitude, COMMAND_1V16_LATITUDE, Message::ANSWER, LATITUDE_R, LATITUDE_A);
            CommandAttr::init(&position, COMMAND_1V16_POSITION, Message::ANSWER, POSITION_R, POSITION_A);
            CommandAttr::init(&speed, COMMAND_1V16_SPEED, Message::REQUEST, SPEED_R, SPEED_A);
            CommandAttr::init(&direction, COMMAND_1V16_DIRECT, Message::REQUEST, DIRECT_R, DIRECT_A);
            CommandAttr::init(&speedAverage, COMMAND_1V16_SPEED_AVERAGE, Message::REQUEST, SPEED_AVERAGE_R, SPEED_AVERAGE_A);
            CommandAttr::init(&directionAverage, COMMAND_1V16_DIRECT_AVERAGE, Message::REQUEST, DIRECT_AVERAGE_R, DIRECT_AVERAGE_A);
            CommandAttr::init(&meteo, COMMAND_1V16_METEO, Message::ANSWER, METEO_R, METEO_A);
            CommandAttr::init(&meteoAccurate, COMMAND_1V16_METEO_ACCURATE, Message::REQUEST, METEO_ACCURATE_R, METEO_ACCURATE_A);
            CommandAttr::init(&meteoFormalize, COMMAND_1V16_METEO_FORMALIZE, Message::REQUEST, METEO_FORMALIZE_R, METEO_FORMALIZE_A);
            CommandAttr::init(&parameters1b65, COMMAND_1V16_WEATHER_PARAMETERS_1B65, Message::REQUEST, PARAMETERS_1B65_R, PARAMETERS_1B65_A);
            CommandAttr::init(&actualWind, COMMAND_1V16_ACTUAL_WIND, Message::REQUEST, ACTUAL_WIND_R, ACTUAL_WIND_A_AMS_T);
            CommandAttr::init(&dateTime, COMMAND_1V16_TIME, Message::ANSWER, TIME_R, TIME_A);
            CommandAttr::init(&weatherFrom1v16, COMMAND_1V16_WEATHER_FROM_1V16, Message::ANSWER, WEATHER_R, WEATHER_A);
            CommandAttr::init(&stationState, COMMAND_1V16_STATION_STATE, Message::REQUEST, STATION_STATE_R, STATION_STATE_A);
            CommandAttr::init(&sondingStart, COMMAND_1V16_SONDING_START, Message::REQUEST, SONDING_START_R, SONDING_START_A);
            CommandAttr::init(&realHeight, COMMAND_1V16_REALWIND_HEIGHT, Message::REQUEST, REALWIND_HEIGHT_R, REALWIND_HEIGHT_A);
            CommandAttr::init(&averageHeight, COMMAND_1V16_AVERAGEWIND_HEIGHT, Message::REQUEST, AVERAGEWIND_HEIGHT_R, AVERAGEWIND_HEIGHT_A);
            CommandAttr::init(&meteo11Upd, COMMAND_1V16_METEO11_UPDATED, Message::REQUEST, METEO11_UPDATED_R, METEO11_UPDATED_A);
            CommandAttr::init(&formMeteoUpd, COMMAND_1V16_FORM_METEO11_UPDATED, Message::REQUEST, FORM_METEO11_UPDATED_R, FORM_METEO11_UPDATED_A);
        }
        else {
            CommandAttr::init(&longitude, COMMAND_1V16_LONGITUDE, Message::REQUEST, LONGITUDE_R, LONGITUDE_A);
            CommandAttr::init(&latitude, COMMAND_1V16_LATITUDE, Message::REQUEST, LATITUDE_R, LATITUDE_A);
            CommandAttr::init(&position, COMMAND_1V16_POSITION, Message::REQUEST, POSITION_R, POSITION_A);
            CommandAttr::init(&speed, COMMAND_1V16_SPEED, Message::ANSWER, SPEED_R, SPEED_A);
            CommandAttr::init(&direction, COMMAND_1V16_DIRECT, Message::ANSWER, DIRECT_R, DIRECT_A);
            CommandAttr::init(&speedAverage, COMMAND_1V16_SPEED_AVERAGE, Message::ANSWER, SPEED_AVERAGE_R, SPEED_AVERAGE_A);
            CommandAttr::init(&directionAverage, COMMAND_1V16_DIRECT_AVERAGE, Message::ANSWER, DIRECT_AVERAGE_R, DIRECT_AVERAGE_A);
            CommandAttr::init(&meteo, COMMAND_1V16_METEO, Message::REQUEST, METEO_R, METEO_A);
            CommandAttr::init(&meteoAccurate, COMMAND_1V16_METEO_ACCURATE, Message::ANSWER, METEO_ACCURATE_R, METEO_ACCURATE_A);
            CommandAttr::init(&meteoFormalize, COMMAND_1V16_METEO_FORMALIZE, Message::ANSWER, METEO_FORMALIZE_R, METEO_FORMALIZE_A);
            CommandAttr::init(&parameters1b65, COMMAND_1V16_WEATHER_PARAMETERS_1B65, Message::ANSWER, PARAMETERS_1B65_R, PARAMETERS_1B65_A);
            CommandAttr::init(&actualWind, COMMAND_1V16_ACTUAL_WIND, Message::ANSWER, ACTUAL_WIND_R, ACTUAL_WIND_A_AMS_T);
            CommandAttr::init(&dateTime, COMMAND_1V16_TIME, Message::REQUEST, TIME_R, TIME_A);
            CommandAttr::init(&weatherFrom1v16, COMMAND_1V16_WEATHER_FROM_1V16, Message::REQUEST, WEATHER_R, WEATHER_A);
            CommandAttr::init(&stationState, COMMAND_1V16_STATION_STATE, Message::ANSWER, STATION_STATE_R, STATION_STATE_A);
            CommandAttr::init(&sondingStart, COMMAND_1V16_SONDING_START, Message::ANSWER, SONDING_START_R, SONDING_START_A);
            CommandAttr::init(&realHeight, COMMAND_1V16_REALWIND_HEIGHT, Message::ANSWER, REALWIND_HEIGHT_R, REALWIND_HEIGHT_A);
            CommandAttr::init(&averageHeight, COMMAND_1V16_AVERAGEWIND_HEIGHT, Message::ANSWER, AVERAGEWIND_HEIGHT_R, AVERAGEWIND_HEIGHT_A);
            CommandAttr::init(&meteo11Upd, COMMAND_1V16_METEO11_UPDATED, Message::ANSWER, METEO11_UPDATED_R, METEO11_UPDATED_A);
            CommandAttr::init(&formMeteoUpd, COMMAND_1V16_FORM_METEO11_UPDATED, Message::ANSWER, FORM_METEO11_UPDATED_R, FORM_METEO11_UPDATED_A);
        }

    }
};

struct CommandsForBUOIFromBV {
    CommandAttr test;
    CommandAttr mode;
    CommandAttr coordinates;
    CommandAttr start;
    CommandAttr data;
    CommandAttr funcControl;
    CommandAttr bottomWind;
    CommandAttr inputData;
    CommandAttr averageWind;
    CommandAttr realWind;
    CommandAttr training;
    CommandAttr checkAngle;
    CommandAttr actualWind;
    CommandAttr antenna;
    CommandAttr time;

    CommandsForBUOIFromBV() {
        CommandAttr::init(&test, COMMAND_BUOI_TEST, Message::ANSWER, TEST_R, TEST_A);
        CommandAttr::init(&mode, COMMAND_BUOI_MODE, Message::ANSWER, MODE_R, MODE_A);
        CommandAttr::init(&coordinates, COMMAND_BUOI_COORDINATES, Message::ANSWER, COORDINATES_R, COORDINATES_A);
        CommandAttr::init(&start, COMMAND_BUOI_START, Message::ANSWER, START_R, START_A);
        CommandAttr::init(&data, COMMAND_BUOI_DATA, Message::ANSWER, DATA_R, DATA_A);
        CommandAttr::init(&funcControl, COMMAND_BUOI_FUNC_CONTROL, Message::ANSWER, FUNC_CONTROL_R, FUNC_CONTROL_A);
        CommandAttr::init(&bottomWind, COMMAND_BUOI_BOTTOM_WIND, Message::ANSWER, BOTTOM_WIND_R, BOTTOM_WIND_A);
        CommandAttr::init(&inputData, COMMAND_BUOI_AVERAGE_WIND_1, Message::ANSWER, AVERAGE_WIND_1_R, AVERAGE_WIND_1_A);
        CommandAttr::init(&averageWind, COMMAND_BUOI_AVERAGE_WIND_2, Message::ANSWER, AVERAGE_WIND_2_R, AVERAGE_WIND_2_A);
        CommandAttr::init(&realWind, COMMAND_BUOI_REAL_WIND, Message::ANSWER, REAL_WIND_R, REAL_WIND_A);
        CommandAttr::init(&training, COMMAND_BUOI_CLISTRON_TRAINING, Message::ANSWER, CLISTRON_TRAINING_R, CLISTRON_TRAINING_A);
        CommandAttr::init(&checkAngle, COMMAND_BUOI_CHECK_ANGLE, Message::ANSWER, CHECK_ANGLE_R, CHECK_ANGLE_A);
        CommandAttr::init(&actualWind, COMMAND_BUOI_ACTUAL_WIND, Message::ANSWER, ACTUAL_WIND_R, ACTUAL_WIND_A_AMS_T);
        CommandAttr::init(&antenna, COMMAND_BUOI_ANTENNA, Message::ANSWER, ANTENNA_R, ANTENNA_A);
        CommandAttr::init(&time, COMMAND_BUOI_TIME, Message::ANSWER, BUOI_TIME_R, BUOI_TIME_A);
    }
};

struct CommandsForBVFromBUOI {
    CommandAttr test;
    CommandAttr mode;
    CommandAttr coordinates;
    CommandAttr start;
    CommandAttr data;
    CommandAttr funcControl;
    CommandAttr bottomWind;
    CommandAttr inputData;
    CommandAttr averageWind;
    CommandAttr realWind;
    CommandAttr training;
    CommandAttr checkAngle;
    CommandAttr actualWind;
    CommandAttr antenna;
    CommandAttr time;

    CommandsForBVFromBUOI() {
        CommandAttr::init(&test, COMMAND_BUOI_TEST, Message::REQUEST, TEST_R, TEST_A);
        CommandAttr::init(&mode, COMMAND_BUOI_MODE, Message::REQUEST, MODE_R, MODE_A);
        CommandAttr::init(&coordinates, COMMAND_BUOI_COORDINATES, Message::REQUEST, COORDINATES_R, COORDINATES_A);
        CommandAttr::init(&start, COMMAND_BUOI_START, Message::REQUEST, START_R, START_A);
        CommandAttr::init(&data, COMMAND_BUOI_DATA, Message::REQUEST, DATA_R, DATA_A);
        CommandAttr::init(&funcControl, COMMAND_BUOI_FUNC_CONTROL, Message::REQUEST, FUNC_CONTROL_R, FUNC_CONTROL_A);
        CommandAttr::init(&bottomWind, COMMAND_BUOI_BOTTOM_WIND, Message::REQUEST, BOTTOM_WIND_R, BOTTOM_WIND_A);
        CommandAttr::init(&inputData, COMMAND_BUOI_AVERAGE_WIND_1, Message::REQUEST, AVERAGE_WIND_1_R, AVERAGE_WIND_1_A);
        CommandAttr::init(&averageWind, COMMAND_BUOI_AVERAGE_WIND_2, Message::REQUEST, AVERAGE_WIND_2_R, AVERAGE_WIND_2_A);
        CommandAttr::init(&realWind, COMMAND_BUOI_REAL_WIND, Message::REQUEST, REAL_WIND_R, REAL_WIND_A);
        CommandAttr::init(&training, COMMAND_BUOI_CLISTRON_TRAINING, Message::REQUEST, CLISTRON_TRAINING_R, CLISTRON_TRAINING_A);
        CommandAttr::init(&checkAngle, COMMAND_BUOI_CHECK_ANGLE, Message::REQUEST, CHECK_ANGLE_R, CHECK_ANGLE_A);
        CommandAttr::init(&actualWind, COMMAND_BUOI_ACTUAL_WIND, Message::REQUEST, ACTUAL_WIND_R, ACTUAL_WIND_A_AMS_T);
        CommandAttr::init(&antenna, COMMAND_BUOI_ANTENNA, Message::REQUEST, ANTENNA_R, ANTENNA_A);
        CommandAttr::init(&time, COMMAND_BUOI_TIME, Message::REQUEST, BUOI_TIME_R, BUOI_TIME_A);
    }
};

struct CommandsFor1B67agentFromBUOI {
    CommandAttr longitude;
    CommandAttr latitude;
    CommandAttr position;
    CommandAttr speed;
    CommandAttr direction;
    CommandAttr speedAverage;
    CommandAttr directionAverage;
    CommandAttr meteo;
    CommandAttr meteoAccurate;
    CommandAttr parameters1b65;
    //
    //CommandAttr testMessage;

    CommandsFor1B67agentFromBUOI() {
        CommandAttr::init(&longitude, COMMAND_1V16_LONGITUDE, Message::REQUEST, LONGITUDE_R, LONGITUDE_A);
        CommandAttr::init(&latitude, COMMAND_1V16_LATITUDE, Message::REQUEST, LATITUDE_R, LATITUDE_A);
        CommandAttr::init(&position, COMMAND_1V16_POSITION, Message::REQUEST, POSITION_R, POSITION_A);
        CommandAttr::init(&speed, COMMAND_1V16_SPEED, Message::ANSWER, SPEED_R, SPEED_A);
        CommandAttr::init(&direction, COMMAND_1V16_DIRECT, Message::ANSWER, DIRECT_R, DIRECT_A);
        CommandAttr::init(&speedAverage, COMMAND_1V16_SPEED_AVERAGE, Message::ANSWER, SPEED_AVERAGE_R, SPEED_AVERAGE_A);
        CommandAttr::init(&directionAverage, COMMAND_1V16_DIRECT_AVERAGE, Message::ANSWER, DIRECT_AVERAGE_R, DIRECT_AVERAGE_A);
        CommandAttr::init(&meteo, COMMAND_1V16_METEO, Message::REQUEST, METEO_R, METEO_A);
        CommandAttr::init(&meteoAccurate, COMMAND_1V16_METEO_ACCURATE, Message::ANSWER, METEO_ACCURATE_R, METEO_ACCURATE_A);
        CommandAttr::init(&parameters1b65, COMMAND_1V16_WEATHER_PARAMETERS_1B65, Message::ANSWER, PARAMETERS_1B65_R, PARAMETERS_1B65_A);
        //
        //CommandAttr::init(&testMessage, COMMAND_TEST_MESSAGE, Message::ANSWER, TEST_MESSAGE_R, TEST_MESSAGE_A);
    }
};

/*
#define COM1	"/dev/ttyS0"
#define COM2	"/dev/ttyS1"
#define COM3	"/dev/ttyS2"
#define COM4	"/dev/ttyS3"
*/
const unsigned char bodyTestMessage[] = {
    0xFF, 0xFF, 0xFF, 0xFF,
    0xEE, 0xEE, 0xEE, 0xEE,
    0xDD, 0xDD, 0xDD, 0xDD,
    0xCC, 0xCC, 0xCC, 0x00
};

#endif

