// Author: Dan Pankani - Geosyntec Consultants Inc. Portland OR. Dpankani@geosyntec.com 
// Date: 10/11/2011
// Version: 0.1.0

// Update: A.C. Rowney - ACR, llc
// Mainly split control file into two (so one can be generated by the Framework), 
// added output of metadata file (needed for framework processing), and updated
// output of header file listing parameters
// Date: 20120109

// SWMMDrivers.cpp : Defines the entry point for the console application. This application extracts
// timeseries from a SWMM5 binary output file. This code was developed and test with 
// SWMM 5.0 Build 5.0.022
//

#include "stdafx.h"
#include <stdlib.h>
#include "errno.h"
#include <ctype.h>
#include <string>
#include <Windows.h>

struct SWMMMetaData {
	int returnresult;
    int fyear;
    int fmonth;
	int fday;
	double fhour;
	int tyear;
	int tmonth;
	int tday;
	double thour;
	int numread;
	double timeStep;
};

SWMMMetaData output_open(FILE* fcontrol, FILE * fout, FILE* ftimeSeries, char** inputs, int* targetPollutantSWMMOrder);

SWMMMetaData metaFileData;

int ErrorCode;
int Nobjects[MAX_OBJ_TYPES];			// Number of each object type
TNode*     Node;						// Array of nodes // from globals.h
TPollut*   Pollut;						// Array of pollutants
long Nperiods;							// Number of reporting periodstypedef double DateTime;  //Taken from datetime.h
char* targetNodeID;
char* targetPollutants[7];
char* targetFRWPollutants[7];
char* targetSWMPollutants[7];
float targetPollutantFactors[7];
float flowConversionFactor = 0.0;
int targetPollutantSWMMOrder[7];
int targetPollutantFRWOrder[7];
int totalNumOfFRWPollutants = 0;
int totalNumOfMatchedFRWPollutants = 0;
int targetNodeIndex = 0;
REAL8 reportStartDate = 0.0;
INT4 reportTimeInterv = 0;
DateTime StartDateTime;


