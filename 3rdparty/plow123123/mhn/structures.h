//------------------------------------------------------------
//  MHN SCOU
//------------------------------------------------------------
// structures.h
// Центральное КБ Аппаратостроения, Тула
//
// Модуль содержит структуры данных, используемые в модулях
// восстановления профиля ветра.
//------------------------------------------------------------

#ifndef _STRUCTURES_H
#define	_STRUCTURES_H

#include "mbulletin.h"

//#pragma pack(push, 1)
//#include "types.h"

typedef struct {
 	float z;                     //��������� ������������
    float m;                     //�������������� ������������
    float height;                //������
} ProPoint;

typedef struct {
	unsigned char year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
} Date;

struct MeteoMidDate {
	int day;
	int hour;
	int tensOfMins;
};

typedef struct
{
	float z;  
	float m;                     
	float height;                
	short curr;
	
	void setPoint(float z, float m, float height) {
		this->z = z;
		this->m = m;
		this->height = height;
	}
	
} ComponentProfilePoint;
typedef ComponentProfilePoint *pComponentProfilePoint;

//#pragma pack(pop)

#endif	/* _STRUCTURES_H */

