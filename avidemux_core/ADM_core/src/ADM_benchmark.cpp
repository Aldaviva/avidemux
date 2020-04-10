/***************************************************************************
                          TLK_clock.cpp  -  description
                             -------------------
	Handle a simple clock/timer class

	The API used returns the amound of days/hour/minute/seconds since
		1st Jan 1970
	As we only use part of the information (seconds and useconds) it
	may wrap around anytime

	The first time a clock is created, we store the "now date" and
	do the computation with getTime-"now date"
	It will wrap after 25 days, so it can be considerate safe


    begin                : Fri Apr 20 2003
    copyright            : (C) 2003 by mean
    email                : fixounet@free.fr
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ADM_default.h"
#include "ADM_clock.h"
/**
    \fn
*/
ADMBenchmark::ADMBenchmark(void)
{
    bMin=1000000;
    bMax=0;
    bCumul=0;
    nbRound=0;
}
/**
    \fn
*/
void ADMBenchmark::start(void)
{
    clk.reset();
}
/**
    \fn
*/
void ADMBenchmark::end(void)
{
    uint64_t r=clk.getElapsedUS();
    if(r<bMin) bMin=r;
    if(r>bMax) bMax=r;
    bCumul+=r;
    nbRound++;
}
/**
    \fn
*/
void ADMBenchmark::printResult(void)
{
    float f=(float)bCumul;
    f/=nbRound;
    ADM_info("Average Time :%f us\n",f);
    ADM_info("Min Time     : %d us\n",bMin);
    ADM_info("Max Time     : %d us\n",bMax);
}
/**
    \fn getResult
    \brief get min,max & average benchmark result
*/
void ADMBenchmark::getResult(float &avg, int &bmin,int &bmax)
{
    avg=(float)bCumul;
    if(nbRound)
        avg/=nbRound;
    else    
        avg=0;
    avg=avg/1000;
    bmin=(float)(bMin/1000);
    bmax=bMax/1000;
}
void ADMBenchmark::getResultUs(float &avg, int &bmin,int &bmax)
{
    avg=(float)bCumul;
    if(nbRound)
        avg/=nbRound;
    else    
        avg=0;
    bmin=(float)bMin;
    bmax=(float)bMax;
}

// EOF