int main()
//int main(int argc, char* argv[])
//  Input:   one commandline argument representing the path to the control file
//  Output:  creates file with timeseries of outputs for the rest of the framework
//  Purpose: This function is the entry point for the component of the framework that extracts timeseries from SWMM5 output.
//
{
    FILE* reportFile;
    FILE* binaryFile;
	FILE* timeSeriesOutFile;
	FILE* metaFile;
	FILE* controlFile;

	char* binaryFilePath; 
	char* timeSeriesOutFilePath;
	char* inputs[20];
	char* returnvalues[20];
	char* tempToken;
    char* controlFilePath = ".\\swmmconvertstring.txt";
	char* metaFilePath = ".\\swmmconverterdefaults.mta";

	char line[40];
	double thisTime;

	//	if ( argc != 2 ){ 
//        printf( "\nUsage: SwmmDrivers.exe \"MetafilePath\"");
//		return 1;
//	}
//    else 
//        metaFilePath = argv[1];

	//1.) open, access and read control file for location of input series -------------------
	printf("\nSWMM Converter Version 1.0.20130513");
	printf("\nOpening control file: %s ",controlFilePath);
	controlFile = openAnyFile(controlFilePath, 0);

	//read the control file
	input_readData2(controlFile, inputs);
	
	binaryFilePath = inputs[0];
    //binaryFilePath ="C:\\SWMMExtract04162012\\SWMMModel\\RockCreekDemo.out"; //example
	printf("\nSWMM output binary file to read from: %s ",binaryFilePath);

    targetNodeID = inputs[1];
	//targetNodeID = "Outfall"; //example
	printf("\nTarget SWMM Node: %s ",targetNodeID);

	//2.) create the output scratch file ----------------------------------------
	timeSeriesOutFilePath=".\\SCRATCH";
	printf("\nGenerated framework scratch file output filepath: %s ",timeSeriesOutFilePath);

	//3.) open and read the converer defaults metafile ----------------------------------------------
	metaFile = openAnyFile(metaFilePath, 0);
	printf("\nOpening Metafile: %s ",metaFilePath);
	
	//read the metadata file
	input_readData2(metaFile, inputs);

	flowConversionFactor = atof(inputs[0]);
	printf("\nFlow conversion factor: %f ",flowConversionFactor);

	totalNumOfFRWPollutants = atoi(inputs[1]);
	printf("\nTotal number of pollutants: %d ",totalNumOfFRWPollutants);

	//read pollutant info
	for(int i = 0; i<totalNumOfFRWPollutants;i++){

		targetPollutants[i] = inputs[i+2];
		tempToken = strtok (inputs[i+1],"=");
		targetFRWPollutants[i] = strtok (inputs[i+2],"=");

		targetFRWPollutants[i] = trimwhitespace(targetFRWPollutants[i]);

		targetSWMPollutants[i] = strtok (NULL, "/");
		targetSWMPollutants[i] = trimwhitespace(targetSWMPollutants[i]);

		targetPollutantFactors[i] = atof(strtok (NULL, "/"));
		printf("\nPollutant %d: framework name: %s SWMM name: %s conversion factor: %f",i,targetFRWPollutants[i],targetSWMPollutants[i],targetPollutantFactors[i]);
	}

	//4.) open binary file with input series, and text file with output series
	binaryFile = openAnyFile(binaryFilePath, 0);
	timeSeriesOutFile = openAnyFile(timeSeriesOutFilePath, 1);

	//5.) do conversion of series (read series from SWMM, write it to scratch file)
	metaFileData = output_open(controlFile, binaryFile, timeSeriesOutFile, inputs, targetPollutantSWMMOrder);

	//6.) create the return metadata file
	if(metaFileData.returnresult == 0){ 
		controlFile = openAnyFile(controlFilePath, 1);
		fputs(binaryFilePath,controlFile);fputs("\n",controlFile);
		fputs(targetNodeID,controlFile);fputs("\n",controlFile);

		metaFileData.timeStep/=3600.0;
		
		sprintf(line, "%5.3f", metaFileData.timeStep);fputs(line, controlFile);fputs("\n",controlFile);
		fputs("2",controlFile);fputs("\n",controlFile);
		fputs("Converted from SWMM 5",controlFile);fputs("\n",controlFile);
		sprintf(line, "%d", metaFileData.fyear);fputs(line, controlFile);fputs("\n",controlFile);
		sprintf(line, "%d", metaFileData.fmonth);fputs(line, controlFile);fputs("\n",controlFile);
		sprintf(line, "%d", metaFileData.fday);fputs(line, controlFile);fputs("\n",controlFile);
		
		metaFileData.fhour/=3600.0;
		sprintf(line,  "%5.3f", metaFileData.fhour);fputs(line, controlFile);fputs("\n",controlFile);
		sprintf(line, "%d", metaFileData.tyear);fputs(line, controlFile);fputs("\n",controlFile);
		sprintf(line, "%d", metaFileData.tmonth);fputs(line, controlFile);fputs("\n",controlFile);
		sprintf(line, "%d", metaFileData.tday);fputs(line, controlFile);fputs("\n",controlFile);
		
		metaFileData.thour/=3600.0;
		sprintf(line,  "%5.3f", metaFileData.thour);fputs(line, controlFile);fputs("\n",controlFile);
		sprintf(line, "%d", metaFileData.numread);fputs(line, controlFile);fputs("\n",controlFile);
		fclose(controlFile);
	   return 0;   //888
	} else {
	   return 1;
	}
}



void report_writeLine(char *line, FILE* frpt)
//  Input:   line = line of text
//  Output:  none
//  Purpose: writes line of text to report file.
//
{
    if (frpt) fprintf(frpt, "\n  %s", line);
}


//=============================================================================

void output_readDateTime(int period, DateTime* days, int bytesPerPeriod, FILE* fout, int outputStartPos)
//  Input:   period = index of reporting time period
//  Output:  days = date/time value
//  Purpose: retrieves the date/time for a specific reporting period
//           from the binary output file.
//
{
    INT4 bytePos = outputStartPos + (period-1)*bytesPerPeriod;
    fseek(fout, bytePos, SEEK_SET);
    *days = NO_DATE;
    fread(days, sizeof(REAL8), 1, fout);
}

void datetime_dateToStr2(DateTime date, char* outResultStr, int format)
//  Input:   date = encoded date/time value
//  Output:  s = formatted date string
//  Purpose: represents DateTime date value as a formatted string.

{
    int  y, m, d;
    char dateStr[DATE_STR_SIZE];
    datetime_decodeDate(date, &y, &m, &d);
    switch (format)
    {
      case Y_M_D:
        sprintf(dateStr, "%4d,%02d,%02d", y, m, d);
        break;

      case M_D_Y:
        sprintf(dateStr, "%3s-%02d-%4d", m, d, y);
        break;

      default:
        sprintf(dateStr, "%02d,%02d,%4d", d, m, y);
    }
    strcpy(outResultStr, dateStr);
}

//=============================================================================


void datetime_timeToStr2(DateTime time, char* s)

//  Input:   time = decimal fraction of a day
//  Output:  s = time in hr:min:sec format
//  Purpose: represents DateTime time value as a formatted string.

{
    int  hr, min, sec;
    char timeStr[TIME_STR_SIZE];
	float timeDecimal = 0.0;
    datetime_decodeTime(time, &hr, &min, &sec);
    //sprintf(timeStr, "%02d,%10f", hr, (min/60 +  sec/3600));
	//timeDecimal = min/60.0 + sec/3600.0;
	timeDecimal = hr + min/60.0 + sec/3600.0;
	sprintf(timeStr, "%8.5f", timeDecimal);
    strcpy(s, timeStr);
}

void datetime_decodeTime(DateTime time, int* h, int* m, int* s)

//  Input:   time = decimal fraction of a day
//  Output:  h = hour of day (0-24)
//           m = minute of hour (0-60)
//           s = second of minute (0-60)
//  Purpose: decodes DateTime value to hour:minute:second.

{
    int secs;
    int mins;
    secs = (int)(floor((time - floor(time))*SecsPerDay + 0.5));
    divMod(secs, 60, &mins, s);
    divMod(mins, 60, h, m);
}

void datetime_decodeDate(DateTime date, int* year, int* month, int* day)

//  Input:   date = encoded date/time value
//  Output:  year = 4-digit year
//           month = month of year (1-12)
//           day   = day of month
//  Purpose: decodes DateTime value to year-month-day.

{
    int  D1, D4, D100, D400;
    int  y, m, d, i, k, t;

    D1 = 365;              //365
    D4 = D1 * 4 + 1;       //1461
    D100 = D4 * 25 - 1;    //36524
    D400 = D100 * 4 + 1;   //146097

    t = (int)(floor (date)) + DateDelta;
    if (t <= 0)
    {
        *year = 0;
        *month = 1;
        *day = 1;
    }
    else
    {
        t--;
        y = 1;
        while (t >= D400)
        {
            t -= D400;
            y += 400;
        }
        divMod(t, D100, &i, &d);
        if (i == 4)
        {
            i--;
            d += D100;
        }
        y += i*100;
        divMod(d, D4, &i, &d);
        y += i*4;
        divMod(d, D1, &i, &d);
        if (i == 4)
        {
            i--;
            d += D1;
        }
        y += i;
        k = isLeapYear(y);
        m = 1;
        for (;;)
        {
            i = DaysPerMonth[k][m-1];
            if (d < i) break;
            d -= i;
            m++;
        }
        *year = y;
        *month = m;
        *day = d + 1;
    }
}

int isLeapYear(int year)

//  Input:   year = a year
//  Output:  returns 1 if year is a leap year, 0 if not
//  Purpose: determines if year is a leap year.

{
    if ((year % 4   == 0)
    && ((year % 100 != 0)
    ||  (year % 400 == 0))) return 1;
    else return 0;
}

void divMod(int n, int d, int* result, int* remainder)

//  Input:   n = numerator
//           d = denominator
//  Output:  result = integer part of n/d
//           remainder = remainder of n/d
//  Purpose: finds integer part and remainder of n/d.

{
    if (d == 0)
    {
        *result = 0;
        *remainder = 0;
    }
    else
    {
        *result = n/d;
        *remainder = n - d*(*result);
    }
}

//Dp Modified
FILE* openAnyFile(char *f1, int type)
//
//  Input:   f1 = name of file to open
//           type = determines whether opening report or binary output file
//  Output:  File pointer
//  Purpose: used to open text and scratch input or output files.
//
{
	FILE* file;
    
	// --- open input and report files - read/write text file
    if (type == 0){
		if((file = fopen(f1,"rt")) == NULL)
		{
			writecon(FMT12);
			writecon(f1);
			//ErrorCode = ERR_INP_FILE;
			return NULL;
		}
	}

	// --- open output file - write text file
	if (type == 1){
		if ((file = fopen(f1,"wt")) == NULL)  
		{
		   writecon(FMT13);
		   //ErrorCode = ERR_RPT_FILE;
		   return NULL;
		}
	}

	return file;
}

void  writecon(char *s)
//
//  Input:   s = a character string
//  Output:  none
//  Purpose: writes string of characters to the console.
//
{                                                                    //(5.0.014 - LR)
   fprintf(stdout,s);
   fflush(stdout);
}

SWMMMetaData output_open(FILE* fcontrol, FILE * fout, FILE* ftimeSeries, char** inputs, int* targetPollutantSWMMOrder)
//
//  Input:   none
//  Output:  returns an error code
//  Purpose: writes basic project data to binary output file.
//
{
	//loop counters and temp int vars
    int   j = 0;
    int   m = 0;
    INT4  k = 0;
	int numberOfPeriods = 0;
	int dateTimeFormat = 0; //

	INT4  numProperities = 0;
	const int NUMSUBCATCHVARS = 7; //includes NsubcatchResults,SUBCATCH_RAINFALL,SUBCATCH_SNOWDEPTH,SUBCATCH_LOSSES,SUBCATCH_RUNOFF,SUBCATCH_GW_FLOW,SUBCATCH_GW_ELEV;
	const int NUMNODEVARS = 7; //NnodeResults,NODE_DEPTH,NODE_HEAD,NODE_VOLUME,NODE_LATFLOW,NODE_INFLO,NODE_OVERFLOW;
	const int NUMLINKVARS = 6; //NlinkResults,LINK_FLOW,LINK_DEPTH,LINK_VELOCITY,LINK_FROUDE,LINK_CAPACITY;

	INT4 bytePos;
	INT4 magicNum = 0;
	INT4 flowUnits;
	INT4 version;
	int byteOffset;
	INT4 numNodes;
	INT4 numSubcatchs;
	INT4 numLinks;
	INT4 numPolls;
	int yr;
	int mo;
	int dy;
	int hr;
	int min;
	int sec;
	int numCharsInID = 0;
	char* tempID;
	DateTime days;
    char     theDate[20];
    char     theTime[20];
	
    char  line[MAXLINE+1];

	SWMMMetaData metaDataFound;

	// add series name (set = node name)
	fputs(targetNodeID,fcontrol);
    fputs(line,fcontrol);
	fputs(line,fcontrol);
	fflush(fcontrol);
	fclose(fcontrol);

    // --- get number of objects reported on                                   //(5.0.014 - LR)
    NumSubcatch = 0;
    NumNodes = 0;
    NumLinks = 0;
	NumPolls = 0;

	//First get number of periods from the end of the file
	fseek(fout, -(4*sizeof(INT4)), SEEK_END);
	fread(&OutputStartPos, sizeof(INT4), 1, fout);   // the byte position where the Computed Results section of the file begins (4-byte integer)  
	fread(&numberOfPeriods, sizeof(INT4), 1, fout);   // number of periods
	metaDataFound.numread=numberOfPeriods;
	rewind(fout);

	fseek(fout, 0, SEEK_SET);
    fread(&magicNum, sizeof(INT4), 1, fout);   // Magic number
    fread(&version, sizeof(INT4), 1, fout);    // Version number
    fread(&flowUnits, sizeof(INT4), 1, fout);  // Flow units //(5.0.014 - LR)
    fread(&numSubcatchs, sizeof(INT4), 1, fout);   // # subcatchments                                                             //(5.0.014 - LR)
    fread(&numNodes, sizeof(INT4), 1, fout);   // # nodes                                                             //(5.0.014 - LR) 
    fread(&numLinks, sizeof(INT4), 1, fout);   // # links
    fread(&numPolls, sizeof(INT4), 1, fout);   // # pollutants

	int numNodeResults = MAX_NODE_RESULTS - 1 + numPolls;
	int numSubcatchResults = MAX_SUBCATCH_RESULTS - 1 + numPolls;
	int numlinkResults = MAX_LINK_RESULTS - 1 + numPolls;
	REAL4* nodeResultsForPeriod= (REAL4 *) calloc(numNodeResults, sizeof(REAL4)); // same as swmms NodeResults var
	

	int bytesPerPeriod = sizeof(REAL8)                                             //(5.0.014 - LR)
        + numSubcatchs * numSubcatchResults * sizeof(REAL4)                       //(5.0.014 - LR)
        + numNodes * numNodeResults * sizeof(REAL4)                              //(5.0.014 - LR)
        + numLinks * numlinkResults * sizeof(REAL4)                              //(5.0.014 - LR)
        + MAX_SYS_RESULTS * sizeof(REAL4);

	
	/*	//Skip catchment IDs
	bytePos = ftell(fout);
	byteOffset = (numSubcatchs) * sizeof(INT4);
    fseek(fout, byteOffset, SEEK_CUR);*/

	//Create an array of Nodes to hold node info
	TSubcatch* SubcatchArr = (TSubcatch*)malloc(sizeof(TSubcatch) * numSubcatchs);
	//Create an array of Nodes to hold node info
	TNode* NodeArr = (TNode*)malloc(sizeof(TNode) * numNodes);
	//Create an array of Pollutants to hold pollutant info
	TPollut* PollutArr = (TPollut*)malloc(sizeof(TPollut) * numPolls);

	//Read all subcatchment IDs
	for (j=0; j<numSubcatchs; j++)
    {
		fread(&numCharsInID, sizeof(INT4), 1, fout);
		tempID = (char*) calloc (numCharsInID,sizeof(char)+1);
		fread(tempID, sizeof(char), numCharsInID, fout);
		SubcatchArr[j].ID = tempID; 
    }


	//Read all node IDs
	for (j=0; j<numNodes; j++)
    {
		fread(&numCharsInID, sizeof(INT4), 1, fout);
		tempID = (char*) calloc (numCharsInID,sizeof(char)+1);
		fread(tempID, sizeof(char), numCharsInID, fout);
		NodeArr[j].ID = tempID; 
    }

	//Find index of requested node if in default single node mode
	targetNodeIndex = -1;
	for (j=0; j<numNodes;   j++){
		if(strcmp(targetNodeID,NodeArr[j].ID) == 0){
			targetNodeIndex = j;
			break;
		}
	}
	if(targetNodeIndex == -1){
		ErrorCode = 1;
		metaDataFound.returnresult = ErrorCode;
		return metaDataFound;
	}

	//Read all link IDs - skipping not straight forward since name lenghts vary so read
	for (j=0; j<numLinks; j++)
    {
		fread(&numCharsInID, sizeof(INT4), 1, fout);
		tempID = (char*) calloc (numCharsInID,sizeof(char)+1);
		fread(tempID, sizeof(char), numCharsInID, fout); 
    }

    //POLLUTANTS ----------------------------------------------------------------------------
	//Read pollutant ids
	for (j=0; j<numPolls;   j++){
		fread(&numCharsInID, sizeof(INT4), 1, fout);
		tempID = (char*) calloc (numCharsInID,sizeof(char)+1);
		fread(tempID, sizeof(char), numCharsInID, fout);
		PollutArr[j].ID = tempID; 
	}

	//Match swmm pollutants to framework pollutants and determine pollutant order - order by framework pollutants
	k = 0;
	for(int i = 0; i<totalNumOfFRWPollutants; i++){
		for (j=0; j<numPolls;   j++){
			if(strcmp(targetSWMPollutants[i],PollutArr[j].ID) == 0){
				targetPollutantSWMMOrder[i] = j;
				targetPollutantFRWOrder[k] = i;
				k++;
				break;
			}
		}
	}
	totalNumOfMatchedFRWPollutants = k;

	// --- save codes of pollutant concentration units
    for (j=0; j<numPolls; j++){
        fread(&k, sizeof(INT4), 1, fout);
		PollutArr[j].units = k;
    }

	/*for (j=0; j<numPolls;   j++){
		fread(&numCharsInID, sizeof(INT4), 1, fout);
		tempID = (char*) calloc (numCharsInID,sizeof(char)+1);
		fread(tempID, sizeof(char), numCharsInID, fout);
		PollutArr[j].ID = tempID; 
	}*/
	//-------------------------------------------------------------------------------

    // --- skip subcatchment area and associated codes
	fread(&k, sizeof(INT4), 1, fout); // 1 - number of subcatchment properties - area
	fread(&k, sizeof(INT4), 1, fout); // 1 - code number for subcatchment property - area
	if(numSubcatchs){
		bytePos = ftell(fout);
		byteOffset = (numSubcatchs) * sizeof(REAL4);
		fseek(fout, byteOffset, SEEK_CUR);
	}

    // --- save node type, invert, & max. depth
    fread(&k, sizeof(INT4), 1, fout); // 3 - number of node properties 
    fread(&k, sizeof(INT4), 1, fout); // INPUT_TYPE_CODE code number of property 1
    fread(&k, sizeof(INT4), 1, fout); // INPUT_INVERT code number of property 2
    fread(&k, sizeof(INT4), 1, fout); // INPUT_MAX_DEPTH; code number of property 3
    for (j=0; j<numNodes; j++)
    {
        fread(&k, sizeof(INT4), 1, fout);                                     //(5.0.014 - LR)
        NodeArr[j].type = k;
		fread(&k, sizeof(REAL4), 1, fout);
		NodeArr[j].invertElev = k;
		fread(&k, sizeof(REAL4), 1, fout);
		NodeArr[j].fullDepth = k;
    }

	// --- skip link type, offsets, max. depth, & length
    fread(&numProperities, sizeof(INT4), 1, fout); //5 - number of link properties;
    fread(&k, sizeof(INT4), 1, fout); //INPUT_TYPE_CODE 
    fread(&k, sizeof(INT4), 1, fout); //INPUT_OFFSET
    fread(&k, sizeof(INT4), 1, fout); //INPUT_OFFSET
    fread(&k, sizeof(INT4), 1, fout); // INPUT_MAX_DEPTH
    fread(&k, sizeof(INT4), 1, fout); //INPUT_LENGTH

	//Type code is int rest are real4 so read type code seperately
	if(numLinks){
		bytePos = ftell(fout);
		byteOffset = (numLinks) * sizeof(INT4) + (numLinks) * (numProperities - 1) * sizeof(REAL4);
		fseek(fout, byteOffset, SEEK_CUR);
	}

	//Get Start date and reporting timestep
	fseek(fout, OutputStartPos -(sizeof(REAL8) + sizeof(INT4)), SEEK_SET);
	fread(&reportStartDate,  sizeof(REAL8), 1, fout);
	//StartDateTime  = getDateTime(reportStartDate);
    fread(&reportTimeInterv,  sizeof(INT4), 1, fout);

	metaDataFound.timeStep=reportTimeInterv;

	//print header records ============================================================

    //fprintf(ftimeSeries, "#%s \n#NodeID:%s\n#\n#","SWMM Trial Run Under pre-BMP conditions", targetNodeID); // write header
	//fprintf(ftimeSeries, "#%s \n#NodeID:%s\n#\n#",inputs[0], targetNodeID); // write header
	//fprintf(ftimeSeries, "\n#%11s,%8s,%9s","Year,MM,DD","hours","flow"); // write header

	for (int p = 0; p < totalNumOfMatchedFRWPollutants; p++)
        if(p>0){
		   fprintf(ftimeSeries, ",%9s",targetFRWPollutants[targetPollutantFRWOrder[p]]);
		}else{
           fprintf(ftimeSeries, "Q,%9s",targetFRWPollutants[targetPollutantFRWOrder[p]]);
		}
	fprintf(ftimeSeries,"\n");

	//get node results for all time periods ============================================
	for (int period = 1; period <= numberOfPeriods; period++ )
    {
		days = reportStartDate + (reportTimeInterv * period/86400.0);
		if (period == 1)
		{  
			datetime_decodeDate(days, &yr, &mo, &dy);
			metaDataFound.fyear=yr;
			metaDataFound.fmonth=mo;
			metaDataFound.fday=dy;
			datetime_decodeTime(days, &hr, &min, &sec); 	
	        metaDataFound.fhour = hr*3600 + min*60 + sec;
		}
        datetime_dateToStr2(days, theDate, dateTimeFormat);
        datetime_timeToStr2(days, theTime);
		memset(nodeResultsForPeriod,0,numNodeResults*sizeof(REAL4));
		output_readNodeResults(nodeResultsForPeriod, period, targetNodeIndex,numNodeResults,numSubcatchs , numSubcatchResults , OutputStartPos, bytesPerPeriod, fout);                             //(5.0.014 - LR)
		if (period != 1) 
		   {
		       fprintf(ftimeSeries, "\n");
		   }
			  fprintf(ftimeSeries, " %11s,%8s,%9.3f",theDate, theTime, nodeResultsForPeriod[NODE_INFLOW] * flowConversionFactor);
		   
		for (int p = 0; p < totalNumOfMatchedFRWPollutants; p++){
			if(nodeResultsForPeriod[NODE_INFLOW] < MIN_WQ_FLOW){
				fprintf(ftimeSeries, ",%9.3f", 0.0f);
			}else{
				fprintf(ftimeSeries, ",%9.3f", nodeResultsForPeriod[NODE_QUAL + targetPollutantSWMMOrder[p]] * targetPollutantFactors[p]);
			}
		}
	}
	datetime_decodeDate(days, &yr, &mo, &dy);
	metaDataFound.tyear=yr;
	metaDataFound.tmonth=mo;
	metaDataFound.tday=dy;
	datetime_decodeTime(days, &hr, &min, &sec); 	
	metaDataFound.thour = hr*3600 + min*60 + sec;
    
	free(nodeResultsForPeriod);
	free(PollutArr);
	free(NodeArr);

	metaDataFound.returnresult = 0;
	
    return metaDataFound;
}

//=============================================================================

float* output_readNodeResults(float* results, int period, int nodeIndex, int numNodeResults, int numSubCatchs, int numSubCatchRslts, int outputStartPos, int bytesPerPeriod, FILE* fout)
//
//  Input:   period = index of reporting time period
//           index = node index
//  Output:  none
//  Purpose: reads computed results for a node at a specific time period.
//
{
	//float* results;
    INT4 bytePos = outputStartPos + (period-1)*bytesPerPeriod;
    bytePos += sizeof(REAL8) + numSubCatchs * numSubCatchRslts * sizeof(REAL4);     //(5.0.014 - LR)
    bytePos += nodeIndex * numNodeResults * sizeof(REAL4);
    fseek(fout, bytePos, SEEK_SET);
    fread(results, sizeof(REAL4), numNodeResults, fout);
	return results;
}

int input_readData2(FILE* finp, char* inputs[20])
//
//  Input:   none
//  Output:  returns error code
//  Purpose: reads input file to determine input parameters for each object.
//
{
    char  line[MAXLINE+1];        // line from input data file
    char  wLine[MAXLINE+1];       // working copy of input line
    char* comment;                // ptr. to start of comment in input line
    //int   sect, newsect;          // data sections
    int   errsum;         // error code & total error count
    //int   lineLength;             // number of characters in input line
    //int   i;
    long  lineCount = 0;
	int numTokens = 0;
	char** outToks;
	int inputCount = 0;

    //sect = 0;
    errsum = 0;
	outToks = (char**)calloc(sizeof(char*),40);
    rewind(finp);
    while ( fgets(line, MAXLINE, finp) != NULL )
    {
        // --- make copy of line and scan for tokens
        lineCount++;
        strcpy(wLine, line);
        numTokens = getTokens2(wLine, outToks);

        // --- skip blank lines and comments
        if ( numTokens == 0 ) continue;
		if ( *outToks[0] == '#' ) continue;
		
		printf("\nReading MTA: %s ",*outToks);
		inputs[inputCount] = (char*)calloc(sizeof(char),strlen(*outToks)+1);
		strcpy(inputs[inputCount], *outToks);
		inputCount++;
    }   /* End of while */

    // --- check for errors
    if (errsum > 0)  ErrorCode = 48 ;  //ERR_INPUT;
    return ErrorCode;
}

int  getTokens2(char *s, char** outToks)
//
//  Input:   s = a character string
//  Output:  returns number of tokens found in s
//  Purpose: scans a string for tokens, saving pointers to them
//           in shared variable Tok[].
//
//  Notes:   Tokens can be separated by the characters listed in SEPSTR
//           (spaces, tabs, newline, carriage return) which is defined
//           in CONSTS.H. Text between quotes is treated as a single token.
//
{
    int  len, m, n;
    char *c;
	char commentChar = '#';

    // --- begin with no tokens
    for (n = 0; n < MAXTOKS; n++) outToks[n] = NULL;
    n = 0;

    // --- truncate s at start of comment 
    c = strchr(s,commentChar);
    if (c) *c = '\0';
    len = strlen(s);

    // --- scan s for tokens until nothing left
    while (len > 0 && n < MAXTOKS)
    {
        m = strcspn(s,SEPSTR);              // find token length 
        if (m == 0) s++;                    // no token found
        else
        {
            if (*s == '"' || *s == '\'')                  // token begins with quote
            {
                s++;                        // start token after quote
                len--;                      // reduce length of s
                m = strcspn(s,"\"\n\'");      // find end quote or new line
            }
            s[m] = '\0';                    // null-terminate the token
            outToks[n] = s;                     // save pointer to token 
            n++;                            // update token count
            s += m+1;                       // begin next token
        }
        len -= m+1;                         // update length of s
    }
    return(n);
}



char* trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;
  
  return str;
}



void datetime_dateToStr(DateTime date, char* s)

//  Input:   date = encoded date/time value
//  Output:  s = formatted date string
//  Purpose: represents DateTime date value as a formatted string.

{
    int  y, m, d;
    char dateStr[DATE_STR_SIZE];
    datetime_decodeDate(date, &y, &m, &d);
    switch (DateFormat)
    {
      case Y_M_D:
        sprintf(dateStr, "%4d-%3s-%02d", y, MonthTxt[m-1], d);
        break;

      case M_D_Y:
        sprintf(dateStr, "%3s-%02d-%4d", MonthTxt[m-1], d, y);
        break;

      default:
        sprintf(dateStr, "%02d-%3s-%4d", d, MonthTxt[m-1], y);
    }
    strcpy(s, dateStr);
}

//=============================================================================

void datetime_timeToStr(DateTime time, char* s)

//  Input:   time = decimal fraction of a day
//  Output:  s = time in hr:min:sec format
//  Purpose: represents DateTime time value as a formatted string.

{
    int  hr, min, sec;
    char timeStr[TIME_STR_SIZE];
    datetime_decodeTime(time, &hr, &min, &sec);
    sprintf(timeStr, "%02d:%02d:%02d", hr, min, sec);
    strcpy(s, timeStr);
}

//==BEGIN SWMM INPUT FILE ROUTINES==============================================



void write_inflow_block(FILE* swmmInputFile, int totalNumOfFRWPollutants, char *targetNodeID, char** targetFRWPollutants, char **targetSWMPollutants, char **targetPollutantFactors){
	char* buffer;
	char* tsParam;
	char* tsNameSuffix = "_TimeSeries";
	char tsName[32];

	WRITE("\n[INFLOWS]", swmmInputFile);
	WRITE("\n;;                                                 Param    Units    Scale    Baseline Baseline", swmmInputFile);
	WRITE("\n;;Node           Parameter        Time Series      Type     Factor   Factor   Value    Pattern", swmmInputFile);
	WRITE("\n;;-------------- ---------------- ---------------- -------- -------- -------- -------- --------", swmmInputFile);
	for(int i = 0; i<totalNumOfFRWPollutants;i++){
		tsParam = get_timeseriesProperties(1, targetFRWPollutants[i]);
		sprintf(tsName, "%s%s", targetSWMPollutants[i], tsNameSuffix);
		buffer = format_inflow_blockline(targetNodeID, targetSWMPollutants[i], tsName, tsParam, 1.0, atof(targetPollutantFactors[i])); 
		//WRITE(buffer, swmmInputFile);
	}
}

char* format_inflow_blockline(char *nodeName, char *tsType, char *tsName, char *tsParam, float tsUnitsFactor, float tsScaleFactor){
	char buffer [50];
	sprintf(buffer, "\n%s        %s             %s         %s     %6.6f      %6.6f                               ",nodeName, tsType, tsName, tsParam, tsUnitsFactor, tsScaleFactor); 
	return buffer;
}

//1 - Parameter Name code in SWMM
char* get_timeseriesProperties(int propertyType, char* frameworkTSName){
	if(propertyType == 1){
		if(strcmp("FLOW", frameworkTSName)) return "FLOW";
	}
}